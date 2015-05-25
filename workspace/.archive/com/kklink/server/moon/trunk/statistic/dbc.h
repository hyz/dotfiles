#ifndef _DB_H_
#define _DB_H_

#include <stdexcept>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <mysql/mysql.h>
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

    std::list<_c_type> cs_;
};

struct datas_row_type
{
    typedef datas_row_type self_type;

    operator bool() const { return bool(row_); }

    const char* at(unsigned int idx)
    {
        // if (idx >= fields_.size()) throw std::logic_error("fields index error");
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
        exec(sql);
    }

    template <typename Sql>
    explicit datas(const Sql& sql) : dbc_(dbc::instance().connect())
    {
        result_ = 0;
        exec(sql);
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
    std::vector<std::string> fields_;
    // unsigned int _index(const std::string& field);

    void exec(const std::string& sql, bool affair_flag=false);

    void exec(const boost::format& sql, bool affair_flag=false){ 
        exec(sql.str(), affair_flag); 
    }

    friend std::ostream& operator<<(std::ostream &os, datas& selected_datas);
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

void exec(const std::string& sql, const dbc::connection& c);

inline void exec(const boost::format& sql, const dbc::connection& c)
{
    exec(sql.str(), c);
}

template <typename Sql> inline void exec(const Sql& sql)
{
    exec(sql, dbc::instance().connect());
}

} // namespace


#endif
