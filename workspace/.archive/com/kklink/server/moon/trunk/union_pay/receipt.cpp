#include <syslog.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <openssl/md5.h>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
// #include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/format.hpp>
#include "util.h"
#include "jss.h"
#include "log.h"
#include "dbc.h"

namespace sys = boost::system;
namespace placeholders = boost::asio::placeholders;
using boost::asio::ip::tcp;
using boost::format;

typedef unsigned int UID;

//"http://222.66.233.198:8080/gateway/merchant/trade"
//"http://222.66.233.198:8080/gateway/merchant/query"
static std::string up_trade_host_ = "222.66.233.198";
static std::string up_trade_port_ = "8080";
static std::string up_trade_path_ = "/gateway/merchant/trade";
static std::string up_query_path_ = "/gateway/merchant/query";
static std::string up_merid_ = "880000000000652";
static std::string up_security_key_ = "dy5TXvapLMudUJmhd9QQ4BPoSmph8yBP";
static std::string up_backendurl_ = "http://payment.kklink.com:9080/p/notify/up/back";
static std::string up_frontendurl_ = "http://payment.kklink.com:9080/p/notify/up/front";
static std::string up_cert_ = "etc/moon.d/unionpay";

#define APPLE_HOST          "gateway.sandbox.push.apple.com"
#define APPLE_PORT          "2195"
#define APPLE_FEEDBACK_HOST "feedback.sandbox.push.apple.com"
#define APPLE_FEEDBACK_PORT "2196"
#define RSA_CLIENT_KEY      "PushChatKey.pem"
#define RSA_CLIENT_CERT     "PushChatCert.pem"

// static const char* certs_dir_ = "etc/moon.d";

std::string timef(const char* fmt, time_t second=0)
{
    char s[32]={0};
    if (0 == second)
        second = time(0);
    strftime(s,sizeof(s), fmt, localtime(&second));
    return std::string(s);
}

std::string md5(const std::string & s)
{
    std::string md(MD5_DIGEST_LENGTH,'\0');
    MD5((unsigned char const *)(s.data()), s.size(), (unsigned char*)&md[0]);
    return md;
}

std::string hexstring(const std::string & s)
{
    std::string ret(s.size() * 2, 0);
    char *d = &ret[0];
    for (char const *p=s.data(), *end=s.data()+s.size(); p != end; ++p)
    {
        char h = (*p >> 4) & 0x0f;
        char l = (*p) & 0x0f;
        *d++ = ((h) + ((h) < 10 ? '0' : 'a'-10));
        *d++ = ((l) + ((l) < 10 ? '0' : 'a'-10));
    }
    return ret;
}

tcp::endpoint resolv(boost::asio::io_service & io_service, const std::string& h, const std::string& p)
{
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(h, p);
    tcp::resolver::iterator i = resolver.resolve(query);
    return *i;
}

static std::string unhex(const std::string & hs)
{
    std::string buf;
    const char* s = hs.data();
    for (unsigned int x = 0; x+1 < hs.length(); x += 2)
    {
        char h = s[x];
        char l = s[x+1];
        h -= (h <= '9' ? '0': 'a'-10);
        l -= (l <= '9' ? '0': 'a'-10);
        buf.push_back( char((h << 4) | l) );
    }
    return buf;
}

template <typename Int> static std::string& pkint(std::string & pkg, Int x)
{
    Int i;
    switch (sizeof(Int)) {
        case sizeof(uint32_t): i = htonl(x); break;
        case sizeof(uint16_t): i = htons(x); break;
        default: i = x; break;
    }
    char *pc = (char*)&i;
    pkg.insert(pkg.end(), &pc[0], &pc[sizeof(Int)]);
    return pkg;
}

static std::string & pkstr(std::string & pkg, const std::string & s)
{
    uint16_t len = s.length();
    len = htons(len);
    char *pc = (char*)&len;
    pkg.insert(pkg.end(), &pc[0], &pc[sizeof(uint16_t)]);
    return (pkg += s);
}

std::string pkaps(std::string const & tok, int id, std::string const & aps)
{
    BOOST_ASSERT(tok.length() == 64);

    std::string buf; //(len, 0);
    pkstr(pkstr(pkint(pkint(pkint( buf
            , uint8_t(1)) // command
            , uint32_t(id)) // /* provider preference ordered ID */
            , uint32_t(time(NULL)+(60*60*6))) // /* expiry date network order */
            , unhex(tok)) // binary 32bytes device token
            , aps)
            ;
    return buf;
}

struct bdescribe
{
    explicit bdescribe(int id=0)
    {
        id_ = id;
        uid = 0;
        pid = 0;
        amount = 0;
        //host = "127.0.0.1"; //
        //port = "8080"; //"buy.itunes.apple.com";
        //host = up_trade_host_; //"buy.itunes.apple.com";
        //port = up_trade_port_;
        //cert = "etc/moon.d";
        //{ host = "sandbox.itunes.apple.com"; cert = "etc/moon.d"; }
    }

    unsigned int id_;
    UID uid;
    unsigned int pid;
    std::string time;
    unsigned int amount;
    std::string out_trade_no;
    std::string trade_no;
    //std::string host;
    //std::string port;
    //std::string cert;
};

std::ostream & operator<<(std::ostream & out, bdescribe const & inf)
{
    return out << inf.id_ 
        <<" "<< inf.amount
        <<" "<< inf.uid <<" "<< inf.pid;
}

bool compfirst(keyvalues::value_type const & x , keyvalues::value_type const & y)
{
    return x.first < y.first;
}

std::string to_out_trade_no(bdescribe const & inf)
{
    char n[40];
    snprintf(n,16, "%xs%xs%xs", inf.id_, inf.uid, inf.pid);
    return std::string(n);
}

bdescribe* from_out_trade_no(bdescribe & inf, std::string trade_no)
{
    BOOST_ASSERT(std::count(trade_no.begin(),trade_no.end(),'s') == 3);

    int i = 0;
    char * s = const_cast<char*>(trade_no.c_str());
    inf.id_ = strtol(s, 0, 16);

    i = trade_no.find('s', i+1);
    inf.uid = strtol(s+i+1, 0, 16);

    i = trade_no.find('s',i+1);
    inf.pid = strtol(s+i+1, 0, 16);

    return &inf;
}

template <typename Tag>
struct obuffers
{
    sys::error_code operator()(const std::string & m, Tag const & tag)
    {
        sl_.push_back(std::make_pair(m, tag));
        return sys::error_code();
    }

    bool empty() const { return sl_.empty(); }

    boost::asio::const_buffers_1 const_buffer() const
    {
        if (sl_.empty())
            return boost::asio::buffer(static_cast<const char*>(0), 0);
        return boost::asio::buffer(sl_.front().first);
    }

    Tag consume(sys::error_code const & ec)
    {
        Tag tmp = sl_.front().second;
        sl_.pop_front();
        LOG_I << tmp <<" "<< ec <<" "<< ec.message();
        return tmp;
    }

    Tag const & tag() const { return sl_.front().second; }

    ~obuffers()
    {
        if (!sl_.empty())
            LOG_I << sl_.size() <<" "<< tag();
    }
private:
    std::list<std::pair<std::string, Tag> > sl_;
};

std::string make_upquery(std::string const & trade_no, int amount, const std::string & time)
{
    request q("POST", up_trade_path_);

    keyvalues params;
    put(params, "version", "1.0.0");
    put(params, "charset", "UTF-8");
    put(params, "transType", "01");
    put(params, "merId", up_merid_);
    put(params, "backEndUrl", urlencode(up_backendurl_));
    put(params, "frontEndUrl", urlencode(up_frontendurl_));
    put(params, "orderDescription", "OK");
    put(params, "orderTime", time);
    //put(params, "orderTimeout", timef("%Y%m%d%H%M%S",time(0)+60*60*24*7));
    put(params, "orderNumber", trade_no);
    put(params, "orderAmount", amount);
    put(params, "orderCurrency", "156");
    put(params, "reqReserved", "OK");
    put(params, "merReserved", "OK");

    basic_keyvalues<std::vector<std::pair<std::string,std::string> > > skv(params.begin(), params.end());
    erase(skv, "backEndUrl"); erase(skv, "backEndUrl");
    erase(skv, "frontEndUrl"); erase(skv, "frontEndUrl");
    put(skv, "backEndUrl", up_backendurl_);
    put(skv, "frontEndUrl", up_frontendurl_);
    std::sort(skv.begin(), skv.end(), &compfirst);
    std::string k = hexstring(md5(up_security_key_));
    //LOG_I << k; LOG_I << join_pairs(skv,"=","&");
    std::string signature = hexstring(md5(join_pairs(skv, "=","&") + "&" + k));

    put(params, "signature", signature);
    put(params, "signMethod", "MD5");
    //version=1.0.0&charset=UTF-8&transType=01&merId=880000000000652&backEndUrl=http%3A%2F%2Fmoon.kklink.com%3A9080%2Fp%2Fnotify%2Fup%2Fback&frontEndUrl=http%3A%2F%2Fmoon.kklink.com%3A9080%2Fp%2Fnotify%2Fup%2Ffront&orderDescription=OK&orderTime=20131106175351&orderNumber=2013110617535151&orderAmount=1&orderCurrency=156&reqReserved=OK&merReserved=OK&signature=76bc4e7955e5ce0b9008834ce7292edf&signMethod=MD5

    put(q.headers(), "Host", up_trade_host_ + std::string(":") + up_trade_port_);
    put(q.headers(), "Accept", "*/*");
    put(q.headers(), "Content-Type", "application/x-www-form-urlencoded");
    q.content() = join_pairs(params, "=", "&");

    return q.encode();
    // LOG_I << sl_.back().first <<" "<< sl_.back().second;
}

#define PERM_PASSWORD "abc123"
static std::string getpwd( std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) { return PERM_PASSWORD; }

// template <typename Tab> static int max_index(int x)
// {
//     static int idx_ = 0;
//     if (idx_ == 0)
//     {
//         idx_ = 8;
// 
//         sql::datas datas(std::string("SELECT MAX(id) FROM ") + Tab::name());
//         if (sql::datas::row_type row = datas.next())
//         {
//             if (row[0])
//                 idx_ = atoi(row[0]) + 1;
//         }
//     }
//     int ret = idx_;
//     idx_ += x;
//     return ret;
// }
struct Tab_unionpay {
    static const char *db_table_name() { return "unionpay"; } };

int alloc_id_() { return max_index<Tab_unionpay>(1); }

int get_amount(int pid)
{
    sql::datas datas(format("SELECT money FROM money WHERE id=%1%") % pid);
    if(sql::datas::row_type row = datas.next())
    {
        return boost::lexical_cast<int>(row.at(0));
    }
    return 0;
}

struct apple_push : boost::noncopyable
{
#define DESTROY(i) this->destroy(i,__LINE__,__FUNCTION__)
    typedef apple_push this_type;

    struct stage { enum type {
            error = 0
            , connecting = 1, handshaking
            , idle
            , writing, reading
        };
    };

    struct ap_context;
    typedef std::map<int,ap_context> contexts_type;
    typedef contexts_type::iterator iterator;

    struct ap_context
    {
        obuffers<bdescribe> obuffer_;
        bdescribe info;

        explicit ap_context() // (boost::asio::io_service & ios) : _timer(new boost::asio::deadline_timer(ios))
        {
            _c = 0;
            // working = false;
            nfail = 0;
            eof = 0;
        }

        void check()
        {
        }

        boost::shared_ptr<boost::asio::deadline_timer> _timer;

        // bool working;
        bool eof;
        unsigned char nfail;

        http_server::connection* _c;
    };

    explicit apple_push(boost::asio::io_service & io_service)
        : timer_(io_service)
    {
    }

    ~apple_push()
    {
        LOG_I << __FILE__;
    }

    sys::error_code _eof(ap_context * ctx)
    {
        ctx->_c = 0;
        return make_error_code(boost::asio::error::eof);
    }

    sys::error_code message(http_server::connection& con, sys::error_code const& ec, http_server::connection::message const& m)
    {
        try
        {
            return this->main(con, ec, m);
        }
        catch (my_exception const & ex)
        {
            LOG_E << ex.error_code() <<" "<< ex.error_code().message();
        }
        catch (std::exception const & ex)
        {
            LOG_E << ex.what();
        }
        return make_error_code(boost::system::errc::bad_message);
    }

    sys::error_code main(http_server::connection& con, sys::error_code const& ec, http_server::connection::message const& m)
    {
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message() <<" "<< con.id_;
            if (ec == boost::asio::error::eof)
            {
                if (&con == main_serv_ctx_._c)
                {
                    main_serv_ctx_._c = 0;
                    return ec;
                }
                //ap_context * ctx = 0;
                //else if (con.id_)
                //{
                //    iterator i = contexts_.find(con.id_);
                //    if (i != contexts_.end())
                //        ctx = &i->second;
                //}
                //if (ctx)
                //{
                //    return _eof(ctx);
                //}
            }
            if (con.id_)
                contexts_.erase(con.id_);
            return ec;
        }
        LOG_I << m;
        LOG_I << m.content();

        if (m.path() == "/-pull")
        {
            main_serv_ctx_._c = &con;
            //  BOOST_ASSERT (m.params().empty() == fronts_.empty())
            //if (!m.params().empty())
            //{
            //    int id_ = get<int>(m.params(),"id_");
            //    ap_context & ctx = contexts_.at(id_);
            //    bdescribe & inf = ctx.info;

            //    inf.bid = get(m.params(),"bid");
            //    inf.amount = get<int>(m.params(),"amount");
            //    inf.id_ = id_;

            //    unionpay_requests_( make_upquery(to_out_trade_no(id_), inf.amount), inf );
            //    unionpay_check(0, __LINE__,__FUNCTION__);
            //}
            async_response(&main_serv_ctx_, 0);
            return sys::error_code();
        }

        if (m.path() == "/bills")
        {
            if (get<int>(m.params(), "channel") != 2) // union-pay only
            {
                return make_error_code(boost::system::errc::bad_message);
            }

            UID userid = get<UID>(m.params(), "userid");
            int pid = get<int>(m.params(),"pid");
            int amount = get_amount(pid);
            int id_ = alloc_id_();
            ap_context & ctx = contexts_[id_];
            ctx._c = &con;
            con.id_ = ctx.info.id_ = id_;

            // ===
            // keyvalues params = m.params();
            // put(params, "path_", m.path());
            // put(params, "id_", id_);
            // main_serv_ctx_.obuffer_( join_pairs(params,"=","&") );
            // main_serv_check();
            bdescribe & inf = ctx.info;

            //inf.bid = get(m.params(),"bid");
            //inf.amount = get<int>(m.params(),"amount", "1");
            inf.id_ = id_;
            inf.uid = userid;
            inf.pid = pid;
            inf.time = timef("%Y%m%d%H%M%S");
            inf.amount = amount;

            sql::exec(format("INSERT INTO unionpay(id,userid,time,amount,merid,status)"
                        " VALUES(%1%,%2%,'%3%',%4%,'%5%',1)")
                    % inf.id_ % inf.uid % inf.time % inf.amount % up_merid_);

            unionpay_requests_( make_upquery(to_out_trade_no(inf), inf.amount, inf.time), inf );
            unionpay_check(0, __LINE__,__FUNCTION__);
            return sys::error_code();
        }

        if (m.path() == "/p/notify/up/back")
        {
            LOG_I << m <<" "<< m.content();
            keyvalues params;
// orderTime=20131111140401&settleDate=0912&respCode=00&orderNumber=00000023&exchangeRate=0&charset=UTF-8&signature=a6ee0a6cdb267d86fd4cdbb359c006ed&sysReserved=%7BtraceTime%3D1111140401%26acqCode%3D00215800%26traceNumber%3D043076%7D&settleCurrency=156&version=1.0.0&transType=01&settleAmount=1&signMethod=MD5&transStatus=00&reqReserved=OK&merId=880000000000652&qn=201311111404010430761
            parse_keyvalues(params, m.content());
            int status = get<int>(params, "transStatus");
            LOG_I << status;
            if (status == 0)
            {
                int rc = get<int>(params, "respCode");
                std::string merid = get(params, "merId");
                std::string out_trade_no = get(params, "orderNumber");
                int amount = get<int>(params, "settleAmount");
                std::string time = get(params, "orderTime");
                std::string qn = get(params, "qn");

                bdescribe inf;
                LOG_I << out_trade_no;
                if (from_out_trade_no(inf, out_trade_no))
                {
                    sql::exec(format("UPDATE unionpay SET status=0 WHERE id=%1%") % inf.id_);

                    json::object js;
                    js.put("_", "/bills");
                    js.put("id_", inf.id_);
                    js.put("channel", 2);
                    js.put("uid", inf.uid);
                    js.put("pid", inf.pid);
                    js.put("merid", merid);
                    js.put("trade_no", qn);
                    js.put("out_trade_no", out_trade_no);

                    response rsp(200);
                    rsp.content() = json::encode(js);

                    LOG_I << inf << rsp.content();
                    main_serv_ctx_.obuffer_( rsp.encode(), inf );
                    async_response(&main_serv_ctx_, 1);
                    LOG_I << main_serv_ctx_.obuffer_.const_buffer();
                    LOG_I << main_serv_ctx_._c;
                }
            }

            int id_ = alloc_id_();
            ap_context & ctx = contexts_[id_];
            ctx._c = &con;
            con.id_ = ctx.info.id_ = id_;
            {
                response rsp(200);
                rsp.content() = std::string("ok");
                ctx.obuffer_(rsp.encode(), bdescribe(id_));
            }
            async_response(&ctx, 1);

            return sys::error_code();
        }
        if (m.path() == "/p/notify/up/front")
        {
            LOG_I << m <<" "<< m.content();
            return sys::error_code();
        }

        contexts_.erase(con.id_);
        return make_error_code(boost::system::errc::bad_message);
    }

private:
    // void destroy(iterator i, int ln,char const *fn)
    // {
    //     LOG_I << ln <<":"<< fn;
    //     if (i->_c)
    //     {
    //         i->_c->notify_close(ln,fn);
    //         i->_c = 0;
    //     }

    //     //sys::error_code ec; i->_timer->cancel(ec);

    //     LOG_I << i->_pi.operator->() <<" "<< iqueue_.end().operator->();

    //     //if (i->_pi != iqueue_.end()) iqueue_.erase(i->_pi);
    //     contexts_.erase(i);
    // }

    void async_response(ap_context * cx, bool clear)
    {
        LOG_I << cx->info <<" "<< cx->_c <<" "<< clear;

        if (cx->obuffer_.empty())
        {
            int id_ = cx->info.id_;
            if (id_ && clear)
                contexts_.erase(id_);
            return;
        }

        if (!cx->_c)
            return;
        LOG_I << cx->_c->socket().local_endpoint();

        boost::asio::async_write(cx->_c->socket()
                , cx->obuffer_.const_buffer()
                , boost::bind(&this_type::handle_rsp_write, this, cx, placeholders::error));
    }

    void handle_rsp_write(ap_context *cx, sys::error_code const & ec)
    {
        int id_ = cx->info.id_;
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message() <<" "<< id_;
            contexts_.erase(id_);
            return;
        }
        LOG_I << cx->obuffer_.const_buffer();
        cx->obuffer_.consume(ec);
        async_response(cx, 1);
    }

    void handle_write(sys::error_code const & erc)
    {
        if (erc)
        {
            return handle_error(erc,__LINE__,__FUNCTION__);
        }

        if (ssl_->enabled)
        {
        boost::asio::async_read_until(ssl_->socket()
                , unionpay_sbuf_
                , "\r\n"
                , boost::bind(&this_type::handle_status_line, this, placeholders::error));
        }
        else
        {
        boost::asio::async_read_until(ssl_->next_layer()
                , unionpay_sbuf_
                , "\r\n"
                , boost::bind(&this_type::handle_status_line, this, placeholders::error));
        }
    }

    void handle_error(sys::error_code const & ec, int ln, char const *fn)
    {
        LOG_I << ec <<" "<< ec.message() <<" "<< ln <<":"<< fn;

        if (ec == boost::asio::error::operation_aborted)
        {
            // stage_ = stage::error;
            return;
        }

        unionpay_check(3, ln,fn);
    }

    void handle_status_line(const sys::error_code& erc)
    {
        if (erc)
        {
            return handle_error(erc,__LINE__,__FUNCTION__);
        }

        unionpay_response_.clear();
        sys::error_code ec = unionpay_response_.parse_status_line(unionpay_sbuf_);
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        if (ssl_->enabled)
        {
        boost::asio::async_read_until(ssl_->socket()
                , unionpay_sbuf_
                , "\r\n\r\n"
                , bind(&this_type::handle_headers, this, placeholders::error));
        }
        else
        {
        boost::asio::async_read_until(ssl_->next_layer()
                , unionpay_sbuf_
                , "\r\n\r\n"
                , bind(&this_type::handle_headers, this, placeholders::error));
        }
    }

    void handle_headers(const sys::error_code& erc)
    {
        if (erc)
        {
            return handle_error(erc, __LINE__,__FUNCTION__);
        }

        sys::error_code ec = unionpay_response_.parse_header_lines(unionpay_sbuf_);
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        unsigned int len = get<int>(unionpay_response_.headers(), "content-length", 0);

        if (unionpay_sbuf_.size() >= len)
        {
            return handle_content(erc);
        }

        if (ssl_->enabled)
        {
        boost::asio::async_read(ssl_->socket()
                , unionpay_sbuf_
                , boost::asio::transfer_at_least(len - unionpay_sbuf_.size())
                , bind(&this_type::handle_content, this, placeholders::error));
        }
        else
        {
        boost::asio::async_read(ssl_->next_layer()
                , unionpay_sbuf_
                , boost::asio::transfer_at_least(len - unionpay_sbuf_.size())
                , bind(&this_type::handle_content, this, placeholders::error));
        }
    }

    void handle_content(const sys::error_code& ec)
    {
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        unionpay_response_.parse_content(unionpay_sbuf_); // streambuf_.sgetn(&rsp.content_[0], req_.content_.size());

        LOG_I << unionpay_requests_.tag();
        LOG_I << unionpay_requests_.const_buffer();
        LOG_I << unionpay_response_.content();
        bdescribe inf = unionpay_requests_.consume(ec);
        unionpay_check(0, __LINE__,__FUNCTION__);

        iterator i = contexts_.find(inf.id_);
        if (i == contexts_.end())
        {
            LOG_I << "Not found " << inf;
            return;
        }

        keyvalues params;
        parse_keyvalues(params, unionpay_response_.content());
        std::string rc = get(params, "respCode");
        LOG_I << rc;
        if (strtol(rc.c_str(), 0, 0) == 0)
        {
            std::string tn = get(params, "tn");
            //std::string tt = get(params, "transType");
            //std::string st = get(params, "signature");
            response rsp(200);
            {
                json::object rj;
                rj.put("trade_no", tn);
                rj.put("channel", 2);
                // rj.put("out_trade_no", to_out_trade_no(inf));
                rsp.content() = json::encode(rj);
            }
            i->second.obuffer_(rsp.encode(), inf);
        }

        async_response(&i->second, 1);
    }

    void handle_connect(const sys::error_code& ec)
    {
        if (ec)
        {
            return handle_error(ec, __LINE__,__FUNCTION__);
        }

        if (ssl_->enabled)
        {
            LOG_I << unionpay_requests_.const_buffer();
            if (ssl_->handshaked)
            {
                boost::asio::async_write(ssl_->socket()
                        , unionpay_requests_.const_buffer()
                        , boost::bind(&this_type::handle_write, this, placeholders::error));
            }
            else
            {
                ssl_->handshaked = 1;
                ssl_->socket().async_handshake(
                        boost::asio::ssl::stream_base::client,
                        boost::bind(&this_type::handle_connect, this, placeholders::error));
            }
        }
        else
        {
            boost::asio::async_write(ssl_->next_layer()
                    , unionpay_requests_.const_buffer()
                    , boost::bind(&this_type::handle_write, this, placeholders::error));
        }
        //boost::asio::async_read(socket_, streambuf_
        //        , boost::asio::transfer_at_least(32)
        //        , boost::bind(&this_type::handle_read, this, placeholders::error));
    }

    void timed_connect(sys::error_code ec)
    {
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message(); // (ec == boost::asio::error::operation_aborted)
            return;
        }
        if (unionpay_requests_.empty())
            return;

        // bdescribe const & inf = i->fwd_.tag();
        boost::asio::io_service & io_service = timer_.get_io_service();
        tcp::endpoint endpoint = resolv(io_service, up_trade_host_, up_trade_port_);
        LOG_I << "resolved " << endpoint;

        //socket_.lowest_layer().close(ec);
        //socket_.lowest_layer().open(tcp::v4());
        //socket_.lowest_layer().async_connect(endpoint_, boost::bind(&this_type::handle_connect, this, placeholders::error));

        ssl_.reset(new wrapsocket(io_service, up_cert_));
        ssl_->enabled = (up_trade_port_ == "https");
        // i->second.working = true;
        ssl_->lowest_layer().async_connect( endpoint
                , boost::bind(&this_type::handle_connect, this, placeholders::error));
        LOG_I << "connecting " << endpoint;
    }

    void unionpay_check(int ns, int ln, char const *fn)
    {
        if (unionpay_requests_.empty())
            return;
        timer_.expires_from_now(boost::posix_time::seconds(ns));
        timer_.async_wait(boost::bind(&this_type::timed_connect, this, boost::asio::placeholders::error));
    }

private:
    // stage::type stage_;

    struct wrapsocket : boost::noncopyable
    {
        explicit wrapsocket(boost::asio::io_service & io_service, const std::string & cert)
            : context_(boost::asio::ssl::context::sslv3)
            , socket_(io_service, init_context(context_,cert))
        { enabled=1; handshaked=0; }

        boost::asio::ssl::stream<tcp::socket>::lowest_layer_type & lowest_layer() { return socket_.lowest_layer(); }
        boost::asio::ssl::stream<tcp::socket>::next_layer_type & next_layer() { return socket_.next_layer(); }

        boost::asio::ssl::stream<tcp::socket> & socket() { return socket_; }

        bool enabled;
        bool handshaked;

    private:
        boost::asio::ssl::context context_;
        boost::asio::ssl::stream<tcp::socket> socket_;

        static boost::asio::ssl::context & init_context(boost::asio::ssl::context & ctx, const std::string & cert)
        {
            //ctx.set_password_callback(getpwd);
            //LOG_I << cert << "/" << RSA_CLIENT_CERT;

            //ctx.use_certificate_file(cert + "/" + RSA_CLIENT_CERT, boost::asio::ssl::context::pem);
            //ctx.use_private_key_file(cert + "/" + RSA_CLIENT_KEY, boost::asio::ssl::context::pem);
            return ctx;
        }
    };

    // std::list<iterator> iqueue_;

    boost::shared_ptr<wrapsocket> ssl_;
    //boost::asio::ssl::context context_;
    //boost::asio::ssl::stream<tcp::socket> socket_;
    boost::asio::deadline_timer timer_;

    boost::asio::streambuf unionpay_sbuf_;
    response unionpay_response_;
    obuffers<bdescribe> unionpay_requests_;

    std::map<int,ap_context> contexts_;
    ap_context main_serv_ctx_;
};

// Array
// (
//     [version] => 1.0.0
//     [charset] => UTF-8
//     [transType] => 01
//     [merId] => 880000000000652
//     [backEndUrl] => http://moon.kklink.com:9080/p/notify/up/back
//     [frontEndUrl] => http://moon.kklink.com:9080/p/notify/up/front
//     [orderDescription] => 订单描述
//     [orderTime] => 20131106141429
//     [orderTimeout] =>
//     [orderNumber] => 2013110614142929
//     [orderAmount] => 1
//     [orderCurrency] => 156
//     [reqReserved] => 透传信息
//     [merReserved] => {test=test}
// )
// Array
// (
//     [respCode] => 00
//     [tn] => 201311061414290035562
//     [signMethod] => MD5
//     [transType] => 01
//     [charset] => UTF-8
//     [reqReserved] => 透传信息
//     [signature] => ec0c20dc0ec088d3338f99547de3d6ce
//     [version] => 1.0.0
// )

