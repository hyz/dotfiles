#ifndef READX_H__
#define READX_H__

#include <string>
#include <vector>
#include <list>
#include <ostream>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include "async.h"
#include "jss.h"

struct Http_reader;
typedef std::list<std::pair<std::string, std::string> > keyvalue_type;

class Http_kvals : public keyvalue_type
{
public:
    using std::list<std::pair<std::string, std::string> >::iterator;
    using std::list<std::pair<std::string, std::string> >::const_iterator;

    iterator find(const std::string& k)
    {
        iterator i = begin();
        for (; i != end(); ++i)
            if (i->first == k)
                break;
        return i;
    }

    const_iterator find(const std::string& k) const
        { return const_cast<Http_kvals*>(this)->find(k); }


    const std::string get(const std::string& k, const std::string& defa) const
    {
        const_iterator i = find(k);
        return (i == end() ? defa : i->second);
    }

    const std::string& get(const std::string& k) const
    {
        const_iterator i = find(k);
        if (i == end())
            throw std::runtime_error("key not found");
        return i->second;
    }

    iterator insert(const value_type & kv)
    {
        return keyvalue_type::insert(end(), kv);
    }

    // std::string& operator[](const std::string& k)
    // {
    //     iterator i = find(k);
    //     if (i == end())
    //         i = this->insert(this->end(), std::make_pair(k, std::string()));
    //     return i->second;
    // }

    // const std::string& operator[](const std::string& k) const { return const_cast<Http_kvals*>(this)->operator[](k); }
};

void parse_keyvalues(Http_kvals &params, const char* beg, const char* end);

inline void parse_keyvalues(Http_kvals &params, const std::string & cont)
{
    return parse_keyvalues(params, cont.data(), cont.data()+cont.size());
}

std::ostream& operator<<(std::ostream& out, const Http_kvals& kv);

typedef Http_kvals Http_headers;

std::ostream& operator<<(std::ostream& outs, const Http_kvals& headers);

template <typename T>
inline bool is_exist(const T & c, const std::string& k) { return c.find(k) != c.end(); }

struct Http_request
{
    const std::string& param(const std::string& name) const;
    const std::string  param(const std::string& name, const std::string& defa) const;

    const std::string& header(const std::string& name) const;
    const std::string  header(const std::string& name, const std::string& defa) const;

    void set_header(const std::string& name, const std::string& val)
    {
        headers_.push_back(std::make_pair(name, val));
    }

    void set_param(const std::string& name, const std::string& val)
    {
        params_.push_back(std::make_pair(name, val));
    }

    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }

    const std::string& content() const { return content_; }

    const Http_kvals & params() const { return params_; }
    const Http_kvals & headers() const { return headers_; }

    bool keep_alive() const { return false; }

    Http_request() { content_length_ = 0; }
    // Http_request(const std::string& method, const std::string& path) { content_length_ = 0; }

    bool parse_method_line(const std::string & line);
    bool parse_header_line(const std::string & line);

private:
    std::string method_;
    std::string path_;
    std::string http_version_;
    Http_kvals params_;

    Http_kvals headers_;

    std::string content_;

    template <typename T>
    static typename T::iterator ifind(T const & c, const std::string& name) { return const_cast<T&>(c).find(name); }

    int content_length() const { return content_length_; }

    int content_length_;

    friend struct Http_reader;
    friend std::ostream& operator<<(std::ostream& out, const Http_request& req);
};

std::ostream& operator<<(std::ostream& out, const Http_request& req);

class Http_response
{
    int status_code_;

    Http_kvals headers_;

    std::string content_;

    bool keep_alive_;

    std::string method_;

public:
    bool keep_alive() const { return keep_alive_; }
    void keep_alive(bool ka) { keep_alive_ = ka; }

    int status_code() const { return status_code_; }
    void status_code(int c) { status_code_ = c; }

public:
    Http_response(const std::string& meth, bool ka);

    void header(const std::string& k, const std::string& val);

    void content(const json::object& jso) { content_ = json::encode(jso); }
    void content(const std::string& cont) { content_ = cont; }
    void content(const boost::format& fmt) { content_ = boost::str(fmt); }
    // std::string& content() { return content_; }

    std::string str() const;
    std::string str_header() const;

    bool empty() const { return content_.empty(); }

private:
    static std::string code_str(int code);
};

struct Http_reader : boost::enable_shared_from_this<Http_reader> //, boost::noncopyable
{
    typedef Http_request DataType;

    typedef Http_reader This;

    Http_reader(Socket_ptr soc, boost::function<bool (Socket_ptr, Http_request&)> rsp)
        : socket_(soc)
        , fwdp_(rsp)
    {
    }

    ~Http_reader();

    void start();

private:
    // typedef boost::shared_ptr<This> This_Shptr;

    Socket_ptr socket_;
    boost::asio::streambuf rsbuf_;
    Http_request req_;

    boost::function<bool (Socket_ptr, Http_request&)> fwdp_;

private:
    void handle_method_line(const boost::system::error_code& err);

    void handle_headers(const boost::system::error_code& err);

    void handle_content(const boost::system::error_code& err);

    void handle_err(const boost::system::error_code& err, int lineno, const char* filename);
};

enum {
    Bad_Request_400 = 400
        , Unauthorized_401 = 401
        , Forbidden_403 = 403
        , Not_Found_404 = 404

        , Internal_Server_Error_500 = 500
};

namespace http {
    typedef Http_request request;
    typedef Http_response response;
    typedef Http_kvals params;
    typedef Http_reader reader;

    struct error_category : ::myerror_category
    {
        error_category();
    };
}

#endif

