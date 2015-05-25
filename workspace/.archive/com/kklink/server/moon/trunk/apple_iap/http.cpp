#include <syslog.h>
#include <iterator>
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

void listen(tcp::acceptor & a, tcp::endpoint const & endpoint)
{
    a.open(endpoint.protocol());
    a.set_option(tcp::acceptor::reuse_address(true));
    a.bind(endpoint);

    a.listen();
    LOG_I << "listen " << a.local_endpoint();
}

namespace http { namespace error {
    enum http_parse_error
    {
        empty_lines,
        invalid_header_line,
        invalid_path_params,
        invalid_status_line,
    };
} }

//namespace boost { namespace system {
//    template<> struct is_error_code_enum<http::error::http_parse_error>
//    {
//        static const bool value = true;
//    };
//} }

//sys::error_code make_error_code(http::error::http_parse_error e)
//{
//    return sys::error_code();
//}

inline std::string unescape(std::string::const_iterator b, std::string::const_iterator end)
{
    return urldecode(std::string(b, end));
}

struct keyvalues : std::list<std::pair<std::string, std::string> >
{
    iterator find(const std::string& k) const
    {
        return const_cast<keyvalues*>(this)->find(k);
    }
    iterator find(const std::string& k)
    {
        iterator i = begin();
        for (; i != end(); ++i)
            if (boost::iequals(i->first, k))
                break;
        return i;
    }

    iterator insert(const value_type & kv)
    {
        return std::list<std::pair<std::string, std::string> >::insert(end(), kv);
    }
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

template <typename T>
T get(keyvalues const &l, const std::string& name, const T& defa)
{
    keyvalues::const_iterator i = l.find(name);
    if (i == l.end())
        return defa;
    return boost::lexical_cast<T>(i->second);
}

const std::string & get(keyvalues const &l, const std::string& name)
{
    keyvalues::const_iterator i = l.find(name);
    if (i == l.end())
        THROW_EX(EN_HTTPParam_NotFound);
    return i->second;
}

template <typename T>
inline T get(keyvalues const &l, const std::string& name)
{
    return boost::lexical_cast<T>(get(l,name));
}

inline bool exist(keyvalues const &l, const std::string& name)
{
    return (l.end() != l.find(name));
}

inline void put(keyvalues &l, const std::string& name, std::string const & val)
{
    l.insert(std::make_pair(name, val));
}

std::ostream& operator<<(std::ostream& out, const keyvalues& kv)
{
    for (keyvalues::const_iterator i = kv.begin(); i != kv.end(); ++i)
        out << i->first << "=" << i->second << "#";
    return out;
}

struct response
{
    explicit response(int status = 200);

    sys::error_code parse_status_line(std::streambuf & sbuf);
    sys::error_code parse_header_lines(std::streambuf & sbuf);
    sys::error_code parse_content(std::streambuf & sbuf)
    {
        // content_.resize(sbuf.size());
        // sbuf.sgetn(&content_[0], sbuf.size());
        content_.insert(content_.end(), std::istreambuf_iterator<char>(&sbuf), std::istreambuf_iterator<char>());
        return sys::error_code();
    }

    void status(int c) { status_code_ = c; }
    int status() const { return status_code_; }

    void header(const std::string& k, const std::string& val);
    keyvalues const & headers() const { return headers_; }

    void content(const json::object& jso) { content_ = json::encode(jso); }
    void content(const std::string& cont) { content_ = cont; }
    void content(const boost::format& fmt) { content_ = boost::str(fmt); }
    std::string const & content() const { return content_; }

    std::string encode(const char *eq=": ", const char *lf="\r\n") const;
    std::string encode_header(const char *eq=": ", const char *lf="\r\n") const;

    void clear()
    {
        status_code_ = 0;
        headers_.clear();
        content_.clear();
    }
private:
    int status_code_;
    keyvalues headers_;
    std::string content_;

    static const char* code_str(int code);
};

template <typename T>
std::string join_pairs(T const &t, const char* eq=": ", const char *lf="\r\n")
{
    std::string ret;

    for (typename T::const_iterator i = boost::begin(t); i != boost::end(t); ++i)
    {
        ret += i->first + eq + i->second + lf;
        //hdrs += i->first + ": " + i->second + "\r\n";
    }

    return ret;
}

std::ostream & operator<<(std::ostream & out, response const & rsp)
{
    return out << "HTTP " << rsp.status() << "|" << rsp.encode_header("=","#");
}

response::response(int status)
{
    status_code_ = status;
    header("Content-Type", "text/plain;charset=UTF-8");
}

sys::error_code response::parse_status_line(std::streambuf & sbuf)
{
    using namespace boost;
    sys::error_code ec;

    char space;
    std::string ver;

    std::istream ins(&sbuf);
    ins.unsetf(std::ios_base::skipws);

    ins >> ver >> space >> status_code_ >> space;
    std::string lin;
    getline(ins, lin);

    LOG_I << ver <<" "<< status_code_;

    return ec;
}

sys::error_code response::parse_header_lines(std::streambuf & sbuf)
{
    using namespace boost;
    sys::error_code ec;

    std::istream ins(&sbuf);
    std::string line;
    regex expr("^\\s*([^:]+)\\s*:\\s*(.*?)\\s*$");
    while (getline(ins, line) && !line.empty() && line[0] != '\r')
    {
        smatch res;
        if (!regex_match(line, res, expr))
        {
            return sys::error_code(http::error::invalid_header_line,myerror_category::inst<myerror_category>());
        }

        std::string k(res[1].first, res[1].second);
        std::string val(res[2].first, res[2].second);

        //if (boost::iequals(k, "Content-Length"))
        //{
        //    size_t siz = boost::lexical_cast<size_t>(val);
        //    if (siz > 1024*1024*8)
        //        return sys::error_code(http::error::invalid_header_line,error_category::inst<myerror_category>());
        //    content_.resize(siz);
        //}

        headers_.insert( std::make_pair(k, val) );
    }

    return ec;
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

std::string response::encode(const char *eq, const char *lf) const
{
    return encode_header(eq, lf)
        + str(boost::format("Content-Length%1%%2%%3%") % eq % content_.size() % lf)
        + lf
        + content_;
}

std::string response::encode_header(const char* eq, const char *lf) const
{
    // std::string hdrs;

    // for (keyvalues::const_iterator i = headers_.begin(); i != headers_.end(); ++i)
    // {
    //     //if (i->first != "Content-Length")
    //         hdrs += i->first + eq + i->second + lf;
    //     //hdrs += i->first + ": " + i->second + "\r\n";
    // }

    return str(boost::format("HTTP/1.1 %d %s\r\n") % status_code_ % code_str(status_code_))
        + join_pairs(headers_,eq,lf);
}

struct request
{
    // explicit request(sys::error_code const & ec) : ec_(ec) { }
    explicit request() { }
    explicit request(const std::string & method, const std::string & path)
        : method_(method)
        , path_(path)
    {
    }

    void clear();

    sys::error_code parse_request_line(std::streambuf & sbuf);
    sys::error_code parse_header_lines(std::streambuf & sbuf);
    sys::error_code parse_content(std::streambuf & sbuf)
    {
        //content_.resize(sbuf.size());
        //sbuf.sgetn(&content_[0], sbuf.size());
        content_.assign(std::istreambuf_iterator<char>(&sbuf), std::istreambuf_iterator<char>());
        return sys::error_code();
    }

    std::string const & method() const { return method_; }
    std::string const & path() const { return path_; }
    keyvalues const & params() const { return params_; }
    keyvalues const & headers() const { return headers_; }
    std::string const & content() const { return content_; }

    // sys::error_code const & error() const { return ec_; }
    keyvalues & params() { return params_; }
    keyvalues & headers() { return headers_; }
    void content(std::string const & cont) { content_ = cont; }

    std::string encode_header(const char* eq=": ", const char *lf="\r\n") const
    {
        return str(boost::format("%1% %2% HTTP/1.1%3%") % method_ % path_ % lf)
            + join_pairs(headers_,eq,lf);
    }

    std::string encode(const char *eq=": ", const char *lf="\r\n") const
    {
        return encode_header(eq, lf)
            + str(boost::format("Content-Length%1%%2%%3%") % eq % content_.size() % lf)
            + lf
            + content_;
    }

private:
    template <typename Params>
    static bool path_params(std::string& path, Params& params, const std::string& fullpath);

    // short ver_;
    std::string method_;
    std::string path_;

    keyvalues params_;
    keyvalues headers_;

    std::string content_;

    // sys::error_code ec_;
};

template <typename Params>
bool request::path_params(std::string& path, Params& params, const std::string& fullpath)
{
    std::string::const_iterator i, b, eop, end;

    i = fullpath.begin(); // if (*i == '/') ++i;
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

void request::clear()
{
    method_.clear();
    path_.clear();

    params_.clear();
    headers_.clear();

    content_.clear();
}

sys::error_code request::parse_header_lines(std::streambuf & sbuf)
{
    using namespace boost;
    sys::error_code ec;

    std::istream ins(&sbuf);
    std::string line;
    regex expr("^\\s*([^:]+)\\s*:\\s*(.*?)\\s*$");
    while (getline(ins, line) && !line.empty() && line[0] != '\r')
    {
        smatch res;
        if (!regex_match(line, res, expr))
        {
            return sys::error_code(http::error::invalid_header_line,myerror_category::inst<myerror_category>());
        }

        std::string k(res[1].first, res[1].second);
        std::string val(res[2].first, res[2].second);

        if (boost::iequals(k, "Content-Length"))
        {
            size_t siz = boost::lexical_cast<size_t>(val);
            if (siz > 1024*1024*8)
                return sys::error_code(http::error::invalid_header_line,myerror_category::inst<myerror_category>());
            content_.resize(siz);
        }

        headers_.insert( std::make_pair(k, val) );
    }

    return ec;
}

sys::error_code request::parse_request_line(std::streambuf & sbuf)
{
    using namespace boost;
    sys::error_code ec;

    clear();

    char sp1, sp2, cr, lf;
    std::string fullp;
    std::string ver;

    std::istream ins(&sbuf);
    ins.unsetf(std::ios_base::skipws);
    ins >> method_ >> sp1 >> fullp >> sp2 >> ver >> cr >> lf;
    LOG_I << method_ <<" "<< fullp;

    if (!ins || !path_params(path_, params_, fullp))
    {
        return sys::error_code(http::error::invalid_path_params,myerror_category::inst<myerror_category>());
    }
    LOG_I << params_;

    return ec;
}

std::ostream& operator<<(std::ostream& out, const request& req)
{
    return out << req.method() << " " << req.path() << "|" << req.params()
        << "|" << req.headers();
}

template <typename server>
struct acceptor : tcp::acceptor
{
    typedef acceptor<server> this_type;

    explicit acceptor(boost::asio::io_service & ios, server * s)
        : tcp::acceptor(ios)
    {
        s_ = s;
    }

    void start(tcp::endpoint const & ep)
    {
        ::listen(*this, ep); 

        tcp::socket & s = s_->new_socket();
        async_accept(s, boost::bind(&this_type::handle_accept, this, &s, placeholders::error));
    }

private:
    server* s_;

    void handle_accept(tcp::socket *s, sys::error_code const & ec)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        LOG_I << s->remote_endpoint();
        s_->handle_new_socket(ec);
        if (ec)
        {
            LOG_E << ec <<" "<< ec.message() <<" acceptor";
            return;
        }
        tcp::socket & ns = s_->new_socket();
        async_accept(ns, boost::bind(&this_type::handle_accept, this, &ns, placeholders::error));
    }
};

struct http_server;

struct http_connection : boost::noncopyable
{
    void notify_close(int ln, const char* fn);

    tcp::socket & socket() { return socket_; }

    boost::asio::io_service & get_io_service() { return socket_.get_io_service(); }

    typedef http_connection this_type;
    typedef request message;

    typedef boost::function<void (sys::error_code const &, message const&)> message_handler;

    explicit http_connection(http_server & s);
    ~http_connection();

private:
    http_server * server_;

    boost::asio::streambuf rsbuf_;
    message message_;

    tcp::socket socket_;
    boost::asio::deadline_timer deadline_;

    message_handler messager_;
    typedef boost::shared_ptr<http_connection> connection_ptr;
    typedef std::map<int,connection_ptr>::iterator iterator;
    iterator iself_;

    friend struct http_server;
};

struct http_server : acceptor<http_server>
{
    typedef http_server this_type;
    typedef http_connection connection;

    typedef boost::shared_ptr<connection> connection_ptr;
    typedef std::map<int,connection_ptr>::iterator iterator;

    typedef request message;

    typedef boost::function<void (sys::error_code const &, message const &)> messager_fn;
    typedef boost::function<messager_fn (connection&, sys::error_code const &, message const &)> messager_fn0;

    explicit http_server(boost::asio::io_service & ios);

    void start(tcp::endpoint const & ep, messager_fn0 const & fn);

public:
    tcp::socket & new_socket();
    void handle_new_socket(sys::error_code const & ec);

    //void _destroy(iterator i);//(, int ln,const char* fn);

private:
    void handle_method_line(iterator i, sys::error_code const &ec);

    void handle_headers(iterator i, sys::error_code const & ec);

    void handle_content(iterator i, sys::error_code const & ec);

    void handle_error(iterator i, sys::error_code const & ec, int ln,char const *fn);

    void _request(iterator i, sys::error_code const & ec, message const & m);

private:
    std::map<int,connection_ptr> cs_;
    connection_ptr c_;

    messager_fn0 messager0_;

    void ierase(iterator i) { cs_.erase(i); }
    friend struct http_connection;
};

void http_server::_request(iterator i, sys::error_code const & ec, message const & m)
{
    if (i->second->messager_)
    {
        i->second->messager_(ec, m);
        return;
    }
    i->second->messager_ = messager0_(*i->second, ec, m);
}

void http_server::handle_error(iterator i, sys::error_code const & ec, int ln,char const *fn)
{
    LOG_I << ec <<" "<< ec.message();
    if (ec == boost::asio::error::operation_aborted)
    {
        return;
    }

    connection_ptr & sh = i->second;

    //bool eof = (ec == boost::asio::error::eof);
    //if (!eof)
    {
        sys::error_code ec;
        tcp::endpoint ep = sh->socket().remote_endpoint(ec);
        if (!ec)
            LOG_I << ep;
        sh->socket().close(ec);
    }

    _request(i, ec, message());

    //if (!eof)
    {
        cs_.erase(i);
    }
    // _destroy ed
}

void http_server::handle_content(iterator i, sys::error_code const & ec)
{
    connection_ptr & sh = i->second;
    if (ec)
    {
        return handle_error(i, ec, __LINE__,__FUNCTION__);
    }

    sh->message_.parse_content(sh->rsbuf_);
    _request(i, ec, sh->message_);

    boost::asio::async_read_until(sh->socket(), sh->rsbuf_, "\r\n"
            , boost::bind(&this_type::handle_method_line, this, i, placeholders::error));
}

void http_server::handle_headers(iterator i, sys::error_code const & ec)
{
    connection_ptr & sh = i->second;
    if (ec)
    {
        return handle_error(i, ec, __LINE__,__FUNCTION__);
    }

    sys::error_code er;
    if ( (er = sh->message_.parse_header_lines(sh->rsbuf_)))
    {
        return handle_error(i, er, __LINE__,__FUNCTION__);
    }

    unsigned int len = get<int>(sh->message_.headers(), "Content-Length", 0);

    if (len <= sh->rsbuf_.size())
    {
        return handle_content(i, ec);
    }

    boost::asio::async_read(sh->socket(), sh->rsbuf_
            , boost::asio::transfer_at_least(len - sh->rsbuf_.size())
            , boost::bind(&this_type::handle_content, this, i, placeholders::error));
}

void http_server::handle_method_line(iterator i, sys::error_code const &ec)
{
    connection_ptr & sh = i->second;
    if (ec)
    {
        if (ec == boost::asio::error::eof)
            _request(i, ec, message());
        else
            handle_error(i, ec, __LINE__,__FUNCTION__);
        return;
    }

    sys::error_code er;
    if ( (er = sh->message_.parse_request_line(sh->rsbuf_)))
    {
        return handle_error(i, er, __LINE__,__FUNCTION__);
    }

    boost::asio::async_read_until(sh->socket(), sh->rsbuf_, "\r\n\r\n"
            , boost::bind(&this_type::handle_headers, this, i, placeholders::error));
}

http_server::http_server(boost::asio::io_service & ios)
    : acceptor<http_server>(ios, this)
{
}

void http_server::start(tcp::endpoint const & ep, messager_fn0 const & fn)
{
    messager0_ = fn;
    this->acceptor<http_server>::start(ep);
}

tcp::socket & http_server::new_socket()
{
    c_.reset(new connection(*this));
    return c_->socket();
}

void http_server::handle_new_socket(sys::error_code const & ec)
{
    if (ec)
    {
        LOG_I << ec;
        return;
    }
    std::pair<iterator,bool> ret = cs_.insert( std::make_pair(c_->socket().native_handle(), c_) );
    BOOST_ASSERT (ret.second);

    iterator i = ret.first;
    connection_ptr & sh = i->second;
    sh->iself_ = i;

    boost::asio::async_read_until(sh->socket(), sh->rsbuf_, "\r\n"
            , boost::bind(&this_type::handle_method_line, this, ret.first, placeholders::error));
}

//void http_server::_destroy(iterator i)//(, int ln,const char* fn)
//{
//    //LOG_I << ln <<":"<< fn;
//    if (i == cs_.end())
//        return;
//
//    //connection_ptr sp = i->second;
//    //void (connection_ptr::*fp)() = &connection_ptr::reset;
//    //get_io_service().post(boost::bind(fp, sp));
//}

http_connection::http_connection(http_server & s)
    : socket_(s.get_io_service())
    , deadline_(s.get_io_service())
{
    server_ = &s;
}

http_connection::~http_connection()
{
    LOG_I << static_cast<void*>(this);
}

void http_connection::notify_close(int ln, const char* fn)
{
    LOG_I << static_cast<void*>(this) <<" "<<ln <<":"<<fn <<" "<< __FILE__;

    sys::error_code ec;
    this->socket().close(ec);

    //ec = make_error_code(boost::asio::error::connection_aborted);
    socket().get_io_service().post(boost::bind(&http_server::ierase, server_, iself_));

    // c->server_->_destroy(iself_, ln,fn);
    //! this invalided
}

