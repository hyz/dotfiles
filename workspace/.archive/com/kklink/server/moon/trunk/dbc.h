#ifndef _DB_H_
#define _DB_H_

#include <stdexcept>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <mysql/mysql.h>
#include <hiredis/hiredis.h>

#include "myerror.h"
#include "log.h"

namespace sql { // namespace

struct dbc;
struct datas;

typedef MYSQL* native_handle_type;

struct dbc : boost::noncopyable
{
    struct _c_type
    {
        native_handle_type c_;
        int using_;
        _c_type(native_handle_type c) { using_ = 0; c_ = c; }
    };

    typedef std::list<_c_type>::iterator iterator;

    struct config
    {
        std::string host;
        int port;
        std::string user, pwd;
        std::string db;
        config() : port(0) {}
    };

    struct connection
    {
        ~connection() { --i_->using_; }
        connection(const connection& rhs) : i_(rhs.i_) { ++i_->using_; }
        connection& operator=(const connection& rhs);

        native_handle_type native_handle() const { return i_->c_; }

    private:
        explicit connection(iterator i) : i_(i) { ++i_->using_; }
        iterator i_;
        friend struct dbc;
    };

    connection connect();

    bool empty() const { return cs_.empty(); }

    dbc(const config& cfg)
        : cfg_(cfg)
    {}
    dbc()
    {}

    ~dbc();

    static dbc& instance(dbc::config* c = 0);
    static void init(const boost::property_tree::ptree& ini);

private:
    config cfg_; // std::string host_; int port_; std::string user_, pwd_, db_;

    boost::mutex mutex_;
    std::list<_c_type> cs_;
};

struct datas_row_type
{
    typedef datas_row_type self_type;

    operator bool() const { return bool(row_); }

    const char* at(unsigned int idx)
    {
        // if (idx >= fields_.size()) throw std::logic_error("fields index error");
        // if (!row_[idx]) THROW
        return row_[idx];
    }
    const char* at(unsigned int idx) const { return const_cast<datas_row_type*>(this)->at(idx); }
    const char* at(unsigned int idx, const char* defa)
    {
        return row_[idx] ? row_[idx] : defa;
    }
    const char* at(unsigned int idx, const char *defa) const
        { return const_cast<datas_row_type*>(this)->at(idx,defa); }

    const char* operator[](unsigned int idx) { return at(idx); }
    const char* operator[](unsigned int idx) const { return at(idx); }

private:
    MYSQL_ROW row_;

    explicit datas_row_type(MYSQL_ROW r=0) { row_ = r; }
    friend struct datas;

    // char* operator[](const char* fieldname) { return at(_index(fieldname)); }

    // template <typename T> T& _get(unsigned int idx, T* defa)
    // {
    //     if (!at(idx))
    //     {
    //         if (!defa)
    //             throw std::runtime_error("datas row at: error");
    //         return *defa;
    //     }
    //     return boost::lexical_cast<T>(at(idx));
    // }

    // template <typename T> T get(const char* fieldname) { return this->get<T>(_index(fieldname)); }
    // template <typename T> T get(const std::string& fieldname) { return this->get<T>(_index(fieldname.c_str())); }
};

struct datas : boost::noncopyable
{
    typedef datas_row_type row_type;

    // struct iterator;

    template <typename Sql>
    datas(const Sql& sql, const dbc::connection& c) : dbc_(c)
    {
        result_ = 0;
        query(sql);
    }

    template <typename Sql>
    explicit datas(const Sql& sql) : dbc_(dbc::instance().connect())
    {
        result_ = 0;
        query(sql);
    }

    ~datas()
    {
        if (result_)
            mysql_free_result(result_);
    }

    int count() const { return result_ ? mysql_num_rows(result_) : 0; }

    row_type next()
    {
        return row_type(mysql_fetch_row(result_));
    }


private:
    dbc::connection dbc_;
    MYSQL_RES* result_;
    // std::vector<std::string> fields_;
    // unsigned int _index(const std::string& field);

    void query(const std::string& sql);
    void query(const boost::format& sql) { query(sql.str()); }

public:
    // friend struct iterator;
    // struct iterator : boost::iterator_facade<iterator, row_type, boost::single_pass_traversal_tag>
    // {
    //     iterator()
    //         : result_(0), row_(0)
    //     {}

    //     explicit iterator(MYSQL_RES* t, MYSQL_ROW* r)
    //         : result_(t), row_(r)
    //     {
    //         if (!r)
    //             increment()
    //     }

    // private:
    //     MYSQL_RES* result_; // datas* thiz_;
    //     MYSQL_ROW row_;

    //     friend class boost::iterator_core_access;

    //     void increment() { row_ = mysql_fetch_row(result_); }
    //     row_type dereference() const { return row_type(row_); }
    //     bool equal(iterator const &rhs) const { return row_ == rhs.row_; }
    // };

    // iterator begin() { return iterator(result_, 0); }
    // iterator end() { return iterator(0); }

};

void exec(boost::system::error_code & ec, const std::string& sql, const dbc::connection& c);

inline void exec(boost::system::error_code & ec, const boost::format& sql, const dbc::connection& c)
    { exec(ec, sql.str(), c); }

template <typename Sql> void exec(boost::system::error_code & ec, const Sql& sql)
{
    exec(ec, sql, dbc::instance().connect());
}

template <typename Sql> void exec(const Sql& sql)
{
    boost::system::error_code ec;
    exec(ec, sql, dbc::instance().connect());
    if (ec)
    {
        mythrow(__LINE__,__FILE__,ec);
    }
}

// template <typename Sql> inline void exec(const Sql& sql)
// {
//     exec(sql, dbc::instance().connect());
// }

std::string db_escape(const std::string& param, const dbc::connection& c);
inline std::string escape(const std::string& param, const dbc::connection& c) { return db_escape(param, c); }

inline std::string db_escape(const boost::format& param, const dbc::connection& c)
{
    return db_escape(param.str(), c);
}

template <typename Sql_param> inline std::string db_escape(const Sql_param& param)
{
    return db_escape(param, dbc::instance().connect());
}

struct initializer : boost::noncopyable
{
    initializer (const boost::property_tree::ptree & ini);
    ~initializer();
};

} // namespace

template <typename Tab>
int max_index(int x, char const *tab=0)
{
    static int idx_ = 0;
    if (idx_ == 0)
    {
        idx_ = 1;

        sql::datas datas(std::string("SELECT MAX(id) FROM ") + (tab ? tab : Tab::db_table_name()));
        if (sql::datas::row_type row = datas.next())
        {
            if (row[0])
                idx_ = std::atoi(row[0]) + 1;
        }
    }
    int ret = idx_;
    idx_ += x;
    return ret;
}

#define DEFA_REDIS_HOST  "127.0.0.1"
#define DEFA_REDIS_PORT 6379

template <typename X, typename Y>
std::ostream & operator<<(std::ostream & out, std::pair<X,Y> const & p)
{
    return out << "[" << p.first <<","<< p.second << "]";
}

namespace redis {

typedef boost::shared_ptr<redisContext> context_ptr;
typedef std::pair<std::string, unsigned short> endpoint;
typedef boost::shared_ptr<redisReply> reply;

struct context_helper
{
    static context_helper make(std::string const& host, unsigned short port);
    static context_helper make();

    redis::reply reply();

    ~context_helper();

private:
    // context_helper() { n_reply_ = 0; }
    context_helper(context_ptr ctx, endpoint ep);

    context_ptr ctxp_;
    endpoint endp_;
    int n_reply_;

    context_helper& operator=(const context_helper & rhs);

protected:
    void push_command(int ac, char const* av[], size_t len_v[])
    {
        redisAppendCommandArgv(ctxp_.get(), ac, av, len_v);
        ++n_reply_;
    }

    context_ptr & get_context_ptr() { return ctxp_; }
};

struct context : context_helper
{
    template <typename... A>
    context & append(A... a)
    {
        this->app(a...);
        argv_.clear();
        argv_len_.clear();
        tmps_.clear();
        return *this;
    }

    ~context();
    explicit context(context_helper ctx) : context_helper(ctx) {  }

private:
    std::vector<char const*> argv_;
    std::vector<size_t> argv_len_;
    std::list<std::string> tmps_;

    template <typename T> void app(T const& x)
    {
        this->app2(x);
        LOG_I << argv_;
        this->push_command(argv_.size(), &argv_[0], &argv_len_[0]);
    }

    template <typename T, typename... A> void app(T const& x, A... a)
    {
        this->app2(x);
        this->app(a...);
    }

    void app2(char const * s)
    {
        argv_.push_back(s);
        argv_len_.push_back(strlen(s));
    }
    void app2(std::string const & s)
    {
        argv_.push_back(s.data());
        argv_len_.push_back(s.size());
    }
    template <typename T> void app2(T const & x)
    {
        tmps_.push_back( boost::lexical_cast<std::string>(x) );
        this->app2(tmps_.back());
    }

    context& operator=(context const & rhs);
};

template <typename ...A>
redis::reply command(A... a)
{
    redis::context ctx(redis::context::make());
    ctx.append(a...);
    return ctx.reply();
}

template <typename OIter>
void keys(std::string const & kp, OIter it)
{
    auto reply = redis::command("KEYS", kp);
    if (!reply || reply->type != REDIS_REPLY_ARRAY)
        return;

    for (unsigned int i = 0; i < reply->elements; i++)
        *it++ = (reply->element[i]->str);
}

} // namespace redis

#endif

