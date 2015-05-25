#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
// #include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <mysql/mysql.h>

using namespace std;
using namespace boost;

struct Db_cfg
{
    string host;
    int port;
    string user, pwd;
    string db;
    Db_cfg() : port(0) {}
};

struct MySQL_db : boost::noncopyable
{
    struct _Conn
    {
        MYSQL* c_;
        int using_;
        _Conn(MYSQL* c) { using_ = 0; c_ = c; }
    };

    typedef list<_Conn>::iterator iterator;

    struct Connection
    {
        MYSQL* native_handle() const { return i_->c_; }

        explicit Connection(iterator i) : i_(i) { ++i_->using_; clog << i->c_ << "\n"; }
        Connection(const Connection& rhs) : i_(rhs.i_) { ++i_->using_; }
        ~Connection() {clog << i_->using_ << "\n"; --i_->using_; }

        iterator i_;
    };

    bool empty() const { return cs_.empty(); }

    Connection connect()
    {
        iterator i = cs_.begin();
        while (i != cs_.end())
        {
            if (i->using_ == 0)
            {
                if (!mysql_ping(i->c_))
                {
                    i = cs_.erase(i);
                    continue;
                }
                return Connection(i);
            }
            ++i;
        }

        MYSQL* c = mysql_init(NULL);
        if (!c || !mysql_real_connect(c, cfg_.host.c_str(), cfg_.user.c_str(), cfg_.pwd.c_str(), cfg_.db.c_str(), cfg_.port, NULL, 0))
        {
            throw std::logic_error("db connect error");
        }

        cs_.push_back(_Conn(c));
        return Connection(--cs_.end());
    }

    MySQL_db(const Db_cfg& cfg)
        : cfg_(cfg)
    {}
    MySQL_db()
    {}

    ~MySQL_db()
    {
        for (iterator i = cs_.begin(); i != cs_.end(); ++i)
            if (i->using_ == 0)
                mysql_close(i->c_);
    }

private:
    Db_cfg cfg_; // std::string host_; int port_; std::string user_, pwd_, db_;
    list<_Conn> cs_;
};

static Db_cfg read_dbcfg(istream& ins)
{
    Db_cfg cfg;

    property_tree::ptree pt;
    property_tree::ini_parser::read_ini(ins, pt);
    property_tree::ptree& defa = pt.get_child("database");

    cfg.host = defa.get<string>("host");
    cfg.port = defa.get<int>("port");
    cfg.user = defa.get<string>("user");
    cfg.pwd = defa.get<string>("password");
    cfg.db = defa.get<string>("db");

    clog << format("dbcfg %s:%d %s:%s %s\n") % cfg.host % cfg.port % cfg.user % cfg.pwd % cfg.db;
    return cfg;
}

MySQL_db& db(istream* ins = 0)
{
    static MySQL_db db_(read_dbcfg(*ins));
    return db_;
}

struct Datas_row : boost::noncopyable
{
    struct iterator;

    Datas_row(MySQL_db::Connection dbc)
        : dbc_(dbc), result_(0)
    {
    }

    ~Datas_row()
    {
        if (result_)
            mysql_free_result(result_);
    }

    int exec(const string& sql)
    {
        clog << sql << "\n";

        if (mysql_query(dbc_.native_handle(), sql.c_str()) != 0)
        {
            throw std::logic_error("mysql_query error");
        }
        result_ = mysql_store_result(dbc_.native_handle());
        return 0;
    }

    int exec(const format& sql) { return exec(sql.str()); }

    Datas_row* next()
    {
        row_ = mysql_fetch_row(result_);
        // clog << format("row %s %s %s\n") % row_[0] % row_[1] % row_[2];
        return (row_ ? this : 0);
    }

    char* at(unsigned int idx)
    {
        if (idx >= fields_.size())
            throw std::logic_error("fields index error");
        // clog << format("get %p\n") % row_;
        // clog << format("get %p %s %s %s\n") % row_ % row_[0] % row_[1] % row_[2];
        return row_[idx];
    }

    char* operator[](unsigned int idx) { return at(idx); }
    char* operator[](const char* fieldname) { return at(_index(fieldname)); }

    template <typename T> T get(unsigned int idx) { return lexical_cast<T>(at(idx)); }
    template <typename T> T get(const char* fieldname) { return this->get<T>(_index(fieldname)); }
    template <typename T> T get(const string& fieldname) { return this->get<T>(_index(fieldname.c_str())); }

private:
    unsigned int _index(const string& field)
    {
        if (fields_.empty())
        {
            unsigned int num_fields;
            unsigned int i;
            MYSQL_FIELD *fields;

            num_fields = mysql_num_fields(result_);
            fields = mysql_fetch_fields(result_);
            fields_.reserve(num_fields);
            clog << format("Num rows=%u fields=%u\n") % mysql_num_rows(result_) % num_fields;
            for (i = 0; i < num_fields; i++)
            {
                fields_.push_back(fields[i].name);
                // clog << format("Field %u is %s\n") % i % fields[i].name;
            }
            // if ( (ec = mysql_errno(my)) != 0)
            // {
            //     result_ = -1;
            // }
        }
        for (unsigned int i = 0; i < fields_.size(); ++i)
        {
            // clog << format("fieldx %s\n") % fields_[i];
            if (fields_[i] == field)
                return i;
        }
        return fields_.size();
    }

    MySQL_db::Connection dbc_;
    MYSQL_RES* result_;
    MYSQL_ROW row_;
    vector<string> fields_;

public:
    friend struct iterator;
    struct iterator : boost::iterator_facade<iterator, Datas_row&, boost::single_pass_traversal_tag>
    {
        explicit iterator(Datas_row* t)
            : thiz_(t)
        {}

    private:
        Datas_row* thiz_;

        friend class boost::iterator_core_access;

        void increment() { thiz_ = thiz_->next(); }
        Datas_row& dereference() const { return *thiz_; }
        bool equal(iterator const &rhs) const { return (thiz_==0 && 0==rhs.thiz_) || (thiz_ && rhs.thiz_==thiz_); }
    };

    iterator begin() { return iterator(next()); }
    iterator end() { return iterator(0); }
};

ostream& operator<<(ostream& outs, const vector<string>& c)
{
    vector<string>::const_iterator it = c.begin();
    for ( ; it != c.end(); ++it)
        outs << *it << " ";
    return outs;
}

int main(int ac, char *const av[])
{
    const char * dbcfg = "[database]\n"
        "host=127.0.0.1\n"
        "port=3306\n"
        "user=lindu\n"
        "password=lindu12345\n"
        "db=lindu_test\n"
        ;

    //istringstream ins(dbcfg);
    //db(&ins);
    db(&std::cin);

    {
        Datas_row qq(db().connect());
        qq.exec("SELECT * from users");

        Datas_row qq2(db().connect());
        qq2.exec( "SELECT * from users");
    }

    Datas_row q(db().connect());
    q.exec("SELECT * from users");

    Datas_row q2(db().connect());
    q2.exec("SELECT * from users");

    Datas_row q3(db().connect());
    q3.exec("SELECT * from users");

    // while (q3.next())
    //     clog << format("phone %s\n") % q3.get<string>("user_phone");
    // //while (q2.next())
    // //    clog << format("phone %s\n") % q2.get<string>("user_phone");
    // // vector<string> q2v(q2.begin(), q2.end());
    // // clog << q2v << endl;

    // for (Datas_row::iterator it = q.begin();
    //         it != q.end(); ++it)
    //     clog << format("phone %s\n") % it->get<string>("user_phone");

    // clog << endl;
    // for (Datas_row::iterator it = q.begin();
    //         it != q.end(); ++it)
    //     clog << format("phone %s\n") % it->get<string>("user_phone");

    return 0;
}

