
#include <syslog.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/format.hpp>
#include "MTNMsgApi.h"
#include "EUCPCInterface.h"
#include "MTNProtocol.h"
#include "util.h"
#include "jss.h"
#include "log.h"

namespace sys = boost::system;
namespace placeholders = boost::asio::placeholders;
using boost::asio::ip::tcp;

#define APPLE_HOST          "gateway.sandbox.push.apple.com"
#define APPLE_PORT          "2195"
#define APPLE_FEEDBACK_HOST "feedback.sandbox.push.apple.com"
#define APPLE_FEEDBACK_PORT "2196"
#define RSA_CLIENT_KEY      "PushChatKey.pem"
#define RSA_CLIENT_CERT     "PushChatCert.pem"


static std::string certs_dir_ = "etc/moon.d";

tcp::endpoint resolv(boost::asio::io_service & io_service, const std::string& h, const std::string& p)
{
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(h, p);
    tcp::resolver::iterator i = resolver.resolve(query);
    return *i;
}

static std::string unhex(const std::string & hs)
{
    std::string buf;
    const char* s = hs.data();
    for (unsigned int x = 0; x+1 < hs.length(); x += 2)
    {
        char h = s[x];
        char l = s[x+1];
        h -= (h <= '9' ? '0': 'a'-10);
        l -= (l <= '9' ? '0': 'a'-10);
        buf.push_back( char((h << 4) | l) );
    }
    return buf;
}

template <typename Int> static std::string& pkint(std::string & pkg, Int x)
{
    Int i;
    switch (sizeof(Int)) {
        case sizeof(uint32_t): i = htonl(x); break;
        case sizeof(uint16_t): i = htons(x); break;
        default: i = x; break;
    }
    char *pc = (char*)&i;
    pkg.insert(pkg.end(), &pc[0], &pc[sizeof(Int)]);
    return pkg;
}

static std::string & pkstr(std::string & pkg, const std::string & s)
{
    uint16_t len = s.length();
    len = htons(len);
    char *pc = (char*)&len;
    pkg.insert(pkg.end(), &pc[0], &pc[sizeof(uint16_t)]);
    return (pkg += s);
}

std::string pkaps(std::string const & tok, int id, std::string const & aps)
{
    BOOST_ASSERT(tok.length() == 64);

    std::string buf; //(len, 0);
    pkstr(pkstr(pkint(pkint(pkint( buf
            , uint8_t(1)) // command
            , uint32_t(id)) // /* provider preference ordered ID */
            , uint32_t(time(NULL)+(60*60*6))) // /* expiry date network order */
            , unhex(tok)) // binary 32bytes device token
            , aps)
            ;
    return buf;
}

struct apple_push_data
{
    struct push_data
    {
        push_data(std::string const & tok, int id, std::string const & cont)
            : tok_(tok), cont_(cont)
        {
            id_ = id;
        }

        int id_;
        std::string tok_;
        std::string cont_;
    };

    apple_push_data() {}
    ~apple_push_data()
    {
        //LOG_I << static_cast<void*>(this) <<" "<< push_datas_.empty() <<" "<< push_datas_.empty();
    }

    std::list<push_data>& push_datas() { return push_datas_; }
    std::list<std::string>& response_datas() { return response_datas_; }

    sys::error_code on_REQUEST(http_server::connection::message const & m)
    {
        LOG_I << m << "|" << m.content();

        std::string tok = get(m.params(), "to");
        int id = get<int>(m.params(), "id");

        push_datas_.push_back( push_data(tok, id, m.content()) );
        return sys::error_code();
    }

    sys::error_code callback(sys::error_code const & ec)
    {
        //std::string cont = boost::str(boost::format("%1%\t%2%") % ec.value() % ec.message() )
        response rsp(200);
        rsp.content(boost::format("%1% %2%") % ec.value() % ec.message());
        response_datas_.push_back(rsp.encode());
        LOG_I << response_datas_.front();

        push_datas_.pop_front();
        return sys::error_code();
    }

    static apple_push_data resolv(http_server::connection::message const & m) { return apple_push_data(); }

private:
    std::list<push_data> push_datas_;
    std::list<std::string> response_datas_;
};

#define PERM_PASSWORD "abc123"
std::string getpwd( std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) { return PERM_PASSWORD; }

struct apple_push : boost::noncopyable
{
#define DESTROY(i) this->destroy(i,__LINE__,__FUNCTION__)
    typedef apple_push this_type;

    struct stage { enum type {
            error = 0,
            connecting = 1, handshaking,
            idle,
            writing,
        };
    };

    struct ap_context;
    // typedef boost::shared_ptr<ap_context> ap_context_ptr;
    typedef std::list<ap_context> context_list;
    typedef std::list<ap_context>::iterator iterator;
    typedef std::list<iterator>::iterator piterator;

    struct ap_context
    {
        apple_push_data data_;

        explicit ap_context(boost::asio::io_service & ios)
            : _timer(new boost::asio::deadline_timer(ios))
        {
            _c = 0;
            response_writing = push_writing = false;
        }

        boost::shared_ptr<boost::asio::deadline_timer> _timer;

        http_server::connection* _c;
        piterator pi_;
        bool response_writing;
        bool push_writing;
        bool eof;
        bool _[1];
    };

    static 
    boost::asio::ssl::context & init_context(boost::asio::ssl::context & ctx)
    {
        // ctx.set_password_callback(getpwd);
        // LOG_I << certs_dir_ + "/" + RSA_CLIENT_CERT;

        // ctx.use_certificate_file(certs_dir_ + "/" + RSA_CLIENT_CERT, boost::asio::ssl::context::pem);
        // ctx.use_private_key_file(certs_dir_ + "/" + RSA_CLIENT_KEY, boost::asio::ssl::context::pem);
        return ctx;
    }

    apple_push(boost::asio::io_service & io_service, tcp::endpoint const & ep)
        : thread_deadline_(thread_io_service_)
        , context_(boost::asio::ssl::context::sslv3)
        , socket_(io_service, init_context(context_))
        , timer_(io_service)
        , endpoint_(ep)
    {
        stage_ = stage::idle;
        // stage_ = stage::connecting;
        // timed_connect(sys::error_code());

        thread_timed(sys::error_code());
        thread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &thread_io_service_)) ;

        io_service_ = &io_service;
    }

    ~apple_push()
    {
        sys::error_code ec;
        thread_deadline_.cancel(ec);
        thread_io_service_.post(boost::bind(&boost::asio::io_service::stop, &thread_io_service_));
        thread_.join();
        LOG_I << __FILE__;
    }

    http_server::connection::message_handler message(http_server::connection::message const& m, http_server::connection& c)
    {
        ap_context ctx(c.get_io_service());
        ctx.data_ = apple_push_data::resolv(m);
        iterator i = contexts_.insert(contexts_.end(), ctx);
        // i->i_ = i;
        i->pi_ = push_list_.end();
        i->_c = &c;
        pmessage(i, m);
        return boost::bind(&apple_push::pmessage, this, i, _1);
    }

private:
    void thread_timed(sys::error_code const & ec)
    {
        if (ec)
        {
            ::syslog(LOG_PID|LOG_INFO, "%d %d %s", ec.value(), (ec == boost::asio::error::operation_aborted), __FUNCTION__);
            return;
        }

        //if (!thread_data_.empty())
        //{
        //    thread_io_service_.post(boost::bind(&this_type::thread_write, this));
        //}

        thread_deadline_.expires_at(boost::posix_time::pos_infin);
        thread_deadline_.async_wait(boost::bind(&this_type::thread_timed, this, placeholders::error));
    }

    void thread_push(apple_push_data::push_data data)
    {
        // BOOST_ASSERT(thread_data_.empty());
        // ::syslog(0, "%s", __FUNCTION__);

        //thread_data_ = data.cont_;
        //thread_data_to_ = data.tok_;
        _thread_write(data.tok_, data.cont_);

        //thread_deadline_.expires_from_now(boost::posix_time::milliseconds(10));
        //thread_deadline_.async_wait(boost::bind(&this_type::thread_timed, this, placeholders::error));
    }

    void _thread_write(const std::string& to, const std::string& cont)
    {
        const char* sn = "3SDK-EMY-0130-PEURL"; //"0SDK-EBB-0130-NEVOL";
        const char* key = "3fc2719ce96ebdf42b6dc1a1d8303e1b";
        const char* priority = "1";

        std::string gbkcont(cont.size() * 2, '\0');
        wiconv(gbkcont, "GBK", cont, "UTF-8");

        //LOG_I << format("%s %s %s") % phone % sn % key;

        int ret1=0, ret2 = 0;
        ret1 = MTNMsgApi_NP::SetKey_NoMD5(const_cast<char*>(key));
        ret2 = MTNMsgApi_NP::SendSMS(const_cast<char*>(sn)
                , const_cast<char*>(to.c_str())
                , const_cast<char*>(gbkcont.c_str())
                , const_cast<char*>(priority)
                );
        //LOG_I << format("%d %d") % ret1 % ret2;
        ::syslog(LOG_PID|LOG_INFO, "%s[%s] %d %d", to.c_str(), cont.c_str(), ret1, ret2);

        sys::error_code ec;
        io_service_->post(boost::bind(&this_type::handle_push_write, this, ec));
        // thread_data_.clear();
    }

    //std::string thread_data_to_;
    //std::string thread_data_;
    boost::asio::io_service thread_io_service_;
    boost::asio::deadline_timer thread_deadline_;
    boost::thread thread_;

    boost::asio::io_service *io_service_;

private:
    void pmessage(iterator i, http_server::connection::message const & m)
    {
        LOG_I << stage_;

        BOOST_ASSERT(i != contexts_.end());
        if (m.error())
        {
            LOG_I << m.error() <<" "<< m.error().message();
            if (m.error() == boost::asio::error::eof)
            {
                i->eof = true;
                if (boost::empty(i->data_.push_datas())
                        && boost::empty(i->data_.response_datas()))
                {
                    DESTROY(i);
                }
            }
            else
            {
                i->_c = 0;
                DESTROY(i);
            }
            return;
        }

        sys::error_code ec = i->data_.on_REQUEST( m );
        if (ec)
        {
            DESTROY(i);
            return;
        }

        async_push_append(i);
        async_push();
        async_response(i);
    }

    void destroy(iterator i, int ln,char const *fn)
    {
        LOG_I << ln <<":"<< fn;
        if (i->_c)
        {
            i->_c->notify_close(ln,fn);
            i->_c = 0;
        }

        sys::error_code ec;
        i->_timer->cancel(ec);

        LOG_I << i->pi_.operator->() <<" "<< push_list_.end().operator->();
        // LOG_I << i->i_.operator->() <<" "<< contexts_.end().operator->();

        if (i->pi_ != push_list_.end())
            push_list_.erase(i->pi_);
        contexts_.erase(i);
    }

    void async_response(iterator i)
    {
        LOG_I << i->_c <<" "<< i->response_writing;
        if (!i->_c || i->response_writing)
            return;
        if (boost::empty(i->data_.response_datas()))
        {
            if (boost::empty(i->data_.push_datas()))
                if (i->eof)
                    DESTROY(i);
            return;
        }

        i->response_writing = 1;
        boost::asio::async_write(i->_c->socket()
                , boost::asio::buffer(*boost::begin(i->data_.response_datas()))
                , boost::bind(&this_type::handle_rsp_write, this, i, placeholders::error));
    }

    void handle_rsp_write(iterator i, sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message();
        i->response_writing = 0;
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message();
            DESTROY(i);
            return; //if (ec == boost::asio::error::operation_aborted)
        }

        BOOST_ASSERT(!boost::empty(i->data_.response_datas()));
        i->data_.response_datas().pop_front();

        async_response(i);
    }

    void async_push_append(iterator i)
    {
        LOG_I << __FUNCTION__;
        if (boost::empty(i->data_.push_datas()))
            return;
        if (i->pi_ == push_list_.end())
        {
            i->pi_ = push_list_.insert(push_list_.end(), i);
        }
    }

    void async_push()
    {
        LOG_I << __FUNCTION__;
        if (stage_ != stage::idle)
            return;

        while (!push_list_.empty()
                && boost::empty(push_list_.front()->data_.push_datas()))
        {
            push_list_.front()->pi_ = push_list_.end();
            push_list_.pop_front();
        }
        if (push_list_.empty())
            return;

        iterator i = push_list_.front();
        //BOOST_ASSERT(i == i->i_);

        i->push_writing = true;
        stage_ = stage::writing;
        thread_io_service_.post(boost::bind(&this_type::thread_push, this, *boost::begin(i->data_.push_datas())));
        // boost::asio::async_write(socket_
        //         , boost::asio::buffer(*boost::begin(i->data_.push_datas()))
        //         , boost::bind(&this_type::handle_push_write, this, placeholders::error));
    }

    void handle_push_write(sys::error_code const & erc)
    {
        LOG_I << __FUNCTION__;
        stage_ = stage::idle;

        if (erc)
        {
            return handle_push_error(erc,__LINE__,__FUNCTION__);
        }
        if (push_list_.empty())
        {
            return;
        }

        iterator i = push_list_.front();
        if (i->push_writing)
        {
            i->push_writing = false;
            i->pi_ = push_list_.end();
            push_list_.pop_front();

            sys::error_code ec = i->data_.callback(erc);
            if (ec)
            {
                DESTROY(i);
            }
            else
            {
                async_response(i);
                async_push_append(i);
            }
        }
        async_push();
    }

    void handle_push_error(sys::error_code const & ec, int ln, char const *fn)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< stage_ <<" "<<ln<<":"<< fn;

        if (ec == boost::asio::error::operation_aborted)
        {
            // stage_ = stage::error;
            return;
        }

        if (stage_ == stage::connecting)
            return;
        stage_ = stage::connecting;

        timer_.expires_from_now(boost::posix_time::seconds(1));
        timer_.async_wait(boost::bind(&apple_push::timed_connect, this, boost::asio::placeholders::error));
    }

    void handle_push_read(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< stage_;
        if (ec)
        {
            return handle_push_error(ec,__LINE__,__FUNCTION__);
        }

        LOG_I << &sbuf_;
        boost::asio::async_read(socket_, sbuf_
                , boost::asio::transfer_at_least(32)
                , boost::bind(&this_type::handle_push_read, this, placeholders::error));
    }

    void handle_push_handshake(const boost::system::error_code& ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< stage_;
        if (ec)
        {
            timer_.expires_from_now(boost::posix_time::seconds(10));
            timer_.async_wait(boost::bind(&apple_push::timed_connect, this, boost::asio::placeholders::error));
            return;
        }

        stage_ = stage::idle;
        async_push();

        boost::asio::async_read(socket_, sbuf_
                , boost::asio::transfer_at_least(32)
                , boost::bind(&this_type::handle_push_read, this, placeholders::error));
    }

    void handle_push_connect(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< stage_;
        if (ec)
        {
            timer_.expires_from_now(boost::posix_time::seconds(5));
            timer_.async_wait(boost::bind(&apple_push::timed_connect, this, boost::asio::placeholders::error));
            return;
        }

        // stage_ = stage::handshaking;
        socket_.async_handshake(boost::asio::ssl::stream_base::client,
                boost::bind(&this_type::handle_push_handshake, this, placeholders::error));
    }

    void timed_connect(sys::error_code ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< stage_;
        if (ec)
            return;
        if (stage_ != stage::connecting)
            return;
        socket_.lowest_layer().close(ec);
        socket_.lowest_layer().open(tcp::v4());
        socket_.lowest_layer().async_connect(endpoint_, boost::bind(&this_type::handle_push_connect, this, placeholders::error));
        LOG_I << "connecting ... ";
    }

private:
    stage::type stage_;
    std::list<iterator> push_list_;

    boost::asio::ssl::context context_;
    boost::asio::ssl::stream<tcp::socket> socket_;
    boost::asio::deadline_timer timer_;

    tcp::endpoint endpoint_;
    context_list contexts_;

    boost::asio::streambuf sbuf_;
};


