
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
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/format.hpp>
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
    apple_push_data() {}

    std::list<std::string>& push_datas() { return push_datas_; }
    std::list<std::string>& response_datas() { return response_datas_; }

    sys::error_code on_REQUEST(http_server::connection::message const & m)
    {
        LOG_I << m << "|" << (m.content());

        std::string tok = get(m.params(), "devtok");
        int id = get<int>(m.params(), "id");

        push_datas_.push_back( pkaps(tok, id, m.content()) );
        return sys::error_code();
    }

    sys::error_code on_PUSHED(sys::error_code const & ec)
    {
        //std::string cont = boost::str(boost::format("%1%\t%2%") % ec.value() % ec.message() )
        response rsp(200);
        rsp.content(boost::format("%1% %2%") % ec.value() % ec.message());
        response_datas_.push_back(rsp.encode());
        LOG_I << response_datas_.front();

        push_datas_.pop_front();
        return sys::error_code();
    }

    ~apple_push_data()
    {
        LOG_I << static_cast<void*>(this)
            <<" "<< push_datas_.empty()
            <<" "<< push_datas_.empty();
    }

    static apple_push_data resolv(http_server::connection::message const & m) { return apple_push_data(); }

private:
    std::list<std::string> push_datas_;
    std::list<std::string> response_datas_;
};

#define PERM_PASSWORD "abc123"
std::string getpwd( std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) { return PERM_PASSWORD; }

struct apple_push : boost::noncopyable
{
    typedef apple_push this_type;

    struct stage { enum type {
            error = 0,
            connecting = 1, handshaking,
            idle,
            writing,
        };
    };

    struct ap_context;
    typedef std::list<ap_context> context_list;
    typedef std::list<ap_context>::iterator iterator;
    typedef std::list<iterator>::iterator viterator;

    struct ap_context
    {
        apple_push_data data_;

        explicit ap_context(boost::asio::io_service & ios)
            : _timer(new boost::asio::deadline_timer(ios))
        {
            _c = 0;
        }

        boost::shared_ptr<boost::asio::deadline_timer> _timer;

        http_server::connection* _c;
        iterator _iself;
        viterator _ipush;
        bool response_writing;
        unsigned short pid;
    };

    static 
    boost::asio::ssl::context & init_context(boost::asio::ssl::context & ctx)
    {
        ctx.set_password_callback(getpwd);
        LOG_I << certs_dir_ + "/" + RSA_CLIENT_CERT;

        ctx.use_certificate_file(certs_dir_ + "/" + RSA_CLIENT_CERT, boost::asio::ssl::context::pem);
        ctx.use_private_key_file(certs_dir_ + "/" + RSA_CLIENT_KEY, boost::asio::ssl::context::pem);
        return ctx;
    }

    apple_push(boost::asio::io_service & io_service, tcp::endpoint const & ep)
        : context_(boost::asio::ssl::context::sslv3)
        , socket_(io_service, init_context(context_))
        , timer_(io_service)
        , endpoint_(ep)
    {
        stage_ = stage::connecting;
        timed_connect(sys::error_code());
    }

    http_server::connection::message_handler message(http_server::connection::message const& m, http_server::connection& c)
    {
        ap_context ctx(c.get_io_service());
        ctx.data_ = apple_push_data::resolv(m);
        iterator i = contexts_.insert(contexts_.end(), ctx);
        i->_iself = i;
        i->_ipush = push_list_.end();
        pmessage(i, m);
        return boost::bind(&apple_push::pmessage, this, i, _1);
    }

private:
    void pmessage(iterator i, http_server::connection::message const & m)
    {
        LOG_I << stage_ <<" "<< m.error() <<" "<< m.error().message();

        BOOST_ASSERT(i != contexts_.end());
        if (m.error())
        {
            i->_c = 0;
            destroy(i);
            return;
        }

        bool empty = boost::empty(i->data_.response_datas());

        sys::error_code ec = i->data_.on_REQUEST( m );
        if (ec)
        {
            destroy(i);
            return;
        }

        async_push_append(i);
        async_push();
        if (empty)
            async_response(i);
    }

    void destroy(iterator i)
    {
        if (i->_c)
        {
            i->_c->destroy();
            i->_c = 0;
        }

        sys::error_code ec;
        i->_timer->cancel(ec);
        push_list_.erase(i->_ipush);
        contexts_.erase(i->_iself);
    }

    void async_response(iterator i)
    {
        // LOG_I << stage_ <<" "<< empty;
        // BOOST_ASSERT(_c);
        if (!i->_c || boost::empty(i->data_.response_datas()))
            return;

        boost::asio::async_write(i->_c->socket()
                , boost::asio::buffer(*boost::begin(i->data_.response_datas()))
                , boost::bind(&this_type::handle_rsp_write, this, i, placeholders::error));
    }

    void handle_rsp_write(iterator i, sys::error_code const & ec)
    {
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message();
            if (ec == boost::asio::error::operation_aborted)
                return;
        }
        BOOST_ASSERT(!boost::empty(i->data_.response_datas()));

        i->data_.response_datas().pop_front();
        async_response(i);
    }

    void async_push_append(iterator i)
    {
        if (i->_ipush == push_list_.end() && !boost::empty(i->data_.push_datas()))
        {
            i->_ipush = push_list_.insert(push_list_.end(), i);
        }
    }

    void async_push()
    {
        if (stage_ != stage::idle)
            return;

        while (!push_list_.empty() && boost::empty(push_list_.front()->data_.push_datas()))
            push_list_.pop_front();
        if (push_list_.empty())
            return;

        iterator i = push_list_.front();
        BOOST_ASSERT(i == i->_iself);

        i->pid = ++push_idx_;
        stage_ = stage::writing;
        boost::asio::async_write(socket_
                , boost::asio::buffer(*boost::begin(i->data_.push_datas()))
                , boost::bind(&this_type::handle_push_write, this, i->pid, placeholders::error));
    }

    void handle_push_write(int pid, sys::error_code const & erc)
    {
        stage_ = stage::idle;

        if (erc)
        {
            return handle_push_error(erc,__LINE__,__FILE__);
        }
        if (push_list_.empty())
            return;

        iterator i = push_list_.front();
        if (i->pid == pid)
        {
            push_list_.pop_front();
            bool empty = boost::empty(i->data_.response_datas());
            sys::error_code ec = i->data_.on_PUSHED(erc);
            if (ec)
                destroy(i);
            else if (empty)
                async_response(i);
            async_push_append(i);
        }
        async_push();
    }

    void handle_push_read(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< stage_;
        if (ec)
        {
            return handle_push_error(ec,__LINE__,__FILE__);
        }

        LOG_I << &sbuf_;
        boost::asio::async_read(socket_, sbuf_
                , boost::asio::transfer_at_least(32)
                , boost::bind(&this_type::handle_push_read, this, placeholders::error));
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
    unsigned short push_idx_;
    std::list<iterator> push_list_;

    boost::asio::ssl::context context_;
    boost::asio::ssl::stream<tcp::socket> socket_;
    boost::asio::deadline_timer timer_;

    tcp::endpoint endpoint_;
    context_list contexts_;

    boost::asio::streambuf sbuf_;
};


