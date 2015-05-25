#include "myconfig.h"
#include <boost/assert.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <mysql/mysql.h>
#include "myerror.h"
#include "log.h"
#include "dbc.h"

using namespace std;
using namespace boost;

static std::string redis_host_ = DEFA_REDIS_HOST;
static unsigned short redis_port_ = DEFA_REDIS_PORT;

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

    redis_host_ = ini.get_optional<std::string>("redis_host").get_value_or(DEFA_REDIS_HOST);
    redis_port_ = ini.get_optional<unsigned short>("redis_port").get_value_or(DEFA_REDIS_PORT);

    LOG_I << cfg;
    LOG_I << "redis " << redis_host_ <<" "<< redis_port_;

    dbc::instance(&cfg);
}

dbc::~dbc()
{
    // boost::unique_lock<boost::mutex> scoped_lock(mutex_);
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
    boost::unique_lock<boost::mutex> scoped_lock(mutex_);

    dbc::iterator i = cs_.begin();
    while (i != cs_.end())
    {
        if (i->using_ == 0)
        {
            if (mysql_ping(i->c_) != 0)
            {
                LOG_I << "MySQL ping " << mysql_errno(i->c_) << mysql_error(i->c_);
                mysql_close(i->c_);
                i = cs_.erase(i);
                continue;
            }
            return connection(i);
        }
        ++i;
    }

    LOG_I << "mysql_init " << cs_.size();

    native_handle_type c = mysql_init(NULL);
    if (!c)
    {
        LOG_I << "MySQL init "<< mysql_errno(c) << mysql_error(c);
        THROW_EX(EN_DBConnect_Fail);
    }
    if (!mysql_real_connect(c
                , cfg_.host.c_str(), cfg_.user.c_str(), cfg_.pwd.c_str()
                , cfg_.db.c_str(), cfg_.port, NULL, 0))
    {
        LOG_I << "MySQL real connect "<< mysql_errno(c) << mysql_error(c);
        THROW_EX(EN_DBConnect_Fail);
    }

    if(mysql_set_character_set(c, "utf8"))
    {
        LOG_I << "MySQL character "<< mysql_errno(c) << mysql_error(c);
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
        ++i_->using_;
    }
    return *this;
}

void datas::query(const string& sql)
{
    BOOST_ASSERT(!result_);

    // if(result_){
    //     mysql_free_result(result_);
    //     result_ = NULL;
    // }

    native_handle_type c = dbc_.native_handle();

    // if(affair_flag)
    // {
    //     if(0 != mysql_autocommit(c,0)){
    //         THROW_EX(EN_SQL_Fail);
    //     }
    // }

    LOG_I << sql;
    if(0 != mysql_query(c, sql.c_str()))
    {
        LOG_I << "MySQL query "<< mysql_errno(c) << mysql_error(c);
        THROW_EX(EN_SQL_Fail);
    }

    // if(affair_flag){
    //     if(0 != mysql_commit(c)){
    //         THROW_EX(EN_SQL_Fail);
    //     }
    // }

    result_ = mysql_store_result(c);
}

void exec(boost::system::error_code & ec, const std::string& sql, const dbc::connection& wc)
{
    LOG_I << sql;
    native_handle_type c = wc.native_handle();
    if (mysql_query(c, sql.c_str()) != 0)
    {
        LOG_I << "MySQL query "<< mysql_errno(c) << mysql_error(c);
        ec = boost::system::error_code(EN_SQL_Fail, Error_category::inst<myerror_category>()); // THROW_EX(EN_SQL_Fail);
    }
}

std::string db_escape(const std::string& param, const dbc::connection& wc)
{
    if (param.empty())
        return param;

    std::string escaped(2* param.size() + 1,0);

    native_handle_type c = wc.native_handle();
    unsigned long n = mysql_real_escape_string(c, const_cast<char*>(escaped.c_str()),param.c_str(),param.size());
    // if (0 == mysql_real_escape_string(c, const_cast<char*>(escaped.c_str()),param.c_str(),len))
    if (n == 0)
    {
        LOG_I << "MySQL query "<< mysql_errno(c) << mysql_error(c);
        THROW_EX(EN_SQL_Fail);
    }

    escaped.resize(n);
    return escaped;
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

initializer::initializer (const boost::property_tree::ptree & ini)
{
    sql::dbc::init( ini ); //getcfg(cfg, "database")
}
initializer::~initializer()
{
    LOG_I << __FILE__ << __LINE__;
}

} // namespace 

// std::ostream& operator<<(std::ostream& outs, const std::vector<std::string>& c)
// {
//     std::vector<std::string>::const_iterator it = c.begin();
//     for ( ; it != c.end(); ++it)
//         outs << *it << " ";
//     return outs;
// }

typedef boost::shared_ptr<redisReply> redis_reply_ptr;
struct hiredis;

// struct redis_context
// {
//     redis_reply_ptr reply()
//     {
//         void *rpy;
//         int ret = redisGetReply(ctx_.get(), &rpy);
//         if (ret == REDIS_OK)
//             return redis_reply_ptr(static_cast<redisReply*>(rpy), freeReplyObject);
//         if (ctx_->err == REDIS_ERR_PROTOCOL)
//             return redis_reply_ptr();
//         // reconnect();
//         return redis_reply_ptr();
//     }
// 
//     template <typename... A>
//     redis_context& append(A... a)
//     {
//         this->app(a...);
//         argv_.clear();
//         argv_len_.clear();
//         tmps_.clear();
//         return *this;
//     }
// 
//     ~redis_context();
// 
// private:
//     friend struct hiredis;
//     explicit redis_context(boost::shared_ptr<redisContext> ctx);
// 
//     std::vector<char const*> argv_;
//     std::vector<size_t> argv_len_;
//     std::list<std::string> tmps_;
// 
//     boost::shared_ptr<redisContext> ctx_;
// 
//     template <typename T> void app(T const& x)
//     {
//         this->app2(x);
//         LOG_I << argv_;
//         redisAppendCommandArgv(ctx_.get(), argv_.size(), &argv_[0], &argv_len_[0]);
//     }
// 
//     template <typename T, typename... A> void app(T const& x, A... a)
//     {
//         this->app2(x);
//         this->app(a...);
//     }
// 
//     void app2(char const * s)
//     {
//         argv_.push_back(s);
//         argv_len_.push_back(strlen(s));
//     }
//     void app2(std::string const & s)
//     {
//         argv_.push_back(s.data());
//         argv_len_.push_back(s.size());
//     }
//     template <typename T> void app2(T const & x)
//     {
//         tmps_.push_back( boost::lexical_cast<std::string>(x) );
//         this->app2(tmps_.back());
//     }
// };
// 
// struct hiredis : boost::noncopyable
// {
//     typedef redis_reply_ptr reply;
//     typedef redis_context context;
// 
//     template <typename ...A>
//     reply command(A... a)
//     {
//         hiredis::context ctx(ctx_);
//         ctx.append(a...);
//         return ctx.reply();
//     }
// 
//     hiredis::context make_context() { return hiredis::context(ctx_); }
// 
//     static hiredis& inst();
// 
// private:
//     boost::shared_ptr<redisContext> ctx_;
//     std::string host_;
//     unsigned short port_, x_;
// 
//     hiredis(std::string const & host, int port)
//         : host_(host)
//         , port_(port)
//     {
//         reconnect();
//     }
// 
//     void reconnect();
// };
// 
// hiredis & hiredis::inst()
// {
//     static hiredis obj(redis_host_, redis_port_);
//     return obj;
// }
// 
// void hiredis::reconnect()
// {
//     struct timeval out = { 3, 0 };
//     redisContext *ctx = 0;
// 
//     LOG_I << "redis connect ... " << host_ <<":"<< port_;
//     ctx_.reset();
// 
//     while (!ctx)
//     {
//         ctx = redisConnectWithTimeout(host_.c_str(), port_, out);
//         if (ctx && ctx->err)
//         {
//             LOG_I << "redis connect error: " << ctx->errstr;
//             redisFree(ctx);
//             ctx = 0;
//         }
//     }
//     ctx_ = boost::shared_ptr<redisContext>(ctx, redisFree);
// }
// 
// redis_context::redis_context(boost::shared_ptr<redisContext> ctx )
//     : ctx_(ctx)
// {}
// 
// redis_context::~redis_context()
// {
//     ctx_.reset();
//     if (ctx_.use_count() == 1)
//     {
//         int n = 0;
//         while (reply())
//             ++n;
//         LOG_I << n;
//     }
// }

// #include <asio/ip/tcp.hpp> namespace ip = boost::asio::ip;

namespace redis {


static std::multimap<endpoint, context_ptr> & contexts()
{
    static std::multimap<endpoint, context_ptr> ctxs_;
    return ctxs_;
}

context::~context()
{
}

context_helper::context_helper(context_ptr ctx, endpoint ep)
    : ctxp_(ctx), endp_(ep)
{
    n_reply_ = 0;
}

context_helper::~context_helper()
{
    if (ctxp_.use_count() == 1)
    {
        LOG_I << endp_ <<" "<< n_reply_;
        int n = n_reply_;

        redis::reply rpy;
        while (n-- > 0 && (rpy = reply()))
            ;

        if (n_reply_ == 0 || rpy)
            contexts().insert(std::make_pair(endp_, ctxp_));
    }
}

redis_reply_ptr context_helper::reply()
{
    void *rpy;
    int ret;

    --n_reply_;
    LOG_I << n_reply_;

Pos_again_:
    ret = redisGetReply(get_context_ptr().get(), &rpy);
    if (ret == REDIS_OK)
        return redis_reply_ptr(static_cast<redisReply*>(rpy), freeReplyObject);

    if (ctxp_->err != REDIS_ERR_PROTOCOL)
    {
        context_helper x = make(endp_.first, endp_.second);
        ctxp_ = x.ctxp_;
        goto Pos_again_;
    }

    LOG_I << "redis REDIS_ERR_PROTOCOL";
    return redis_reply_ptr();
}

context_helper context_helper::make() { return make(redis_host_, redis_port_); }

context_helper context_helper::make(std::string const & host, unsigned short port)
{
    endpoint ep = std::make_pair(host,port);
    context_ptr cp;

    auto it = contexts().find(ep);
    if (it != contexts().end())
    {
        cp = it->second;
        contexts().erase(it);
    }
    else
    {
        redisContext *redx = 0;

        LOG_I << "redis connect ... " << ep;
        while (!redx)
        {
            struct timeval out = { 3, 0 };
            redx = redisConnectWithTimeout(ep.first.c_str(), ep.second, out);
            if (redx && redx->err)
            {
                LOG_I << "redis connect error: " << redx->errstr;
                redisFree(redx);
                redx = 0;
            }
        }
        cp = boost::shared_ptr<redisContext>(redx, redisFree);
    }

    return context_helper(cp, ep);
}

} // namespace redis


