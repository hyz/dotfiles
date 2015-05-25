
#include <syslog.h>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <boost/algorithm/string.hpp>
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

//----------------------------------------------------------------------

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

std::string apack(std::string const & tok, int id, std::string const & aps)
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

#define PERM_PASSWORD "abc123"
static std::string getpwd(std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) { return PERM_PASSWORD; }

struct apple_push : boost::noncopyable
{
    typedef apple_push this_type;

    struct push_data
    {
        push_data(connection &c) { c_ = &c; }

        bool connected() const { return c_; }
        void disconnect() { c_ = 0; }

        void relay_back(sys::error_code const & ec)
        {
            bool empty = replys_.empty();
            //std::string cont = boost::str(boost::format("%1%\t%2%") % ec.value() % ec.message() )
            std::string s = boost::str(boost::format("HTTP/1.1 %1% %2%\r\nContent-Length:0\r\n\r\n")
                    % ec.value() % ec.message());
            replys_.push_back(s);

            ps_.pop_front();
            LOG_I << replys_.front();

            if (empty)
            {
                c_->send(boost::asio::buffer(replys_.front())
                        , boost::bind(&push_data::handle_reply, this, placeholders::error));
            }
        }

        void handle_reply(sys::error_code const & ec)
        {
            LOG_I << ec <<" "<< ec.message();
            replys_.pop_front();
            if (c_ && !replys_.empty())
            {
                c_->send(boost::asio::buffer(replys_.front())
                        , boost::bind(&push_data::handle_reply, this, placeholders::error));
            }
        }

        bool empty() const { return ps_.empty(); }
        std::string const & data() const { return ps_.front(); }

        void append(const std::string& m) { ps_.push_back( m ); }

        ~push_data() {
            LOG_I << static_cast<void*>(this) <<" "<< ps_.empty();
        }
    private:
        connection* c_;
        std::list<std::string> ps_;
        std::list<std::string> replys_;
    };

    struct stage { enum type {
            error = 0,
            connecting = 1, handshaking,
            idle,
            writing,
        };
    };

    typedef boost::shared_ptr<push_data> push_data_ptr;

    typedef std::list<push_data_ptr> bufs_type;

    apple_push(boost::asio::io_service & io_service, tcp::endpoint const & ep)
        //: context_(boost::asio::ssl::context::sslv3)
        : timer_(io_service)
        , endpoint_(ep)
    {
        stage_ = stage::connecting;
        timed_connect(sys::error_code());
    }

    connection::message_handler message(connection::message const& m, connection& c)
    {
        push_data_ptr ptr(new push_data(c));
        bufs_.push_back(ptr);
        pmessage(ptr, m);
        return boost::bind(&apple_push::pmessage, this, ptr, _1);
    }

private:
    void pmessage(push_data_ptr ptr, connection::message const & m)
    {
        LOG_I << stage_ <<" "<< m.error() <<" "<< m.error().message();

        BOOST_ASSERT(ptr);
        if (m.error())
        {
            ptr->disconnect();
            return;
        }

        ptr->append( on_REQUEST(m) );

        if (stage_ == stage::idle)
        {
            write_msg();
        }
        // else if (stage_ < stage::ready) { connect(); }
    }

    std::string on_REQUEST(connection::message const & m)
    {
        LOG_I << m << "|" << (m.content());

        std::string tok = get(m.params(), "devtok");
        int id = get<int>(m.params(), "id");

        return apack(tok, id, m.content());
    }

    void write_msg()
    {
        LOG_I << stage_ <<" "<< bufs_.empty();
        if (stage_ != stage::idle || bufs_.empty())
            return;

        for (bufs_type::iterator i = bufs_.begin(); i != bufs_.end(); )
        {
            if ((*i)->connected())
                ++i;
            else
                i = bufs_.erase(i);
        }

        for (bufs_type::iterator i = bufs_.begin(); i != bufs_.end(); ++i)
            if (!(*i)->empty())
            {
                if (i != bufs_.begin())
                    bufs_.splice(bufs_.begin(), bufs_, i);
                break;
            }

        if (bufs_.empty() || bufs_.front()->empty())
            return;

        stage_ = stage::writing;
        boost::asio::async_write(ssl_->socket()
                , boost::asio::buffer(bufs_.front()->data())
                , boost::bind(&this_type::handle_write, this, placeholders::error));
    }

    void handle_write(sys::error_code const & ec)
    {
        BOOST_ASSERT (!bufs_.empty());
        stage_ = stage::idle;

        if (ec)
        {
            return handle_error(ec);
        }

        if (bufs_.empty())
        {
            return;
        }

        push_data_ptr & ptr = bufs_.front();

        if (!ptr->connected())
        {
            bufs_.pop_front();
            write_msg();
            return;
        }

        ptr->relay_back(ec);

        bufs_.splice(bufs_.end(), bufs_, bufs_.begin());
        write_msg();
    }

    void handle_read(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "__FILE__" "<< stage_;
        if (ec)
        {
            return handle_error(ec);
        }

        LOG_I << &sbuf_;
        boost::asio::async_read(ssl_->socket(), sbuf_
                , boost::asio::transfer_at_least(32)
                , boost::bind(&this_type::handle_read, this, placeholders::error));
    }

    void handle_error(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "__FILE__" "<< stage_;

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

    void handle_handshake(const boost::system::error_code& ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "__FILE__" "<< stage_;
        if (ec)
        {
            timer_.expires_from_now(boost::posix_time::seconds(10));
            timer_.async_wait(boost::bind(&apple_push::timed_connect, this, boost::asio::placeholders::error));
            return;
        }

        stage_ = stage::idle;
        write_msg();

        boost::asio::async_read(ssl_->socket(), sbuf_
                , boost::asio::transfer_at_least(32)
                , boost::bind(&this_type::handle_read, this, placeholders::error));
    }

    void handle_connect(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "__FILE__" "<< stage_;
        if (ec)
        {
            timer_.expires_from_now(boost::posix_time::seconds(5));
            timer_.async_wait(boost::bind(&apple_push::timed_connect, this, boost::asio::placeholders::error));
            return;
        }

        // stage_ = stage::handshaking;
        ssl_->socket().async_handshake(boost::asio::ssl::stream_base::client,
                boost::bind(&this_type::handle_handshake, this, placeholders::error));
    }

    void timed_connect(sys::error_code ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" "__FILE__" "<< stage_;
        if (ec)
            return;
        if (stage_ != stage::connecting)
            return;
        ssl_.reset(new wrapssl(timer_.get_io_service(), certs_dir_));
        //socket_.lowest_layer().close(ec);
        //socket_.lowest_layer().open(tcp::v4());
        //socket_.lowest_layer().async_connect(endpoint_, boost::bind(&this_type::handle_connect, this, placeholders::error));
        ssl_->lowest_layer().async_connect(endpoint_, boost::bind(&this_type::handle_connect, this, placeholders::error));
        LOG_I << "connecting ... ";
    }

private:
    stage::type stage_;

    struct wrapssl : boost::noncopyable
    {
        explicit wrapssl(boost::asio::io_service & io_service, const std::string & cert)
            : context_(boost::asio::ssl::context::sslv3)
              , socket_(io_service, init_context(context_,cert))
        {}

        boost::asio::ssl::stream<tcp::socket>::lowest_layer_type & lowest_layer() { return socket_.lowest_layer(); }

        boost::asio::ssl::stream<tcp::socket> & socket() { return socket_; }

    private:
        boost::asio::ssl::context context_;
        boost::asio::ssl::stream<tcp::socket> socket_;

        static boost::asio::ssl::context & init_context(boost::asio::ssl::context & ctx, const std::string& cert)
        {
            ctx.set_password_callback(getpwd);
            LOG_I << cert + "/" + RSA_CLIENT_CERT;

            ctx.use_certificate_file(cert + "/" + RSA_CLIENT_CERT, boost::asio::ssl::context::pem);
            ctx.use_private_key_file(cert + "/" + RSA_CLIENT_KEY, boost::asio::ssl::context::pem);
            return ctx;
        }
    };

    boost::shared_ptr<wrapssl> ssl_;
    //boost::asio::ssl::context context_;
    //boost::asio::ssl::stream<tcp::socket> socket_;
    boost::asio::deadline_timer timer_;

    tcp::endpoint endpoint_;
    bufs_type bufs_;

    boost::asio::streambuf sbuf_;
};


