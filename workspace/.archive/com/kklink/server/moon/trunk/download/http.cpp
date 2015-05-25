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
#include <boost/smart_ptr.hpp>
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
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> Socket_ptr;
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
    return sys::error_code(e, error_category::inst<myerror_category>());
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
    bool keep_alive() const { return false; }

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

struct connection : boost::enable_shared_from_this<connection> 
{
    typedef connection this_type;
    typedef boost::shared_ptr<connection> pointer;
    typedef request message;

    typedef boost::function<void (message const&, connection* const )> message_handler;

    connection(boost::asio::io_service& io_service, message_handler& mh)
        :soc(new boost::asio::ip::tcp::socket(io_service)), messager_(mh)
    {}
    // { 
    //     soc = psoc;
    //     messager_ = mh;
    //     LOG_I<<&mh;
    //     // deadline_(soc->get_io_service());
    // }

    ~connection()
    {
        close();
    }

    void close()
    {
        LOG_I << static_cast<void*>(this);
    }
    Socket_ptr socket() { return soc; }

    void start()
    {
        LOG_I<<"connection ready";
        boost::asio::async_read_until(*soc, rsbuf_, "\r\n", 
                boost::bind(&this_type::handle_method_line, 
                     shared_from_this(), placeholders::error));
    }
private:
    void handle_method_line(sys::error_code const & ecc)
    {
        if(ecc){
            return handle_error(ecc);
        }

        message_.clear();
        sys::error_code ec;
        if ((ec = message_.parse_status_line(rsbuf_))){
            if (ec == http::error::empty_lines){
                boost::asio::async_read_until(*soc, rsbuf_, "\r\n", 
                        boost::bind(&this_type::handle_method_line, 
                             shared_from_this(), placeholders::error));
                return;
            }
            return handle_error(ec);
        }

        boost::asio::async_read_until(*soc, rsbuf_, "\r\n\r\n", 
                boost::bind(&this_type::handle_headers,
                    shared_from_this(), placeholders::error));

    }

    void handle_headers(sys::error_code const & ecc)
    {
        if (ecc){
            return handle_error(ecc);
        }

        sys::error_code ec;
        std::istream ins(&rsbuf_);
        std::string line;
        while (getline(ins,line) && !line.empty() && line[0]!='\r'){
            if ((ec=message_.parse_header_line(line))){
                return handle_error(ec);
            }
        }
        if (message_.content().size() <= rsbuf_.size()){
            return handle_content(ecc);
        }
        else{
            boost::asio::async_read(*soc , rsbuf_, 
                    boost::asio::transfer_at_least(message_.content().size() - rsbuf_.size()), 
                    boost::bind(&this_type::handle_content,  shared_from_this(), placeholders::error));
        }

    }

    void handle_content(sys::error_code const & ec)
    {
        if (ec){
            return handle_error(ec);
        }
        message_.content(rsbuf_);
        LOG_I << soc->remote_endpoint();
        // _request(message_);
        messager_(message_, this);

        if(message_.keep_alive()){
            boost::asio::async_read_until(*soc, rsbuf_, "\r\n"
                    , boost::bind(&this_type::handle_method_line, shared_from_this(), placeholders::error));
        }
    }

private:
    // boost::asio::deadline_timer deadline_;
    boost::asio::streambuf rsbuf_;
    message_handler messager_;
    message message_;
    Socket_ptr soc;

    void handle_error(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message();
    }

    // void _request(message const & msg)
    // {
    //     LOG_I<<msg;
    //     messager_(msg, soc);
    // }
};

struct relay_server : tcp::acceptor
{
    typedef relay_server this_type;
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer;

    explicit relay_server(boost::asio::io_service & ios)
        : tcp::acceptor(ios)
    {
    }

    void accept(tcp::endpoint const & endpoint, connection::message_handler const& fn)
    {
        handler_ = fn;
        open(endpoint.protocol());
        set_option(tcp::acceptor::reuse_address(true));
        bind(endpoint);
        listen();
        LOG_I << "listen " << local_endpoint();
        start_accept();
    }

    // template <typename F> void post(F const & fn) { get_io_service().post(fn); }

    // this_type::timer new_timer()
    // {
    //     typedef typename timer::element_type T;
    //     return timer(new T(get_io_service()));
    // }

    // template <typename F> void post(this_type::timer & t, int seconds, F const & fn)
    // {
    //     t->expires_from_now(boost::posix_time::seconds(seconds));
    //     t->async_wait(boost::bind(fn, t));
    // }

private:
    connection::message_handler handler_;
    void handle_error(sys::error_code const & ec)
    {
        LOG_I << ec <<" "<< ec.message();
    }

    void start_accept()
    {
        connection::pointer new_connection(new connection(get_io_service(), handler_));
        async_accept(*(new_connection->socket()),
                boost::bind(&relay_server::handle_accept, this, new_connection,
                    boost::asio::placeholders::error));
    }
    void handle_accept(connection::pointer new_connection, const boost::system::error_code& ec )
    {
        if (!ec)
        {
            LOG_I << new_connection->socket()->remote_endpoint();
            new_connection->start();
        }
        start_accept();
    }
};


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

