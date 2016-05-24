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
#include <iostream>

namespace ip = boost::asio::ip;

template <typename... As> void err_exit_(int lin_, char const* fmt, As... a) {
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
                            bchunked = (boost::equals(m[2], std::string("chunked")));
                        } else if (boost::starts_with(line, "Content-Encoding")) {
                            gzip_ = (boost::equals(m[2], std::string("gzip")));
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

    //std::vector< std::string > his_; // std::vector<int> counts_; // int last_year_count_ = 0;
    std::vector<std::string> vhis0_;
    std::vector<std::vector<std::string>> vhis1_; // std::vector<int> counts_; // int last_year_count_ = 0;

    template <typename Args>
    Liuhc(boost::asio::io_service& io_s, Args&& a) //, std::string remote_host, std::string remote_path
        : http_client_(this, io_s, a.remote_host, a.remote_path)
    {}

    void setup(int, char*[]) {
        http_client_.change_n_years(6);
        if (http_client_._download_data() == 0) {
            vhis0_ = http_client_.prepare_history_data(vhis1_);
        }
        DBG_MSG("his: %u", vhis0_.size());
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

        std::vector<std::string> prepare_history_data(std::vector<std::vector<std::string>>& vhis) // Prepare
        {
            if (year_ >= oldest_year_) {
                return std::vector<std::string>();
            }
            std::vector<std::string> his0;
            std::vector<std::string> his1;
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
                    his1.push_back( make_string(c.begin(),c.end()) );
                }
                his0.insert(his0.end(), his1.begin(), his1.end());
                vhis.push_back(std::move(his1));
            }
            //this->get_io_service().post([this](){ this->on_success(http_stages::Prepare{}); });
            return std::move(his0);
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

struct VMain : boost::asio::io_service , Liuhc
{
    template <typename Args>
    VMain(Args a) : Liuhc(*this, a) {
        DBG_MSG("VMain:VMain");
    }
    ~VMain() {
        DBG_MSG("VMain:~VMain");
    }

    void setup(int ac, char* av[]) {
        DBG_MSG("VMain:setup");
        Liuhc::setup(ac, av);
        //this->update();
    }
    void teardown() { Liuhc::teardown(); DBG_MSG("VMain:teardown"); }

    typedef std::map<int,std::vector<std::string>, std::greater<int>> comb_res_type;

    static comb_res_type _comb(int marks, std::vector<std::string> const& his)
    {
        comb_res_type mres;
        auto combr = Combination(marks);

        std::string res;
        combr.first(std::back_inserter(res));
        if (!res.empty()) {
            int cnt = his_travel(his, res);
            mres[cnt].push_back(res);
            res.clear();
        }

        //int sa = 1;
        while (combr.next(std::back_inserter(res))) {
            int cnt = his_travel(his, res);
            mres[cnt].push_back(res);
            res.clear();
            //++sa;
        }
        return std::move(mres);
    }

	const char* genlist(int marks)
	{
		if (vhis1_.empty()) {
			vhis0_ = http_client_.prepare_history_data(vhis1_);
			if (vhis1_.empty()) {
				DBG_MSG("downloading...");
                return nullptr;
			}
			DBG_MSG("his: %u", vhis0_.size());
		}

        static char resfn[32] = {};
        /*for (unsigned i = vhis1_.size(); i>0; --i) */{
            comb_res_type mres = _comb(marks, vhis0_);//(marks, vhis1_[i-1]);

            snprintf(resfn,sizeof(resfn), "result-%02d.txt", marks);//(, 2011+i);
            {
                if (FILE* fp = fopen(resfn, "w")) {
                    std::unique_ptr<FILE, decltype(&fclose)> xclose(fp, fclose);

                    int first_n = 0;
                    for (auto& p : mres) {
                        for (auto& s : p.second) {
                            fprintf(fp, "%3d: ", p.first);
                            for (int c : s)
                                fprintf(fp, "%.2d ", c);
                            fprintf(fp, ":");
                            for (auto& y1 : vhis1_)
                                fprintf(fp, "\t%d", his_travel(y1, s));
                            fprintf(fp, "\r\n");
                            if (++first_n >= 1000)
                                break;
                        }
                        fprintf(fp, "\r\n");
                        if (first_n >= 1000)
                            break;
                    }
                }
                DBG_MSG("Done marks %d", marks);
            }
		}
		return resfn;
	}

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

    static int his_travel(std::vector<std::string> const& his, std::string const& s) {
        int nm = 0;
        std::string tmp;
        for (auto& v : his) {
            std::set_intersection(s.begin(),s.end(),v.begin(),v.end(), std::back_inserter(tmp));
            nm += !tmp.empty();
            tmp.clear();
        }
        return nm;
    }

};

//#include <boost/type_erasure/member.hpp>
//BOOST_TYPE_ERASURE_MEMBER((setup_fn), setup, 2)
// BOOST_TYPE_ERASURE_MEMBER((setup_fn), ~, 0)
//boost::type_erasure::any<setup_fn<int(int,char*[])>, boost::type_erasure::_self&> ;

template <typename Instance, typename Args>
struct Main : boost::noncopyable
{
    ~Main() { instance()->~Instance(); }
    Main(int ac, char* av[]) {
        new (&objmem_) Instance(Args(ac, av));
    }

    Instance* instance() { return reinterpret_cast<Instance*>(&objmem_); }
    Instance* operator->() { return instance(); }
    Instance& operator*() { return *instance(); }

    int run(int ac, char* av[]) {
        instance()->setup(ac,av);
        DBG_MSG(":run");
        return boost::asio::io_service::run();
    }
    void stop() {
        instance()->teardown();
        boost::asio::io_service::stop();
    }

    void renew(Args a) {
        instance()->teardown();
        instance()->~Instance();
        new (&objmem_) Instance(a);
        instance()->setup(a.argc,a.argv);
    }

private:
    static int objmem_[sizeof(Instance)/sizeof(int)+1]; // static Wrapper obj_;
};
template <typename Instance, typename Args> int Main<Instance,Args>::objmem_[sizeof(Instance)/sizeof(int)+1] = {};

struct Args
{
    int argc; char** argv;
    Args(int ac, char* av[]) : argc(ac), argv(av)
    {}

    std::string remote_host = "www.y1118.com"; // + "2010"
    std::string remote_path = "/xin-index-1.html?year="; // + "2010"
};

static Main<VMain,Args> *g_;
static const char* genlist_(int marks) { return g_->instance()->genlist(marks); }
static void poll_one_() { g_->instance()->poll_one(); }
static void stop_() { g_->instance()->stop(); }

extern "C" {
int gui_main(char const* (*gen_)(int), void (*poll_)(), void(*stop_)());
}

int WINAPI WinMain(
	HINSTANCE hInstance,       //程序当前实例的句柄，以后随时可以用GetModuleHandle(0)来获得  
	HINSTANCE hPrevInstance,   //这个参数在Win32环境下总是0，已经废弃不用了  
	char * lpCmdLine,          //指向以/0结尾的命令行，不包括EXE本身的文件名，  
							   //以后随时可以用GetCommandLine()来获取完整的命令行  
	int nCmdShow               //指明应该以什么方式显示主窗口  
	)//;int main(int argc, char* argv[])
{
    //BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END
    try {
		int argc = 0; char** argv = 0;
        Main<VMain,Args> s(argc, argv);
        //s.setup(argc,argv);
        boost::asio::signal_set sigs(*s);
        sigs.add(SIGINT);
        sigs.add(SIGTERM); // (SIGQUIT);
        sigs.async_wait( [&s](boost::system::error_code,int){ s->stop(); } );

        s->setup(argc,argv);
        gui_main(&genlist_, &poll_one_, &stop_);
        // s.run(argc, argv);
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

