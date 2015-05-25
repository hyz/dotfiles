#include "myconfig.h"
#include <iostream>

#include <boost/range.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>

#include "myerror.h"
#include "log.h"
#include "util.h"
#include "proto_http.h"

// void sigexit(asio::io_service *io_service) { io_service->stop(); }

using namespace std;
using namespace boost;

enum {
    MYERR_BASE = 50
    , MYERR_1 = 51
    , MYERR_2 = 52
};

#undef Error_Category
#define Error_Category http::error_category

const boost::system::error_category & errcat();

// inline void errr(int ec, int lineno, const char* filename)
// {
//     errr(system::error_code(ec, myerror_category()), lineno, filename);
// }

#define ERRR(x) handle_err(x, __LINE__,__FILE__)

std::ostream& operator<<(std::ostream& out, const Http_kvals& kv)
{
    Http_kvals::const_iterator i = kv.begin();
    for (; i != kv.end(); ++i)
        out << i->first << "=" << i->second << " ";
    return out;
}

std::ostream& operator<<(std::ostream& out, const Http_request& req)
{
    return out << req.method() << " " << req.path() << "|" << req.params()
        << "|" << req.headers(); //req.header("Cookie","");
}

string urldecode(const string &str_source);

inline string unescape(string::const_iterator b, string::const_iterator end)
{
    return urldecode(string(b, end));
}

Http_reader::~Http_reader()
{
    // LOG_I << "~Http_reader()";
}

void Http_reader::start()
{
    asio::async_read_until(*socket_, rsbuf_, "\r\n",
            bind(&This::handle_method_line, shared_from_this(), asio::placeholders::error));
}

void parse_keyvalues(Http_kvals &params, const char* beg, const char* end)
{
    const char *i, *eop;
    while (beg != end)
    {
        eop = std::find(beg, end, '&');
        i = std::find(beg, eop, '=');

        if (i == eop)
            break;
        params.insert(std::make_pair(urldecode(beg, i), urldecode(i+1, eop)));
        if (eop == end)
            break;

        beg = eop;
        ++beg;
    }
}

static bool path_params(string& path, Http_kvals& params, const string& fullpath)
{
    // typedef iterator_range<string::const_iterator> IR;
    // s = find(s, token_finder(is_any_of("#"), token_compress_on));
    const char *i, *b, *end;

    i = fullpath.data();
    end = &i[fullpath.size()];

    while (*i == '/')
        ++i;
    end = find(i, end, '#');

    b = find(i, end, '?');
    if (i != b)
        path.assign(i, b);
    if (b != end)
    {
        ++b;
        parse_keyvalues(params, b, end);
    }
    // clog << path << "\n" << params << "\n";

    return true;
}

bool Http_request::parse_method_line(const std::string& line)
{
    regex e("^\\s*(GET|POST)\\s+([^\\s]+)\\s+(HTTP/1.[01])\\s*$");
    smatch what;
    if (!regex_match(line, what, e))
    {
        return false; // THROW_EX(Bad_Request_400);
    }

    http_version_ = what[3];
    method_ = what[1];

    path_params(path_, params_, what[2]);

    return true;
    // string tmp;
    // ins >> tmp >> req_.http_version_;
    // if (!ins)
    //     return ERRR(MYERR_1); //_error(system::error_code(MYERR_1, myerror_category()));
    // path_params(req_.path_, req_.params_, tmp);
    // // LOG_I << req_.method << " " << req_.path << " " << req_.params << " " << req_.http_version;
    // getline(ins, tmp);
}

bool Http_request::parse_header_line(const std::string & line)
{
    regex expr("^\\s*([^:]+)\\s*:\\s*(.*?)\\s*$");
    smatch res;
    if (!regex_match(line, res, expr))
        return false;

    string k(res[1].first, res[1].second);
    string val(res[2].first, res[2].second);
    headers_.insert(make_pair(k,val));

    if (iequals(k, "Content-Length"))
    {
        content_.resize(lexical_cast<int>(val));
        // content_length_ = lexical_cast<int>(val);
    }

    // "Connection: close";
    // LOG_I << format("%1% %2%\t#%3%") % k % val % line;

    return true;
}

const std::string& Http_request::header(const std::string& name) const
{
    Http_kvals::iterator it = ifind(headers_,name);
    if (it == headers_.end())
        THROW_EX(EN_HTTPHeader_NotFound); // throw std::runtime_error("header name Not found");
    return it->second;
}

const std::string Http_request::header(const std::string& name, const std::string& defa) const
{
    Http_kvals::iterator it = ifind(headers_,name);
    if (it == headers_.end())
        return defa;
    return it->second;
}

const std::string& Http_request::param(const std::string& name) const
{
    Http_kvals::iterator it = ifind(params_,name);
    if (it == params_.end())
        THROW_EX(EN_HTTPParam_NotFound); //throw std::runtime_error("param name Not found");
    return it->second;
}

const std::string Http_request::param(const std::string& name, const std::string& defa) const
{
    Http_kvals::iterator it = ifind(params_,name);
    if (it == params_.end())
        return defa;
    return it->second;
}

void Http_reader::handle_method_line(const boost::system::error_code& err)
{
    if (err)
    {
        if (err == asio::error::eof)
        {
            return;
        }
        return ERRR(err);
    }

    istream ins(&rsbuf_);
    string line;
    while (getline(ins, line)) // while (ins >> req_.method_)
    {
        if (!all(line, is_space()))
            break;
    }
    if (!ins)
    {
        asio::async_read_until(*socket_, rsbuf_, "\r\n",
                bind(&This::handle_method_line, shared_from_this(), asio::placeholders::error)
                );
        return;
    }

    req_ = Http_request();
    if (!req_.parse_method_line(line))
    {
        boost::system::error_code ec(Bad_Request_400, errcat());
        return ERRR(ec);
    }

    asio::async_read_until(*socket_, rsbuf_, "\r\n\r\n",
            bind(&This::handle_headers, shared_from_this(), asio::placeholders::error));
}

void Http_reader::handle_headers(const boost::system::error_code& err)
{
    if (err)
    {
        return ERRR(err);
    }

    istream ins(&rsbuf_);
    string line;
    while (getline(ins, line) && !line.empty() && line[0] != '\r')
    {
        if (!req_.parse_header_line(line))
        {
            boost::system::error_code ec(Bad_Request_400, errcat());
            return ERRR(ec);
        }
    }

    // LOG_I << format("content-length %d/%d") % rsbuf_.size()  % req_.content.size();

    if (rsbuf_.size() >= req_.content_.size())
    {
        return handle_content(err);
    }

    asio::async_read(*socket_ , rsbuf_
            , asio::transfer_at_least(req_.content_.size() - rsbuf_.size())
            , bind(&This::handle_content, shared_from_this(), asio::placeholders::error));
}

void Http_reader::handle_content(const boost::system::error_code& err)
{
    // LOG_I << "content " << err;
    if (err)
    {
        //LOG_I << socket_ <<" "<< req_ << " aborted";
        return ERRR(err);
    }

    rsbuf_.sgetn(&req_.content_[0], req_.content_.size());

    if (fwdp_(socket_, req_))
    {
        // Next Http_request
        asio::async_read_until(*socket_, rsbuf_, "\r\n",
                bind(&This::handle_method_line, shared_from_this(), asio::placeholders::error));
    }
}

void Http_reader::handle_err(const boost::system::error_code& err, int line, const char* file)
{
    LOG_E << socket_ << format(" %1%:%2% %3%:%4%") % err % err.message() % line % file;

    // if (err == asio::error::eof)
    // {
    // }
}

// class Error_category : public system::error_category
// {
// public:
//     Error_category() : system::error_category() { }
// 
//     const char * name() const // BOOST_SYSTEM_NOEXCEPT
//     {
//         return "Http";
//     }
// 
//     system::error_condition default_error_condition( int ev ) const // BOOST_SYSTEM_NOEXCEPT
//     {
//         return ev < MYERR_BASE
//             ? system::error_condition( system::errc::io_error, system::generic_category() )
//             : system::error_condition( ev, ::myerror_category() );
//     }
// 
//     string message( int ev ) const
//     {
//         if ( ev == MYERR_1 ) return string("ERR 1");
//         if ( ev == MYERR_2 ) return string("ERR 2");
//         return string("unknown error");
//     }
// };
// 
// const boost::system::error_code boo_boo( 456, myerror_category );
// const boost::system::error_code big_boo_boo( 789, myerror_category );

Http_response::Http_response(const std::string& meth, bool ka)
    : method_(meth)
{
    status_code_ = 200;
    keep_alive_ = ka;

    header("Content-Type", "text/plain;charset=UTF-8");
}

void Http_response::header(const std::string& k, const std::string& val)
{
    for (Http_kvals::iterator i = headers_.begin(); i != headers_.end(); ++i)
    {
        if (i->first == k)
        {
            i->second = val;
            return;
        }
    }
    headers_.insert(std::make_pair(k, val));
}

string Http_response::code_str(int code)
{
    return http::error_category::inst<http::error_category>().message(code);
}

std::string Http_response::str() const
{
    return str_header() + "\r\n" + content_;
}

std::string Http_response::str_header() const
{
    std::string hdrs;
    Http_kvals::const_iterator it = headers_.begin();
    for ( ; it != headers_.end(); ++it)
    {
        hdrs += it->first + ": " + it->second + "\r\n";
    }
    hdrs += "Content-Length: " + lexical_cast<string>(content_.size()) + "\r\n";

    format lin0("HTTP/1.1 %d %s\r\n");
    lin0 % status_code_ % code_str(status_code_);

    // LOG_I << lin0.str() << hdrs << "\r\n";

    return lin0.str() + hdrs;
}

namespace http {

error_category::error_category()
{
    static code_string ecl[] = {
        { 200, "OK" }
        , { Bad_Request_400, "Bad Request" }
        , { Unauthorized_401, "Unauthorized" }
        , { Forbidden_403, "Forbidden" }
        , { Not_Found_404, "Not Found" }
        , { Internal_Server_Error_500, "Internal Server Error" }
    };

    Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])], __FILE__);
}

} // namespace http

const boost::system::error_category & errcat()
{
    static const http::error_category ecat;
    return ecat;
}

