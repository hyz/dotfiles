// #include <sys/types.h> #include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include <cstdlib>
#include <deque>
//#include <iostream>
#include <boost/static_assert.hpp>
#define BOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/signals2/signal.hpp>
//#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/intrusive/slist.hpp>
#include <thread>
#include <mutex>
#include <chrono> //#include <boost/chrono.hpp>

#if defined(__ANDROID__)
#  include <regex> //<boost/regex.hpp>
namespace re = std;
#else
#  include <boost/regex.hpp>
namespace re = boost;
#endif
namespace chrono = std::chrono;
namespace ip = boost::asio::ip;

#if defined(__ANDROID__)
#  include <android/log.h>
#  define LOG_TAG    "HGSC"
#  define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#  define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#  define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#  define ERR_EXIT(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#  define NOT_PRINT_PROTO 1

#else // !__ANDROID__

template <typename... As> void err_exit_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... As> void err_msg_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
}
#  define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
#  define LOGD(...) err_msg_(__LINE__, "D:%d: " __VA_ARGS__)
#  define LOGW(...) err_msg_(__LINE__, "W:%d: " __VA_ARGS__)
#  define LOGE(...) err_msg_(__LINE__, "E:%d: " __VA_ARGS__)
#endif

template <typename I1, typename I2>
void base64dec(I1 beg, I1 end, I2 out_it)
{
    while (end != beg && *(end-1) == '=')
        --end;
    using namespace boost::archive::iterators;
    using Iter = transform_width<binary_from_base64<I1>,8,6>;
    std::copy(Iter(beg), Iter(end), out_it);
    //return boost::algorithm::trim_right_copy_if( std::string(Iter(beg), Iter(end)), [](char c) { return c == '\0'; });
}

struct rtsp_client
{
    ip::tcp::socket tcpsock_;
    ip::tcp::endpoint endpoint_;
    std::string path_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    std::string session_;

    unsigned char cseq_ = 1;
    unsigned char state_ = 0;
    enum { Stopped=0, Setting=1, Ready, Playing, Error };

    rtsp_client(boost::asio::io_service& io_s, ip::tcp::endpoint ep, std::string path)
        : tcpsock_(io_s), endpoint_(ep), path_(path)
    {
        //BOOST_STATIC_ASSERT(std::is_base_of<This, Derived>::value);
        LOGD("rtsp_client");
    }

    void setup(int, char* const[]) {
        state_ = Setting;
        auto& io_s = tcpsock_.get_io_service();
        boost::asio::spawn(io_s, [this](boost::asio::yield_context yield) {
            LOGD("setup ...");
            boost::system::error_code ec;
            if ( !(ec = do_connect(ec, yield))
                    && !(ec = do_options(ec, yield))
                    && !(ec = do_describe(ec, yield))
                    && !(ec = do_setup(ec, yield)) ) {
                state_ = Ready;
                LOGD("setup OK"); //;
            } else {
                state_ = Error;
                LOGD("setup Fail: %d:%s", ec.value(), ec.message().c_str());
            }
        });
    }

    void play() {
        auto& io_s = tcpsock_.get_io_service();
        boost::asio::spawn(io_s, [this](boost::asio::yield_context yield) {
            boost::system::error_code ec;
            if ( !(ec = do_play(ec, yield)) ) {
                state_ = Playing;
            } else {
                state_ = Error;
                LOGD("setup fail: %d:%s", ec.value(), ec.message().c_str());
            }
        });
    }

    void teardown() {
        auto& io_s = tcpsock_.get_io_service();
        boost::asio::spawn(io_s, [this](boost::asio::yield_context yield) {
            boost::system::error_code ec;
            if ( !(ec = do_teardown(ec, yield)) ) {
                state_ = Stopped;
            } else {
                state_ = Error;
                LOGD("setup fail: %d:%s", ec.value(), ec.message().c_str());
            }
            tcpsock_.close(ec);
        });
    }

    bool is_closed() const { return (state_==Stopped && !tcpsock_.is_open()) || state_==Error; }

private:
    boost::system::error_code do_connect(boost::system::error_code ec, boost::asio::yield_context yield)
    {
        LOGD("connect %s:%d", endpoint_.address().to_string().c_str(), endpoint_.port());
        tcpsock_.async_connect( endpoint_, yield[ec] );
        return ec;
    }

    boost::system::error_code do_options(boost::system::error_code ec, boost::asio::yield_context yield)
    {
        if (!ec) {{
            std::ostream outs(&request_);
            outs << "OPTIONS " << path_ << " RTSP/1.0\r\n"
                << "CSeq: " << int(cseq_++) << "\r\n"
                << "\r\n";
            }
            boost::asio::async_write(tcpsock_, request_, yield[ec] );
        }
        return read_response(ec, yield);
    }

    boost::system::error_code do_describe(boost::system::error_code ec, boost::asio::yield_context yield)
    {
        if (!ec) {{
            std::ostream outs(&request_);
            outs << "DESCRIBE " << path_ << " RTSP/1.0\r\n"
                << "CSeq: " << int(cseq_++) << "\r\n"
                << "Accept: application/sdp\r\n"
                << "\r\n";
            }
            boost::asio::async_write(tcpsock_, request_, yield[ec] );
        }
        return read_response(ec, yield);
    }

    int local_port(int x=0) { return 1234+x; }
    boost::system::error_code do_setup(boost::system::error_code ec, boost::asio::yield_context yield) //Setup
    {
        if (!ec) {
            std::string streamid, sps, pps;
            if ( !(ec =parse_rsp_describe(streamid, sps, pps)) ) {{
                std::ostream outs(&request_);
                outs << "SETUP " << path_ <<"/"<< streamid << " RTSP/1.0\r\n"
                    << "CSeq: " << int(cseq_++) << "\r\n"
                    << "Transport: RTP/AVP;unicast;client_port="<< local_port() <<"-"<< local_port(+1) << "\r\n"
                    << "Range: 0.00-\r\n"
                    << "\r\n";
                }
                this; //sps, pps;
                //LOGD("setup: %s/%s\n\t%s", path_.c_str(), streamid.c_str(), transport.c_str());
                boost::asio::async_write(tcpsock_, request_, yield[ec] );
            }
        }
        return read_response(ec, yield);
    }

    void start_playing() {}
    boost::system::error_code do_play(boost::system::error_code ec, boost::asio::yield_context yield)
    {
        if (!ec) {
            std::string session;
            if ( !(ec =parse_rsp_setup(session)) ) {{
                this->start_playing();

                std::ostream outs(&request_);
                outs << "PLAY " << path_ << " RTSP/1.0\r\n"
                    << "CSeq: " << int(cseq_++) << "\r\n"
                    << "Session: " << session_ << "\r\n"
                    << "\r\n";
                }
                boost::asio::async_write(tcpsock_, request_, yield[ec] );
            }
        }
        return read_response(ec, yield);
    }

    boost::system::error_code do_teardown(boost::system::error_code ec, boost::asio::yield_context yield)
    {
        if (!ec) {{
            std::ostream outs(&request_);
            outs << "TEARDOWN " << path_ << " RTSP/1.0\r\n"
                << "CSeq: " << int(cseq_++) << "\r\n"
                << "Session: " << session_ << "\r\n"
                << "\r\n";
            }
            boost::asio::async_write(tcpsock_, request_, yield[ec] ); // boost::asio::write(tcpsock_, request_);
        }
        return ec;
    }

    //std::string sps_, pps_;

    boost::system::error_code parse_rsp_describe(std::string& streamid, std::string& sps, std::string& pps)
    {
        LOGD("parse:Describe");
        // std::string sps, pps; //, streamid;
        std::istream ins(&response_);
        std::string line;
        bool m_v = 0;
        while (std::getline(ins, line)) {
            boost::trim_right(line);
            LOGD("%d: %s", __LINE__, line.c_str());
            if (boost::starts_with(line, "m=")) {
                m_v = boost::starts_with(line, "m=video");
            } else if (m_v) {
                if (boost::starts_with(line, "a=fmtp:")) {
                    re::smatch m; // fmtp:96 profile-level-id=42A01E;packetization-mode=1;sprop-parameter-sets=
                    re::regex re("sprop-parameter-sets=([^=,]+)=*,([^=,;]+)");
                    if (re::regex_search(line, m, re)) {
                        base64dec(m[1].first, m[1].second, std::back_inserter(sps));
                        base64dec(m[2].first, m[2].second, std::back_inserter(pps));
                    }
                } else if (boost::starts_with(line, "a=control:")) {
                    re::smatch m;
                    re::regex re("a=control:([^=]+=[0-9]+)");
                    if (re::regex_search(line, m, re)) {
                        streamid.assign( m[1].first, m[1].second );
                    }
                }
            }
        }
        return boost::system::error_code();
    }
    boost::system::error_code parse_rsp_setup(std::string& session)
    {
        LOGD("parse:Setup");
        std::istream ins(&response_);
        std::string line;
        while (std::getline(ins, line)) {
            boost::trim_right(line);
            LOGD("%d: %s", __LINE__, line.c_str());
            if (boost::starts_with(line, "Session:")) {
                re::smatch m;
                re::regex re("Session:[[:space:]]*([^[:space:]]+)");
                if (re::regex_search(line, m, re)) {
                    session_.assign(m[1].first, m[1].second);
                }
            }
        }
        return boost::system::error_code();
    }

    boost::system::error_code read_response(boost::system::error_code ec, boost::asio::yield_context yield)
    {
        if (!ec) {
            response_.consume(response_.size());
            boost::asio::async_read_until(tcpsock_, response_, "\r\n\r\n", yield[ec]);
            if (ec) {
                LOGD("resp: %d:%s", ec.value(), ec.message().c_str());
                return ec;
            }
            size_t clen = size_t(-1);
            auto  bufs = response_.data();
            auto* beg = boost::asio::buffer_cast<const char*>(bufs);
            auto* end = beg + boost::asio::buffer_size(bufs);
            decltype(end) eol, p = beg;
            while ( (eol = std::find(p,end, '\n')) != end) {
                auto* e = eol++;
                while (e != p && isspace(*e))
                    --e;
                auto linr = boost::make_iterator_range(p,e+1);
                if (boost::istarts_with(linr, "Content-Length")) {
                    re::regex re_header_("^([^:]+):\\s+(.+)$");
                    re::cmatch m;
                    if (re::regex_match(p,e, m, re_header_)) {
                        clen = atoi(m[2].first);
                        //std::clog << "Content-Length " << clen << "\n";
                    }
                }
                LOGD("%.*s\n", int(e-p), p);
                if (p == e)
                    break;
                p = eol;
            }
            // response_.consume(eol - beg);

            if (response_.size() < clen) {
                if (clen != size_t(-1)) {
                    size_t n_bytes = clen - response_.size();
                    boost::asio::async_read(tcpsock_, response_.prepare(n_bytes), yield[ec]);
                    if (!ec)
                        response_.commit(n_bytes);
                }
                if (ec) {
                    LOGD("rd-cont: %d:%s", ec.value(), ec.message().c_str());
                    return ec;
                }
            }
        }
        return ec;
    }
};

namespace posix = boost::asio::posix;
struct user_input : boost::noncopyable
{
    rtsp_client* c_;
    posix::stream_descriptor input_;
    boost::asio::streambuf input_buffer_;

    user_input(boost::asio::io_service& io_s, rtsp_client& c)
        : input_(io_s, ::dup(STDIN_FILENO))
    {
        c_ = &c;
        singlton = this;
    }

    void setup(int, char* const[]) {
        auto& io_s = input_.get_io_service();
        boost::asio::spawn(io_s, [this](boost::asio::yield_context yield) {
            LOGD("waiting input ...");
            boost::system::error_code ec;
            while ( !(ec = do_read_line(ec, yield)) ) {
                //LOGD("read-line Fail: %d:%s", ec.value(), ec.message().c_str());
            }
        });
    }

    void teardown() {
        c_->teardown();
        auto& io_s = input_.get_io_service();
        io_s.stop();
    }

    static boost::iterator_range<char const*> cptr_trim(char const* p, char const* e) {
        while (e != p && isspace(*(e-1)))
            --e;
        while (p != e && isspace(*p))
            ++p;
        return boost::make_iterator_range(p,e);
    };

    boost::system::error_code do_read_line(boost::system::error_code ec, boost::asio::yield_context yield) {
        boost::asio::async_read_until(input_, input_buffer_, '\n', yield[ec]);
        if (!ec) {
            auto  bufs = input_buffer_.data();
            auto* beg = boost::asio::buffer_cast<const char*>(bufs);
            auto* end = beg + boost::asio::buffer_size(bufs);
            auto* eol = std::find(beg,end, '\n') + 1;
            auto word = cptr_trim(beg, eol);
            if (boost::equals(word, "quit")) {
                c_->teardown();
                // break;
            } else if (boost::equals(word, "play")) {
                c_->play();
            } else if (boost::equals(word, "setup")) {
                c_->setup(0,0);
            }
            input_buffer_.consume(eol - beg);
        }
        return ec;
        //boost::trim_right(line);
    }

    static user_input* singlton;
};
user_input* user_input::singlton = 0;

#include <signal.h>
void sigh(int) {
    if (user_input::singlton)
        user_input::singlton->teardown();
}

int main(int argc, char*const argv[]) {
    signal(SIGINT, sigh);

    ip::tcp::endpoint ep( ip::address::from_string(argv[1]), atoi(argv[2]));
    std::string path = argv[3];

    boost::asio::io_service io_s;
    rtsp_client c(io_s, ep, path);
    user_input ui(io_s, c);

    ui.setup(argc,argv);
    io_s.run();
    io_s.reset();
    int runc_=300;
    while (--runc_ > 1) {
        io_s.poll_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

