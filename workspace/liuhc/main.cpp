// ImGui - standalone example application for Glfw + OpenGL 2, using fixed pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <boost/static_assert.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
//#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/signal_set.hpp>
//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/fstream.hpp>
#include <boost/signals2/signal.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"

namespace ip = boost::asio::ip;

template <typename... As> void err_exit_(int lin_, char const* fmt, As... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... As> void err_msg_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
}
#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
#define DBG_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)

template <typename Derived>
struct http_connection
{
    typedef http_connection<Derived> This;

    ip::tcp::resolver resolver_;
    ip::tcp::socket tcpsock_;
    std::string host_; //ip::tcp::endpoint endpoint_;
    std::string path_;
    std::string session_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    //int cseq_ = 1;

    http_connection(boost::asio::io_service& io_s, std::string host, std::string path)
        : resolver_(io_s),tcpsock_(io_s)
        , host_(host), path_(path)
    {
        //BOOST_STATIC_ASSERT(std::is_base_of<This, Derived>::value);
    }

    boost::asio::io_service& get_io_service() { return tcpsock_.get_io_service(); }

    struct Resolve {};
    struct Connect {};
    struct Query {
        void operator()(Derived* d) const { d->on_success(*this); }
    };

    void close() {
        boost::system::error_code ec;
        tcpsock_.close(ec);
    }

    void resolve() // Resolve
    {
        auto h_resolv = [this](boost::system::error_code ec, ip::tcp::resolver::iterator it) {
            if (ec) {
                derived()->on_error(ec, Resolve{});
            } else {
                connect(it);
            }
        };

        ip::tcp::resolver::query q(host_, "http");
        resolver_.async_resolve(q, h_resolv);
    } // void connect()

    void connect(ip::tcp::resolver::iterator it)
    {
        auto h_connect = [this](boost::system::error_code ec, ip::tcp::resolver::iterator it) {
            if (ec) {
                derived()->on_error(ec, Connect{});
            } else {
                derived()->on_success(Connect{});
            }
        };
        boost::asio::async_connect(tcpsock_, it, h_connect);
    }

    void query(int year) // Query
    {
        {
        std::ostream outs(&request_);
        outs << "GET " << path_ <<""<< year << " HTTP/1.1\r\n"
            << "Host: " << host_ << "\r\n"
            << "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)" << "\r\n"
            << "Accept: */*\r\n"
            << "Connection: close" << "\r\n"
            << "\r\n";
        }
        boost::asio::async_write(tcpsock_, request_, Action_helper<Query>{derived()} );
    }

private:
    Derived* derived() { return static_cast<Derived*>(this); }

    template <typename Action>
    struct Action_helper
    {
        void operator()(boost::system::error_code ec, size_t) const {
            auto* derived = derived_;
            if (ec) {
                derived->on_error(ec, Action{});
            } else {
                derived->response_.consume(derived->response_.size());
                boost::asio::async_read_until(derived->tcpsock_, derived->response_, "\r\n\r\n"
                        , [derived](boost::system::error_code ec, size_t){
                            if (ec) {
                                derived->on_error(ec, Action{});
                            } else {
                                // "RTSP/1.0 200 OK"; // TODO
                                Action fn;
                                fn(derived);
                            }
                    });
            }
        }
        Derived* derived_; // Action_helper(Derived* d) : Action{d} {}
    };
};

struct liuhc_client : boost::noncopyable
{
    typedef liuhc_client This;

    liuhc_client(boost::asio::io_service& io_s
            , std::string remote_host, std::string remote_path
            )
        : http_client_(this, io_s, remote_host, remote_path)
        ///, nalu_(dump_fp) //(outfile, sps, pps)
        ///, udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), local_port()))
    {}

    int setup(int, char*[]) {
        http_client_.resolve();
        return 0;
    }

    void teardown() {
        DBG_MSG("Teardown");
        //http_client_.teardown();

        boost::asio::io_service& io_s = http_client_.get_io_service();
        http_client_.sig_teardown.connect(boost::bind(&boost::asio::io_service::stop, &io_s));
    }

private: // rtsp communication
    /// http://www.y1118.com
    /// path /xin-index-1.html?year=2002
    struct http_client : http_connection<http_client>, boost::noncopyable
    {
        liuhc_client* thiz;
        int begin_year_ = 2010;
        int year_ = 2010;

        http_client(liuhc_client* ptr
                , boost::asio::io_service& io_s
                , std::string remote_host, std::string remote_path)
            : http_connection(io_s, remote_host, remote_path)
        { thiz = ptr; }

        // void on_success(Close) { }

        void on_success(Connect) {
            //year_ = begin_year_;
            query(year_); //(path_ + std::to_string(2015)); //options();
        }
        void on_success(Query)
        {
            std::string y = std::to_string(year_) + "001";
            std::istream ins(&response_);
            std::string line;
            bool line_b = 0;
            int noy = 0;
            std::vector<int> codes;
            std::vector< std::vector<int> > his;
            while (std::getline(ins, line)) {
                if (!line_b)
                    line_b = std::search(line.begin(),line.end(), y.begin(),y.end()) < line.end();
                if (!line_b)
                    continue;

                //<td>2002002</td>
                if (!noy) {
                    boost::smatch m;
                    boost::regex re("<td>([[:digital:]]{7})</td>");
                    if (boost::regex_search(line, m, re)) {
                        auto* s = m[1].first.operator->(); // (m[1].first,m[1].second);
                        noy = atoi(s);
                    }
                    continue;
                }

                //<td class="cp_2">14</td>
                boost::smatch m;
                boost::regex re("<td[[:space:]]+class=\".*\">([[:digital:]]+)</td>");
                if (boost::regex_search(line, m, re)) {
                    char const* s = m[1].first.operator->(); // (m[1].first,m[1].second);
                    int c = atoi(s);
                    codes.push_back(c);

                    //</tr><tr>
                } else if (boost::starts_with(line, "</tr>")) {
                    std::sort(codes.begin(), codes.end());
                    his.push_back( codes );
                    codes.clear();
                    noy=0;

                    //</tr></table>
                } else if (boost::ends_with(line, "</table>")) {
                    break;
                }
            } // while

            thiz->history(his.begin(), his.end());
            ++year_;
            DBG_MSG("year => %d", year_);
            close();

            if (year_ <= current_year()) {
                get_io_service().post([this](){ resolve(); });
            }
        }

        static int current_year() { return 2016; }

        template <typename A> void on_success(A) { DBG_MSG("success:A"); }

        template <typename A>
        void on_error(boost::system::error_code ec, A) {
            ERR_MSG("error");
            // TODO : deadline_timer reconnect
        }

        boost::signals2::signal<void()> sig_teardown;
    };

    template <typename I>
    void history(I b, I e) {
        his_.insert(his_.end(), b, e);
        last_year_count_ = e - b;
    }

    std::vector< std::vector<int> > his_;
    int last_year_count_ = 0;

    http_client http_client_;
    //nal_unit_sink nalu_;

private:
    static int local_port(int p=0) { return 3395+p; } // TODO

    //ip::udp::socket udpsock_; ip::udp::endpoint peer_endpoint_;

    //boost::filesystem::path dir_; int dg_idx_ = 0;
    //std::aligned_storage<1024*64,alignof(int)>::type data_;
    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_); }
    enum { BufSiz = 1024*64 };
    int buf_[BufSiz/sizeof(int)+1];
};

//#include <boost/type_erasure/member.hpp>
//BOOST_TYPE_ERASURE_MEMBER((setup_fn), setup, 2)
// BOOST_TYPE_ERASURE_MEMBER((setup_fn), ~, 0)
//boost::type_erasure::any<setup_fn<int(int,char*[])>, boost::type_erasure::_self&> ;

struct gui
{
    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);
    GLFWwindow* window;

    static void error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error %d: %s\n", error, description);
    }

    gui(int ac,char* const av[])
    {
        // Setup window
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            ERR_EXIT("glfwInit");
        window = glfwCreateWindow(1280, 720, "你好！世界", NULL, NULL);
        glfwMakeContextCurrent(window);

        // Setup ImGui binding
        ImGui_ImplGlfw_Init(window, true);

        // Load Fonts
        // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("/home/wood/.fonts/msyh.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesChinese());
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    }
    ~gui() {
        // Cleanup
        ImGui_ImplGlfw_Shutdown();
        glfwTerminate();
    }

    void event_loop()
    {
        if (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            ImGui_ImplGlfw_NewFrame();

            // 1. Show a simple window
            // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
            {
                static float f = 0.0f;
                ImGui::Text("你好，时间!");
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                ImGui::ColorEdit3("clear color", (float*)&clear_color);
                if (ImGui::Button("Test Window")) show_test_window ^= 1;
                if (ImGui::Button("Another Window")) show_another_window ^= 1;
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }

            // 2. Show another simple window, this time using an explicit Begin/End pair
            if (show_another_window)
            {
                ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
                ImGui::Begin("Another Window", &show_another_window);
                ImGui::Text("Hello");
                ImGui::End();
            }

            // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
            if (show_test_window)
            {
                ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
                ImGui::ShowTestWindow(&show_test_window);
            }

            // Rendering
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui::Render();
            glfwSwapBuffers(window);
        }
    }
};

struct Main : boost::asio::io_service, boost::noncopyable
{
    struct Args {
        Args(int ac, char* av[]) {
        //    if (ac <= 2) {
        //        if (ac==2 && !(dump_fp = fopen(av[1], "rb")))
        //            ERR_EXIT("file %s: fail", av[1]);
        //    } else if (ac > 3) {
        //        endp = ip::tcp::endpoint(ip::address::from_string(av[1]),atoi(av[2]));
        //        //path = av[3];
        //        //if (ac > 4 && !(dump_fp = fopen(av[4], "wb")))
        //        //    ERR_EXIT("file %s: fail", av[2]);
        //    } else {
        //        ERR_EXIT("Usage: %s ...", av[0]);
        //    }
        }
        
        //ip::tcp::endpoint endp;
        std::string host = "www.y1118.com"; // + "2010"
        std::string path = "/xin-index-1.html?year="; // + "2010"
        //FILE* dump_fp = 0;
    };

    gui gui_;

    Main(int ac, char* av[])
        : gui_(ac,av)
        , signals_(*this)
    {
        Args args(ac, av);
        {
            auto* obj = new (&objmem_) liuhc_client(*this, args.host, args.path);
            dtor_ = [obj]() { obj->~liuhc_client(); };
            setup_ = [obj](int ac,char*av[]) { obj->setup(ac,av); };

            signals_.add(SIGINT);
            signals_.add(SIGTERM); // (SIGQUIT);
            signals_.async_wait( [this](boost::system::error_code, int){
                        auto* obj = reinterpret_cast<liuhc_client*>(&objmem_);
                        obj->teardown();
                        this->stop();
                    } );
        }

    }
    ~Main() { dtor_(); }

    int run(int ac, char* av[])
    {
        //setup_(ac, av);
        gui_loop();
        return boost::asio::io_service::run();
    }

private:
    void gui_loop() {
        gui_.event_loop();
        post([this](){ gui_loop(); });
    }

    boost::asio::signal_set signals_;

    std::function<void(int,char*[])> setup_;
    std::function<void()> dtor_;
    static int objmem_[sizeof(liuhc_client)/sizeof(int)+1];
};
int Main::objmem_[sizeof(liuhc_client)/sizeof(int)+1] = {};

int main(int argc, char* argv[])
{
    //BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END
    try {
        Main s(argc, argv);
        s.run(argc, argv);
    } catch (std::exception& e) {
        ERR_MSG("Exception: %s", e.what());
    }

    return 0;
}

//http://www.y1118.com/xin-index-1.html?year=2002
//<td>2002001</td>
//<td class="cp_2">14</td>
//<td class="cp_3">21</td>
//<td class="cp_3">11</td>
//<td class="cp_1">18</td>
//<td class="cp_2">3</td>
//<td class="cp_1">13</td>
//<td class="cp_3">22</td>
//<td>2002-1-3</td>
//</tr><tr>
//<td>2002002</td>
//<td class="cp_3">21</td>
//<td class="cp_2">36</td>
//<td class="cp_1">12</td>
//<td class="cp_3">43</td>
//<td class="cp_3">17</td>
//<td class="cp_1">35</td>
//<td class="cp_2">14</td>

