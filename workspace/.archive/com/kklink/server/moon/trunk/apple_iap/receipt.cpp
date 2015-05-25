
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
// #include <boost/thread.hpp>
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

// static const char* certs_dir_ = "etc/moon.d";

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

struct iap_describe
{
    explicit iap_describe(int x, bool sandbox)
    {
        id_ = x;
        host = "buy.itunes.apple.com";
        cert = "etc/moon.d/apple";
        if (sandbox)
        {
            host = "sandbox.itunes.apple.com";
            cert = "etc/moon.d";
        }
    }

    int id_;
    const char *host;
    const char *cert;
};

std::ostream & operator<<(std::ostream & out, iap_describe const & inf)
{
    return out << inf.id_ <<" "<< inf.host <<" "<< inf.cert;
}

struct response_buf
{
    void operator()(response & rsp, const iap_describe & inf)
    {
        LOG_I << rsp << " response_buf";

        response r(200); // int status = j.get<int>("status");
        {
            json::object j = json::decode(rsp.content());
            j.put("id_", inf.id_);
            r.content(json::encode(j));
        }
        sl_.push_back(std::make_pair(inf, r.encode()));
        LOG_I << sl_.back().first <<" "<< sl_.back().second;

        // json::object receipt = j.get<json::object>("receipt");
        // receipt.get<std::string>("product_id");
        //transaction_id
        //quantity: string
        //unique_vendor_identifier
        //unique_identifier
        //item_id
        //product_id:IAP_1098
        //bid : com.kklink.OnMoon
        //bvrs
        //
        // "{\n\"receipt\":{\"original_purchase_date_pst\":\"2013-10-23 23:20:50 America/Los_Angeles\", \"purchase_date_ms\":\"1382595650083\", \"unique_identifier\":\"e39694a8c122535b2118d197a4149b0a11bf35f5\", \"original_transaction_id\":\"1000000091185916\", \"bvrs\":\"1.0\", \"transaction_id\":\"1000000091185916\", \"quantity\":\"1\", \"unique_vendor_identifier\":\"71CD805C-316A-4185-AA7F-D75955E27E5A\", \"item_id\":\"724295059\", \"product_id\":\"IAP_1098\", \"purchase_date\":\"2013-10-24 06:20:50 Etc/GMT\", \"original_purchase_date\":\"2013-10-24 06:20:50 Etc/GMT\", \"purchase_date_pst\":\"2013-10-23 23:20:50 America/Los_Angeles\", \"bid\":\"com.kklink.OnMoon\", \"original_purchase_date_ms\":\"1382595650083\"}, \"status\":0}";
    }

    void operator()(sys::error_code const & ec, const iap_describe & inf)
    {
        LOG_I << ec <<" "<< ec.message() <<" response-buf";

        response r(200); // int status = j.get<int>("status");
        {
            json::object j;
            j.put("id_", inf.id_);
            j.put("status", ec.value());
            j.put("message", ec.message());
            r.content(json::encode(j));
        }
        sl_.push_back(std::make_pair(inf, r.encode()));
        LOG_I << sl_.back().first <<" "<< sl_.back().second;
    }

    bool empty() const { return sl_.empty(); }

    boost::asio::const_buffers_1 const_buffer() const
    {
        if (sl_.empty())
            return boost::asio::buffer(static_cast<const char*>(0), 0);
        return boost::asio::buffer(sl_.front().second);
    }

    void consume(sys::error_code const & ec)
    {
        if (!ec)
            sl_.pop_front();
    }

    ~response_buf()
    {
        LOG_I << sl_.size() <<" "<< this;
    }
private:
    std::list<std::pair<iap_describe,std::string> > sl_;
};

struct request_buf
{
    sys::error_code operator()(const request & _req)
    {
        LOG_I << _req;

        iap_describe inf( get<int>(_req.params(),"id_") , get<int>(_req.params(),"sandbox") );

        request q("POST" , "/verifyReceipt");
        put(q.headers(), "Host", inf.host);
        put(q.headers(), "Connection", "close");

        json::object o;
        o.put("receipt-data", _req.content());
        q.content(json::encode(o));

        sl_.push_back(std::make_pair(inf, q.encode()));

        LOG_I << sl_.back().first <<" "<< sl_.back().second;

        return sys::error_code();
    }

    bool empty() const { return sl_.empty(); }

    boost::asio::const_buffers_1 const_buffer() const
    {
        if (sl_.empty())
            return boost::asio::buffer(static_cast<const char*>(0), 0);
        return boost::asio::buffer(sl_.front().second);
    }

    template <typename Fn> void consume(sys::error_code const & ec, Fn & fn)
    {
        LOG_I << sl_.front().first <<" "<< ec <<" "<< ec.message();
        fn(ec, sl_.front().first, sl_.front().second);
        sl_.pop_front();
    }

    template <typename Fn> void consume(response & rsp, Fn & fn)
    {
        LOG_I << sl_.front().first <<" "<< rsp;
        fn(rsp, sl_.front().first);
        sl_.pop_front();
    }

    iap_describe const & iap_info() const { return sl_.front().first; }

    ~request_buf()
    {
        LOG_I << sl_.size() <<" "<< this;
    }
private:
    std::list<std::pair< iap_describe,std::string> > sl_;
};

#define PERM_PASSWORD "abc123"
static std::string getpwd( std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) { return PERM_PASSWORD; }

struct apple_push : boost::noncopyable
{
#define DESTROY(i) this->destroy(i,__LINE__,__FUNCTION__)
    typedef apple_push this_type;

    struct stage { enum type {
            error = 0
            , connecting = 1, handshaking
            , idle
            , writing, reading
        };
    };

    struct ap_context;
    // typedef boost::shared_ptr<ap_context> ap_context_ptr;
    typedef std::list<ap_context> context_list;
    typedef std::list<ap_context>::iterator iterator;
    typedef std::list<iterator>::iterator piterator;

    struct ap_context
    {
        request_buf fwd_; // apple_push_buf data_;
        response_buf bwd_;

        explicit ap_context(boost::asio::io_service & ios)
            : _timer(new boost::asio::deadline_timer(ios))
        {
            _c = 0;
            working = false;
            nfail = 0;
            eof = 0;
        }

        boost::shared_ptr<boost::asio::deadline_timer> _timer;

        bool working;
        bool eof;
        unsigned char nfail;
        char _[1];

        piterator _pi;
        http_server::connection* _c;
    };

    explicit apple_push(boost::asio::io_service & io_service)
        //: thread_deadline_(thread_io_service_)
        //, context_(boost::asio::ssl::context::sslv3)
        //, socket_(io_service, init_context(context_))
        : timer_(io_service)
    {
        //stage_ = stage::connecting;
        //timed_connect(sys::error_code());

        // stage_ = stage::idle;
        // thread_timed(sys::error_code());
        // thread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &thread_io_service_)) ;
        //
        //io_service_ = &io_service;
    }

    ~apple_push()
    {
        //sys::error_code ec;
        //thread_deadline_.cancel(ec);
        //thread_io_service_.post(boost::bind(&boost::asio::io_service::stop, &thread_io_service_));
        //thread_.join();
        LOG_I << __FILE__;
    }

    http_server::connection::message_handler message(
            http_server::connection& c
            , sys::error_code const& ec
            , http_server::connection::message const& m)
    {
        iterator i = make_push_ctx(c, m);
        pmessage(i, ec, m);

        return boost::bind(&this_type::pmessage, this, i, _1, _2);
    }

private:
    iterator make_push_ctx(http_server::connection& c, http_server::connection::message const& m)
    {
        ap_context ctx(c.get_io_service());
        iterator i = contexts_.insert(contexts_.end(), ctx);
        i->_pi = iqueue_.end();
        i->_c = &c;
        return i;
    }

private:
    void pmessage(iterator i, sys::error_code const & erc, http_server::connection::message const & m)
    {
        BOOST_ASSERT(i != contexts_.end());
        if (erc)
        {
            LOG_I << erc <<" "<< erc.message();
            if (erc == boost::asio::error::eof)
            {
                i->eof = true;
                if (i->fwd_.empty()
                        && i->bwd_.empty())
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

        bool empty = iqueue_.empty();

        sys::error_code ec = i->fwd_( m );
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message();
            DESTROY(i);
            return;
        }
        if (!i->fwd_.empty())
        {
            if (i->_pi == iqueue_.end())
                i->_pi = iqueue_.insert(iqueue_.end(), i);
        }

#define RESET_CONNECT(x) reset_connect(x, __LINE__,__FUNCTION__)
        if (empty) RESET_CONNECT(0);
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

        LOG_I << i->_pi.operator->() <<" "<< iqueue_.end().operator->();

        if (i->_pi != iqueue_.end())
            iqueue_.erase(i->_pi);
        contexts_.erase(i);
    }

    void async_response()
    {
        if (!iqueue_.empty())
        {
            iterator i = iqueue_.front();
            if (!i->bwd_.empty())
            {
                boost::asio::async_write(i->_c->socket()
                        , i->bwd_.const_buffer()
                        , boost::bind(&this_type::handle_rsp_write, this, placeholders::error));
                return;
            }
        }
        RESET_CONNECT(0);
    }

    void handle_rsp_write(sys::error_code const & ec)
    {
        if (!skipped())
        {
            iterator i = iqueue_.front();
            i->bwd_.consume(ec);
            if (ec)
            {
                LOG_I << ec <<" "<< ec.message();
                DESTROY(i);
            }
        }

        async_response();
    }

    bool skipped() const
    {
        if (iqueue_.empty())
        {
            return true;
        }
        return !(iqueue_.front()->working);
    }

    void handle_write(sys::error_code const & erc)
    {
        if (skipped())
        {
            RESET_CONNECT(0);
            return;
        }

        if (erc)
        {
            return handle_error(erc,__LINE__,__FUNCTION__);
        }

        boost::asio::async_read_until(ssl_->socket()
                , streambuf_
                , "\r\n"
                , boost::bind(&this_type::handle_status_line, this, placeholders::error));
    }

    void handle_error(sys::error_code const & ec, int ln, char const *fn)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< ln <<":"<< fn;

        if (ec == boost::asio::error::operation_aborted)
        {
            // stage_ = stage::error;
            return;
        }

        if (iqueue_.empty())
            return;
        if (skipped())
            return reset_connect(0, ln,fn);

        iterator i = iqueue_.front();
        i->working = 0;
        i->nfail++;

        iqueue_.splice(iqueue_.end(), iqueue_, iqueue_.begin());

        reset_connect(3, ln,fn);
    }

    void handle_status_line(const sys::error_code& erc)
    {
        if (erc)
        {
            return handle_error(erc,__LINE__,__FUNCTION__);
        }

        response_.clear();
        sys::error_code ec = response_.parse_status_line(streambuf_);
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        boost::asio::async_read_until(ssl_->socket()
                , streambuf_
                , "\r\n\r\n"
                , bind(&this_type::handle_headers, this, placeholders::error));
    }

    void handle_headers(const sys::error_code& erc)
    {
        if (erc)
        {
            return handle_error(erc, __LINE__,__FUNCTION__);
        }

        sys::error_code ec = response_.parse_header_lines(streambuf_);
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        unsigned int len = get<int>(response_.headers(), "content-length", 0);

        if (streambuf_.size() >= len)
        {
            return handle_content(erc);
        }

        boost::asio::async_read(ssl_->socket()
                , streambuf_
                , boost::asio::transfer_at_least(len - streambuf_.size())
                , bind(&this_type::handle_content, this, placeholders::error));
    }

    void handle_content(const sys::error_code& ec)
    {
        if (skipped())
        {
            return RESET_CONNECT(0);
        }
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        response_.parse_content(streambuf_); // streambuf_.sgetn(&rsp.content_[0], req_.content_.size());

        iterator i = iqueue_.front();
        // i->working = 0;
        i->fwd_.consume(response_, i->bwd_);
        BOOST_ASSERT(!i->bwd_.empty());

        async_response();
    }

    void handle_handshake(const sys::error_code& ec)
    {
        if (skipped())
        {
            return RESET_CONNECT(0);
        }
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        iterator i = iqueue_.front();
        boost::asio::async_write(ssl_->socket()
                , i->fwd_.const_buffer()
                , boost::bind(&this_type::handle_write, this, placeholders::error));

        //boost::asio::async_read(socket_, streambuf_
        //        , boost::asio::transfer_at_least(32)
        //        , boost::bind(&this_type::handle_read, this, placeholders::error));
    }

    void handle_connect(sys::error_code const & ec)
    {
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }
        ssl_->socket().async_handshake(
                boost::asio::ssl::stream_base::client,
                boost::bind(&this_type::handle_handshake, this, placeholders::error));
    }

    void cleanup()
    {
        while (!iqueue_.empty())
        {
            iterator i = iqueue_.front();
            if (i->fwd_.empty() && i->bwd_.empty())
            {
                LOG_I << " both empty ";
                iqueue_.pop_front();
                i->_pi = iqueue_.end();
                i->working = 0;
                if (i->eof)
                    DESTROY(i);
                continue;
            }
            break;
        }
    }

    void timed_connect(sys::error_code ec)
    {
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message(); // (ec == boost::asio::error::operation_aborted)
            return;
        }
        cleanup();
        if (iqueue_.empty())
        {
            LOG_I << "empty";
            return;
        }

        iterator i = iqueue_.front();
        if (i->working || i->fwd_.empty())
        {
            LOG_I << i->working <<" || "<< i->fwd_.empty();
            return;
        }

        LOG_I << __FILE__;

        iap_describe const & inf = i->fwd_.iap_info();
        tcp::endpoint endpoint = resolv(timer_.get_io_service(), inf.host, "https");
        LOG_I << endpoint << inf;

        //socket_.lowest_layer().close(ec);
        //socket_.lowest_layer().open(tcp::v4());
        //socket_.lowest_layer().async_connect(endpoint_, boost::bind(&this_type::handle_connect, this, placeholders::error));

        ssl_.reset(new wrapssl(timer_.get_io_service(), inf.cert));
        i->working = true;
        ssl_->lowest_layer().async_connect( endpoint
                , boost::bind(&this_type::handle_connect, this, placeholders::error));
        LOG_I << "connecting " << endpoint;
    }

    void reset_connect(int ns, int ln, char const *fn)
    {
        // BOOST_ASSERT (skipped());
        timer_.expires_from_now(boost::posix_time::seconds(ns));
        timer_.async_wait(boost::bind(&this_type::timed_connect, this, boost::asio::placeholders::error));
    }

private:
    // stage::type stage_;

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

        static boost::asio::ssl::context & init_context(boost::asio::ssl::context & ctx, const std::string & cert)
        {
            ctx.set_password_callback(getpwd);
            LOG_I << cert << "/" << RSA_CLIENT_CERT;

            ctx.use_certificate_file(cert + "/" + RSA_CLIENT_CERT, boost::asio::ssl::context::pem);
            ctx.use_private_key_file(cert + "/" + RSA_CLIENT_KEY, boost::asio::ssl::context::pem);
            return ctx;
        }
    };

    std::list<iterator> iqueue_;

    boost::shared_ptr<wrapssl> ssl_;
    //boost::asio::ssl::context context_;
    //boost::asio::ssl::stream<tcp::socket> socket_;
    boost::asio::deadline_timer timer_;

    context_list contexts_;

    boost::asio::streambuf streambuf_;
    response response_;
};

