#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/static_assert.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/spirit/include/qi.hpp>
//#include <boost/spirit/include/qi_repeat.hpp>
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
#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/fstream.hpp>
#include <boost/signals2/signal.hpp>
//#include <boost/iostreams/filtering_stream.hpp>
//#include <boost/iostreams/device/back_inserter.hpp>
//#include <boost/serialization/vector.hpp>
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/text_oarchive.hpp>

#include <imgui.h>
#ifdef BOOST_MSVC
#  define GLFW3_OPENGL
#  include <windows.h>
#  include <ShellApi.h>
void notepad_open(const char* fn) { ShellExecuteA(GetDesktopWindow(), "open", fn, NULL, NULL, SW_SHOW); }
#else
void notepad_open(const char* fn) {}
#endif
#if defined(GLFW3_OPENGL)
#  include <GLFW/glfw3.h>
#  include "imgui_impl_glfw.h"
#else //if defined(SDL_OPENGL)
#  include <SDL.h>
#  include <SDL_opengl.h>
#  include "imgui_impl_sdl.h"
#endif
#include <iostream>

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
struct http_connection : boost::noncopyable
{
    typedef http_connection<Derived> This;

    ip::tcp::resolver resolver_;
    ip::tcp::socket tcpsock_;
    std::string host_; //ip::tcp::endpoint endpoint_;
    std::string path_;

    boost::asio::streambuf request_, response_;
    boost::asio::streambuf content_;

    struct response_buffer : private std::string {
        size_t siz0_ = 0;

        size_t size() const { return std::string::size()-siz0_; }
        size_t capacity() const { return std::string::capacity()-siz0_; }

        char* head() { return const_cast<char*>(std::string::data()); }
        char* begin() { return head() + siz0_; }
        char* end() { return data() + size(); }
        void shrink_leading_fit(char* p=0, size_t siz=0) {
            std::move(head()+siz, data(), size());
            if (siz > 0)
                std::memcpy(head(), p, siz);
            siz0_=siz;
        }

        boost::asio::mutable_buffer prepare(size_t len) {
            if (capacity()-size() < len) {
                std::string::reserve(siz0_+size()+len);
            }
            return boost::asio::mutable_buffer(data(), len);
        }
        void commit(int len) {
            if (len > capacity()-size())
                ERR_EXIT("commit");
            resize(siz0_ + size() + len);
        }

        void consume(int len) { siz0_ += len; }
        void consume_to(char* pos) { siz0_ = pos - head(); }

        boost::iterator_range<char*> iterator_range() {
            return boost::make_iterator_range(data(), data()+size());
        }
    };

    http_connection(boost::asio::io_service& io_s, std::string host, std::string path)
        : resolver_(io_s),tcpsock_(io_s)
        , host_(host), path_(path)
    {
        //BOOST_STATIC_ASSERT(std::is_base_of<This, Derived>::value);
    }

    boost::asio::io_service& get_io_service() { return tcpsock_.get_io_service(); }

    struct Resolve {};
    struct Connect {};
    struct Query {};

    void close() {
        boost::system::error_code ec;
        tcpsock_.close(ec);
    }

    void resovle()
    {
        auto h_resolv = [this](boost::system::error_code ec, ip::tcp::resolver::iterator it) {
            DBG_MSG("@resolv: %d", ec.value());
            if (ec) {
                derived()->on_error(ec, Resolve{});
            } else {
                std::cout <<"resolved: "<< it->endpoint() <<"\n";
                connect(it);
            }
        };
        ip::tcp::resolver::query q(host_, "http");
        resolver_.async_resolve(q, h_resolv);
    }

    void connect(ip::tcp::resolver::iterator it)
    {
        auto h_connect = [this](boost::system::error_code ec, ip::tcp::resolver::iterator it) {
            DBG_MSG("@connect: %d", ec.value());
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
        //std::string remote_path = "/xin-index-1.html?year="; // + "2010"
        {
        std::ostream outs(&request_);
        outs << "GET " << path_ <<""<< year << " HTTP/1.1\r\n"
            << "Host: " << host_ << "\r\n"
            << "User-Agent: curl/7.18" << "\r\n"
            << "Accept: */*" << "\r\n"
            //<< "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:45.0) Gecko/20100101 Firefox/45.0\r\n"
            //<< "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
            //<< "Accept-Language: en-US,zh-CN;q=0.8,en;q=0.5,zh;q=0.3\r\n"
            //<< "Accept-Encoding: gzip, deflate\r\n"
            //<< "Connection: keep-alive\r\n" //<< "Connection: close\r\n"
            << "\r\n";
        } //out.flush();
        auto h = [this](boost::system::error_code ec, size_t) {
            if (ec) {
                derived()->on_error(ec, Query{});
            } else {
                read_response_headers();
            }
        };
        boost::asio::async_write(tcpsock_, request_, h);
    }

private:
//GET /xin-index-1.html?year=2016 HTTP/1.1
//Host: www.x.com
//User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:45.0) Gecko/20100101 Firefox/45.0
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//Accept-Language: en-US,zh-CN;q=0.8,en;q=0.5,zh;q=0.3
//Accept-Encoding: gzip, deflate
//Cookie: cck_lasttime=1461466867278; cck_count=1
//Connection: keep-alive

//HTTP/1.1 200 OK
//Server: nginx
//Date: Sun, 24 Apr 2016 02:57:44 GMT
//Content-Type: text/html;charset=utf-8
//Transfer-Encoding: chunked
//Connection: keep-alive
//Vary: Accept-Encoding
//X-Powered-By: PHP/5.3.29
//Content-Encoding: gzip

    std::string cookie_;
    bool gzip_ = 0;

    Derived* derived() { return static_cast<Derived*>(this); }

    void read_response_headers()
    {
        auto h = [this](boost::system::error_code ec, size_t bytes) {
            DBG_MSG("r headers: %d(%s) %d", ec.value(), ec.message().c_str(), bytes);
            int content_len = 0; // gzip_ = 1;
            bool bchunked = 0;
            if (ec) {
                derived()->on_error(ec, Query{});
                return;
            } else {
                bool end_of_headers_ = 0;

                std::istream ins(&response_);
                std::string line;

                if (std::getline(ins, line)) {
                    DBG_MSG("%s", line.c_str());
                }
                boost::regex re_headline("^([^:]+):\\s+(.+)$");
                while (std::getline(ins, line)) {
                    boost::trim_right(line); // boost::regex re_eoh("^\\s*$");
                    boost::smatch m;
                    if (boost::regex_match(line, m, re_headline)) {
                        if (boost::starts_with(line, "Content-Length")) {
                            content_len = atoi(m[2].first.operator->());
                        } else if (boost::starts_with(line, "Transfer-Encoding")) {
                            bchunked = (boost::equals(m[2], "chunked"));
                        } else if (boost::starts_with(line, "Content-Encoding")) {
                            gzip_ = (boost::equals(m[2], "gzip"));
                        }
                    } else {
                        if (line.empty()) {
                            end_of_headers_ = 1;
                            break;
                        } else {
                            ERR_EXIT("%s",line.c_str());
                        }
                    }
                }
                DBG_MSG("gzip %d, chunk %d, clen %d, eoh %d", gzip_, bchunked, content_len, end_of_headers_);
            }

            if (bchunked) {
                read_chunk_size();
            } else {
                read_content(content_len);
            }
        };
        boost::asio::async_read_until(tcpsock_, response_, "\r\n\r\n", h);
    }
    void read_chunk_size()
    {
        auto h = [this](boost::system::error_code ec, size_t bytes) {
            DBG_MSG("r chunk-size: %d(%s) %d", ec.value(), ec.message().c_str(), bytes);
            if (ec) {
                derived()->on_error(ec, Query{});
            } else {
                char tmp[16];
                auto * bufptr = boost::asio::buffer_cast<char const*>(response_.data());
                unsigned bufsiz = boost::asio::buffer_size(response_.data());
                strncpy(tmp, bufptr, std::min(15u,bufsiz));
                response_.consume(bytes);
                if (!isxdigit(*tmp)) {
                    ERR_EXIT("%s", tmp);
                }
                size_t len = strtol(tmp, NULL, 16); // strtol(const char *nptr, char **endptr, int base);
                if (len == 0) {
                    DBG_MSG("Completed");
                    derived()->on_success(Query{});
                } else {
                    DBG_MSG("r chunk-size: %d, bytes %d, len %d", bufsiz, bytes, len);
                    read_chunk(len);
                }
            }
        };
        boost::asio::async_read_until(tcpsock_, response_, "\r\n", h);
    }
    void read_chunk(size_t len) {
        auto h = [this,len](boost::system::error_code ec, size_t bytes){
            DBG_MSG("r chunk: %d(%s) %d", ec.value(), ec.message().c_str(), bytes);
            if (ec) {
                derived()->on_error(ec, Query{});
            } else {
                std::ostream out(&content_);
                auto* p = boost::asio::buffer_cast<char const*>(response_.data());
                //size_t bufsiz = boost::asio::buffer_size(response_.data());
                out.write(p, len);
                response_.consume(len+2);
                read_chunk_size();
            }
        };
        unsigned bufsiz = boost::asio::buffer_size(response_.data());
        if (len + 2 <= bufsiz) {
            h(boost::system::error_code(), bufsiz);
        } else {
            DBG_MSG("r chunk: %d %d", len, bufsiz);
            boost::asio::async_read(tcpsock_, response_.prepare(len+2-bufsiz), h);
        }
    }
    void read_content(size_t len) {
        auto h = [this](boost::system::error_code ec, size_t bytes){
            DBG_MSG("r content: %d(%s) %d", ec.value(), ec.message().c_str(), bytes);
            if (ec) {
                derived()->on_error(ec, Query{});
            } else {
                DBG_MSG("Completed");
                derived()->on_success(Query{});
            }
        };
        size_t siz = boost::asio::buffer_size(response_.data());
        boost::asio::async_read(tcpsock_, response_.prepare(len-siz), h);
    }

    //template <typename Action>
    //struct Action_helper
    //{
    //    void operator()(boost::system::error_code ec, size_t) const {
    //        auto* derived = derived_;
    //        if (ec) {
    //            derived->on_error(ec, Action{});
    //        } else {
    //            // derived->response_.consume(derived->response_.size());
    //            boost::asio::async_read(derived->tcpsock_, derived->response_.prepare(16384*4)
    //                    , [derived](boost::system::error_code ec, size_t bytes){
    //                        DBG_MSG("read: %d(%s) %d", ec.value(), ec.message().c_str(), bytes);
    //                        if (ec) {
    //                            derived->on_error(ec, Action{});
    //                        } else {
    //                            // "... 200 OK"; // TODO
    //                            derived->response_.commit(bytes);
    //                            Action fn;
    //                            fn(derived);
    //                        }
    //                });
    //        }
    //    }
    //    Derived* derived_; // Action_helper(Derived* d) : Action{d} {}
    //};
};

template <typename I>
std::string make_string(I b, I e) {
    std::string ret;
    for (; b != e; ++b)
        ret += char(*b);
    return std::move(ret);
}

char* c_trim_right(char* h)
{
    char* end = h + strlen(h);
    char* p = end;
    while (p > h && isspace(*(p-1)))
        --p;
    if (p != end)
        *p = 0;
    return h;
}

struct Liuhc : boost::noncopyable
{
    typedef Liuhc This;

    std::vector< std::string > his_; // std::vector<int> counts_; // int last_year_count_ = 0;

    template <typename Args>
    Liuhc(boost::asio::io_service& io_s, Args&& a) //, std::string remote_host, std::string remote_path
        : http_client_(this, io_s, a.remote_host, a.remote_path)
    {}

    void setup(int, char*[]) {
        http_client_.change_n_years(3);
        if (http_client_._download_data() == 0) {
            his_ = http_client_.prepare_history_data();
        }
        DBG_MSG("his: %u", his_.size());
    }

    void teardown() {
        DBG_MSG("Liuhc:teardown");
        //http_client_.teardown();

        boost::asio::io_service& io_s = http_client_.get_io_service();
        http_client_.sig_teardown.connect(boost::bind(&boost::asio::io_service::stop, &io_s));
    }

    boost::asio::io_service& get_io_service() { return http_client_.get_io_service(); }

private: // rtsp communication
    /// path /xin-index-1.html?year=2002
    struct http_client : http_connection<http_client> //, boost::noncopyable
    {
        Liuhc* object;
        int oldest_year_ = 2016;
        int year_;

        http_client(Liuhc* obj
                , boost::asio::io_service& io_s
                , std::string remote_host, std::string remote_path)
            : http_connection(io_s, remote_host, remote_path)
        {
            object = obj;
            year_ = current_year();
        }
        static int current_year() { return 2016; }

        void change_n_years(int n_year)
        {
            year_ = current_year();
            oldest_year_ = year_ +1 - n_year;
        }

        std::vector<std::string> prepare_history_data() // Prepare
        {
            if (year_ >= oldest_year_) {
                return std::vector<std::string>();
            }
            std::vector<std::string> his;
            for (int y = current_year(); y >= oldest_year_; --y) {
                std::string fn = (".year." + std::to_string(y) + ".txt");

                FILE* fp = fopen(fn.c_str(), "r");
                if (!fp) {
                    ERR_EXIT("open %s", fn.c_str());
                }
                std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);

                char linebuf[512];
                while (fgets(linebuf, sizeof(linebuf), fp)) {
                    c_trim_right(linebuf);
                    if (*linebuf == 0) continue;
                    namespace qi = boost::spirit::qi;
                    using qi::int_;
                    std::array<int,7> c;
                    char* pos = linebuf;
                    if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                                , int_ >> int_ >> int_ >> int_ >> int_ >> int_ >> int_
                                , qi::space
                                , c[0], c[1], c[2], c[3], c[4], c[5], c[6])) {
                        ERR_EXIT("parse: %s: %s", fn.c_str(), pos);
                    }
                    his.push_back( make_string(c.begin(),c.end()) );
                }
            }
            //this->get_io_service().post([this](){ this->on_success(http_stages::Prepare{}); });
            return std::move(his);
        }

        int _download_data() // Resolve
        {
            if (year_ < oldest_year_)
                return 0;

            std::string fn = (".year." + std::to_string(year_) + ".txt");
            if (boost::filesystem::exists(fn)) {
                --year_;
                return _download_data();
            }

            DBG_MSG("year =>%d, oldest %d", year_, oldest_year_);

            request_.consume(boost::asio::buffer_size(request_.data()));
            response_.consume(boost::asio::buffer_size(response_.data()));
            content_.consume(boost::asio::buffer_size(content_.data()));
            resovle();
            return 1;
        }
        // void on_success(Close) { }

        void on_success(Connect) {
            //year_ = oldest_year_;
            DBG_MSG(">query: %d", year_);
            //{
            //    boost::filesystem::ifstream in("/tmp/a.htm");
            //    std::ostream ob(&response_);
            //    ob << in.rdbuf();
            //    on_success(Query{});
            //}
            query(year_); //(path_ + std::to_string(2015)); //options();
        }
        void on_success(Query)
        {
            std::string y = "<td>" + std::to_string(year_) + "001</td>";
            std::istream ins(&content_);
            std::string line;
            bool line_b = 0;
            // int yno = 0;
            std::string codes;
            std::vector< std::string > his;
            boost::regex re_yno("<td>(\\d{7})</td>");
            boost::regex re_code("<td\\s+class=\".*\">(\\d+)</td>");
            while (std::getline(ins, line)) {
                boost::trim_right(line);
                // std::cout << line <<"\n";
                if (!line_b)
                    line_b = std::search(line.begin(),line.end(), y.begin(),y.end()) < line.end();
                if (!line_b)
                    continue;

                //<td class="cp_2">14</td>
                boost::smatch m;
                if (boost::regex_search(line, m, re_code)) {
                    char const* s = m[1].first.operator->(); // (m[1].first,m[1].second);
                    int c = atoi(s);
                    codes.push_back(char(c));

                //<td>2002002</td>
                } else if (boost::regex_search(line, m, re_yno)) {
                    // auto* s = m[1].first.operator->(); // (m[1].first,m[1].second);
                    std::sort(codes.begin(), codes.end());
                    // for (char c : codes) std::cout <<" "<< int(c); std::cout <<"\n"; // 
                    his.push_back( codes );
                    codes.clear();

                    //</tr></table>
                } else if (boost::ends_with(line, "</table>")) {
                    break;
                }
            } // while

            {
                std::string fn = (".year." + std::to_string(year_) + ".txt");
                if (FILE* fp = fopen(fn.c_str(), "w")) {
                    for (auto& s : his) {
                        for (int c : s)
                            fprintf(fp, "%d ", c);
                        fprintf(fp, "\n");
                    }
                    fclose(fp);
                }
            }

            this->close();
            --year_;
            get_io_service().post([this](){ _download_data(); });
        }

        template <typename A> void on_success(A) { DBG_MSG("success:A"); }

        template <typename A>
        void on_error(boost::system::error_code ec, A) {
            ERR_MSG("error: %d", ec.value());
            // TODO : deadline_timer reconnect
        }

        boost::signals2::signal<void()> sig_teardown;
    };
protected:
    http_client http_client_;
};

extern "C" void inittwiddle( int m, int n, int *p );
extern "C" int twiddle( int *x, int *y, int *z, int *p);

struct VMain : private Liuhc
{
    static void error_callback(int error, const char* description)
    {
        fprintf(stderr, "Error %d: %s\n", error, description);
    }

    template <typename Args>
    VMain(boost::asio::io_service& io_s, Args a) : Liuhc(io_s, a)
    {
#if defined(GLFW3_OPENGL)
        // Setup window
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            ERR_EXIT("glfwInit");
        window = glfwCreateWindow(1280, 720, "Marks Six", NULL, NULL);
        glfwMakeContextCurrent(window);

        // Setup ImGui binding
        ImGui_ImplGlfw_Init(window, true);
#else //if defined(SDL_OPENGL)
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            ERR_EXIT("%s\n", SDL_GetError());
        }
        // Setup window
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode(0, &current);
        window = SDL_CreateWindow("ImGui SDL2+OpenGL example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
        glcontext = SDL_GL_CreateContext(window);

        // Setup ImGui binding
        ImGui_ImplSdl_Init(window);
#endif

        // Load Fonts
        // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
        ImGuiIO& io = ImGui::GetIO();
		auto* fnt = "C:/Windows/Fonts/simsun.ttc";
        //auto* fnt = "/home/wood/.local/share/fonts/Monaco_Yahei.ttf"; //"/home/wood/.fonts/msyh.ttf"
        io.Fonts->AddFontFromFileTTF(fnt, 18.0f, NULL, io.Fonts->GetGlyphRangesChinese());
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
        //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
        //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        DBG_MSG("VMain:VMain");
    }
    ~VMain() {
        DBG_MSG("VMain:~VMain");
#if defined(GLFW3_OPENGL)
        ImGui_ImplGlfw_Shutdown();
        glfwTerminate();
#else //if defined(SDL_OPENGL)
        ImGui_ImplSdl_Shutdown();
        SDL_GL_DeleteContext(glcontext);  
        SDL_DestroyWindow(window);
        SDL_Quit();
#endif
    }

    void setup(int ac, char* av[]) {
        DBG_MSG("VMain:setup");
        Liuhc::setup(ac, av);
        this->update();
    }
    void teardown() { Liuhc::teardown(); DBG_MSG("VMain:teardown"); }

private:
    struct Combination {
        enum { N=49 };
        int M;
        int psv_[51], ofv_[49]; //, cv[7];

        Combination(int m=1) {
            M = m;
        }

        template <typename I>
        void first(I it) {
            inittwiddle(M, N, psv_);
            int i;//, x, y, z, psv_[52], ofv_[50];
            for(i = 0; i != N-M; i++) {
                ofv_[i] = 0;
            }
            while(i != N) {
                ofv_[i++] = 1;
            }
            copy_to(it);
        }
        template <typename I>
        int* next(I it)
        {
            int x,y,z;
            if (!twiddle(&x, &y, &z, psv_)) {
                ofv_[x] = 1;
                ofv_[y] = 0;
                copy_to(it);
                return ofv_;
            }
            return 0;
        }
        template <typename I>
        void copy_to(I it) {
            for(int i = 0; i != N; i++)
                if (ofv_[i])
                    *it++ = char(i+1); //putchar(b[i]? '1': '0');
        }
    };

    Combination comb_;
    std::map<int,std::vector<std::string>> mlis_;

    int his_travel(std::string const& s) {
        int nm = 0;
        std::string sm;
        for (auto& v : his_) {
            std::set_intersection(s.begin(),s.end(),v.begin(),v.end(), std::back_inserter(sm));
            nm += !sm.empty();
            sm.clear();
        }
        return nm;
    }

    const char* genlist(int marks)
    {
        if (his_.empty()) {
            his_ = http_client_.prepare_history_data();
            if (his_.empty()) {
                DBG_MSG("downloading...");
            }
            DBG_MSG("his: %u", his_.size());
        }
        mlis_.clear();
        comb_ = Combination(marks);

        std::string res;
        comb_.first(std::back_inserter(res));
        int cnt = his_travel(res);
        mlis_[cnt].push_back(res);

        int sa = 1;
        res.clear();
        while (comb_.next(std::back_inserter(res))) {
            cnt = his_travel(res);
            mlis_[cnt].push_back(res);
            ++sa;
            res.clear();
        }

        char const* resfn = "result_marks.txt";
        {
        //while (sa > 300) {
        //    auto it = --mlis_.end();
        //    if (sa - it->second.size() > 100) {
        //        sa -= it->second.size();
        //        mlis_.erase(it);
        //    } else break;
        //}
        //"=== 次数 总次数 码数码数, %u ==="
        if (FILE* fp = fopen(resfn, "w")) {
            std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);

            //fprintf(fp, "=== 次数, 码数 ===\r\n");
            fprintf(fp, "=== 出现次数 ===\r\n");
            int n = 0;
            for (auto& p: mlis_) {
                if (n++ > 300)
                    break;
                fprintf(fp, "=== %d ===\r\n", p.first); // , p.second.size()
                for (auto& s : p.second) {
                    for (int c : s)
                        fprintf(fp, "%.2d ", c);
                    fprintf(fp, "\r\n");
                }
                fprintf(fp, "\r\n");
            }
        }
        DBG_MSG("Done marks %d", marks);
        }
        return resfn;
    }

    void vMain()
    {
        //ImGui::BeginChild("SubA", ImVec2(0,40), true);
        //ImGui::Columns(5);
        ImGui::PushItemWidth(-1);
        if (ImGui::Button("一码")) {
            notepad_open(genlist(1));
        }
        //ImGui::NextColumn();
        if (ImGui::Button("两码")) {
            notepad_open(genlist(2));
        }
        //ImGui::NextColumn();
        if (ImGui::Button("三码")) {
            notepad_open(genlist(3));
        }
        //ImGui::NextColumn();
        if (ImGui::Button("四码")) {
            notepad_open(genlist(4));
        }
        //ImGui::NextColumn();
        if (ImGui::Button("五码")) {
            notepad_open(genlist(5));
        }
        ImGui::PopItemWidth();
        //ImGui::EndChild();

        //ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);
        //ImGui::BeginChild("Sub2", ImVec2(0,0), true);
        //ImGui::Text("With border");
        ////ImGui::Columns(3);
        //for (int i = 0; i < 90; i++) {
        //    if (i == 30 || i == 60)
        //        ImGui::NextColumn();
        //    char buf[32];
        //    sprintf(buf, "%08x", i*5731);
        //    ImGui::Button(buf, ImVec2(-1.0f, 0.0f));
        //}
        //ImGui::EndChild();
        //ImGui::PopStyleVar();
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"

        //{
        //    static float f = 0.0f;
        //    ImGui::Text("你好，时间!");
        //    ImGui::InputText("时间", buf_, sizeof(buf_));
        //    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        //    ImGui::ColorEdit3("clear color", (float*)&clear_color);
        //    if (ImGui::Button("Test Window"))
        //        show_test_window ^= 1;
        //    if (ImGui::Button("Another Window"))
        //        show_another_window ^= 1;
        //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        //}

        //// 2. Show another simple window, this time using an explicit Begin/End pair
        //if (show_another_window)
        //{
        //    ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
        //    ImGui::Begin("Another Window", &show_another_window);
        //    ImGui::Text("Hello");
        //    ImGui::End();
        //}

        //// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        //if (show_test_window)
        //{
        //    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        //    ImGui::ShowTestWindow(&show_test_window);
        //}
    }

#ifdef BOOST_MSVC
	GLFWwindow* window;
#else
    SDL_Window *window;
    SDL_GLContext glcontext;
#endif
    
    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    char buf_[64];

    void update()
    {
#if defined(GLFW3_OPENGL)
        if (!glfwWindowShouldClose(window)) {
            auto scoped_fn = [this](Liuhc*){
                int w, h;
                glfwGetFramebufferSize(this->window, &w, &h);
                glViewport(0, 0, w, h);
                glClearColor(this->clear_color.x, this->clear_color.y, this->clear_color.z, this->clear_color.w);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui::Render();
                glfwSwapBuffers(this->window);

                //DBG_MSG("scoped-fn");
                get_io_service().post([this](){ this->update(); });
            };
            std::unique_ptr<Liuhc,decltype(scoped_fn)> scoped(this,scoped_fn);
            glfwPollEvents();
            ImGui_ImplGlfw_NewFrame();

            vMain();
            (void)scoped;
            return;
        }
#else //if defined(SDL_OPENGL)
        static bool done_ = false;
        if (!done_) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSdl_ProcessEvent(&event);
                if (event.type == SDL_QUIT) {
                    done_ = true;
                }
            }
            auto scoped_fn = [this](Liuhc*){
                glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
                glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui::Render();
                SDL_GL_SwapWindow(window);

                get_io_service().post([this](){ this->update(); });
            };
            std::unique_ptr<Liuhc,decltype(scoped_fn)> scoped(this,scoped_fn);
            ImGui_ImplSdl_NewFrame(window);

            vMain();
            (void)scoped;
            return;
        }
#endif
        teardown();
        get_io_service().stop();
    }
};

//#include <boost/type_erasure/member.hpp>
//BOOST_TYPE_ERASURE_MEMBER((setup_fn), setup, 2)
// BOOST_TYPE_ERASURE_MEMBER((setup_fn), ~, 0)
//boost::type_erasure::any<setup_fn<int(int,char*[])>, boost::type_erasure::_self&> ;

template <typename Instance, typename Args>
struct Main : boost::asio::io_service, boost::noncopyable
{
    typedef Main<Instance,Args> This;
    struct Interface {
        virtual ~Interface() {}
        virtual void teardown() = 0;
        virtual void setup(int,char*[]) = 0;
    };
    struct Wrapper : Interface {
        Instance obj_;
        Wrapper(This& m, Args a) : obj_(m, a) {}
        virtual void teardown() { obj_.teardown(); }
        virtual void setup(int ac,char* av[]) { obj_.setup(ac,av); }
    };

    ~Main() { reinterpret_cast<Wrapper*>(&objmem_)->~Wrapper(); }

    Main(int ac, char* av[]) {
        new (&objmem_) Wrapper(*this, Args(ac, av));
    }
    int run(int ac, char* av[]) {
        reinterpret_cast<Wrapper*>(&objmem_)->setup(ac,av);
        DBG_MSG(":run");
        return boost::asio::io_service::run();
    }
    void stop() {
        reinterpret_cast<Wrapper*>(&objmem_)->teardown();
        boost::asio::io_service::stop();
    }

    void renew(Args a) {
        reinterpret_cast<Wrapper*>(&objmem_)->teardown();
        reinterpret_cast<Wrapper*>(&objmem_)->~Wrapper();
        new (&objmem_) Wrapper(*this, std::move(a));
        reinterpret_cast<Wrapper*>(&objmem_)->setup(a.argc,a.argv);
    }
private:
    static int objmem_[sizeof(Wrapper)/sizeof(int)+1]; // static Wrapper obj_;
};
template <typename Instance, typename Args> int Main<Instance,Args>::objmem_[sizeof(Wrapper)/sizeof(int)+1] = {};

struct Args
{
    int argc; char** argv;
    Args(int ac, char* av[]) : argc(ac), argv(av)
    {}

    std::string remote_host = "www.y1118.com"; // + "2010"
    std::string remote_path = "/xin-index-1.html?year="; // + "2010"
};

int main(int argc, char* argv[])
{
    //BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END
    try {
        Main<VMain,Args> s(argc, argv);
        //s.setup(argc,argv);
        boost::asio::signal_set sigs(s);
        sigs.add(SIGINT);
        sigs.add(SIGTERM); // (SIGQUIT);
        sigs.async_wait( [&s](boost::system::error_code,int){ s.stop(); } );
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


//# boost::filesystem::path full_path( boost::filesystem::current_path() );
//# std::cout << "Current path is : " << full_path << std::endl;
//# namespace fs = boost::filesystem;
//# 
//# fs::path full_path = fs::system_complete("../asset/toolbox");
//# 
//# 			ShellExecuteA(GetDesktopWindow(), "open", "hello.txt", NULL, NULL, SW_SHOW);
//# 
//#include <windows.h>
//#include <ShellApi.h>
//# 			ShellExecuteA(GetDesktopWindow(), "open", "hello.txt", NULL, NULL, SW_SHOW);

