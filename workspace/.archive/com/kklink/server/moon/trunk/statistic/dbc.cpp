#include <boost/assert.hpp>
#include <mysql/mysql.h>
#include "log.h"
#include "dbc.h"

using namespace std;
using namespace boost;

namespace sql {

    ostream& operator<< (ostream& out, const dbc::config& cfg)
    {
        return out << boost::format("Database %s:%d %s:%s %s") % cfg.host % cfg.port % cfg.user % cfg.pwd % cfg.db;
    }

    void dbc::init(const property_tree::ptree& ini)
    {
        config cfg;

        cfg.host = ini.get<std::string>("host");
        cfg.port = ini.get<int>("port");
        cfg.user = ini.get<std::string>("user");
        cfg.pwd = ini.get<std::string>("password");
        cfg.db = ini.get<std::string>("db");
        dbc::instance(&cfg);
    }

    dbc::~dbc()
    {
        for (iterator i = cs_.begin(); i != cs_.end(); ++i)
            if (i->using_ == 0)
                mysql_close(i->c_);
    }

    dbc& dbc::instance(dbc::config* c)
    {
        static dbc db_(*c);
        return db_;
    }

    dbc::connection dbc::connect()
    {
        dbc::iterator i = cs_.begin();
        while (i != cs_.end())
        {
            if (i->using_ == 0)
            {
                if (mysql_ping(i->c_) != 0)
                {
                    mysql_close(i->c_);
                    i = cs_.erase(i);
                    continue;
                }
                return connection(i);
            }
            ++i;
        }

        native_handle_type c = mysql_init(NULL);
        if (!c || !mysql_real_connect(c, cfg_.host.c_str(), cfg_.user.c_str(), 
                    cfg_.pwd.c_str(), cfg_.db.c_str(), cfg_.port, NULL, 0))
        {
            throw std::logic_error("db connect error");
        }

        if(mysql_set_character_set(c, "utf8")){
            cout<<"failed to set charset"<<endl;
        }

        cs_.push_back(_c_type(c));
        return connection(--cs_.end());
    }

    dbc::connection& dbc::connection::operator=(const dbc::connection& rhs)
    {
        if (this != &rhs)
        {
            --i_->using_;
            i_ = rhs.i_;
        }
        return *this;
    }

    void datas::exec(const string& sql, bool affair_flag)
    {
        BOOST_ASSERT(!result_);

        // if(result_){
        //     mysql_free_result(result_);
        //     result_ = NULL;
        // }

        native_handle_type pconn = dbc_.native_handle();

        if(affair_flag)
        {
            if(0 != mysql_autocommit(pconn,0)){
                throw std::logic_error("mysql_autommit error");
            }
        }

        if(0 != mysql_query(pconn, sql.c_str())){
            throw std::logic_error("mysql_query error");
        }

        if(affair_flag){
            if(0 != mysql_commit(pconn)){
                throw std::logic_error("mysql_commit error");
            }
        }

        result_ = mysql_store_result(pconn);
        unsigned int num_fields;
        unsigned int i;
        MYSQL_FIELD *fields;
        num_fields = mysql_num_fields(result_);
        fields = mysql_fetch_fields(result_);
        for(i = 0; i < num_fields; i++){   
            fields_.push_back(fields[i].name);
        }

    }

    void exec(const std::string& sql, const dbc::connection& c)
    {
        if (mysql_query(c.native_handle(), sql.c_str()) != 0)
        {
            throw std::runtime_error("mysql_query error");
        }
    }

    ostream& operator<<(ostream &os,datas& selected_datas)
    {
        string delim(100, '*');
        if(selected_datas.fields_.empty()) return os;
        os<<delim<<endl;
        for(std::vector<std::string>::iterator pos=selected_datas.fields_.begin();
                pos!=selected_datas.fields_.end(); ++pos){
            os<<"| "<<*pos;
        }
        os<<" |"<<endl;
        os<<delim<<endl;
        while(sql::datas::row_type row = selected_datas.next()){
            for(int i=0; i<selected_datas.fields_.size(); ++i){
                const char* data = row.at(i,"NULL");
                cout<<"| "<<data;
            }
            os<<" |"<<endl;
        }
        os<<delim<<endl;
        return os;
    }
}

// unsigned int datas::_index(const string& field)
// {
//     if (fields_.empty())
//     {
//         unsigned int num_fields;
//         unsigned int i;
//         MYSQL_FIELD *fields;
// 
//         num_fields = mysql_num_fields(result_);
//         fields = mysql_fetch_fields(result_);
//         fields_.reserve(num_fields);
//         clog << format("Num rows=%u fields=%u\n") % mysql_num_rows(result_) % num_fields;
//         for (i = 0; i < num_fields; i++)
//         {
//             fields_.push_back(fields[i].name);
//             // clog << format("Field %u is %s\n") % i % fields[i].name;
//         }
//         // if ( (ec = mysql_errno(my)) != 0)
//         // {
//         //     result_ = -1;
//         // }
//     }
//     for (unsigned int i = 0; i < fields_.size(); ++i)
//     {
//         // clog << format("fieldx %s\n") % fields_[i];
//         if (fields_[i] == field)
//             return i;
//     }
//     return fields_.size();
// }


// std::ostream& operator<<(std::ostream& outs, const std::vector<std::string>& c)
// {
//     std::vector<std::string>::const_iterator it = c.begin();
//     for ( ; it != c.end(); ++it)
//         outs << *it << " ";
//     return outs;
// }


