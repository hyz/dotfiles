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
#include "myerror.h"

namespace sys = boost::system;
namespace placeholders = boost::asio::placeholders;
using boost::asio::ip::tcp;

void listen(tcp::acceptor & a, tcp::endpoint const & endpoint)
{
    a.open(endpoint.protocol());
    a.set_option(tcp::acceptor::reuse_address(true));
    a.bind(endpoint);

    a.listen();
    LOG_I << "listen " << a.local_endpoint();
}

//----------------------------------------------------------------------
namespace http { namespace error {
    enum http_parse_error
    {
        empty_lines,
        invalid_header_line,
        invalid_path_params,
        invalid_status_line,
    };
} }

namespace boost { namespace system {
    template<> struct is_error_code_enum<http::error::http_parse_error>
    {
        static const bool value = true;
    };
} }

sys::error_code make_error_code(http::error::http_parse_error e)
{
    return sys::error_code(e, Error_category::inst<myerror_category>());
}

inline std::string unescape(std::string::const_iterator b, std::string::const_iterator end)
{
    return urldecode(std::string(b, end));
}

template <typename Params>
static bool path_params(std::string& path, Params& params, const std::string& fullpath)
{
    std::string::const_iterator i, b, eop, end;

    i = fullpath.begin();
    if (*i == '/')
        ++i;
    end = std::find(i, fullpath.end(), '#');

    b = std::find(i, end, '?');
    if (b == i)
        return false;

    path.assign(i, b);

    while (b != end)
    {
        ++b;
        eop = std::find(b, end, '&');
        i = std::find(b, eop, '=');
        if (i != eop)
        {
            params.insert(make_pair(unescape(b, i), unescape(i+1, eop)));
        }

        b = eop;
    }

    return true;
}

struct keyvalues : private std::list<std::pair<std::string, std::string> >
{
    typedef std::list<std::pair<std::string, std::string> > base_type;

    using base_type::clear;
    using base_type::iterator;
    using base_type::const_iterator;

    iterator find(const std::string& k) const
    {
        return const_cast<keyvalues*>(this)->find(k);
    }
    iterator find(const std::string& k)
    {
        iterator i = begin();
        for (; i != end(); ++i)
            if (i->first == k)
                break;
        return i;
    }

    iterator insert(const value_type & kv)
    {
        return base_type::insert(end(), kv);
    }

    iterator begin() const { return const_cast<keyvalues*>(this)->begin(); }
    iterator begin() { return base_type::begin(); }
    iterator end() const { return const_cast<keyvalues*>(this)->end(); }
    iterator end() { return base_type::end(); }
};

const char* get(keyvalues const &l, const std::string& name, const char* defa)
{
    keyvalues::const_iterator i = l.find(name);
    if (i == l.end())
        return defa;
    return i->second.c_str();
}

std::string get(keyvalues const &l, const std::string& name, const std::string& defa)
{
    keyvalues::const_iterator i = l.find(name);
    if (i == l.end())
        return defa;
    return i->second;
}
// std::string & get(keyvalues &l, const std::string& name, std::string& defa)
// {
//     keyvalues::iterator i = l.find(name);
//     if (i == l.end())
//         return defa;
//     return i->second;
// }

template <typename T>
T get(keyvalues const &l, const std::string& name, const T& defa)
{
    const char *p = get(l, name, static_cast<const char*>(0));
    if (p == 0)
        return defa;
    return boost::lexical_cast<T>(p);
}

const std::string& get(keyvalues const &l, const std::string& name)
{
    keyvalues::const_iterator i = l.find(name);
    if (i == l.end())
        THROW_EX(EN_HTTPParam_NotFound);
    return i->second;
}

template <typename T>
T get(keyvalues const &l, const std::string& name)
{
    return boost::lexical_cast<T>(get(l,name));
}

bool exist(keyvalues const &l, const std::string& name)
{
    return (l.end() != l.find(name));
}

struct response
{
    explicit response(int status = 200);

    void status(int c) { status_code_ = c; }
    int status() const { return status_code_; }

    void header(const std::string& k, const std::string& val);

    void content(const json::object& jso) { content_ = json::encode(jso); }
    void content(const std::string& cont) { content_ = cont; }
    void content(const boost::format& fmt) { content_ = boost::str(fmt); }

    std::string encode() const;
    std::string encode_header() const;

private:
    int status_code_;
    keyvalues headers_;
    std::string content_;

    static const char* code_str(int code);
};

response::response(int status)
{
    status_code_ = status;
    header("Content-Type", "text/plain;charset=UTF-8");
}

void response::header(const std::string& k, const std::string& val)
{
    for (keyvalues::iterator i = headers_.begin(); i != headers_.end(); ++i)
    {
        if (i->first == k)
        {
            i->second = val;
            return;
        }
    }
    headers_.insert(std::make_pair(k, val));
}

const char* response::code_str(int code)
{
    static struct { int code; const char* str; } ecl[] = {
        { 200, "OK" }
        , { 400, "Bad Request" }
        , { 401, "Unauthorized" }
        , { 403, "Forbidden" }
        , { 404, "Not Found" }
        , { 500, "Internal Server Error" }
    };
    for (unsigned int i = 0; i < sizeof(ecl)/sizeof(ecl[0]); ++i)
        if (ecl[i].code == code)
            return ecl[i].str;
    return "Unknown";
}

std::string response::encode() const
{
    return encode_header() + "\r\n" + content_;
}

std::string response::encode_header() const
{
    std::string hdrs;

    for (keyvalues::iterator it = headers_.begin(); it != headers_.end(); ++it)
    {
        hdrs += it->first + ": " + it->second + "\r\n";
    }
    hdrs += "Content-Length: " + boost::lexical_cast<std::string>(content_.size()) + "\r\n";

    return str(boost::format("HTTP/1.1 %d %s\r\n") % status_code_ % code_str(status_code_)) + hdrs;
}

struct request
{
    request() { }

    explicit request(sys::error_code const & ec)
        : ec_(ec)
    {
    }

    void clear()
    {
        method_.clear();
        path_.clear();

        params_.clear();
        headers_.clear();

        content_.clear();

        sys::error_code();
    }

    sys::error_code const & error_code() const { return ec_; }

    sys::error_code parse_status_line(std::streambuf & sb)
    {
        using namespace boost;
        sys::error_code ec;

        std::istream ins(&sb);
        std::string line;
        while (getline(ins, line)) // while (ins >> message_.method_)
        {
            if (!boost::all(line, is_space()))
                break;
        }
        if (!ins)
        {
            return make_error_code(http::error::empty_lines);
        }

        regex e("^\\s*(GET|POST)\\s+([^\\s]+)\\s+(HTTP/1.[01])\\s*$");
        smatch what;
        if (!regex_match(line, what, e))
        {
            return make_error_code(http::error::invalid_status_line);
        }

        // ver_ = what[3];
        method_ = what[1];

        if (!path_params(path_, params_, what[2]))
        {
            return make_error_code(http::error::invalid_path_params);
        }

        return ec;
    }

    sys::error_code parse_header_line(const std::string & line)
    {
        using namespace boost;
        sys::error_code ec;

        regex expr("^\\s*([^:]+)\\s*:\\s*(.*?)\\s*$");
        smatch res;
        if (!regex_match(line, res, expr))
        {
            return (make_error_code(http::error::invalid_header_line));
        }

        std::string k(res[1].first, res[1].second);
        std::string val(res[2].first, res[2].second);

        if (boost::iequals(k, "Content-Length"))
        {
            content_.resize(boost::lexical_cast<int>(val));
        }

        headers_.insert( std::make_pair(k, val) );

        return ec;
    }

    std::string const & method() const { return method_; }
    std::string const & path() const { return path_; }
    keyvalues const & params() const { return params_; }
    keyvalues const & headers() const { return headers_; }
    std::string const & content() const { return content_; }

    void content(std::streambuf & sbuf) { sbuf.sgetn(&content_[0], content_.size()); }

    sys::error_code const & error() const { return ec_; }

private:
    // short ver_;
    std::string method_;
    std::string path_;

    keyvalues params_;
    keyvalues headers_;

    std::string content_;

    sys::error_code ec_;
};


template <class connection>
struct relay_server : tcp::acceptor
{
    typedef relay_server<connection> this_type;

    typedef boost::shared_ptr<boost::asio::deadline_timer> timer;

    typedef boost::shared_ptr<connection> shared;
    typedef typename std::map<int,shared>::iterator iterator;

    typedef typename connection::message message;
    // typedef typename connection::callback callback;
    typedef boost::function<void (message const &, connection&)> messager_fn;
    typedef boost::function<messager_fn (message const &, connection&)> messager_fn0;

    explicit relay_server(boost::asio::io_service & ios)
        : tcp::acceptor(ios)
    {
    }

    void start(tcp::endpoint const & ep, messager_fn0 const & fn)
    {
        messager0_ = fn;
        ::listen(*this, ep); 

        c_.reset(new connection(*this));
        async_accept(c_->socket(), boost::bind(&this_type::handle_accept, this, placeholders::error));
    }

    template <typename F> void post(F const & fn) { get_io_service().post(fn); }

    this_type::timer new_timer()
    {
        typedef typename timer::element_type T;
        return timer(new T(get_io_service()));
    }

    template <typename F> void post(this_type::timer & t, int seconds, F const & fn)
    {
        t->expires_from_now(boost::posix_time::seconds(seconds));
        t->async_wait(boost::bind(fn, t));
    }

public:
    //friend class connection;

    iterator destroy(iterator i)
    {
        if (i == cs_.end())
            return i;

        shared sp = i->second;

        cs_.erase(i);

        void (shared::*fp)() = &shared::reset;
        post(boost::bind(fp, sp));

        return cs_.end();
    }

    messager_fn handle_message(message const & m, connection & c) { return messager0_(m, c); }

private:
    std::map<int,shared> cs_;
    shared c_;

    messager_fn0 messager0_;

    void handle_error(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message();
    }

    void handle_accept(sys::error_code const & ec)
    {
        if (ec)
        {
            return handle_error(ec);
        }
        LOG_I << c_->socket().remote_endpoint();

        std::pair<iterator,bool> ret = cs_.insert( std::make_pair(c_->socket().native_handle(), c_) );
        BOOST_ASSERT (ret.second);
        c_->ready( *this, ret.first );

        c_.reset(new connection(*this));
        async_accept(c_->socket(), boost::bind(&this_type::handle_accept, this, placeholders::error));
    }
};

struct connection : tcp::socket
{
    typedef connection this_type;
    typedef request message;

    typedef boost::function<void (message const&, connection&)> message_handler;

    typedef relay_server<connection> server;

    connection(server & s)
        : tcp::socket(s.get_io_service())
        , deadline_(s.get_io_service())
    {
    }

    ~connection()
    {
        LOG_I << static_cast<void*>(this) <<" "<< static_cast<void*>(server_);
        if (server_)
        {
            sys::error_code ec = boost::asio::error::make_error_code(boost::asio::error::connection_aborted);
            _request(message(ec));

            close();
        }
    }

    tcp::socket & socket() { return *this; }

    connection::server & get_server() { BOOST_ASSERT(server_); return *server_; }

    void close()
    {
        LOG_I << static_cast<void*>(this) <<" "<< static_cast<void*>(server_);
        if (server_)
        {
            iself_ = server_->destroy(iself_);
            server_ = 0;
        }
        messager_ = message_handler();
        write_ls_.clear();
    }

    template <typename ConstBufferSequence, typename WriteHandler>
    void send(const ConstBufferSequence& buffers, BOOST_ASIO_MOVE_ARG(WriteHandler) handler);

private:
    void handle_write(boost::function<void (sys::error_code const&)> fn, sys::error_code const& ec/*, std::size_t bytes*/)
    {
        if (ec)
        {
            fn(ec);
        }

        write_ls_.pop_front();
        if (!write_ls_.empty())
        {
            std::pair<boost::asio::const_buffers_1
                , boost::function<void (sys::error_code const&)> >& p = write_ls_.front();
            boost::asio::async_write(socket()
                    , p.first
                    , boost::bind(&connection::handle_write, this
                        , p.second, placeholders::error/*, placeholders::bytes_transferred*/));
        }
    }

    friend struct relay_server<connection>;

    void ready(server & s, server::iterator iter)
    {
        server_ = &s;
        iself_ = iter;

        boost::asio::async_read_until(socket(), rsbuf_, "\r\n"
                , boost::bind(&this_type::handle_method_line, this, placeholders::error));
    }

    void handle_method_line(sys::error_code const & ecc)
    {
        if (ecc)
        {
            return handle_error(ecc);
        }

        message_.clear();

        sys::error_code ec;
        if ( (ec = message_.parse_status_line(rsbuf_)))
        {
            if (ec == http::error::empty_lines)
            {
                boost::asio::async_read_until(socket(), rsbuf_, "\r\n"
                        , boost::bind(&this_type::handle_method_line, this, placeholders::error));
                return;
            }
            return handle_error(ec);
        }

        boost::asio::async_read_until(socket(), rsbuf_, "\r\n\r\n"
                , boost::bind(&this_type::handle_headers, this, placeholders::error));

    }

    void handle_headers(sys::error_code const & ecc)
    {
        if (ecc)
        {
            return handle_error(ecc);
        }

        sys::error_code ec;
        std::istream ins(&rsbuf_);
        std::string line;
        while (getline(ins, line) && !line.empty() && line[0] != '\r')
        {
            if ( (ec = message_.parse_header_line(line)))
            {
                return handle_error(ec);
            }
        }

        if (message_.content().size() <= rsbuf_.size())
        {
            return handle_content(ec);
        }

        boost::asio::async_read(socket() , rsbuf_
                , boost::asio::transfer_at_least(message_.content().size() - rsbuf_.size())
                , boost::bind(&this_type::handle_content, this, placeholders::error));
    }

    void handle_content(sys::error_code const & ec)
    {
        if (ec)
        {
            return handle_error(ec);
        }

        message_.content(rsbuf_);

        _request(message_);

        if (!server_)
        {
            return;
        }
        boost::asio::async_read_until(socket(), rsbuf_, "\r\n"
                , boost::bind(&this_type::handle_method_line, this, placeholders::error));
    }

private:
    server* server_;
    server::iterator iself_;

    boost::asio::deadline_timer deadline_;

    boost::asio::streambuf rsbuf_;

    message_handler messager_;
    message message_;

    std::list<std::pair<boost::asio::const_buffers_1, boost::function<void (sys::error_code const&)> > > write_ls_;

    void handle_error(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message();
        _request(message(ec));

        iself_ = server_->destroy(iself_);
    }

    void _request(message const & msg)
    {
        if (!server_)
            return;

        if (!messager_)
        {
            messager_ = server_->handle_message(msg, *this);
            return;
        }
        messager_(msg, *this);
    }

};

template <typename ConstBufferSequence, typename WriteHandler>
    void
connection::send(const ConstBufferSequence& buffers, BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    bool empty = write_ls_.empty();
    write_ls_.push_back(std::make_pair(buffers, handler));
    if (empty)
    {
        std::pair<boost::asio::const_buffers_1
            , boost::function<void (sys::error_code const&)> >& p = write_ls_.front();
        boost::asio::async_write(socket()
                , p.first
                , boost::bind(&connection::handle_write, this
                    , p.second, placeholders::error/*, placeholders::bytes_transferred*/));
    }
    LOG_I << empty <<" "__FILE__;
}

std::ostream& operator<<(std::ostream& out, const keyvalues& kv)
{
    for (keyvalues::iterator i = kv.begin(); i != kv.end(); ++i)
        out << i->first << "=" << i->second << "#";
    return out;
}

std::ostream& operator<<(std::ostream& out, const request& req)
{
    return out << req.method() << " " << req.path() << "|" << req.params()
        << "|" << req.headers();
}

