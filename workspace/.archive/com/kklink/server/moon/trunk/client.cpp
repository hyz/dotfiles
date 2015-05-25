// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1
#include "myconfig.h"
#include <stdint.h>
#include <sstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <list>
#include <vector>
#include <limits>
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iterator.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/tokenizer.hpp>

#include <ctime>
#include <cstdlib>

#include "log.h"
#include "dbc.h"
#include "jss.h"
#include "util.h"
#include "client.h"
#include "myerror.h"
// #include "sqls.h"
#include "chat.h"
#include "ios_push.h"
#include "mail.h"
#include "asclient.h"
#include "smsbuf.h"
#include "iapbuf.h"
// #include "operate_db.h"
#include "bars.h"
#include "dynamic_state.h"

using namespace std;
using namespace boost;
using namespace boost::posix_time;
namespace ip = boost::asio::ip;

enum { SECONDS_PERDAY = (60*60*24) };
enum { SECONDS_PERHOUR = (60*60) };
enum { DEVTYPE_WEB=4 }; // web-client
enum { TINV_EXPIRE_ACK=16 };

static const char* LKEY_GLOBAL_MESSAGE = "message/global/list";
static const char* HKEY_USER_GLOBAL_MESSAGE_LAST = "user/message/global/last";
static const char* HKEY_USER_MESSAGE_LAST = "user/message/last";

const char* HKEY_USER_LUCK_SHAKE_COUNT_LEFT = "user/luckshake/count/left";

////////
//
//
////////
//
//

std::list<imessage::message> public_messages;

static string web_url_("http://yx-api.kklink.com");
static boost::filesystem::path htm_dir_("etc/moon.d/html");

static service_fn_type service_fn_;

static boost::function<send_fn_type (socket_ptr)> assoc_g_;

std::ofstream timer_log("/tmp/my_auto_cpu_timer.txt", std::fstream::app);

#define SESSION_KEY "MT"

static std::map<std::string, service_fn_type > urlmap_;

static bool aps_push_disabled_ = 0;
static bool init_wan_ipaddr_ = 0;
static ip::address wan_ipaddress_;

struct auto_cpu_timer_helper : timer::auto_cpu_timer
{
    auto_cpu_timer_helper(std::string const & p)
        : timer::auto_cpu_timer(out_file::inst(), lexical_cast<std::string>(time(0)) + " %w " + p + "\n") {}
    //: timer::auto_cpu_timer(out_file::inst(), "\n%w %u %s #" + p) {}

    struct out_file : boost::filesystem::ofstream {
        out_file(std::string const & fn)
            : boost::filesystem::ofstream(fn +"."+ lexical_cast<std::string>(getpid())) {}
        static std::ostream & inst() {
            static out_file out("/tmp/auto-cpu-timer.out");
            return out;
        }
    };
};

struct monitor_summary
{
    std::map<UID,std::vector<std::string> > datas_;
    boost::filesystem::ofstream out_file_;
    time_t tp_fopen_;

    monitor_summary() { tp_fopen_ = 0; }

    // static const char *filename() { return "/tmp/yx-monitor-summary"; }
    // mutable boost::shared_ptr<boost::filesystem::ofstream> file_;

    // UInt line_count_;

    // struct Line : std::string 
    // { 
    //     friend std::istream & operator>>(std::istream & ins, Line & lin)
    //             { return std::getline(ins, lin); }
    // };

    // boost::make_function_output_iterator

    // struct tabrows : std::map<UID, std::string>
    // {
    //     void operator()(std::string const & line)
    //     {
    //         UID uid = atoi(line.c_str());
    //         if (!uid)
    //             return;
    //         std::string tds;

    //         //std::string tmps;
    //         //std::istringstream fis(line);
    //         //while (getline(fis, tmps, '\t'))
    //         //    tds += "<td>" + tmps + "</td>";
    //         typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    //         tokenizer toks(line, boost::char_separator<char>("\t"));
    //         for (tokenizer::iterator it = toks.begin(); it != toks.end(); ++it)
    //             tds += "<td>" + *it + "</td>";

    //         insert(std::make_pair( uid, "<tr>" + tds + "</tr>"));
    //     }
    // };

    // struct linesbuf : boost::circular_buffer<Line>
    // {
    //     UInt count;
    //     void operator()(Line const & line)
    //     {
    //         push_back(line);
    //         ++count;
    //     }
    //     linesbuf(int n) : boost::circular_buffer<Line>(n) { count=0; }
    // };

    // monitor_summary()
    //     //: file_(new boost::filesystem::fstream(filename(), ios_base::out|ios_base::app|ios_base::in))
    // {
    //     //_reset_file(500);
    // }

    //void _reset_file(int nlimit)
    //{
    //    linesbuf lines(nlimit);

    //    if (file_)
    //        file_->reset();

    //    {
    //        boost::filesystem::ifstream ins(filename());
    //        std::istream_iterator<Line> it(ins), end;
    //        std::copy(it, end, std::back_inserter(lines));
    //    }

    //    if (lines.count >= nlimit)
    //    {
    //        file_.reset(new boost::filesystem::ofstream(filename(), ios_base::app|ios_base::trunc));
    //        // std::ofstream ofs(filename(), ios_base::app|ios_base::trunc);
    //        std::ostream_iterator<Line> it(ofs, "\n");
    //        std::copy(lines.begin(), lines.end(), it);
    //    }
    //    else
    //    {
    //        file_.reset(new boost::filesystem::fstream(filename(), ios_base::out|ios_base::app|ios_base::in));
    //    }

    //    line_count_ = lines.count;
    //}

    template <typename ...A> void operator()(UID uid, A... a)
    {
        std::vector<std::string> toks;
        cat(toks, a...);

        BOOST_FOREACH(auto const & x , toks)
            out_file_ << x << "\t";
        out_file_ << "\n";

        auto p = datas_.insert( std::make_pair(uid, toks) );
        if (!p.second)
            p.first->second.swap( toks );

        time_t tpcur = time(0);
        if (tpcur - tp_fopen_ >= Seconds_PDay)
        {
            if (out_file_)
                out_file_.close();
            out_file_.clear();

            std::string fn = time_format<128>("/tmp/yx-clients-info.%d", tpcur);
            if (tp_fopen_ == 0)
                out_file_.open(fn, ios_base::app);
            else
                out_file_.open(fn);
            tp_fopen_ = (tpcur - tpcur % Seconds_PDay);
        }

        //if (++line_count_ > 5000)
        //    _reset_file(500);
    }

    template <typename C, typename X> void cat(C& v, X const & x)
    {
        v.push_back( boost::lexical_cast<std::string>(x) );
    }

    template <typename C, typename X, typename ...A> void cat(C& v, X const& x, A... a)
    {
        cat(v, x);
        cat(v, a...);
    }

};
static monitor_summary monit_summary_;

static std::string html_clients_info(monitor_summary const& y)
{
    extern time_t tp_startup_;
    // tabrows rows;

    std::string hds = "<table><tr>"
        "<td>服务器时间：</td><td>" + time_string() + "</td>"
        "<td/>"
        "<td>服务器启动时间：</td><td>" + time_string(tp_startup_) + "</td>"
        "</tr></table>"
        ;
    std::string tbody = "<tr>"
        "<td>用户ID</td>"
        "<td>用户昵称</td>"
        "<td>酒吧ID</td>"
        "<td>是否后台</td>"
        "<td>是否socket在线</td>"
        "<td>重连次数(fg/bg)</td>"
        "<td>最后socket ip:port</td>"
        "<td>最后socket创建</td>"
        "<td>最后socket接收</td>"
        "<td>最后socket发送</td>"
        "<td>最后活跃</td>"
        "<td>Apple Device Id</td>"
        "</tr>"
        ;

    BOOST_FOREACH(auto const & p, y.datas_)
    {
        std::string tds = str(boost::format(
                    "<td><a href=\"/yx-socket?userid=%1%\">%1%</a></td>") % p.first);
        BOOST_FOREACH(auto const & x, p.second)
            tds += "<td>" + x + "</td>";
        tbody += "<tr>" + tds + "</tr>";
    }
    return hds + "<table border=\"1\" bordercolor=\"#000000\">" + tbody + "</table>";
}

// monitor_socket monitor_socket_;

int alloc_bills_id();
//////////
//
//
//
namespace boost { namespace asio { namespace ip {
    bool is_private(boost::asio::ip::address const & addr)
    {
        boost::asio::ip::address_v4::bytes_type a = addr.to_v4().to_bytes();
        // char s[32];
        // snprintf(s,sizeof(s),"%u.%u.%u.%u", a[0] , a[1] , a[2] , a[3]);
        // LOG_I << s;
        return (a[0] == 192 && a[1] == 168)
            || (a[0] == 10)
            || (a[0] == 172 && (a[1] >= 16 && a[1] <= 31))
            || addr.is_loopback()
            ;
    }
}}}

struct ValidCode
{
    enum { exit_gap=15*60 };
    enum Usage{ PASSWORD=1, REGIST=3, BIND };
    string phone;
    string code;
    time_t t;
    enum Usage usage;

    static ValidCode create_valid_code(string ph, string cd, time_t tm, const enum Usage& us)
    {
        return ValidCode(ph, cd, tm, us);
    }

private:
    ValidCode(string ph, string cd, time_t tm, enum Usage us){
        phone = ph;
        code = cd;
        t = tm;
        usage = us;
    };
}; 
struct TagPhone{};
// struct TagCode{};
struct TagTime{};

typedef boost::multi_index_container<
ValidCode,
    boost::multi_index::indexed_by<
    boost::multi_index::ordered_non_unique<
boost::multi_index::tag<TagPhone>, BOOST_MULTI_INDEX_MEMBER(ValidCode, string, phone)
    >,
    boost::multi_index::ordered_non_unique<
boost::multi_index::tag<TagTime>, BOOST_MULTI_INDEX_MEMBER(ValidCode, time_t, t)
    >
    >
> ValidCodeType;
static ValidCodeType valid_codes;
ValidCodeType::iterator Pfind_code( const string& phone, enum ValidCode::Usage code_usage ) 
{
    auto& idxs = boost::get<TagPhone>(valid_codes);
    auto is = idxs.equal_range(phone);
    for( auto i=is.first; i!=is.second; ++i ) {
        if ( i->usage == code_usage ) return i;
    }

    return valid_codes.end();
}

void Premove_code( const string& phone, enum ValidCode::Usage code_usage ) 
{
    auto& idxs = boost::get<TagPhone>(valid_codes);
    auto is = idxs.equal_range(phone);
    for ( auto i=is.first; i!=is.second; ) {
        if ( i->usage == code_usage ) {
            i = idxs.erase(i);
        } else {
            ++i;
        }
    }
}
void insert_code(const ValidCode& code)
{
    Premove_code(code.phone, code.usage);
    valid_codes.insert(code);
}

void Tremove_code(const time_t bad_time=0)
{
    time_t bt = bad_time;
    if ( 0 == bt ) {
        bt = time(NULL) - ValidCode::exit_gap;
    }

    auto & idxs = boost::get<TagTime>(valid_codes);
    for ( auto i = idxs.begin(); i!=idxs.end(); ) {
        if ( bt > i->t ) { i = idxs.erase(i); }
        else { break; }
    }
}

string readall(const filesystem::path & fp, filesystem::path const & dir)
{
    return readfile(dir / fp);
    // ostringstream outs;
    // {
    //     filesystem::ifstream ins(dir / fp);
    //     outs << ins.rdbuf();
    // }
    // string s= outs.str();
    // LOG_I << s << fp;
    // return s;
}

boost::format file_format(const filesystem::path & fp)
{
    return boost::format(readfile(fp.string()));
}
boost::format file_format(const filesystem::path & fp, const filesystem::path & parent)
{
    return file_format(fp / parent);
}

/////////////
//
//

client_error_category::client_error_category()
{
    static code_string ecl[] = {
        { EN_Login_Other, "帐号在其他地方登陆" },
        { EN_Unauthorized, "未通过身份认证" },
        { EN_User_NotFound, "帐号不存在" },
        { EN_Login_Fail, "登录帐号或密码错误" },

        { EN_Password_Incorrect, "密码错误" },
        { EN_Password_Invalid, "无效的密码" },
        { EN_AuthCode_Invalid, "无效的验证码" },
        { EN_AuthCode_Timeout, "验证码已经过期" },

        { EN_UserName_NotAvail, "帐号名不可用" },
        { EN_UserName_Invalid, "无效的帐号名" },
        { EN_NickName_Invalid, "无效的昵称" },

        { EN_Phone_Invalid, "无效的手机号码" },
        { EN_Phone_Incorrect, "手机号码错误" },
        { EN_Phone_Not_Bind, "手机号码未绑定" },
        { EN_Phone_Bind_Already, "手机号码已注册" },

        { EN_Mail_Invalid, "无效的邮箱地址" },
        { EN_Mail_Not_Bind, "邮箱未绑定" },
        { EN_Mail_Bind_Already, "已经绑定邮箱" },

        { EN_Input_Form, "请求参数格式错误" },
        { EN_Input_Data, "请求参数值异常" },

        { EN_Operation_NotPermited, "不被允许的操作" },

        { EN_Distance_Invalid, "距离超出有效范围" },
    };

    Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])], __FILE__);
}

////////
//
//

bool is_match(const std::string& name, const std::string &ex)
{
    regex e(ex);
    smatch what;
    return regex_match(name, what, e);
}

template <typename Tab> static int _userId_index(int x)
{
    static int idx_ = 0;
    if (idx_ == 0)
    {
        idx_ = 1;

        sql::datas datas(string("SELECT MAX(userId) FROM ") + Tab::db_table_name());
        if (sql::datas::row_type row = datas.next())
        {
            if (row[0])
                idx_ = atoi(row[0]) + 1;
        }
    }

    int ret = idx_;
    idx_ += x;
    return ret;
}

struct Tab_users { static const char *db_table_name() { return "users"; } };
static UID alloc_user_id() { return _userId_index<Tab_users>(1); }

template <typename X, typename Y> std::ostream & operator<<(std::ostream & out, const index_key<X,Y> & i)
{
    return out << i.val_;
}

inline index_key<int,socket_ptr> index_by(socket_ptr const & s)
{
    return index_key<int,socket_ptr>(s->native_handle());
}

inline index_key<std::string,tag_aps_tok> index_by_aps_tok(std::string const & s)
{
    return index_key<std::string,tag_aps_tok>(s);
}

template <typename T>
map<T, client_ptr>& indexobj()
{
    static map<T, client_ptr, less<T> > idx;
    return idx;
}

template <typename T> void xindex::clear(T const & id)
{
    indexobj<T>().clear();
}

std::string debug_id_;
template <typename M> void debug_print(M & m, clientid_type const & id)
{
    if (!debug_id_.empty())
    {
        LOG_I << &m << " " << id;
        if (debug_id_ == id)
        {
            BOOST_FOREACH(auto const & p , m)
            {
                LOG_I << p.first <<" : " << p.second;
            }
        }
    }
}
template <typename M, typename T> void debug_print(M & m, T const & id)
{
    if (!debug_id_.empty())
    {
        LOG_I << &m << " " << id;
    }
}


template <typename T>
client_ptr xindex::get(T const & id, bool memonly)
{
//    RECORD_TIMER(timer_log);
    typedef typename map<T, client_ptr>::iterator iterator;
    map<T, client_ptr>& m = indexobj<T>();

    debug_print(m, id);

    iterator it = m.find(id);
    if (it != m.end())
        return it->second;

    if (!memonly)
    {
        client_ptr cli = Client::load(id);
        if (cli)
        {
            BOOST_ASSERT(cli->user_id()>0);
            xindex::set(cli->user_id(), cli);
            if (!cli->client_id().empty())
                xindex::set(cli->client_id(), cli);
            const string & tok = cli->apple_devtoken_; //cli->cache().get<string>("aps_token","");
            if (!tok.empty())
                xindex::set(index_by_aps_tok(tok), cli);
            return cli;
        }
    }
    return client_ptr();
}

void Client::set_apple_devtoken(client_ptr & cli, std::string const & tok)
{
    if (tok != cli->apple_devtoken_)
    {
        if (!cli->apple_devtoken_.empty())
            xindex::set(index_by_aps_tok(cli->apple_devtoken_), client_ptr());

        cli->apple_devtoken_ = tok;
        if (!tok.empty())
            xindex::set(index_by_aps_tok(tok), cli);
    }

    // if (save_cache)
    // {
    //     sql::exec(format("UPDATE client SET AppleDevToken='%2%' cache='%3%'"
    //                 " WHERE UserId=%1% LIMIT 1")
    //             % cli->user_id() % tok % sql::db_escape(cli->cache().encode()) );
    // }
    // else
    //     sql::exec(format("UPDATE client SET AppleDevToken='%1%' WHERE UserId=%2% LIMIT 1") % tok % cli->user_id());
}

template <typename T>
client_ptr xindex::set(T const & id, client_ptr const & c)
{
    typedef typename map<T, client_ptr>::iterator iterator;
//    RECORD_TIMER(timer_log);

    map<T, client_ptr>& m = indexobj<T>();
    pair<iterator, bool> res = m.insert( make_pair(id, c) );

    debug_print(m, id);

    client_ptr ret = res.first->second;

    if (!c)
    {
        if (res.second)
            LOG_I << "erase Not exist client: " << id;
        m.erase(res.first);
    }
    else if (!res.second)
    {
        res.first->second = c;
    }
    LOG_I << c << " index k=" << id;

    return ret;
}

client_ptr Client::sqload(format & sql)
{
#define UCV_FIELDS "UserId,Token,cache,mac,spot,UserName,password,nick,sex,icon,age,signature,UserPhone,mail,constellation,regist_time,background,AppleDevToken,ver"
                  //0     ,1 ,2    ,3  ,4   ,5       ,6       ,7   ,8  ,9   ,10 ,11       ,12       ,13  ,14
                  //15
//    RECORD_TIMER(timer_log);
    const char* statistic_charms = "select sum(type) from statistics_charisma where UserId=%1%";
    const char* statistic_rich = "select sum(money) from statistics_rich where UserId=%1%";
    sql::datas datas(sql % UCV_FIELDS);
    sql::datas::row_type row = datas.next();
    if (!row)
    {
        LOG_W << "NO record:" << sql;
        return client_ptr();
    }

    // client_ptr cli(new Client(lexical_cast<UID>(row.at(0)), row.at(1,"")));
    client_ptr cli(make_shared<Client>(lexical_cast<UID>(row.at(0)), row.at(1,"")));

    cli->cache_ = json::decode(row.at(2,"{}"));
    cli->macaddr_ = row.at(3,"");

    string user = row.at(5, "");

    cli->user_info_.put("userid", cli->user_id());
    cli->user_info_.put("user", user);
    // cli->user_info_.put("pwd", row.at(6));
    cli->user_info_.put("nick", row.at(7));
    cli->user_info_.put("gender", row.at(8,"M"));

    cli->user_info_.put("icon", complete_url(row.at(9,"")));

    cli->user_info_.put("age", row.at(10,"2014-01-01"));
    cli->user_info_.put("signature", row.at(11,""));
    cli->user_info_.put("phone", row.at(12,""));
    cli->user_info_.put("mail", row.at(13,""));
    cli->user_info_.put("constellation", row.at(14,"摩羯座"));
    cli->user_info_.put("regist_time", row.at(15,""));
    cli->user_info_.put("role", 1);
    cli->user_info_.put("userStatus", 3);
    std::string bg;
    if (row.at(16,NULL))
        bg = complete_url(row.at(16));
    cli->user_info_.put("background", bg);

    if(ends_with(user, "@QQ") || ends_with(user, "@weibo"))
        cli->flags_.is_third_part = 1;
    cli->flags_.is_guest = is_match(user, REG_USER_GUEST);
    cli->flags_.is_iosrel = cli->cache_.get<bool>("isrel",false);
    cli->apple_devtoken_ = row.at(17,"");

    cli->richs = GetStatistic(format(statistic_rich) %cli->user_id());
    cli->charms = GetStatistic(format(statistic_charms) %cli->user_id());
    cli->user_info_ver_ = lexical_cast<UInt>(row.at(18,"1"));

    cli->pub_msg_id = 0;
    auto reply = redis::command("HGET", HKEY_USER_GLOBAL_MESSAGE_LAST, cli->user_id());
    if (reply && reply->type == REDIS_REPLY_STRING) {
        cli->pub_msg_id = lexical_cast<MSGID>(reply->str);
    }

    cli->lucky_shake_count = 3;
    auto rep = redis::command("HGET", HKEY_USER_LUCK_SHAKE_COUNT_LEFT, cli->user_id());
    if (rep && rep->type == REDIS_REPLY_STRING) {
        cli->lucky_shake_count = lexical_cast<MSGID>(rep->str);
    }

    LOG_I << cli << " load";
    return cli;
}

client_ptr Client::load(const clientid_type& cid)
{
    if (cid.empty())
        return client_ptr();
    format sql("SELECT %2% FROM user_client_view WHERE Token='%1%'");
    sql % cid;
    return sqload(sql);
}

client_ptr Client::load(UID uid)
{
    if (uid == 0)
        return client_ptr();
    format sql("SELECT %2% FROM user_client_view WHERE UserId='%1%'");
    sql % uid;
    return sqload(sql);
}

client_ptr Client::load(const index_key<std::string,tag_aps_tok>& id)
{
    if (id.val_.empty())
        return client_ptr();
    format sql("SELECT %2% FROM user_client_view WHERE AppleDevToken='%1%'");
    sql % id.val_;
    return sqload(sql);
}

void Client::load_black()
{
    if(uid_ == INVALID_UID) return;
    format fmt("SELECT OtherId FROM contacts WHERE UserId=%1% and relation=2");
    sql::datas datas(fmt %uid_);
    while(sql::datas::row_type row = datas.next()){
        const char* pUser_ID = row.at(0);
        if(NULL != pUser_ID) black_list.insert(lexical_cast<UID>(pUser_ID));
    }
    load_blacks_flag = true;
}

//version 1.1
void Client::load_fans()
{
    if(uid_ == INVALID_UID) return;
    format fmt("SELECT UserId,admire_time FROM contacts WHERE OtherId=%1% and relation=1");
    sql::datas datas(fmt %uid_);
    while(sql::datas::row_type row = datas.next()){
        const char* pUser_ID = row.at(0);
        const char* padmire_time = row.at(1,"");
        if(NULL!=pUser_ID || NULL!=padmire_time){
            fans.insert(make_pair(lexical_cast<UID>(pUser_ID),string(padmire_time)));
        }
    }

    string regist_time = user_info_.get<string>("regist_time","");
    fans.insert(make_pair(SYSADMIN_UID, regist_time));
    load_fans_flag = true;
}

void Client::load_admires()
{
    if(uid_ == INVALID_UID) return;
    format fmt("SELECT OtherId,OtherName,admire_time FROM contacts WHERE UserId=%1% and relation=1");
    sql::datas datas(fmt %uid_);
    while(sql::datas::row_type row = datas.next()){
        const char* pUser_ID = row.at(0);
        const char* pother_name = row.at(1,"");
        const char* padmire_time = row.at(2,"");
        if(NULL!=pUser_ID || NULL!=padmire_time){
            admires.insert(make_pair(lexical_cast<int>(pUser_ID), make_pair(padmire_time,pother_name)));
        }
    }
    string regist_time = user_info_.get<string>("regist_time","");
    string system_name = sysadmin_client_ptr()->user_info().get<string>("nick","月下");
    admires.insert(make_pair(SYSADMIN_UID, make_pair(regist_time,system_name)));
    load_admires_flag = true;
}

// void Client::load_charm()
// {
//     const char* SELECT_CHARISMA = "select sum(type) as num,SessionId "
//         " from statistics_charisma "
//         " where UserId=%1% group by SessionId";
//     sql::datas datas(format(SELECT_CHARISMA) %this->uid_);
//     while ( sql::datas::row_type row = datas.next() ) {
//         string sessionId = row.at(1,"");
//         if ( !sessionId.empty() ) {
//             bar_charms_[sessionId] = lexical_cast<int>(row.at(0,"0"));
//         }
//     }
// 
//     load_charm_flag = true;
// }

// typedef boost::function<json::object (http::request&, client_ptr&)> > httpfn_type;

//template <typename ...A> static void record_connect(UID uid, A... a)
//{
//    static boost::filesystem::ofstream file_out;
//    static time_t file_time = 0;
//
//    time_t tpcur = time(0);
//
//    if (tpcur - file_time >= Seconds_PDay)
//    {
//        file_time = (tpcur - tpcur % Seconds_PDay);
//        struct tm tm;
//        localtime_r(&file_time, &tm);
//        std::string outfn = str(format("/tmp/yx-socket-closed.%1%-%2%") % tm.tm_mon % tm.tm_mday);
//        if (file_out)
//            file_out.close();
//        file_out.clear();
//        file_out.open(outfn, ios_base::app);
//    }
//
//    outs_helper(file_out, uid)(a...);
//
//    //file_out << uid
//    //    <<"\t"<< tpo
//    //    <<"\t"<< tpc
//    //    <<"\t"<< bg
//    //    <<"\t"<< closed_by
//    //    <<"\n";
//    file_out.flush();
//}

struct myrecorder : boost::noncopyable
{
    boost::filesystem::ofstream file_out;
    std::string file_name;
    int mday_;

    struct outs_helper
    {
        template <typename X> void operator()(X const & x)
                { os_ << "\t" << x; }
        template <typename X, typename ...A> void operator()(X const& x, A... a)
                { (*this)(x); (*this)(a...); }
        template <typename X> outs_helper(std::ostream & s, X const& ix)
                : os_(s) { os_ << ix; }
        ~outs_helper() { os_ << "\n"; }
        std::ostream & os_;
    };

    myrecorder(std::string const & fn)
        : file_name(fn) // ("/tmp/yx-socket-closed.%1%-%2%")
        , mday_(-1)
    {}

    void openfile(time_t tpcur)
    {
        struct tm tm;
        localtime_r(&tpcur, &tm);
        if (tm.tm_mday != mday_) // (tpcur - file_time >= Seconds_PDay)
        {
            if (file_out)
            {
                file_out.close();
                file_out.clear();
            }
            auto fn = time_format<128>(file_name, tm);
            if (mday_ == -1)
                file_out.open(fn, ios_base::app);
            else
                file_out.open(fn);
            mday_ = tm.tm_mday;
        }
    }

    template <typename X, typename ...A> void operator()(X ix, A... a)
    {
        time_t tpcur = time(0);
        openfile(tpcur);
        outs_helper(file_out, ix)(a...);
    }

    void flush() { file_out.flush(); }
};

static myrecorder record_connect("/tmp/yx-socket-closed.%d");
static myrecorder record_msgtime("/tmp/yx-msgtime.%d");

struct yx_socket_record
{
    UID uid;
    time_t begin, end;
    int bg;
    int close_operator;

    ip::address ipa; // TODO
    unsigned short port;

    friend std::istream& operator>>(std::istream& in, yx_socket_record& rec)
    {
        in >> rec.uid >> rec.begin >> rec.end >> rec.bg >> rec.close_operator;
        std::string tmp;
        return getline(in, tmp);
    }

    yx_socket_record() { memset(this, 0, sizeof(*this)); }
};

static std::string html_yx_socket(UID uid, int day=0)
{
    time_t tpcur = time(0);

    struct tm tm;
    localtime_r(&tpcur, &tm);
    if (day != 0)
        tm.tm_mday = day;
    boost::filesystem::ifstream ifs(time_format<128>(record_connect.file_name, tm));

    std::string thead = "<td>离线时长</td><td>在线时间</td><td>离线时间</td><td>在线时长</td><td>C/S</td>";
    if (uid == 0)
        thead = "<td>id</td>" + thead;

    std::string tbody = "<tr>" + thead + "</tr>";
    UInt sum_online = 0, sum_offline = 0;
    time_t prev_end = 0;

    yx_socket_record rec;
    while (ifs >> rec)
    {
        std::string tds;
        if (uid == 0)
            tds += str(boost::format("<td>%1%</td>") % rec.uid);
        else if (rec.uid != uid )
            continue;

        if (prev_end == 0)
            prev_end = rec.begin;
        int on_secs = rec.end - rec.begin;
        int off_secs = rec.begin - prev_end;
        prev_end = rec.end;

        sum_offline += off_secs;
        sum_online += on_secs;
        tds += str(boost::format("<td>%1%</td>") % (off_secs));
        tds += time_format<64>("<td>%T</td>", rec.begin);
        tds += time_format<64>("<td>%T</td>", rec.end);
        tds += str(boost::format("<td>%1%</td>") % (on_secs));
        tds += str(boost::format("<td>%1%</td>") % (rec.close_operator));
        tbody += "<tr>" + tds + "</tr>";
    }

    std::string sums = "<table border=\"0\" bordercolor=\"#000000\"><tr>"
        + str(boost::format("<td>总在线时长: </td><td>%1%</td>") % sum_online)
        + "<td>&nbsp;</td>"
        + str(boost::format("<td>总离线时长: </td><td>%1%</td>") % sum_offline)
        + "</tr></table>"
        ;
    return "<a href=\"/clients-info\">clients-info</a>"
        + sums
        + "<table border=\"1\" bordercolor=\"#000000\">" + tbody + "</table>"
        + std::string("C/S: 0-服务器关闭，1-客户端关闭")
        ;
}

void Client::connection_notify(int y)
{
    time_t tpcur = time(0);
    if (y)
    {
        if (flags_.is_app_background)
            count_.connect.bg++;
        else
            count_.connect.fg++;
        pt_.sock.open = second_clock::local_time(); // tp_.sock_open = tpcur;
        closed_by_ = Closed_By_Local;

        static bool is_rd = 0;
        if (!is_rd)
        {
            is_rd = 1;
            auto reply = redis::command("HGET", "socket/time", user_id());
            if (reply && reply->type == REDIS_REPLY_STRING)
            {
                 time_t t = boost::lexical_cast<time_t>(reply->str);
                 record_connect(user_id(), t, tpcur, 0, int(Closed_By_Local), endpoint_r);
            }
        }
        redis::command("HSET", "socket/time", user_id(), time(0));
    }
    else
    {
        pt_.sock.close = second_clock::local_time(); // tp_.sock_close = tpcur;
        if (uid_ > 0)
        {
            record_connect(user_id()
                    , pt_.sock.open.time_of_day(), pt_.sock.close.time_of_day()
                    , int(flags_.is_app_background), int(closed_by_), endpoint_r);
            redis::command("HDEL", "socket/time", user_id());
        }
    }
}

void incr_charms(client_ptr& cli, int charm, UID fromuid) 
{ 
    string sessionid("");
    if (!(cli->gwid().empty())) { sessionid=cli->gwid();}
    cli->set_charms(charm);

    UID uid = cli->user_id();
    if (fromuid == 0)
        fromuid = uid;

    const char* INSERT_STATISTICS_CHARISMA = "INSERT INTO statistics_charisma(UserId,FromId,type,SessionId) "
        " VALUES(%1%, %2%, %3%, '%4%')";
    sql::exec( boost::format(INSERT_STATISTICS_CHARISMA) % uid % fromuid % charm %sessionid );

    // push_charm(cli, fromuid, charm);
    imessage::message msg("chat/charm", fromuid, make_p2pchat_id(uid, fromuid)); 
    msg.body("content", charm);
    chatmgr::inst().send(msg, cli);
}

static string gen_token(unsigned int pfx=0)
{
    static unsigned int seq_ = randuint();
    char tmp[64];
    snprintf(tmp,sizeof(tmp), "%x-%08x%08x", pfx, (unsigned int)time(0), seq_);
    ++seq_;
    return string(tmp);
}

static UID regist_us(string user, string pwd, string nick, string gender, string icon_path
        , UID uid=0
        , string age="2014-01-01", string constellation="", string signature="")
{
    if (uid == 0)
        uid = alloc_user_id();

    if (pwd.empty())
        pwd = "*";

    if (nick.empty())
        nick = user;

    if ( age.empty() ) {
        age = "2014-01-01";
        constellation = "摩羯座";
    }

    pwd = sql::db_escape(pwd);
    nick = sql::db_escape(nick);
    icon_path = sql::db_escape(icon_path);
    age = sql::db_escape(age);
    constellation = sql::db_escape(constellation);
    signature = sql::db_escape(signature);

    format fmt("INSERT INTO"
            " users(userid,username,password,nick,sex,icon,age,constellation,signature)"
            " VALUES(%1%,'%2%','%3%','%4%','%5%','%6%','%7%','%8%','%9%')");
    sql::exec(fmt % uid % user % pwd % nick % gender %icon_path %age %constellation %signature);

    return uid;
}
UID _user_regist(std::string const & user, std::string const & pwd, string icon_path="")
{
    return regist_us(user, pwd, "", "M", icon_path);
}


struct account
{
    // systemVersion	string	手机系统版本
    // channel	int	1.sina,2.QQ,3.用户名登录
            // longtitude	float	经度
            // latitude	float	纬度
            // deviceType	int	1.iphone,2.ipad,3.android
            // macAddress	string	mac地址
            // deviceToken	string	device Token (从苹果服务器获取)
    // option
    // phone	string	手机号码、邮箱、用户名
    // type	int	1.手机号码，2.邮箱，3.用户名
    // password	string	密码
    //
    // openId	string	新浪用户ID、QQ用户ID(用户登录为空)
    // gender	string	性别(男.M,女.F)
    // userName	string	昵称
    // headIcon	string	头像地址（用户登录为空）
    //
    // sessionid	string	会话ID

    static UID thirdparty_sign_in(const char* tag, http::request & req)
    {
        string openid = req.param("openId");
        if (!is_match(openid,REG_OPENID))
        {
            MYTHROW(EN_UserName_Invalid,client_error_category);
        }

        sql::datas datas(format("SELECT userId FROM %1%Users WHERE id='%2%'")
                % tag % sql::db_escape(openid));
        if (sql::datas::row_type row = datas.next())
        {
            return lexical_cast<UID>(row.at(0));
        }

        string gender = req.param("gender","");
        string nick = req.param("userName","");
        string headIcon = req.param("fileName","");
        // LOG_I << cli << " " <<req;
        // LOG_I << cli << " " <<gender <<" "<< nick <<" "<<headIcon;
        if (!is_match(gender,REG_GENDER)
                || !is_match(nick,REG_NICK))
        {
            MYTHROW(EN_Input_Form,client_error_category);
        }

        const string & content = req.content();
        string icon_path = headIcon;
        if(!headIcon.empty() && !starts_with(headIcon, "http:") && !content.empty()){
            string filename = uniq_relfilepath(headIcon, "headicons");
            if(!filename.empty())icon_path = write_file(filename, content);
        }
        if (icon_path.empty()){
            icon_path =  req.param("headIcon","");
        }

        if (!icon_path.empty() && !is_match(icon_path,REG_URL))
        {
            MYTHROW(EN_Input_Form,client_error_category);
        }

        UID uid = alloc_user_id();
        string age = req.param("age", "2014-01-01");
        string constellation = req.param("constellation", "摩羯座");
        string signature = req.param("sign", "");
        string user = str(format("%1%@%2%") % uid % tag);
        // regist_us(user, string(), nick, gender, icon_path, uid);
        regist_us(user, string(), nick, gender, icon_path, uid, age,constellation, signature);

        sql::exec(format("INSERT into %1%Users(id,userId) values('%2%', %3%)")
                % tag % sql::db_escape(openid) % uid);

#define INSERT_INDIVIDUALDATAS1 ("insert into IndividualDatas(UserId) values(%1%)")
        sql::exec(format(INSERT_INDIVIDUALDATAS1) % uid);

        return uid;
    }

    static UID _sign_in(http::request & req)
    {
        enum { SIGN_PHONE=1, SIGN_EMAIL, SIGN_USER };

        string pwd = req.param("password");
        string u = req.param("phone");
        int tp = lexical_cast<int>(req.param("type"));

        UID uid = INVALID_UID;

        if (!is_match(pwd,REG_PWD))
        {
            MYTHROW(EN_Password_Invalid,client_error_category);
        }

        switch (tp)
        {
            case SIGN_PHONE:
                if (!is_match(u,REG_MOBILE_PHONE))
                {
                    MYTHROW(EN_Phone_Invalid,client_error_category);
                }
                uid = passwd_verify(pwd, "UserPhone", u);
                break;
            case SIGN_EMAIL:
                if (!is_match(u,REG_EMAIL))
                {
                    MYTHROW(EN_Mail_Invalid,client_error_category);
                }
                uid = passwd_verify(pwd, "mail", u);
                break;
            case SIGN_USER:
                if (!is_match(u,REG_USER))
                {
                    MYTHROW(EN_UserName_Invalid,client_error_category);
                }
                uid = passwd_verify(pwd, "UserName", u);
                break;

            default:
                MYTHROW(EN_Input_Data,client_error_category);
        }

        if (uid == INVALID_UID)
        {
            MYTHROW(EN_Login_Fail,client_error_category);
        }

        return uid;
    }

    static client_ptr & sysadmin_client()
    {
        // static client_ptr cli = Client::load(SYSADMIN_UID);
        static client_ptr cli = xindex::get(SYSADMIN_UID); 
        return cli;
    }

    static service_fn_type service_fn(http::request & req, client_ptr & cli);
};

client_ptr & sysadmin_client_ptr()
{
    return account::sysadmin_client();
}

std::ostream& operator<<(std::ostream& out, const client_ptr& c)
{
    if (!c)
        return out << "Null";
    return out << c->uid_
        <<"/"<< c->flags_.devtype
        <<"/"<< c->native_fd_
        ;
}

// template <typename Int>
// inline Int host2net(Int val)
// {
//     switch (sizeof(Int)) {
//         case sizeof(int32_t): return htonl(val);
//         case sizeof(int16_t): return htons(val);
//     }
//     return val;
// }

template <typename Int>
static string & hexsb(string & ret, Int val)
{
    char tmp[sizeof(Int)*2];
    char * p = (char*)&val;
    char * end = &p[sizeof(Int)];
    for (char *d = &tmp[0]; p != end; ++p)
    {
        char h = (*p >> 4) & 0x0f;
        char l = (*p) & 0x0f;
        *d++ = ((h) + ((h) < 10 ? '0' : 'a'-10));
        *d++ = ((l) + ((l) < 10 ? '0' : 'a'-10));
    }
    ret.insert(ret.end(), &tmp[0], &tmp[sizeof(tmp)]);
    return ret;
}

static string gen_client_id(UID uid)
{
    static unsigned int ran = 0;
    static unsigned short ind = 0;
    if ((ind & 0x0f) == 0)
    {
        if (ran == 0)
            ind = randuint(0, 255);
        ran = randuint();
    }
    ind++;

    string ret;
    hexsb(hexsb(hexsb(ret , short(ind)), short(time(0))) , ran);
    char tmp[16];
    snprintf(tmp,sizeof(tmp), "%x", uid);
    return ret + tmp;

    // char tmp[64];
    // snprintf(tmp,sizeof(tmp), "%08x%08x%x", ran, (unsigned int)~time(0), uid);
    // ++ran;
    // return string(tmp);
}

Client::Client(UID uid)
    : cid_(gen_client_id(uid))
{
    uid_ = uid;
    LOG_I << "::Client " << uid_ <<" "<< cid_ <<" "<< this;
    native_fd_ = 0;
    whisper_love_count = 3;
    pic_charm_count = 10;
    load_blacks_flag=load_fans_flag=load_admires_flag=0; //load_charm_flag=false;
    richs = charms = 0;
    user_info_ver_ = 0;
    // tp_app_background = 0;
}

Client::Client(UID uid, const std::string & cid)
    : cid_(cid)
{
    uid_ = uid;
    LOG_I << "::Client " << uid_ <<" "<< cid_ <<" "<< this;
    native_fd_ = 0;
    whisper_love_count = 3;
    pic_charm_count = 10;
    load_blacks_flag=load_fans_flag=load_admires_flag=0; //load_charm_flag=false;
    richs = charms = 0;
    user_info_ver_ = 0;
    // tp_app_background = 0;
}

Client::~Client()
{
    writer_ = boost::function<void (const string&)>();
    if (timer_)
    {
        // timer_->cancel();
        // timer_->ack.cancel();
        // timer_.reset();
        LOG_I << "cancel, Should not reach";
    }
    LOG_I << "::~Client " << uid_ <<" "<< cid_ <<" "<< this;
}

json::object Client::brief_user_info() const
{
    const json::object & i = user_info();
    int attribute = 0x01;
    return json::object()
        ("userid", user_id())
        ("icon", i.get<string>("icon",""))
        ("userName",  i.get<string>("nick",""))
        ("public_flag", i.get<int>("public_flag", 0))
        ("gender",  i.get<string>("gender",""))
        ("attribute",  attribute)
        ("guest", bool(is_guest()))
        ;
}

enum
{
    PAYCHAN_ALIPAY = 1,
    PAYCHAN_UNIONPAY = 2,
    PAYCHAN_IAP = 3,
};
#define ALIPAY_PARTNER "2088011708281512"

static json::object bills(http::request & req, client_ptr& cli)
{
    extern int alloc_bills_id();

    BOOST_ASSERT_MSG(cli && cli->is_authorized(),"client should authrozied");
#define INSERT_BILLS11 "INSERT INTO bills" \
    " (UserId,out_trade_no,channel,subject_id,subject,seller_id,quantity,price,coins,body,productid)" \
    " VALUES(%1%,'%2%',%3%,%4%,'%5%','%6%',%7%,%8%,%9%,'%10%','%11%')"

    if(cli->is_guest()) THROW_EX(EN_Operation_NotPermited);

    int channel = lexical_cast<int>(req.param("channel"));
    string partner;
    if (channel == PAYCHAN_ALIPAY) // this is alipay payment
        partner = ALIPAY_PARTNER;

    int pid = lexical_cast<int>(req.param("pid"));
    int quantity = 1;

    UID userid = cli->user_id();
    format fmt("select * from money where id=%1%");
    sql::datas datas(fmt%pid);
    if(sql::datas::row_type row = datas.next())
    {
        const char *psubject = row.at(1);
        int coins = lexical_cast<int>(row.at(2));
        int money = lexical_cast<int>(row.at(3));
        //const char *pbrief = row.at(4,"");
        const char *pintroduce = row.at(5,"");
        const char *pproductid = row.at(6,"");
        int id= alloc_bills_id();
        string out_trade_no = lexical_cast<string>(userid)+"_"+lexical_cast<string>(id);
        format fmt(INSERT_BILLS11);
        fmt%userid %out_trade_no %channel %id %psubject %partner %quantity %money %coins %pintroduce %pproductid;
        sql::exec(fmt);  

        if (channel == PAYCHAN_IAP) // ios in-app-purchases
        {
            int sandbox = 0;
            string envi = req.param("environment");
            if (envi.find("andbox") != string::npos)
                sandbox = 1;
            asclient<iapbuf>::instance()->send(id,pproductid,req.content(),sandbox);
        }
        return json::object()("partner", partner)
                             ("out_trade_no", out_trade_no)
                             ("seller_id", partner);
    }
    return json::object();
}

//sign=J9dWgMMWbax8jP4bxU28VG9Xp%2BXCD3D7dyvay%2BJQHpEuXVjUqUtwkOk%2F9QHNqsYuxzMeYQxCTANjrhSbLN10nIRY8gzjLRxUIBLBtoTeWzIXl0YK2dQbCCpE4fLfZIvc1poNRbB7ZxSEoxh58pcwg%2Byc58NoyH92ox46THQf7EE%3D
//&sign_type=RSA
//&notify_data=%3Cnotify%3E%3Cpartner%3E2088011708281512%3C%2Fpartner%3E%3Cdiscount%3E0.00%3C%2Fdiscount%3E%3Cpayment_type%3E1%3C%2Fpayment_type%3E%3Csubject%3E%E5%A5%97%E9%A4%902%3C%2Fsubject%3E%3Ctrade_no%3E2013102543352269%3C%2Ftrade_no%3E%3Cbuyer_email%3E236246334%40qq.com%3C%2Fbuyer_email%3E%3Cgmt_create%3E2013-10-25+11%3A49%3A20%3C%2Fgmt_create%3E%3Cquantity%3E1%3C%2Fquantity%3E%3Cout_trade_no%3E1025114901-2021%3C%2Fout_trade_no%3E%3Cseller_id%3E2088011708281512%3C%2Fseller_id%3E%3Ctrade_status%3ETRADE_FINISHED%3C%2Ftrade_status%3E%3Cis_total_fee_adjust%3EN%3C%2Fis_total_fee_adjust%3E%3Ctotal_fee%3E12.00%3C%2Ftotal_fee%3E%3Cgmt_payment%3E2013-10-25+11%3A49%3A21%3C%2Fgmt_payment%3E%3Cseller_email%3Eralph%40kklink.com%3C%2Fseller_email%3E%3Cgmt_close%3E2013-10-25+11%3A49%3A21%3C%2Fgmt_close%3E%3Cprice%3E12.00%3C%2Fprice%3E%3Cbuyer_id%3E2088102409487698%3C%2Fbuyer_id%3E%3Cuse_coupon%3EN%3C%2Fuse_coupon%3E%3C%2Fnotify%3E
static client_ptr alipay_notify(http::response& rsp, http::request & req)
{
    LOG_I << "===notify=alipay android";
    LOG_I << req.params();
    LOG_I << req.headers();
    LOG_I << req.content();
#define SELECT_BILLS1 ("select id,subject,quantity,price,coins,UserId,channel from bills where out_trade_no='%1%'")

#define UPDATE_BILLS5 ("update bills set trade_no='%1%',status='%2%',notify_time=CURRENT_TIMESTAMP,buyer_email='%3%',payment_type=%4% where out_trade_no='%5%'")

#define UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2 ("UPDATE IndividualDatas set money=money+%1% WHERE UserId=%2%")

    try {
        Http_kvals param;
        parse_keyvalues(param, req.content()) ;
        std::string xml = param.get("notify_data");
        LOG_I << xml;

        property_tree::ptree pt;
        std::istringstream f(xml);
        property_tree::xml_parser::read_xml(f, pt);
        string seller_email = pt.get<string>("notify.seller_email", "");
        string partner = pt.get<string>("notify.partner", "");
        string payment_type = pt.get<string>("notify.payment_type", "");
        string buyer_email = pt.get<string>("notify.buyer_email", "");
        string trade_no = pt.get<string>("notify.trade_no", "");
        string buyer_id = pt.get<string>("notify.buyer_id", "");
        // int quantity = lexical_cast<int>(pt.get<string>("notify.quantity", "0"));
        float total_fee = lexical_cast<double>(pt.get<string>("notify.total_fee", "0"));
        string use_coupon = pt.get<string>("notify.use_coupon", "");
        string is_total_fee_adjust = pt.get<string>("notify.is_total_fee_adjust", "");
        float notify_price = lexical_cast<double>(pt.get<string>("notify.price", "0"));
        string out_trade_no = pt.get<string>("notify.out_trade_no", "");
        string gmt_create = pt.get<string>("notify.gmt_create", "");
        string seller_id = pt.get<string>("notify.seller_id", "");
        string subject = pt.get<string>("notify.subject", "");
        string trade_status = pt.get<string>("notify.trade_status", "");
        // float discount = lexical_cast<double>(pt.get<string>("notify.discount", "0"));

        LOG_I <<out_trade_no;
        LOG_I <<trade_status;

        format fmt(SELECT_BILLS1);
        sql::datas datas(fmt% sql::db_escape(out_trade_no));
        if(sql::datas::row_type row = datas.next()){
           // int id = lexical_cast<int>(row.at(0));
           // const char* psubject = row.at(1);
           int quantity = lexical_cast<int>(row.at(2));
           int price = lexical_cast<int>(row.at(3));
           int coins = lexical_cast<int>(row.at(4));
           UID userid = lexical_cast<UID>(row.at(5));
           int channel = lexical_cast<int>(row.at(6));
           if(price/100.0-notify_price<0.001 && total_fee-quantity*price/100.0<0.001){
               if("TRADE_FINISHED"==trade_status || "TRADE_SUCCESS"==trade_status){
                   sql::exec(format(UPDATE_BILLS5)%trade_no %trade_status %buyer_email %payment_type %out_trade_no);
                   sql::exec(format(UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2)%(quantity*coins)%userid);  

                   client_ptr cli = xindex::get(userid);
                   if (cli){
                       imessage::message msg("status/bill", SYSADMIN_UID, make_p2pchat_id(userid, SYSADMIN_UID));
                       msg.body
                           ("out_trade_no",out_trade_no)
                           ("status", true)
                           ("coins", quantity*coins)
                           ("channel",channel)
                           ;
                       sendto(msg, cli);
                       rsp.content( string("success") );
                   }
                   else{
                       LOG_I<<"failed to find buyer";
                   }
               }
               else{
                   LOG_I<<"unfinished bill";
               }
           }
           else{
               LOG_I<<"illegal bill";
           }
        }
        else{
            LOG_I<<"failed to find bill:"<<out_trade_no;
        }
    }
    catch (...) {}


    return client_ptr();
}

static client_ptr alipay_notify_ios(http::response& rsp, http::request & req)
{
    LOG_I << "===notify=alipay ios";
    LOG_I << req.params();
    LOG_I << req.headers();
    LOG_I << req.content();
#define SELECT_BILLS1 ("select id,subject,quantity,price,coins,UserId,channel from bills where out_trade_no='%1%'")

#define UPDATE_BILLS5 ("update bills set trade_no='%1%',status='%2%',notify_time=CURRENT_TIMESTAMP,buyer_email='%3%',payment_type=%4% where out_trade_no='%5%'")

#define UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2 ("UPDATE IndividualDatas set money=money+%1% WHERE UserId=%2%")

    try {
        Http_kvals param;
        parse_keyvalues(param, req.content()) ;
        // std::string xml = param.get("notify_data");
        LOG_I << param;

        // property_tree::ptree pt;
        // std::istringstream f(xml);
        // property_tree::xml_parser::read_xml(f, pt);
        
        string seller_email = param.get("seller_email", "");
        string partner = param.get("partner", "");
        string payment_type = param.get("payment_type", "");
        string buyer_email = param.get("buyer_email", "");
        string trade_no = param.get("trade_no", "");
        string buyer_id = param.get("buyer_id", "");
        // int quantity = lexical_cast<int>(param.get("quantity", "0"));
        float total_fee = lexical_cast<double>(param.get("total_fee", "0"));
        string use_coupon = param.get("use_coupon", "");
        string is_total_fee_adjust = param.get("is_total_fee_adjust", "");
        float notify_price = lexical_cast<double>(param.get("price", "0"));
        string out_trade_no = param.get("out_trade_no", "");
        string gmt_create = param.get("gmt_create", "");
        string seller_id = param.get("seller_id", "");
        string subject = param.get("subject", "");
        string trade_status = param.get("trade_status", "");
        // float discount = lexical_cast<double>(param.get("discount", ""));

        LOG_I <<out_trade_no;
        LOG_I <<trade_status;

        format fmt(SELECT_BILLS1);
        sql::datas datas(fmt% sql::db_escape(out_trade_no));
        if(sql::datas::row_type row = datas.next()){
           // int id = lexical_cast<int>(row.at(0));
           // const char* psubject = row.at(1);
           int quantity = lexical_cast<int>(row.at(2));
           int price = lexical_cast<int>(row.at(3));
           int coins = lexical_cast<int>(row.at(4));
           UID userid = lexical_cast<UID>(row.at(5));
           int channel = lexical_cast<int>(row.at(6));
           if(price/100.0-notify_price<0.001 && total_fee-quantity*price/100.0<0.001){
               if("TRADE_FINISHED"==trade_status || "TRADE_SUCCESS"==trade_status){
                   sql::exec(format(UPDATE_BILLS5)%trade_no %trade_status %buyer_email %payment_type %out_trade_no);
                   sql::exec(format(UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2)%(quantity*coins)%userid);  

                   client_ptr cli = xindex::get(userid);
                   if(cli){

                       imessage::message msg("status/bill", SYSADMIN_UID, make_p2pchat_id(userid, SYSADMIN_UID));
                       msg.body
                           ("out_trade_no",out_trade_no)
                           ("status", true)
                           ("coins", quantity*coins)
                           ("channel",channel)
                           ;
                       sendto(msg, cli);

                       rsp.content( string("success") );
                   }
                   else{
                       LOG_I<<"failed to find buyer";
                   }
               }
               else{
                   LOG_I<<"unfinished bill";
               }
           }
           else{
               LOG_I<<"illegal bill";
           }
        }
        else{
            LOG_I<<"failed to find bill:"<<out_trade_no;
        }
    }
    catch (...) {}


    return client_ptr();
}

void iap_notify(int id, bool success)
{
    try
    {
        LOG_I << "===notify=iap " << id <<" "<< success;
        // json::object j = json::decode(_receipt); // int id = j.get<int>("id_");

        sql::datas datas(format("SELECT UserId,out_trade_no,quantity,coins,channel FROM bills WHERE id='%1%'") % id);
        sql::datas::row_type row = datas.next();
        if (!row)
        {
            LOG_E << "bill not found";
            return;
        }

        UID userid = lexical_cast<UID>(row.at(0));
        LOG_I << userid;
        const char * out_trade_no = row.at(1);
        LOG_I << out_trade_no;
        int quantity = lexical_cast<int>(row.at(2));
        LOG_I << quantity;
        int coins = lexical_cast<int>(row.at(3));
        LOG_I << coins;
        int channel = lexical_cast<int>(row.at(4));
        LOG_I << channel;

        const char *ps = "TRADE_SUCCESS";
        if (!success)
            ps = "TRADE_FAIL";

        boost::system::error_code ec;
        sql::exec(ec, format("UPDATE bills SET status='%1%',notify_time=CURRENT_TIMESTAMP WHERE id='%2%'") % ps % id);
        sql::exec(ec, format("UPDATE IndividualDatas set money=money+%1% WHERE UserId=%2%")%(quantity*coins)%userid);  

        if (client_ptr cli = xindex::get(userid))
        {
            imessage::message msg("status/bill", SYSADMIN_UID, make_p2pchat_id(userid, SYSADMIN_UID));
            msg.body
                ("out_trade_no",out_trade_no)
                ("status", success)
                ("coins", quantity*coins)
                ("channel",channel)
                ;
            sendto(msg, cli);
            return;
        }
    }
    catch (std::exception const & e)
    {
        LOG_E << e.what();
    }
    catch (...) {}

    LOG_E << "fail";
}

void union_pay_notify(UID uid, int pid
        , std::string const & merid
        , std::string const &trade_no, std::string const &out_trade_no)
{
    try
    {
        LOG_I << "===notify=union pay";
        if (client_ptr cli = xindex::get(uid))
        {
            sql::datas datas(format("SELECT subject,coins,money,introduce,productid FROM money WHERE id=%1% LIMIT 1") % pid);
            if(sql::datas::row_type row = datas.next())
            {
                int coins = lexical_cast<int>(row.at(1));
                int price = lexical_cast<int>(row.at(2));
                const char *psubject = row.at(0);
                const char *pintroduce = row.at(3,"");
                const char *pproductid = row.at(4,"");
                int bid = alloc_bills_id();

                sql::exec(format("UPDATE IndividualDatas SET money=money+%1% WHERE UserId=%2%") % coins % uid);  

                const char * sql_up_bill = "INSERT INTO bills("
                    " id,channel,status,UserId"
                    ",out_trade_no,trade_no,seller_id"
                    ",quantity,price,coins"
                    ",productid,subject_id,subject,body"
                    ") VALUES("
                    " %1%,2,'TRADE_SUCCESS',%2%"
                    ",'%3%','%4%','%5%'"
                    ",1,%6%,%7%"
                    ",'%8%','%9%','%10%','%11%'"
                    ")" ;
                sql::exec(format(sql_up_bill)
                        % bid % uid
                        % out_trade_no % trade_no % merid
                        % price % coins
                        % pproductid % pid % psubject % pintroduce);  

                imessage::message msg("status/bill", SYSADMIN_UID, make_p2pchat_id(uid, SYSADMIN_UID));
                msg.body
                    ("out_trade_no", out_trade_no)
                    ("status", true)
                    ("coins", coins)
                    ("channel", int(PAYCHAN_UNIONPAY))
                    ;
                sendto(msg, cli);
            }
        }
    }
    catch (...) {}
}

json::object charges(http::request & req, client_ptr& cli)
{
    LOG_I <<cli<< req.params();
    LOG_I <<cli<< req.headers();
    LOG_I <<cli<< req.content();
    if(cli->is_guest()) MYTHROW(EN_Unauthorized, client_error_category);

    json::object jo = json::decode(req.content());
    int id = jo.get<int>("pid");
    string out_trade_no = jo.get<string>("out_trade_no","");
    string subject = jo.get<string>("subject");
    string payment_type = jo.get<string>("payment_type","1");
    string seller_id = jo.get<string>("seller_id","");
    double total_fee = jo.get<double>("total_fee");
    string body = jo.get<string>("body","");

#define SELECT_MONEY_ID1 ("select * from money where id=%1%")
    format fmt(SELECT_MONEY_ID1);
    sql::datas datas(fmt % id);
    if(sql::datas::row_type row = datas.next()){
        const char *psubject = row.at(1);
        const char *pcoins = row.at(2);
        float money = lexical_cast<int>(row.at(3)) /100.0;
        // const char *pbrief = row.at(4,"");
        // const char *pintroduce = row.at(5,"");

        if(subject!=psubject || total_fee-money>0.01){
            MYTHROW(EN_Input_Data, client_error_category);
        }

#define UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2 ("UPDATE IndividualDatas set money=money+%1% WHERE UserId=%2%")
        sql::exec(format(UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2)%pcoins %cli->user_id());  
    }
    else{
        MYTHROW(EN_Input_Data, client_error_category);
    }

    return json::object();
}

json::object appleCharges(http::request & req, client_ptr& cli)
{
    LOG_I <<cli<< req.params();
    LOG_I <<cli<< req.headers();
    LOG_I <<cli<< req.content();
    if(cli->is_guest()) MYTHROW(EN_Unauthorized, client_error_category);

    json::object jo = json::decode(req.content());
    int id = jo.get<int>("pid");
    string bid = jo.get<string>("bid");
    string bvrs = jo.get<string>("bvrs");
    // int item_id = jo.get<int>("item_id");
    string purchase_date = jo.get<string>("purchase_date");
    string product_id = jo.get<string>("product_id");
    // int transaction_id = jo.get<int>("transaction_id");
    string unique_identifier = jo.get<string>("unique_identifier");
    int quantity = jo.get<int>("quantity");
    float total_fee = jo.get<float>("total_fee");
    // int deviceType = jo.get<int>("deviceType");

#define SELECT_MONEY_ID1 ("select * from money where id=%1%")
    format fmt(SELECT_MONEY_ID1);
    sql::datas datas(fmt % id);
    if(sql::datas::row_type row = datas.next()){
        // const char *psubject = row.at(1);
        const char *pcoins = row.at(2);
        float money = lexical_cast<int>(row.at(3)) /100.0;
        const char *pproductid = row.at(6,"");
        // const char *pbrief = row.at(4,"");
        // const char *pintroduce = row.at(5,"");

        if(product_id!=pproductid || total_fee-money>0.01){
            MYTHROW(EN_Input_Data, client_error_category);
        }

        int coins = lexical_cast<int>(pcoins) * quantity;
#define UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2 ("UPDATE IndividualDatas set money=money+%1% WHERE UserId=%2%")
        sql::exec(format(UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2)%coins %cli->user_id());  
    }
    else{
        MYTHROW(EN_Input_Data, client_error_category);
    }

    return json::object();
}


// userid	int	用户ID
// type	int	1.手机号码，2.邮箱地址
// deviceType	int	1.iphone,2.ipad,3.android
// option
// phone	string	手机号码
// mail	string	邮箱地址

json::object Client::getauth(http::request& req, client_ptr& cli)
{
    int type = lexical_cast<int>(req.param("type","1"));
    if (type != 1) // email
    {
        MYTHROW(EN_Input_Data,client_error_category);

        // string email = req.param("mail");
        // if (!is_match(email,REG_EMAIL))
        // {
        //     THROW_EX(EN_Input_Form);
        // }
        // string code = lexical_cast<string>( randuint(100000,999999) );
        // ...
        // cli->temps_.put("auth",
        //         json::object()("code", code)("email", email));

        // return json::object();
    }

    // string user = req.param("userid");
    string phone = req.param("phone");

    if (!is_match(phone,REG_MOBILE_PHONE))
    {
        MYTHROW(EN_Phone_Invalid,client_error_category);
    }

    json::object auth;

    string tmp = "【月下】";
    sql::datas datas(format("SELECT password FROM users WHERE UserPhone='%1%' LIMIT 1") % phone);
    sql::datas::row_type row = datas.next();

    ValidCode::Usage us = ValidCode::Usage::BIND;
    int usage = lexical_cast<int>( req.param("application") );
    if (usage == 1) // resetpwd
    {
        if (!row) // not bind
        {
            MYTHROW(EN_Phone_Not_Bind,client_error_category);
        }
        auth.put("pwd", row.at(0));
        tmp += "您正在申请找回月下账号密码，验证码：";
        us = ValidCode::Usage::PASSWORD;
    }
    else if (usage == 3)
    {
        if(row)
        {
            MYTHROW(EN_Phone_Invalid,client_error_category);
        }
        tmp += "您正在注册月下，验证码：";
        us = ValidCode::Usage::REGIST;
    }
    else // bind phone
    {
        if (row) // already bind
        {
            MYTHROW(EN_Phone_Bind_Already,client_error_category);
        }
        tmp += "您正在申请绑定手机号码，验证码：";
    }

    string code;
    Tremove_code();
    auto i = Pfind_code(phone, us);
    if ( i == valid_codes.end() ) {
        code = lexical_cast<string>( randuint(1000,9999) );
        // sql::exec(format("INSERT INTO sms_authcode(phone,code,usage) VALUES('%1%','%2%',%3%)") % phone % code % usage);
        insert_code(ValidCode::create_valid_code(phone, code, time(NULL), us));
    } else {
        code = i->code;
    }

    auth.put("code", code);
    auth.put("phone", phone);
    auth.put("dtl", (unsigned int)time(0)+120);
            /*("user", user)*/
    cli->temps_.put("auth", auth);

    LOG_I << format("sms send %1% %2%") % phone % code;
    
    tmp += code+" (2分钟内有效)，如非本人操作，请忽略本短信";
    LOG_I<<tmp;

    // string tmp = code + "（月下验证码）【凯凯无线】";
    asclient<smsbuf>::instance()->send(phone, tmp);
    // sendsms(phone, code + "（月下验证码）【凯凯无线】");

    return json::object();
}

// mobileMailFindPwd
//
// userid	int	用户id
// type	int	1.手机找回密码，2.邮箱找回密码
// deviceType	int	1.iphone,2.ipad,3.android
// Option
// validCode	string	验证码
// oldpwd	string	旧密码
// newpwd	string	新密码
// mail	string	邮箱地址
// phone	string	手机号码
//

static client_ptr mailpwd(http::response& rsp, http::request & req)
{
    string mail = req.param("mail");
    string k = req.param("k");
    string t = req.param("t");
    string c = req.param("c");
    bool step2 = is_exist(req.params(),"p");

    UID uid = 0;
    string nick;
    {
        sql::datas datas(format("SELECT id,uid,mail,token,deadline,nick FROM mailPwd WHERE id=%1% LIMIT 1") % k);
        sql::datas::row_type row = datas.next();
        if (!row)
        {
            rsp.content( readall("mailpwd-expired.htm",htm_dir_) );
            return client_ptr();
        }
        uid = lexical_cast<UID>(row.at(1)); //const char* puid = row.at(1);
        const char* pmail = row.at(2);
        const char* ptoken = row.at(3);
        const char* pdeadline = row.at(4);
        nick = row.at(5);

        if (t != ptoken || mail != pmail)
        {
            rsp.content( readall("mailpwd-expired.htm",htm_dir_) );
            return client_ptr();
        }
    }

    const char* hidden =
        "<input type='hidden' name='mail' value='%1%' />"
        "<input type='hidden' name='t' value='%2%' />"
        "<input type='hidden' name='k' value='%3%' />"
        "<input type='hidden' name='c' value='%4%' />"
        ;
    format hids = format(hidden) % mail % t % k % c;
    string url = web_url_ + "/a/mail/pwd";

    if (step2)
    {
        string p = req.param("p");
        LOG_I << p;
        // string q = req.param("q");

        if (/*p != q ||*/ !is_match(p,REG_PWD))
        {
            rsp.content(file_format("mailpwd.htm",htm_dir_) % url % hids % nick );
            return client_ptr();
        }

        boost::system::error_code ec;
        sql::exec(ec, format("UPDATE users SET password='%1%' WHERE UserId=%2% LIMIT 1") % p % uid);
        sql::exec(ec, format("DELETE FROM mailPwd WHERE uid=%1%") % uid);

        rsp.content( readall("mailpwd-success.htm",htm_dir_) );
        return client_ptr();
    }

    rsp.content(file_format("mailpwd.htm",htm_dir_) % url % hids % nick );
    return client_ptr();
}

static json::object resetpwd_mail(http::request& req, client_ptr& cli)
{
    BOOST_ASSERT_MSG(cli,"client_ptr should not Null");
    string mail = req.param("mail");

    if (!is_match(mail,REG_EMAIL))
    {
        MYTHROW(EN_Mail_Invalid,client_error_category);
    }

    sql::datas datas(format("SELECT UserId,nick FROM users WHERE mail='%1%' LIMIT 1") % mail);
    sql::datas::row_type row = datas.next();
    if (!row)
    {
        MYTHROW(EN_Mail_Not_Bind,client_error_category);
    }
    const char *puid = row.at(0);
    const char *pnick = row.at(1,"");

    email m(mail);
    string tok = gen_token();
    string cid = cli->client_id();
    format url = format("%1%/a/mail/pwd?mail=%2%&t=%4%&k=%3%&c=%5%") % web_url_ % mail % m.id() % tok % cid;
    m.fill("重置密码", str(file_format("mailpwd-mail.htm",htm_dir_) % url % pnick));

    LOG_I << m; // << " "<< m.content();

    sql::exec(format(
                "INSERT INTO mailPwd(id,mail,token,uid,deadline,nick) VALUES(%1%,'%2%','%3%',%4%,FROM_UNIXTIME(%5%),'%6%')")
            % m.id() % mail % tok % puid % (time(0) + 60*60*24*1) % pnick);
    mailer::instance().send(m);

    return json::object();
}

json::object Client::resetpwd(http::request& req, client_ptr& cli)
{
    BOOST_ASSERT_MSG(cli,"client_ptr should not Null");
    int type = lexical_cast<int>(req.param("type"));
    if (type != 1)
    {
        return resetpwd_mail(req, cli);
    }

    json::object au;
    au = cli->temps_.get<json::object>("auth", au);
    if (au.empty())
    {
        MYTHROW(EN_AuthCode_Invalid,client_error_category);
    }

    string phone = au.get<string>("phone");
    // string uid = au.get<string>("userid");

    if (/*uid != req.param("userid") ||*/ phone != req.param("phone"))
    {
        MYTHROW(EN_Phone_Incorrect,client_error_category);
    }

    Premove_code( phone, ValidCode::Usage::PASSWORD);
    time_t dtl = au.get<unsigned int>("dtl");
    if (time(0) > dtl)
    {
        MYTHROW(EN_AuthCode_Timeout,client_error_category);
    }
    string code = au.get<string>("code");
    if (code != req.param("validCode"))
    {
        MYTHROW(EN_AuthCode_Invalid,client_error_category);
    }

    // string pwd = au.get<string>("pwd");
    // string oldpwd = req.param("oldpwd");
    string newpwd = req.param("newpwd");
    if (!is_match(newpwd, REG_PWD))
    {
        MYTHROW(EN_Password_Invalid,client_error_category);
    }
    // if (oldpwd == newpwd || oldpwd != pwd)
    // {
    //     MYTHROW(EN_Password_Incorrect,client_error_category);
    // }

    cli->temps_.erase("auth");
    sql::exec(format("UPDATE users SET password='%1%' WHERE UserPhone=%2% LIMIT 1") % sql::db_escape(newpwd) % phone);

    return json::object();
}

json::object Client::chpwd(http::request & req, client_ptr& cli)
{
    BOOST_ASSERT_MSG(cli && cli->is_authorized(),"client should authrozied");

    UID uid = cli->user_id();
    string oldpwd = req.param("oldpwd");
    string newpwd = req.param("newpwd");

    if (!is_match(oldpwd,REG_PWD) || !is_match(newpwd,REG_PWD))
    {
        MYTHROW(EN_Password_Invalid,client_error_category);
    }

    if (passwd_verify(oldpwd, "UserId", uid) != uid)
    {
        MYTHROW(EN_Password_Incorrect,client_error_category);
    }

    // UPDATE("users", format("password='%1%'") % sql::db_escape(newpwd), format("UserId=%1% LIMIT 1") % uid);
    sql::exec(format("UPDATE users SET password='%1%' WHERE UserId=%2% LIMIT 1")
            % sql::db_escape(newpwd) % uid);

    return json::object();
}

// http://passport.cnblogs.com/activate.aspx?id1=d2e8ca87-d1aa-e011-b0da-842b2b196315&id2=jywww%40qq.com
// http://weibo.com/signup/signup_active.php?username=jywww@qq.com&rand=46bb48a4639f31bdde4e8e24c8774866&code=1821657323&entry=weiyonghu&r2=f11fe598664e4c800e8375e94cd44279
//

static client_ptr bindmail(http::response& rsp, http::request & req)
{
    string mail = req.param("mail");
    string k = req.param("k");
    string t = req.param("t");

    UID uid = 0;
    {
        sql::datas datas(format("SELECT id,uid,mail,token,deadline FROM bindMail WHERE id=%1% LIMIT 1") % k);
        sql::datas::row_type row = datas.next();
        if (!row)
        {
            rsp.content( readall("bindmail-expired.htm",htm_dir_) );
            return client_ptr();
        }

        uid = lexical_cast<UID>(row.at(1));
        const char* pmail = row.at(2);
        const char* ptoken = row.at(3);
        const char* pdeadline = row.at(4);

        if (t != ptoken || mail != pmail)
        {
            rsp.content( readall("bindmail-expired.htm",htm_dir_) );
            return client_ptr();
        }
    }

    boost::system::error_code ec;
    sql::exec(ec, format("UPDATE users SET mail='%1%' WHERE UserId=%2% LIMIT 1") % mail % uid);
    if (ec)
    {
        rsp.content( readall("bindmail-expired.htm",htm_dir_) );
        return client_ptr();
    }
    sql::exec(ec, format("DELETE FROM bindMail WHERE uid=%1%") % uid);

    if (client_ptr c = xindex::get(uid,1))
    {
        c->user_info().put("mail", mail);

        imessage::message msg("notice/mail/bind", SYSADMIN_UID, make_p2pchat_id(c->user_id(), SYSADMIN_UID));
        msg.body.put("content", mail) ;
        sendto(msg, c);
    }

    rsp.content( readall("bindmail-success.htm",htm_dir_) );

    return client_ptr();
}

static json::object binda_mail(http::request & req, client_ptr& cli)
{
    string pwd = req.param("password");
    string mail = req.param("mail");
    if (!is_match(mail,REG_EMAIL))
    {
        MYTHROW(EN_Mail_Invalid,client_error_category);
    }
    if(!is_match(pwd, REG_PWD))
    {
        MYTHROW(EN_Password_Invalid,client_error_category);
    }
    sql::datas datas(format("SELECT password,mail FROM users WHERE UserId='%1%' LIMIT 1") % cli->user_id());
    sql::datas::row_type row = datas.next();
    if (!row)
    {
        MYTHROW(EN_User_NotFound,client_error_category);
    }
    if (pwd != row.at(0))
    {
        MYTHROW(EN_Password_Incorrect,client_error_category);
    }
    if (row.at(1,0) != 0)
    {
        MYTHROW(EN_Mail_Bind_Already,client_error_category);
    }

    email m(mail);
    string tok = gen_token();
    format url = format("%1%/a/mail/bind?mail=%2%&t=%4%&k=%3%&c=%5%") % web_url_ % mail % m.id() % tok % cli->client_id();
    m.fill("绑定邮箱", str(file_format("bindmail.htm",htm_dir_) % cli->user_nick() % url));

    LOG_I << m; // << " "<< m.content();

    sql::exec(format(
                "INSERT INTO bindMail(id,mail,token,uid,deadline) VALUES(%1%,'%2%','%3%',%4%,FROM_UNIXTIME(%5%))")
            % m.id() % mail % tok % cli->user_id() % (time(0) + 60*60*24*1));
    mailer::instance().send(m);

    return json::object();
}

json::object Client::bindaccount(http::request & req, client_ptr& cli)
{
    BOOST_ASSERT_MSG(cli && cli->is_authorized(),"client should authrozied");
//deviceType:3#password:4168a9b2fd424554d03a2f68301253b4#type:1#phone:13265426666#validCode:阿斯顿的#
    if(cli->is_third_part()) return json::object();
    UID uid = cli->user_id();

    int type = lexical_cast<int>(req.param("type"));
    if (type != 1)
    {
        return binda_mail(req, cli);
    }

    json::object au = cli->temps_.get<json::object>("auth", json::object());
    cli->temps_.erase("auth");
    if (au.empty())
    {
        MYTHROW(EN_AuthCode_Invalid,client_error_category);
    }

    string phone = au.get<string>("phone");
    // string uid = au.get<string>("userid");
    string code = au.get<string>("code");

    // string pwd = au.get<string>("pwd","");
    // if (!pwd.empty())
    // {
    //     MYTHROW(EN_Phone_Bind_Already,client_error_category);
    // }

    if (/*uid != req.param("userid") ||*/ phone != req.param("phone"))
    {
        MYTHROW(EN_Phone_Incorrect,client_error_category);
    }
    if (code != req.param("validCode"))
    {
        MYTHROW(EN_AuthCode_Invalid,client_error_category);
    }

    Premove_code( phone, ValidCode::Usage::BIND);
    // string pwd = au.get<string>("pwd","");
    // if(pwd != req.param("password")){
    if (uid != passwd_verify(req.param("password"), "UserId", uid)){
        MYTHROW(EN_Password_Incorrect,client_error_category);
    }

    sql::exec(format("UPDATE users SET UserPhone='%1%' WHERE UserId=%2%") % phone % cli->user_id());
    cli->user_info().put("phone", phone);

    return json::object();
}

static json::object logout_fwd(client_ptr& c)
{
    http::request req;
    req.parse_method_line("GET /logout HTTP/1.0\r\n");
    return service_fn_(req, c);
}

json::object Client::login_fwd(http::request & req, client_ptr& cli)
{
    http::request fwdreq;
    fwdreq.parse_method_line(str(format("GET /login?sessionid=%1%&type=1 HTTP/1.0\r\n") % req.param("sessionid","")));
    return service_fn_(fwdreq, cli);
}

json::object Client::sign_out(http::request& req, client_ptr& cli)
{
    if (!cli)
        return json::object();

    LOG_I << cli << " sign_out ..." << cli->gwid();

    json::object ret;
    boost::system::error_code ec;

    if (cli->is_authorized())
    {
        // "INSERT INTO history(uid,token) VALUES(%1%,'%2%')";

        const string & spot = cli->gwid();
        if (!spot.empty())
        {
            // chat_group & cg = chatmgr::inst().chat_group(spot);
            // cg.remove(cli->user_id(), sysadmin_client_ptr());
            auto pbar = bars_mgr::inst().get_session_bar(spot, &ec);
            if (pbar)
                pbar->leave_spot(cli);
        }

        try
        {
            close_socket(cli);

            spot_main_entry(cli, bar_ptr());
            // if (!cli->roomsid_.empty() && is_barchat(cli->uigid_))
            // {
            //     chat_group & cg = chatmgr::inst().chat_group(cli->uigid_);
            //     cg.remove(cli->user_id(), sysadmin_client_ptr());
            // }

            ret = logout_fwd(cli);
            // signals::sign()(cli, false);
        }
        catch (...) {}

        cli->cache().clear();
        cli->db_save(1);
    }

    if (!cli->client_id().empty())
        xindex::set(cli->client_id(), client_ptr());
    if (cli->user_id() > 0)
        xindex::set(cli->user_id(), client_ptr());

    const string & tok = cli->apple_devtoken_; //cli->cache().get<string>("aps_token","");
    if(!tok.empty())
    {
        //TODO cli->apple_devtoken_.clear();
        xindex::set(index_by_aps_tok(tok), client_ptr());
    }

    cli->timer_.reset();

    return ret;
}

void Client::record_sign_out(client_ptr& cli, bool is_forced)
{
#define LOGOUT_RECORD ("INSERT INTO logout(userid, token, is_forced) VALUES(%1%,'%2%',%3%)")
    if ( !cli ) {
        return;
    }

    sql::exec(format(LOGOUT_RECORD) %cli->user_id() %cli->client_id() %is_forced);
}

json::object Client::sign_in(http::request& req, client_ptr& cli)
{
//    RECORD_TIMER(timer_log);
    string sysver = req.param("systemVersion");
    string plat = req.param("deviceType");
    int chan = lexical_cast<int>(req.param("channel"));

    // double longtitude = lexical_cast<double>(req.param("longtitude"));
    // double latitude = lexical_cast<double>(req.param("latitude"));

    UID uid = INVALID_UID;

    enum { WEIBO_SIGN_IN=1, QQ_SIGN_IN, ACCOUNT_SIGN_IN };
    switch (chan)
    {
        case WEIBO_SIGN_IN:
            uid = account::thirdparty_sign_in("weibo", req);
            break;
        case QQ_SIGN_IN:
            uid = account::thirdparty_sign_in("QQ", req);
            break;
        case ACCOUNT_SIGN_IN: // Account/Phone/Email
            uid = account::_sign_in(req);
            break;
        default:
            MYTHROW(EN_Input_Data,client_error_category);
    }

    if (uid == INVALID_UID)
    {
        MYTHROW(EN_User_NotFound,client_error_category);
    }

    cli = boost::make_shared<Client>(uid);
    // if(WEIBO_SIGN_IN==chan || QQ_SIGN_IN==chan) cli->flags_.is_third_part = 1;

    return json::object(); // login_fwd(cli);
}

boost::signals2::signal<void (client_ptr, bool)> & signals::online()
{
    static boost::signals2::signal<void (client_ptr,bool)> sig;
    return sig;
}

static bool is_key_avail(string const & k, string const & val)
{
    sql::datas datas(format("SELECT 1 FROM users WHERE %1%='%2%'") % k % val);
    return (datas.count() == 0);
}

json::object Client::user_regist(http::request& req, client_ptr& cli)
{
    string nickname = req.param("nickName");
    string user = req.param("account");
    string pwd = req.param("password");
    string gender = req.param("gender", "M");
    string age = req.param("age", "2014-01-01");
    string constellation = req.param("constellation", "摩羯座");

    if (!is_match(user,REG_USER) || is_match(user,REG_USER_GUEST))
    {
        MYTHROW(EN_UserName_Invalid,client_error_category);
    }
    if(!is_match(pwd,REG_PWD)) 
    {
        MYTHROW(EN_Password_Invalid,client_error_category);
    }
    if (!is_key_avail("userName", user))
    {
        MYTHROW(EN_UserName_NotAvail,client_error_category);
    }

    string filename = req.param("fileName","");
    const string & content = req.content();
    string icon_path;
    if (!filename.empty() && !content.empty()){
        filename = uniq_relfilepath(filename, "headicons");
        icon_path = write_file(filename, content);
    }

    UID uid = regist_us(user, pwd, nickname, gender, icon_path, 0, age,constellation);
    cli = boost::make_shared<Client>(uid);
    // cli.reset(new Client( regist_us(user, pwd, nickname, gender, icon_path, 0, sql::db_escape(age),sql::db_escape(constellation))));

#define INSERT_INDIVIDUALDATAS1 ("insert into IndividualDatas(UserId) values(%1%)")
    sql::exec(format(INSERT_INDIVIDUALDATAS1) %cli->uid_);

    return json::object(); // login_fwd(cli);
}

json::object Client::guest_regist(http::request& req, client_ptr& cli)
{
    // string path = req.param("fileName","");

    int uid = alloc_user_id();
    string uid_s = lexical_cast<string>(uid);

    string user = "#" + uid_s;
    string nick = "游客" + uid_s;
    string gender = req.param("gender","");
    if (!gender.empty() && gender != "M" && gender != "F")
        gender = "";
    regist_us(user, "*", nick, gender, "", uid);
    cli = boost::make_shared<Client>( uid );

    return json::object(); // login_fwd(cli);
}

json::object Client::unregist(http::request& req, client_ptr& cli)
{
    if (cli)
    {
        sign_out(req, cli);
        //sql::exec(format(SQL_CHPWD) % "x" % cli->user_id());
        boost::system::error_code ec;
        sql::exec(ec, format("DELETE FROM IndividualDatas WHERE UserId=%1%") % cli->user_id());
        sql::exec(ec, format("DELETE FROM client WHERE UserId=%1%") % cli->user_id());
        sql::exec(ec, format("DELETE FROM users WHERE UserId=%1%") % cli->user_id());
    }
    return json::object();
}

json::object Client::pushNotification(http::request & req, client_ptr& cli)
{
    BOOST_ASSERT_MSG(cli && cli->is_authorized(),"client should authrozied");
    string devicetype = req.param("deviceType");
    string status = req.param("status");

    int opcode = lexical_cast<int>(status);
    int devtype = lexical_cast<int>(devicetype);

    switch (devtype)
    {
        case 1: case 2: break;
        default: MYTHROW(EN_Input_Data,client_error_category);
    }
    switch (opcode)
    {
        case 1: case 2: break;
        default: MYTHROW(EN_Input_Data,client_error_category);
    }

    // cache_type & cc = cli->cache();
    // cache_type::iterator i = cc.find("aps_token");

    if (1==opcode)
    {
        if (!cli->apple_devtoken_.empty())
        {
            set_apple_devtoken(cli, std::string());
            cli->db_save();
        }
    }
    else // if (2==opcode)
    {
        string devicetoken = req.param("deviceToken");
        if (devicetoken.length() != 64)
            MYTHROW(EN_Input_Data,client_error_category);

        if (cli->apple_devtoken_ != devicetoken)
        {
            set_apple_devtoken(cli, devicetoken);
            cli->db_save();
        }
    }

    return json::object();
}

json::object Client::pushSetting(http::request & req, client_ptr& cli)
{
    bool enablePushAudio = lexical_cast<bool>(req.param("enablePushAudio","1"));
    bool enablePushMessage = lexical_cast<bool>(req.param("enablePushMessage","1"));
    cli->cache().put("enablePushAudio", enablePushAudio);
    cli->cache().put("enablePushMessage", enablePushMessage);

    cli->db_save();
    return json::object();
}

json::object Client::app_state(http::request & req, client_ptr& cli)
{
    if (cli)
    {
        cli->flags_.is_app_background = lexical_cast<int>(req.param("bg"));
        // cli->tp_app_background = time(0);
    }
    return json::object();
}

//static bool is_pc_browser(http::params& hdrs)
//{
// Gecko, Webkit, ...
//    const string& ua = hdrs["User-Agent"];
//    return empty(find_first(ua, "iPhone")) && empty(find_first(ua, "Android"));
//}

static string get_client_id(const string& cv)
{
    // regex ex(SESSION_KEY"=([[:xdigit:]]{16})[^[:xdigit:]]?" );
    // smatch what;
    // if (!regex_search(cv, what, e))
    // {
    //     return string();
    // }
    // string val(res[2].first, res[2].second);
    // return what;

    string se;

    string::const_iterator i = cv.begin();
    while (i < cv.end())
    {
        string::const_iterator eop = find(i, cv.end(), ';');
        string::const_iterator j = find(i, eop, '=');
        if (j != eop)
        {
            string tmp(i, j);
            trim(tmp);
            if (tmp == SESSION_KEY)
            {
                se = string(j+1,eop);
                break;
            }
        }

        i = eop;
        if (i != cv.end())
            ++i;
    }

    return se;
}

std::string impack(std::string const& shead, std::string const& sbody)
{
    union {
        uint16_t h[2];
        char s[sizeof(uint16_t) * 2];
    } u;
    u.h[0] = htons(shead.size());
    u.h[1] = htons(sbody.size());

    LOG_I << shead;
    // LOG_I << sbody;

    return string(&u.s[0], sizeof(u)) + shead + sbody;
}

// std::string Client::get_spot_or(std::string const & x) const
// {
//     time_t tpcur = time(0);
//     std::string spotx(spot_);
//     if (spotx.empty())
//     {
//         if (tpcur - tp_.spot_changed < 10)
//             if (!oldspot_.empty())
//                 spotx = oldspot_;
//     }
//     LOG_I << user_id() << " YX-ONLINE=" << spotx;
//     return (spotx.empty() ? x : spotx);
// }

static void set_extra_header(http::response & rsp, client_ptr & cli)
{
    // rsp.header("X-HTTP-YX-ONLINE", cli->get_spot_or("0"));
    rsp.header("X-Date", time_string());

    auto & dynst = get_dynamic_stat(cli->user_id());
    if (dynst.synced_dynshare_id < dynst.last_dynshare_id)
    {
        std::string val = get_head_icon(dynst.last_dynshare_userid);
        if (!val.empty())
            rsp.header("X-DynamicUserIcon", val);
    }
    if (dynst.unread.count > 0)
    {
        rsp.header("X-DynamicComments"
                , boost::lexical_cast<std::string>(dynst.unread.count));
    }
}

static void set_extra_header(json::object & jhd, client_ptr & cli)
{
    // jhd ("X-HTTP-YX-ONLINE", cli->get_spot_or("0"));

    auto & dynst = get_dynamic_stat(cli->user_id());
    if (dynst.synced_dynshare_id < dynst.last_dynshare_id)
    {
        std::string val = get_head_icon(dynst.last_dynshare_userid);
        if (!val.empty())
            jhd.put("X-DynamicUserIcon", val);
    }

    if (dynst.unread.count > 0)
    {
        jhd.put("X-DynamicComments"
                , boost::lexical_cast<std::string>(dynst.unread.count));
    }
}

static std::string impack_head(client_ptr& cli, MSGID seq, std::string const& method)
{
    cli->count_.msg.id = seq;

    json::object jhd;
    jhd ("sequence", UInt(seq))
        ("method", method)
        ;
    set_extra_header(jhd, cli);

cli->pt_.sock.write = microsec_clock::local_time();

    return json::encode( jhd );
}

json::object impack_body_object(const imessage::message & m)
{
    json::object body = m.body;
    if (!m.gid.empty())
    {
        body.put("sid", m.gid);
    }
    if (m.from > 0 && body.find("from") == body.end())
    {
        client_ptr c = xindex::get(m.from);
        if (c)
            body.put("from", c->brief_user_info());
    }
    body.put("time", time_string(m.time()));

    return (body);
}

template <typename Rng>
std::string impack_sbody_list(Rng const& rng)
{
    std::string sbody;
    if (boost::empty(rng))
        return sbody;

    auto it = boost::begin(rng);
    auto end = boost::end(rng);
    sbody += "[";
    sbody += *it;
    for (++it; it != end; ++it)
    {
        sbody += ",";
        sbody += *it;
    }
    sbody += "]";
    return sbody;
}

inline bool is_barhis(std::string const & ty)
{
    return ty.empty();
}

struct normal_mixs : std::vector<std::string>
{
    enum { MAX_MSG_HEAD = 256 };
    bool operator()(imessage::message const& m)
    {
        if (is_barhis(m.type))
            return false;

        std::string s = json::encode(
                json::object()
                    ("sequence", m.id())
                    ("method", m.type)
                    ("body", impack_body_object(m)
                ));
        if (count_siz + s.size() > (0x07fff - MAX_MSG_HEAD - 2) - size())
            return false;

        insert(end(), s);
        count_siz += s.size();
        LOG_I << m;
        return true;
    }

    normal_mixs() { count_siz = 0; }
    UInt count_siz;
};

struct barhis_mixs : std::vector<std::string>
{
    enum { MAX_MSG_HEAD = 256 };
    bool operator()(imessage::message const& m)
    {
        if (m.id() <= last_id || m.time() + SECONDS_PERHOUR < time(0))
            return false;
        std::string s = json::encode(
                json::object()
                    ("sequence", m.id())
                    ("method", m.type)
                    ("body", impack_body_object(m)
                ));
        if (count_siz + s.size() > (0x07fff - MAX_MSG_HEAD - 2) - size())
            return false;

        insert(end(), s);
        count_siz += s.size();
        LOG_I << m;
        return true;
    }

    barhis_mixs(MSGID lid) { count_siz = 0; last_id = lid; }
    UInt count_siz;
    MSGID last_id;
};

// template <typename Iter>
// std::string impack_barhis(MSGID mid, std::string const & gid, Iter it, Iter end/*, excl_uid*/)
// {
//     std::string sbody;
//     std::string shead;
// 
//     MSGID mid = 0;
//     sbody = str(format("{\"sid\":\"%1%\",\"his\":[") % gid);
//     for (; it != end; ++it)
//     {
//         if (it->from == excl_uid)
//             continue;
// 
//         std::string s = json::encode( json::object()
//                 ("method", it->type)
//                 ("sequence", it->id())
//                 ("body", impack_body_object(*it))
//                 );
//         if (sbody.size() + s.size() > 0x07fff - 256)
//             break;
// 
//         sbody += s + ",";
//         mid = it->id();
//         // statistics::inst().message_packed(it->from, mid, it->type);
// 
//         LOG_I << cli <<" "<< *it;
//     }
// 
//     if (mid == 0)
//         return std::string();
//     *sbody.rbegin() = ']';
//     sbody += "}";
//     shead = json::encode( json::object()
//             ("method", "data/barhis")
//             ("sequence", 0) // No-ack required
//             );
// }

std::string impack_single(client_ptr& cli, imessage::message const& msg)
{
    std::string sbody = json::encode( impack_body_object(msg) );
    std::string shead = impack_head(cli, msg.id(), msg.type);
    return impack(shead, sbody);
}

std::string impack_barhis(client_ptr& cli, imessage::message const & tagmsg)
{
    auto const & mlis = chatmgr::inst().chat_group(tagmsg.gid).messages();
    MSGID last_id = cli->get_lastack(tagmsg.gid);

    auto begin = boost::rbegin(mlis);
    auto end = boost::rend(mlis);
    barhis_mixs mixs(last_id);
    std::string sbody = impack_sbody_list( boost::adaptors::reverse(for_self(mixs, begin, end)) );
    if (sbody.empty())
        return std::string();
    std::string shead = impack_head(cli, tagmsg.id(), "data/multipart");
    return impack(shead, sbody);
}

template <typename L>
static std::string impack(client_ptr& cli, L const & l)
{
    BOOST_ASSERT (!boost::empty(l));
    if (boost::empty(l))
        return std::string();

    auto begin = boost::begin(l);
    auto end = boost::end(l);
    LOG_I << cli <<" "<< *begin;

    if (is_barhis(begin->type))
        return impack_barhis(cli, *begin);

    auto last = begin;
    ++last;
    if (last == end || is_barhis(last->type))
        return impack_single(cli, *begin);

    normal_mixs mixs;
    std::string sbody = impack_sbody_list( for_self(mixs, begin, end, &last) );
    if (sbody.empty())
        return std::string();
    --last; // = --end;
    std::string shead = impack_head(cli, last->id(), "data/multipart");
    return impack(shead, sbody);
}

static std::string impack_exc(const imessage::message & m)
{
    string shead = json::encode(
            json::object()
                ("method", m.type)
                ("sequence", m.id()
            ));
    string sbody = json::encode( impack_body_object(m) );
    return impack(shead, sbody);
}

struct Ideq
{
    explicit Ideq(int x) { id_ = x; }
    int id_;
    template <typename T> bool operator()(const T& x) const { return x.id() == id_; }
};

void Client::GetUnpushedMessage()
{
//    RECORD_TIMER(timer_log);
    std::set<std::string> gs;

    std::string k = str(format("user/%1%/message") % uid_);
    auto reply = redis::command("LRANGE", k, 0, -1) ;
    if (!reply || reply->type != REDIS_REPLY_ARRAY)
        return;

    for (unsigned int i = 0; i < reply->elements; i++)
    {
        MSGID mid = lexical_cast<MSGID>(reply->element[i]->str);
        if (messages_.find(mid) != messages_.end())
            continue;

        boost::format fmt2("SELECT SessionId,UserId,MsgType,content,UNIX_TIMESTAMP(MsgTime) FROM messages WHERE id=%1%");
        sql::datas datas2(fmt2 % mid);
        if (sql::datas::row_type row2 = datas2.next())
        {
            std::string sessionid = row2.at(0,"");
            UID userid = lexical_cast<UID>(row2.at(1,"0"));
            std::string msgtype = row2.at(2,"");
            const char *pcontent = row2.at(3);
            time_t mtime = boost::lexical_cast<time_t>(row2.at(4,"0"));
            if (sessionid.empty() || 0==userid || msgtype.empty() || NULL==pcontent) {
                LOG_E << "message error " << mid;
                continue;
            } else if ( "." == sessionid ) {
                sessionid = make_p2pchat_id(userid, uid_);
            }

            if (!boost::ends_with(pcontent, "}"))
            {
                LOG_I << "message error, incompleted " << mid;
                continue;
            }

            imessage::message msg(mid, msgtype, userid, sessionid, mtime);
            msg.body = json::decode(pcontent);

            messages_.insert(messages_.end(), msg);
            gs.insert(msg.gid);
        }
    }

    BOOST_FOREACH(auto const & g , gs)
        clear_message(g, imessage::max_nlimit(g));
}
// {
//     std::set<std::string> gps;
// 
//     boost::format fmt("SELECT MsgId,UserId FROM UnpushedMessages WHERE UserId=%1%");//" ORDER BY MsgId DESC"
//     sql::datas datas1(fmt % uid_);
//     while (sql::datas::row_type row1 = datas1.next())
//     {
//         MSGID mid = lexical_cast<MSGID>(row1.at(0));
//         boost::format fmt2("SELECT SessionId,UserId,MsgType,content,UNIX_TIMESTAMP(MsgTime) FROM messages WHERE id=%1%");
//         sql::datas datas2(fmt2 % mid);
//         if (sql::datas::row_type row2 = datas2.next())
//         {
//             std::string sessionid = row2.at(0,"");
//             UID userid = lexical_cast<UID>(row2.at(1,"0"));
//             std::string msgtype = row2.at(2,"");
//             const char *pcontent = row2.at(3);
//             time_t mtime = boost::lexical_cast<time_t>(row2.at(4,"0"));
//             if (sessionid.empty() || 0==userid || msgtype.empty() || NULL==pcontent)
//             {
//                 LOG_E << "message error " << mid;
//                 continue;
//             }
//             if (!boost::ends_with(pcontent, "}"))
//             {
//                 LOG_I << "message error, incompleted " << mid;
//                 continue;
//             }
// 
//             imessage::message msg(mid, msgtype, userid, sessionid, mtime);
//             msg.body = json::decode(pcontent);
// 
//             cli->messages_.push_back(m); // cli->messages_.insert( m ); messages_.push_front( (msg) );
//             gps.insert(msg.gid);
//         }
//     }
// 
//     BOOST_FOREACH(auto const & g , gps)
//         clear_message(g, imessage::max_nlimit(g));
//     // LOG_I << format("%1% %2% %3%") % n_p2pmsg % n_groupmsg % dels.size();
// }

void Client::pushmsg(client_ptr & cli, const imessage::message & m)
{
    BOOST_ASSERT(cli);
    //    RECORD_TIMER(timer_log);
    bool empty = boost::empty(cli->messages_);
    LOG_I << cli <<" "<< m <<" "<< empty <<" "<< cli->flags_.is_app_background;

    cli->db_save_message(m, imessage::max_nlimit(m.gid));

    if (!cli->is_authorized())
        return;

    if (!cli->writer_) // if (cli->flags_.is_app_background)
    {
        if (!aps_push_disabled_ && !is_barchat(m.gid))
        {
            const string & tok = cli->apple_devtoken_; //cli->cache().get<string>("aps_token","");
            bool enable_pm = cli->cache().get<bool>("enablePushMessage", true);
            if (enable_pm && !tok.empty())
            {
                LOG_I << cli <<" aps token: "<< tok <<" "<< cli->flags_.is_iosrel;
                if (cli->flags_.is_iosrel)
                    apple_push_c::instance()->send(m, tok, cli->user_id());
                else
                    apple_push_c::sandbox()->send(m, tok, cli->user_id());
            }
        }
        return;
    }

    cli->messages_.insert(cli->messages_.end(), m);
    cli->clear_message(m.gid, imessage::max_nlimit(m.gid));

    if (empty)
        write_socket(cli);
}

void Client::pushmsg(UID rcpt, const imessage::message & m)
{
    client_ptr cli = xindex::get(rcpt);
    if (!cli)
    {
        LOG_E << "Not found " << rcpt;
        return;
    }
    Client::pushmsg(cli, m);
}

template <typename I>
int Client::db_delete_message(I beg, I end)
{
    int n = 0;
    // MSGID mid = 0;
    for (auto i = beg; i != end; ++i, ++n)
    {
        // if (is_barchat(i->gid))
        // {
        //     if (imessage::is_usermsg(i->type) || i->type == "chat/gift")
        //     // TODO if is control
        //     {
        //         lastack_[i->gid] = i->id();
        //     }
        // }
        //char k[96];
        //snprintf(k,sizeof(k), "user/%u/chat/%s", uid, i->gid);

        auto & lastack = lastack_[i->gid];
        if (lastack < i->id())
        {
            lastack = i->id();
        }

        while (1)
        {
            std::string k = str(format("user/%1%/message") % uid_);

            redis::context ctx( redis::context::make() );
            ctx.append("LPOP", k)
                // .append("EXPIRE", k, 60*60*24*3)
                ;
            auto reply = ctx.reply();
            if (!reply || reply->type != REDIS_REPLY_STRING)
                break;

            LOG_I << "redis LPOP " << k <<" -> "<< reply->str;
            if (lexical_cast<MSGID>(reply->str) >= i->id())
            {
                break;
            }
        }
    }
    // LOG_I << uid <<" "<< mid;
    return n;
}

void Client::db_save_message(imessage::message const & m, int n_max)
{
    BOOST_ASSERT(user_id());

    // if (is_barchat(m.gid))
    // { }
    // else if (m.id() == 0)
    // { }
    // else
    // {
    //     boost::system::error_code ec;
    //     boost::format INSERT_UnpushedMessages("INSERT INTO UnpushedMessages(UserId,MsgId) VALUES(%1%, %2%)");
    //     sql::exec(ec, INSERT_UnpushedMessages % cli->user_id() % m.id());
    // }

    std::string k = str(format("user/%1%/message") % uid_);
    redis::context ctx( redis::context::make() );
    ctx.append("RPUSH", k, m.id())
        // .append("LTRIM", k, -n_max, -1)
        ;
    ctx.reply();
}

// struct GMIdEqCmp {
//     bool operator()(imessage::message const &m, std::string const& gid) const { return m.gid<gid; }
//     bool operator()(std::string const& gid, imessage::message const &m) const { return gid<m.gid; }
// };

void Client::clear_message(std::string const & gid, int n_max)
{
    time_t tpcur = time(0);

    // check expire
    {
        auto & idxs = boost::get<TagExpire>(messages_);
        auto i = boost::begin(idxs);
        while (i != idxs.end() && i->expire_ < tpcur)
            ++i; // = idxs.erase(i);
        // db_delete_message(boost::begin(idxs), i);
        idxs.erase(boost::begin(idxs), i);
    }

    // check max-limits
    if (!gid.empty())
    {
        auto & idxs = boost::get<TagGMId>(messages_);
        auto rng = idxs.equal_range(boost::make_tuple(gid)); // idxs.equal_range(gid, GMIdEqCmp());
        int n = distance(rng);
        if (n > n_max)
        {
            auto i = boost::begin(rng);
            std::advance(i, n - n_max);
            // db_delete_message(boost::begin(rng), i);
            idxs.erase(boost::begin(rng), i);
        }
    }

    // if (is_barchat(gid))
    // {
    //     int & n = count[gid];
    //     if (n > 20 || mtime + (60*60*1) < time(0))
    //     {
    //         dels.push_back(mid);
    //         continue;
    //     }
    //     ++n;
    // }
    // else if (imessage::is_usermsg(msgtype))
    // {
    //     if (is_groupchat(gid))
    //     {
    //         int & n = count[gid];
    //         if (n > 99 || mtime + (60*60*24*3) < time(0))
    //         {
    //             dels.push_back(mid);
    //             continue;
    //         }
    //         ++n;
    //     }
    //     else if (is_p2pchat(gid))
    //     {
    //         int & n = count[gid];
    //         if (n > 99 || mtime + (60*60*24*7) < time(0))
    //         {
    //             dels.push_back(mid);
    //             continue;
    //         }
    //         ++n;
    //     }
    // }
    //
    // for (std::vector<MSGID>::iterator i = dels.begin(); i != dels.end(); ++i)
    // {
    //     boost::system::error_code ec;
    //     sql::exec(ec, format("DELETE FROM UnpushedMessages WHERE MsgId='%1%' AND UserId=%2%") %*i %uid_);
    // }
}

void Client::write_socket(client_ptr & cli)
{
    if (cli->tp_.expire_ack != std::numeric_limits<time_t>::max()) // writing now
        return;

    if (boost::empty(cli->messages_) || !cli->writer_)
    {
        cli->tp_.expire_ack = std::numeric_limits<time_t>::max();
        return;
    }
    // RECORD_TIMER(timer_log);

    time_t tpcur = time(0);
    while (!boost::empty(cli->messages_))
    {
        std::string buf = impack(cli, cli->messages_);
        if (!buf.empty())
        {
            // cli->pt_.sock.write = microsec_clock::local_time();
            cli->tp_.expire_ack = tpcur + TINV_EXPIRE_ACK;
            cli->count_.msg.write++;
            // auto const & b = boost::begin(cli->messages_);
            // monitor_socket_(cli->native_fd_, time(0), cli->user_id(), b->type, b->id());
            cli->writer_( buf );
            break;
        }
        auto beg = boost::begin(cli->messages_);
        cli->messages_.erase(beg);
    }
}

// static void message_finish(imessage::message const & m, UID touid)
// {
//     boost::system::error_code ec;
//     auto b = bars_mgr::inst().get_bar_byuid(m.from, ec);
//     if (b)
//         b->message_finish(m, touid);
// }

void Client::ack(client_ptr & cli, MSGID mid)
{
    // time_t tpcur = time(0);
    auto & idxs = boost::get<TagMsgId>(cli->messages_);
    auto const begin = boost::begin(idxs);
    auto it = idxs.find(mid);
    int n = 0;
    if (it != boost::end(idxs))
    {
        bool is_bh = is_barhis(it->type);
        ++it;
        if (!is_bh)
        {
            n = cli->db_delete_message(begin, it);
            redis::command("HSET", HKEY_USER_MESSAGE_LAST, cli->user_id(), mid);
        }

        LOG_I << "erase N " << n;
        idxs.erase(begin, it);

        cli->tp_.expire_ack = std::numeric_limits<time_t>::max();
        ptime pt = microsec_clock::local_time();
        record_msgtime(
                cli->user_id()
                , cli->count_.msg.id, mid
                , 0
                , (pt - cli->pt_.sock.write).total_milliseconds()
                , cli->endpoint_r
                , int(cli->flags_.devtype)
                , cli->flags_.is_app_background
                , cli->pt_.sock.write.time_of_day()
                );
        zeros(cli->count_.msg);
    }

    // int y = 0;
    //if (beg2 != boost::end(cli->messages_))
    //{
        // for (auto it = boost::begin(cli->messages_); it != beg2; ++it, ++y)
        // {
        //     // message_finish(*it, cli->user_id());
        //     LOG_I << cli << " ack "<< it->id() <<" "<< it->type;
        //     if (is_barchat(it->gid))
        //     {
        //         if (imessage::is_usermsg(it->type) || it->type == "chat/gift")
        //             cli->lastack_[it->gid] = it->id();
        //     }
        //     else
        //     {
        //     }
        // }
    //}

    LOG_I << cli <<" ack "<< mid <<" "<< time(0);
}

static imessage::message warn_login_other(const std::string & cid)
{
    imessage::message msg(0, "warn/other-login", SYSADMIN_UID, ".1000.0",time(0));
    msg.body.put("content", cid);
    LOG_I << msg.type <<" "<< cid;
    return msg;
}

void Client::close_socket(client_ptr & cli)
{
    LOG_W << cli << " close " << cli->native_fd_ <<" "<< time(0);
    if (cli->writer_)
    {
        if (cli->is_authorized())
        {
            if (cli->flags_.imsg_login_other)
            {
                cli->flags_.imsg_login_other = 0;
                imessage::message msg = warn_login_other(cli->client_id());
                // monitor_socket_(cli->native_fd_, time(0), cli->user_id(), msg.type, msg.id());
                cli->writer_(impack_exc(msg));
            }
            // cli->uptime(1); // cli->db_save();
        }
        cli->connection_notify(0);

        // monitor_socket_(cli->native_fd_, time(0), cli->user_id(), "local-close");
        cli->writer_(std::string());
        cli->writer_ = boost::function<void (const std::string&)>();
        LOG_I << cli; // << " use " << soc.use_count();
        // cli->endpoint_r = ip::tcp::endpoint();
    }

    if (cli->native_fd_ > 0)
    {
        int x = cli->native_fd_;
        cli->native_fd_ = 0;
        xindex::set(index_key<int,socket_ptr>(x), client_ptr());
    }

    cli->tp_.expire_hello = cli->tp_.expire_ack = std::numeric_limits<time_t>::max();
}

void Client::db_save(bool logout)
{
//    RECORD_TIMER(timer_log);
    uptime();
    sql::exec(format("REPLACE INTO client(UserId,Token,mac,cache,AppleDevToken)"
                " VALUES(%1%,'%2%','%3%','%4%','%5%')")
            % user_id()
            % (logout ? string():client_id())
            % macaddr_
            % sql::db_escape(json::encode(cache_))
            % apple_devtoken_);
}

static char const* const MsgType_Chatroom = "";
void Client::barhis(std::string const & gid, chat_group const & cg)
{
    imessage::message msg(MsgType_Chatroom, 0, cg.id());
    messages_.insert(messages_.end(), msg);

    //MSGID last_id = lastack_[gid];
    //time_t tpcur = time(0);

    //auto const & msgs = cg.messages();
    //for (auto it = msgs.begin(); it != msgs.end(); ++it)
    //{
    //    LOG_I << *it <<" "<< (it->time() + SECONDS_PERHOUR < tpcur);
    //    if (it->id() <= last_id || it->time() + SECONDS_PERHOUR < tpcur)
    //        continue;
    //    messages_.insert(messages_.end(), *it);
    //}
    //LOG_I << gid <<" "<< last_id;
}

static int get_today(time_t tp)
{
    if (tp == 0)
        tp = time(0);

    struct tm tm;
    localtime_r(&tp, &tm);
    return tm.tm_mday;
}

struct daily_visitor : std::map<UID, std::set<UID> >
{
    daily_visitor()
    {
        mday_ = get_today(0);
    }

    bool welcome(UID baruid, UID uid)
    {
        if (upday(mday_))
            reset_this_day();

        std::string k = str(boost::format("spot/%1%/day/%2%") % baruid % mday_);
        iterator it;
        {
            auto p = insert(std::make_pair(baruid, std::set<UID>()));
            if (p.second)
            {
                auto reply = redis::command("SMEMBERS", k);
                if (reply && reply->type == REDIS_REPLY_ARRAY)
                {
                    for (UInt i = 0; i < reply->elements; i++)
                    {
                        UInt uid = lexical_cast<UInt>(reply->element[i]->str);
                        p.first->second.insert(uid);
                    }
                }
            }
            it = p.first;
        }

        auto p = it->second.insert(uid);
        if (p.second)
            redis::command("SADD", k, uid);
        return (p.second);
    }

private:
    void reset_this_day()
    {
        clear();

        std::string kglob = str(boost::format("spot/*/day/%1%") % mday_);
        std::vector<std::string> keys;
        redis::keys(kglob, std::back_inserter(keys));
        BOOST_FOREACH(auto const & k, keys)
            redis::command("DEL", k);
    }

    static bool upday(int & mday)
    {
        int cd = get_today(0);
        LOG_I << mday <<" "<< cd << " welcome";
        if (mday == cd)
            return false;
        mday = cd;
        return true;
    }

    //auto reply = redis::command("KEYS", "spot/*/day/*");
    //if (reply && reply->type == REDIS_REPLY_ARRAY)
    //{
    //    for (UInt i = 0; i < reply->elements; i++)
    //    {
    //        redis::context ctx( redis::context::make() );
    //        ctx.append("DEL", reply->element[i]);
    //        ctx.reply();
    //    }
    //}

    int mday_;
};

static void welcome(client_ptr cli, bar_ptr pbar)
{
    static daily_visitor daily_visited_;

    if (daily_visited_.welcome(pbar->trader_id, cli->user_id()))
    {
        LOG_I << cli << " welcome";
        imessage::message msg("chat/text", pbar->trader_id, pbar->sessionid);
        msg.body("content", string("欢迎来到") + pbar->barname);
        sendto(msg, cli);
    }

    // imessage::message msg("chat/text", SYSADMIN_UID, make_p2pchat_id(cli->user_id(), SYSADMIN_UID));
    // if(cli->is_guest()){
    //     const string welcome2 = "欢迎来到月下，您当前使用的是游客模式，请登录后获取更好的体验。";
    //     msg.body ("content", welcome2) ;
    // } else {
    //     const string welcome1 = "欢迎来到月下，如果您在使用中有任何的问题或建议，记得给我反馈哦；或者在系统设置中发送意见反馈给我们。";
    //     msg.body ("content", welcome1) ;
    // }
    // sendto(msg, cli);
}

void Client::check_join_chat(client_ptr & cli, const string & roomsid)
{
    LOG_I << roomsid <<"%"<< cli->roomsid_;
    if (roomsid == cli->roomsid_)
        return;
    boost::system::error_code ec;

    // old ui gid - leave
    if (!cli->roomsid_.empty())
    {
        auto b = bars_mgr::inst().get_session_bar(cli->roomsid_, &ec);
        if (b)
            b->leave_chat(cli);
        cli->roomsid_.clear();
    }

    // new ui gid - enter
    if (!roomsid.empty() && is_barchat(roomsid))
    {
        // if (roomsid == cli->spot_) {}
        auto pbar = bars_mgr::inst().get_session_bar(roomsid, &ec);
        if (pbar)
        {
            cli->roomsid_ = roomsid;
            chat_group* cg = pbar->join_chat(cli);
            if (cg)
            {
                cli->barhis(roomsid, *cg);
                write_socket(cli);
            }
        }
    }
}

struct ipaddress_spot_info
{
    ip::address_v4 ipa;
    UID uid; // std::string spot;
    time_t tp_live;

    explicit ipaddress_spot_info(UID u, ip::address_v4 const& a)
        : ipa(a), uid(u)
    { tp_live = time(0); }

    friend std::ostream& operator<<(std::ostream& out, ipaddress_spot_info const & si)
    {
        return out << si.ipa <<" "<< si.uid <<" "<< time_format<64>("%T", si.tp_live);
    }
};

typedef boost::multi_index_container<
    ipaddress_spot_info,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::ordered_unique<
            BOOST_MULTI_INDEX_MEMBER(ipaddress_spot_info,ip::address_v4,ipa)>
    >
> ipaddr_spot_idx_t;

typedef boost::multi_index_container<
    ipaddress_spot_info,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            BOOST_MULTI_INDEX_MEMBER(ipaddress_spot_info,ip::address_v4,ipa)>,
        boost::multi_index::ordered_non_unique<
            BOOST_MULTI_INDEX_MEMBER(ipaddress_spot_info,UID,uid)>
    >
> ipaddr_spot_expires_t;

static ipaddr_spot_idx_t ipaddr_spot_idx_;
static ipaddr_spot_expires_t ipaddr_spot_expires_;

static void release_ipa(std::string const& a)
{
    auto ipa = ip::address_v4::from_string(a);
    ipaddr_spot_expires_.erase(ipa);
}

static void check_expires()
{
    ipaddr_spot_idx_t& ipx = ipaddr_spot_idx_;
    ipaddr_spot_expires_t& expires = ipaddr_spot_expires_;

    time_t tpcur = time(0);
    while (!ipx.empty())
    {
        auto beg = ipx.begin();
        if (beg->tp_live + 60*5 > tpcur)
            break;
        LOG_I << *beg << " expired";

        {
            std::string val = str(format("%1% %2%") % beg->uid % beg->tp_live);
            redis::command("HSET", "spot/ips/dead", beg->ipa, val);
        }

        ipaddress_spot_info spx = *beg;
        ipx.erase(beg);

        expires.erase(spx.ipa);
        {
            auto & uIdx = boost::get<1>(expires);
            auto p = uIdx.equal_range(spx.uid);
            // auto insp = p.second;

            if (p.first != p.second)
                --p.second;

            std::string k = str(boost::format("spot/%1%/ips") % spx.uid);
            for (auto i = p.first; i != p.second; ++i)
                redis::command("SREM", k, i->ipa);

            uIdx.erase(p.first, p.second);
            uIdx.insert( uIdx.upper_bound(spx.uid), spx ); //(insp, spx);
        }
    }
}

static void release_expires(ip::address_v4 ipa)
{
    ipaddr_spot_expires_t& expires = ipaddr_spot_expires_;
    expires.erase(ipa);
}

void load_ipaddr_spot_binds()
{
    boost::system::error_code ec;

    std::vector<std::string> keys;
    redis::keys("spot/*/ips", std::back_inserter(keys));

    BOOST_FOREACH(auto const & k, keys)
    {
        UID uid = lexical_cast<UID>(std::string(k.begin() + 5, k.end() - 4));
        bar_ptr b = bars_mgr::inst().get_bar_byuid(uid, ec);
        if (!b)
        {
            redis::command("DEL", k);
            continue;
        }

        auto reply = redis::command("SMEMBERS", k);
        if (reply && reply->type == REDIS_REPLY_ARRAY)
        {
            for (unsigned int i = 0; i < reply->elements; i++)
            {
                auto a = ip::address_v4::from_string(reply->element[i]->str);
                ipaddr_spot_idx_.push_back( ipaddress_spot_info(uid, a) );
            }
        }
    }
}

static std::string _get_gwid(http::request & req, client_ptr & cli)
{
    std::string gwid = req.param("gwid","");
    if ( gwid.empty() && cli ) {
        string user = cli->user_info().get<string>("user");
        if ( boost::ends_with(user, "_assist") ){
            LOG_I << user;
            boost::erase_last(user,"_assist");
            sql::datas datas(format("SELECT SessionId FROM bars WHERE trader='%1%'") % user);
            if ( sql::datas::row_type row = datas.next() ) {
                gwid = row.at(0,"");
            }
        }
    }
    return gwid;
}

static json::object gwNotification(http::request & req, ip::address_v4 ipa, client_ptr & cli)
{
    boost::system::error_code ec;

    std::string gwid = _get_gwid(req, cli);
    if (gwid.empty())
        return json::object();

    bar_ptr bptr = bars_mgr::inst().get_session_bar(gwid, &ec);
    if (!bptr)
        return json::object();

    UID uid = bptr->trader_id;
    {
        auto & idxs = boost::get<1>(ipaddr_spot_idx_);
        auto it = idxs.find(ipa);
        if (it != idxs.end())
            idxs.erase(it);
        else
            redis::command("SADD", str(format("spot/%1%/ips") % uid), ipa);
    }

    // live again
    ipaddr_spot_idx_.push_back( ipaddress_spot_info(uid, ipa) );

    release_expires(ipa);
    check_expires(); //(ipaddr_spot_idx_, ipaddr_spot_expires_);

    return json::object();
}

static bar_ptr get_bar_byip(ip::address_v4 ipa)
{
    boost::system::error_code ec;
    bar_ptr pbar;

    auto & idxs = boost::get<1>(ipaddr_spot_idx_);
    auto it = idxs.find(ipa);
    if (it == idxs.end())
    {
        auto it2 = ipaddr_spot_expires_.find(ipa);
        if (it2 == ipaddr_spot_expires_.end())
            return bar_ptr();

        LOG_I << *it2 <<" reuse expired";
        if ( !(pbar = bars_mgr::inst().get_bar_byuid(it2->uid, ec)))
        {
            redis::command("DEL", str(format("spot/%1%/ips") % it2->uid));
            ipaddr_spot_expires_.erase(it2);
        }
    }
    else
    {
        LOG_I << *it;
        if ( !(pbar = bars_mgr::inst().get_bar_byuid(it->uid, ec)))
        {
            redis::command("DEL", str(format("spot/%1%/ips") % it->uid));
            idxs.erase(it);
        }
    }

    return pbar;
}

void Client::spot_main_entry(client_ptr& cli, bar_ptr pbar)
{
    boost::system::error_code ec;
    UID uid = cli->user_id();
    std::string const& bsid = (pbar ? pbar->sessionid : std::string());

    LOG_I << uid <<" "<< cli->spot_ <<" "<< bsid;
    if (cli->spot_ == bsid)
        return;

    cli->tp_.spot_changed = time(0);

    if (!cli->spot_.empty())
    {
        if (auto b = bars_mgr::inst().get_session_bar(cli->spot_, &ec))
        {
            //if (!cli->roomsid_.empty())
            //    b->leave_chat(cli, 1);
            b->leave_spot(cli);
            cli->oldspot_ = cli->spot_;
        }
        cli->roomsid_.clear();
        cli->spot_.clear();
    }

    if (pbar)
    {
        cli->spot_ = bsid;
        pbar->enter_spot(cli);
        welcome(cli, pbar);
        cli->tp_.mylocation = time(0);
    }
}

static void reply_close(socket_ptr & soc, const clientid_type& cid, const imessage::message & msg)
{
    send_fn_type fn = assoc_g_(soc);
    if (fn)
    {
        fn( impack_exc(msg) );
    }
}

inline uint32_t ipv4_ul(ip::tcp::endpoint const & ep)
{
    return ep.address().to_v4().to_ulong();
}

static ip::address_v4 get_ipav4(socket_ptr const & soc, std::string const & x_real_ip)
{
    if (!x_real_ip.empty())
    {
        LOG_I << x_real_ip;
        return ip::address_v4::from_string(x_real_ip);
        // return a.to_v4().to_ulong();
    }
    return (soc->remote_endpoint().address().to_v4());
}

void Client::check_deadline(const boost::system::error_code & ec, client_ptr cli)
{
    try
    {
        if (ec)
        {
            LOG_I << ec <<" "<< ec.message() <<" "<< cli.use_count();
            return;
        }
        if (/*!cli->writer_ ||*/ !cli->timer_)
        {
            LOG_I << cli <<" "<< bool(cli->timer_) <<" "<< bool(cli->writer_);
            return;
        }
        time_t tpcur = time(0);

        //LOG_I << "===timed=" << cli
            // <<" "<< tpcur <<" "<< cli->tp_.active
            // <<" "<< cli->tp_.expire_ack <<"/"<< cli->tp_.expire_hello
            ;
        if (cli->native_fd_ > 0)
        {
            if (!cli->flags_.is_app_background)
            {
                if (cli->tp_.expire_ack < tpcur)
                {
                    if (boost::empty(cli->messages_))
                    {
                        cli->tp_.expire_ack = std::numeric_limits<time_t>::max();
                        zeros(cli->count_.msg);
                    }
                    else
                    {
                        LOG_W << cli << " tp expire ack " << cli->count_.msg.id <<" "<< cli->tp_.expire_ack;
                        close_socket(cli);

                        ptime pt = microsec_clock::local_time();
                        record_msgtime(
                                cli->user_id()
                                , cli->count_.msg.id, 0
                                , 1
                                , (pt - cli->pt_.sock.write).total_milliseconds()
                                , cli->endpoint_r
                                , int(cli->flags_.devtype)
                                , cli->flags_.is_app_background
                                , cli->pt_.sock.write.time_of_day()
                                );
                        zeros(cli->count_.msg);
                    }
                }
                else if (cli->tp_.expire_hello < tpcur)
                {
                    LOG_W << cli << " tp expire hello " << cli->tp_.expire_hello;
                    close_socket(cli);
                }
            }
        }
        //else if (0)//(cli->tp_.active + (cli->flags_.is_app_background ? 60*10+10 : 60*3) < tpcur)
        //{
        //    if (!cli->spot_.empty())
        //    {
        //        LOG_I << cli << " tp expire spot: " << cli->spot_;
        //        spot_main_entry(cli, bar_ptr());
        //    }
        //    // if (cli->tp_.active + 60*3 < tpcur)
        //    // {
        //    //     // TODO, rethink
        //    //     // if (!cli->client_id().empty())
        //    //     //     xindex::set(cli->client_id(), client_ptr());
        //    //     // if (cli->user_id() > 0)
        //    //     //     xindex::set(cli->user_id(), client_ptr());
        //    //     return;
        //    // }
        //}
        
        //if (!cli->spot_.empty())
        //{
        //    if (tpcur - cli->tp_.mylocation > SECONDS_PERHOUR*3)
        //    {
        //        LOG_I << cli << " lbs spot tp expire: " << cli->spot_;
        //        spot_main_entry(cli, bar_ptr());
        //    }
        //}

        time_t tpdate = tpcur / (24*60*60);
        if (cli->tp_.today == 0)
            cli->tp_.today = tpdate;
        if (cli->tp_.today != tpdate)
        {
            cli->tp_.today = tpdate;
            cli->lucky_shake_count = 3;
            redis::command("HSET", HKEY_USER_LUCK_SHAKE_COUNT_LEFT, cli->user_id(), cli->lucky_shake_count);
            cli->whisper_love_count = 3;
            cli->pic_charm_count = 10;
        }

        cli->timer_->expires_from_now(boost::posix_time::seconds(7));
        cli->timer_->async_wait(boost::bind(&Client::check_deadline, asio::placeholders::error, cli));

        monit_summary_(cli->user_id()
                , cli->user_nick()
                , cli->spot_
                , cli->flags_.is_app_background
                , cli->native_fd_ > 0
                , str(format("%1%/%2%") % cli->count_.connect.fg % cli->count_.connect.bg)
                , cli->endpoint_r
                , cli->pt_.sock.open.time_of_day()
                , cli->pt_.sock.read.time_of_day()
                , cli->pt_.sock.write.time_of_day()
                , time_format<64>("%T", cli->tp_.active)
                , cli->apple_devtoken_.substr(0,16) + "..."
                );
    }
    catch (std::exception const & e)
    {
        LOG_E << "Ex " << e.what();
    }
}

client_ptr Client::socket(socket_ptr soc, json::object & head, json::object & body)
{
    time_t tpcur = time(0);
    client_ptr cli;
    try
    {
        cli = xindex::get(index_by(soc),1);
        if (head.empty())
        {
            if (cli)
            {
                cli->closed_by_ = Closed_By_Remote;
                if (cli->count_.msg.id)
                {
                    ptime pt = microsec_clock::local_time();
                    record_msgtime(
                            cli->user_id()
                            , cli->count_.msg.id, 0
                            , 2
                            , (pt - cli->pt_.sock.write).total_milliseconds()
                            , cli->endpoint_r
                            , int(cli->flags_.devtype)
                            , cli->flags_.is_app_background
                            , cli->pt_.sock.write.time_of_day()
                            );
                    zeros(cli->count_.msg);
                }
            }
            goto Pos_close_;
        }

        clientid_type cid = head.get<string>("token","");
        string meth = head.get<string>("method", "");
        auto_cpu_timer_helper x_time(meth);

        LOG_I << cli <<" "<< soc <<" "<< meth <<" cid="<< cid;

        if (cid.empty() || meth.empty())
            goto Pos_close_;
        if (meth != "ack" && meth != "hello")
            goto Pos_close_;

        if (cli)
        {
            if (cid != cli->client_id())
            {
                LOG_W << cli <<" TOKEN " << cid <<" invalid "<< soc;
                goto Pos_close_;
            }
        }
        else if ( (cli = xindex::get(cid)))
        {
            if (cli->native_fd_ > 0)
            {
                LOG_E << cli << " MULTI-SOCKET " << soc;
                close_socket(cli); // goto Pos_close_;
            }
            cli->native_fd_ = 0;
            if (!cli->is_authorized() || meth != "hello")
            {
                LOG_E << cli <<" invalid "<< meth;
                goto Pos_close_;
            }
        }
        else
        {
            LOG_W << cli <<" TOKEN " << cid <<" invalid "<< soc;
            reply_close(soc, cid, warn_login_other(cid));
            goto Pos_close_;
        }
        cli->tp_.active = tpcur;
        cli->pt_.sock.read = second_clock::local_time();

        if (meth == "hello")
        {
            {
                auto i = body.find("bg");
                if (i != body.end())
                    cli->flags_.is_app_background = boost::get<int>(i->second);
                cli->flags_.devtype = body.get<int>("deviceType",0);
            }

            if (cli->native_fd_ == 0) // first hello
            {
                cli->native_fd_ = soc->native_handle();
                if (cli->native_fd_ <= 0)
                {
                    LOG_E << cli <<" SOCK "<< cli->native_fd_ <<" "<< soc;
                    goto Pos_close_;
                }
                cli->writer_ = assoc_g_(soc);
                cli->endpoint_r = soc->remote_endpoint();
                xindex::set(index_by(soc), cli);

                // int devtype = body.get<int>("deviceType",0); // (devtype DEVTYPE_WEB)
                // (, devtype==DEVTYPE_WEB ? std::string() : spotsid);
                // cli->messages_.clear();

                if (!cli->timer_)
                {
                    cli->tp_.expire_hello = cli->tp_.expire_ack = std::numeric_limits<time_t>::max();
                    cli->timer_ = boost::make_shared<boost::asio::deadline_timer>(boost::ref(soc->get_io_service()));
                    check_deadline(boost::system::error_code(), cli);
                }
                cli->GetUnpushedMessage();

                // auto ipv4 = get_ipav4(soc,head.get<std::string>("X-Forwarded-For",""));
                // spot_main_entry(cli, get_bar_byip(ipv4));

                // signals::online()(cli, head, body, soc);
                if (!init_wan_ipaddr_)
                {
                    auto const & a = soc->local_endpoint().address();
                    if (!boost::asio::ip::is_private(a))
                        init_wan_ipaddr_ = 1;
                    wan_ipaddress_ = a;
                }

                cli->connection_notify(1);
            }
            // monitor_socket_(soc, time(0), cli->user_id(), meth, 0);

            if (boost::empty(cli->messages_))
            {
                if ( !public_messages.empty() 
                        && public_messages.rbegin()->id() > cli->pub_msg_id)
                {
                    for ( auto it = public_messages.begin(); it!=public_messages.end(); ++it) {
                        if ( it->id() > cli->pub_msg_id ){
                            cli->pub_msg_id = it->id();
                            redis::command("HSET", HKEY_USER_GLOBAL_MESSAGE_LAST, cli->user_id(), it->id());
                            imessage::message msg(it->type, it->from, make_p2pchat_id(it->from, cli->user_id()));
                            //(0, it->type, it->from, make_p2pchat_id(it->from, cli->user_id()), it->time());
                            msg.body = it->body;
                            cli->messages_.insert(cli->messages_.end(), msg);
                            break;
                        }
                    }
                }
            }

            if (boost::empty(cli->messages_))
            {
                imessage::message msg(0, "noop", 0, "None", tpcur);
                msg.body("content", string("..."));
                cli->writer_( impack_single(cli, msg) );
            }
            else
            {
                write_socket(cli);
            }
        }
        else if (meth == "ack")
        {
            UInt seq = head.get<int>("sequence");
            // monitor_socket_(soc, time(0), cli->user_id(), meth, seq);
            ack(cli, seq);
            write_socket(cli);
        }

        cli->tp_.expire_hello = cli->tp_.active + (cli->flags_.is_app_background ? 60*20+10 : 200);
        // LOG_I << cli <<" "<< soc <<" "<< cli.use_count();
        return cli;
    }
    catch (my_exception const & e)
    {
        LOG_E << soc << " Ex: " << diagnostic_information(e);
    }
    catch (std::exception const & e)
    {
        LOG_E << soc << " Ex: " << e.what();
    }
    catch (...)
    {
    }

Pos_close_:
    LOG_E << cli <<" close "<< soc;
    if (cli)
        close_socket(cli);

    return client_ptr();
}

static client_ptr download_file(client_ptr& cli, http::response & rsp, http::request & req)
{
    if (cli)
        LOG_I << cli <<" "<< req.path();
    else
        LOG_I << req;

    rsp.content( get_file(string(req.path().begin()+5,req.path().end()), true) );

    return cli;
}

static std::string try_get_macaddr(http::request & req)
{
    std::string mac = req.header("mac","");
    if (mac.empty())
    {
        mac = req.param("macAddress","");
        if (!mac.empty())
        {
            if (boost::ends_with(mac, ":00:00:00:00:00"))
                return std::string();
            boost::to_lower(mac);
        }
    }
    return mac;
}

//version 1.1

json::object Client::confirmValidCode(http::request& req, client_ptr& cli)
{
    std::string phone = req.param("phone","");
    std::string validCode = req.param("validCode","");
    if(!phone.empty() && !validCode.empty()){
        json::object au = cli->temps_.get<json::object>("auth", json::object());
        if(!au.empty() && au.get<unsigned int>("dtl") > time(NULL)){
            if (au.get<string>("phone") == phone)
            {
                Premove_code( phone, ValidCode::Usage::PASSWORD);
                if (validCode == au.get<string>("code"))
                {
                    goto ok_pos;
                }
            }
        }
    }
    MYTHROW(EN_AuthCode_Invalid,client_error_category);

ok_pos:
    return json::object();
}

json::object Client::otherLogin(http::request& req, client_ptr& cli)
{
    return  sign_in(req, cli);
}

json::object Client::firstLoginToOtherPlatform(http::request & req, client_ptr& cli)
{
    int channel = lexical_cast<int>(req.param("channel","0"));
    string openid = req.param("openId","");
    if(openid.empty()) THROW_EX(EN_Input_Data);

    string table;
    enum { WEIBO_SIGN_IN=1, QQ_SIGN_IN };
    switch (channel)
    {
        case WEIBO_SIGN_IN:
            table="weiboUsers";
            break;
        case QQ_SIGN_IN:
            table="QQUsers";
            break;
        default:
            MYTHROW(EN_Input_Data,client_error_category);
            break;
    }

    sql::datas datas(format("SELECT userId FROM %1% WHERE id='%2%'") % table % openid);
    if (sql::datas::row_type row = datas.next())
    {
        return json::object()("flag", false);
    }
    return json::object()("flag", true);
}

json::object Client::phoneRegist(http::request& req, client_ptr& cli)
{
    string phone = req.param("phone");
    string pwd = req.param("password");
    string nick = req.param("nickName");
    string gender = req.param("gender", "M");
    string age = req.param("age", "2014-01-01");
    string signature = req.param("sign", "");
    string constellation = req.param("constellation", "摩羯座");

    if (!is_match(phone,REG_MOBILE_PHONE))
    {
        MYTHROW(EN_Phone_Incorrect,client_error_category);
    }
    if (!is_key_avail("UserPhone", phone))
    {
        MYTHROW(EN_Phone_Bind_Already,client_error_category);
    }
    if(!is_match(pwd,REG_PWD)) 
    {
        MYTHROW(EN_Password_Invalid,client_error_category);
    }
    if(!is_match(nick,REG_NICK)) 
    {
        MYTHROW(EN_NickName_Invalid,client_error_category);
    }

    string filename = req.param("fileName","");
    const string & content = req.content();
    string icon_path;
    if(!filename.empty() && !content.empty()){
        filename = uniq_relfilepath(filename, "headicons");
        icon_path = write_file(filename, content);
    }
    
const char* INSERT_PHONE_USERS =  "INSERT INTO users(userid,UserPhone,password,"
    " nick,sex,icon,age,constellation,UserName,signature)"
    " VALUES(%1%,'%2%','%3%','%4%','%5%','%6%','%7%','%8%','?%1%','%9%')";

    UID uid = alloc_user_id();

    sql::exec(format(INSERT_PHONE_USERS) % uid %sql::db_escape(phone) %sql::db_escape(pwd) 
            %sql::db_escape(nick) %sql::db_escape(gender) %sql::db_escape(icon_path) 
            %sql::db_escape(age) %sql::db_escape(constellation) %sql::db_escape(signature));

#define INSERT_INDIVIDUALDATAS1 ("insert into IndividualDatas(UserId) values(%1%)")
    sql::exec(format(INSERT_INDIVIDUALDATAS1) %uid);

    cli = boost::make_shared<Client>(uid);

    return json::object(); // login_fwd(cli);
}

/// service_fn
//
// return:
//   - client
//   - service_fn_type
service_fn_type account::service_fn(http::request & req, client_ptr& client)
{
//    RECORD_TIMER(timer_log);
    std::map<std::string, service_fn_type>::iterator it = urlmap_.find(req.path());
    if (it == urlmap_.end())
    {
        return service_fn_;
    }

    if (it->second == &Client::sign_in
            || it->second == &Client::otherLogin
            || it->second == &Client::user_regist
            || it->second == &Client::phoneRegist
            || it->second == &Client::guest_regist)
    {
        if (it->second == &Client::phoneRegist)
        {
            if (!client)
                MYTHROW(EN_Unauthorized,client_error_category);
//phoneRegist
//password=fc1fa8c923ffce8736f86c1c4e038a21
//age=1986-4-12
//constellation=摩羯座
//version=2.1.1
//deviceToken=6c392cb3bf8f69b4c106599863d999d72db3a03aba19d7d108ea21f3efbfeb96
//fileName=image20140516155532.png
//bundleID=com.kkli.OnMoon
//gender=F
//phone=18688874715
//nickName=Justin
//deviceType=1
//validCode=7390
            string phone = req.param("phone","");
            json::object au = client->temps_.get<json::object>("auth", json::object());
            string validCode = req.param("validCode","");
            if ( !phone.empty() && au.get<string>("phone") != phone)
            {
                if (validCode != au.get<string>("code"))
                {
                    MYTHROW(EN_AuthCode_Invalid,client_error_category);
                }
                MYTHROW(EN_Phone_Invalid,client_error_category);
            }
            // client->temps_.erase("auth");
        }

        if (client)
        {
            // if (client->is_authorized())
            // {
            //     if (client->user_id() == uid)
            //     {
            //         LOG_I << client << " relogin self " << req.path();
            //         goto Pos_fwd_;
            //     }
            // }
            LOG_I << client << " @sign_out token";
            Client::record_sign_out( client, true);
            Client::sign_out(req, client);
            client.reset();
        }

        client_ptr cli;

        it->second(req, cli);
        BOOST_ASSERT(cli);
        UID uid = cli->user_id();

        if (it->second == &Client::sign_in
                || it->second == &Client::otherLogin)
        {
            if (client_ptr c = xindex::get(uid))
            {
                if (c->is_authorized())
                {
                    c->flags_.imsg_login_other = 1;
                }
                LOG_I << c << " @sign_out user-id";
                Client::record_sign_out( client, true);
                Client::sign_out(req, c);
            }
        }

        {
            string apstok;
            apstok = req.param("deviceToken", apstok);
            if (apstok.length() == 64)
            {
                if (client_ptr c = xindex::get(index_by_aps_tok(apstok)))
                {
                    LOG_I << c << " @sign_out ios-device-token " << apstok;
                    Client::record_sign_out( client, true);
                    Client::sign_out(req, c);
                }

                Client::set_apple_devtoken(cli, apstok); // cli->cache().put("aps_token", apstok);

                string bundle = req.param("bundleID","com.kkli.OnMoon");
                cli->flags_.is_iosrel = (bundle == "com.kkli.OnMoon");
                cli->cache().put("isrel", bool(cli->flags_.is_iosrel));
            }
        }
        cli->macaddr_ = try_get_macaddr(req);
        cli->db_save();

        cli.reset();
        client = xindex::get( uid );
        BOOST_ASSERT(client && client->is_authorized());

        if (lexical_cast<int>(req.param("deviceType")) == DEVTYPE_WEB)
        {
            boost::system::error_code ec;
            if (bar_ptr pbar = bars_mgr::inst().get_bar_byuid(uid, ec))
                client->user_info().put("public_flag", 1);
        }

        // if (it->second == &Client::user_regist
        //         || it->second == &Client::phoneRegist
        //         || (!uid_p && it->second == &Client::guest_regist))
        //     welcome(client);

        // token_client.insert(make_pair(apstok, client));

//Pos_fwd_:
        it = urlmap_.find("login-fwd");
        BOOST_ASSERT(it != urlmap_.end());
        LOG_I << client << " login/regist";
    }
    else if (it->second == &Client::getauth 
            || it->second == &Client::resetpwd
            || it->second == &Client::firstLoginToOtherPlatform)
    {
        if (!client)
            client = boost::make_shared<Client>(0);
    }
    else if (it->second == &bills
            || it->second == &Client::bindaccount
            || it->second == &Client::chpwd
            || it->second == &Client::pushNotification )
    {
        if (!client || !client->is_authorized())
            MYTHROW(EN_Unauthorized,client_error_category);
    } else if ( it->second == &Client::sign_out ) {
        Client::record_sign_out( client, false);
    }

    return it->second;
}

json::object forceOutSpot(http::request& req)
{
    UID otherid = lexical_cast<int>(req.param("userid"));
    client_ptr other = xindex::get(otherid);

    Client::spot_main_entry(other, bar_ptr());

    return json::object();
}

client_ptr Client::http(socket_ptr & soc, http::request& req, http::response& rsp)
{
    auto_cpu_timer_helper x_time(req.path());
// RECORD_TIMER(timer_log);
    boost::system::error_code error_code;
    client_ptr cli;
    try
    {
        clientid_type cid = get_client_id(req.header("Cookie", ""));
        if (!cid.empty())
        {
            cli = xindex::get(cid);
            LOG_I << cli << " " << cli.get();
        }
        LOG_I << cli <<" "<< req;

        if (starts_with(req.path(), "file/"))
        {
            rsp.header("Content-Type", "image/png");
            return download_file(cli, rsp, req);
        }
        if (req.path() == "a/mail/bind")
        {
            rsp.header("Content-Type", "text/html;charset=UTF-8");
            return bindmail(rsp, req);
        }
        if (req.path() == "a/mail/pwd")
        {
            rsp.header("Content-Type", "text/html;charset=UTF-8");
            return mailpwd(rsp, req);
        }
        if (req.path() == "alix/notify")
        {
            return alipay_notify(rsp, req);
        }
        if (req.path() == "alix/notify-ios")
        {
            return alipay_notify_ios(rsp, req);
        }
        if (req.path() == "a/mail/js/md5.js")
        {
            rsp.content( readall("md5.js",htm_dir_) );
            return client_ptr();
        }
        if (req.path() == "my-gwid")
        {
            // rsp.content( gwNotification(req, get_ipav4(soc,req.header("X-Forwarded-For","")), cli) );
            rsp.content(std::string("{}"));
            return client_ptr();
        }
        //if (req.path() == "spot/ipa/del")
        //{
        //    release_ipa(req.param("a"));
        //    rsp.content( std::string("{}") );
        //    return client_ptr();
        //}
        if (req.path() == "clients-info")
        {
            rsp.header("Content-Type", "text/html;charset=UTF-8");
            rsp.content( html_clients_info(monit_summary_) );
            return client_ptr();
        }
        if (req.path() == "yx-socket")
        {
            rsp.header("Content-Type", "text/html;charset=UTF-8");
            rsp.content(html_yx_socket(lexical_cast<UID>(req.param("userid","0"))
                        , lexical_cast<int>(req.param("day","0")) ));
            return client_ptr();
        }
        if (req.path() == "ico")
        {
            UID uid = lexical_cast<UID>(req.param("u"));
            if (client_ptr c = xindex::get(uid))
            {
                std::string p = c->head_icon();
                auto x = p.find("/file/");
                if (x++ != std::string::npos)
                {
                    rsp.header("Content-Type", "image/png");
                    rsp.content( get_file(string(p.begin()+x+5,p.end()), true) );
                }
                return client_ptr();
            }
        }

        if (req.path() == "forceOutSpot")
        {
            forceOutSpot( req );
            rsp.content(std::string("{}"));
            return client_ptr();
        }

        if (!init_wan_ipaddr_)
            wan_ipaddress_ = soc->local_endpoint().address();

        if (cli)
        {
            cli->tp_.active = time(0);
            if (cli->is_authorized())
            {
                // ip::address_v4 ipa = get_ipav4(soc,req.header("X-Forwarded-For",""));
                // spot_main_entry(cli, get_bar_byip(ipa), false);
            }
            if (cli->macaddr_.empty())
            {
                cli->macaddr_ = try_get_macaddr(req);
            }
        }
        service_fn_type fn = account::service_fn(req, cli);

        LOG_I << cli <<" "<< soc <<" "<< cid <<" "<< req.path();
        rsp.content( fn(req, cli) );
        if (cli)
        {
            // cli->ipaddr_ = soc->local_endpoint().address();
            clientid_type newcid;
            if (fn != &Client::sign_out && fn != &Client::unregist)
            {
                newcid = cli->client_id();
            }
            if (cid != newcid) // (cli && cid != cli->client_id()) //( && is_token_back(fn))
            {
                LOG_I << "TOKEN " << cid <<" replaced "<< newcid <<" "<< req.path();
                rsp.header("Set-Cookie", str(format(SESSION_KEY"=%1%") % newcid));
                if (!newcid.empty() && !cli->is_authorized()) // (!newcid.empty()) //
                {
                    xindex::set(newcid, cli);
                }
            }

            set_extra_header(rsp, cli);
        }

        LOG_I << cli <<" "<< soc <<" "<< rsp.str_header();
        return cli;
    }
    catch (my_exception const & e)
    {
        error_code = e.error_code();
        LOG_E << diagnostic_information(e);
    }
    catch (std::exception const & e)
    {
        LOG_E << e.what();
        error_code = boost::system::error_code(500, Error_category::inst<myerror_category>());
    }
    catch (...)
    {
        error_code = boost::system::error_code(500, Error_category::inst<myerror_category>());
    }

    rsp.status_code(400);
    if (&error_code.category() == &Error_category::inst<myerror_category>())
        rsp.status_code(500);
    if (cli)
    {
        set_extra_header(rsp, cli);
    }

    const string & errmsg = error_code.message();

    rsp.content( json::object()
                ("operatorCode", error_code.value())
                ("message", errmsg)
            );

    LOG_E << cli <<" "<< soc <<" error "<< error_code <<":"<< errmsg;// <<" "<< rsp.str();

    // { rsp.header("Set-Cookie", str(format(SESSION_KEY"=; "SESSION_KEY"_O=%1%") % cid)); }

    return client_ptr();
}

json::object Client::favicon(http::request& req, client_ptr& c)
{
    MYTHROW(EN_HTTPRequestPath_NotFound,client_error_category);
    return json::object();
}

UInt Client::incr_user_info_ver()
{
    auto reply = redis::command("INCR", "user/info/ver");
    if (!reply || reply->type != REDIS_REPLY_INTEGER)
        return 1;
    return reply->integer; //lexical_cast<UInt>(reply->str);
}

filesystem::path Client::Files_Dir = "Files";
std::string Client::Files_Url = "http://127.0.0.1:9000/file";

ip::address Client::wan_ipaddress()
{
    return wan_ipaddress_;
}

static void first_run (const boost::property_tree::ptree & ini)
{
    regist_us("admin", "", "月下", "", "admin/icon.png", SYSADMIN_UID);
    regist_us("lindu", "administrator", "lindu", "", "admin/icon.png", 2000);
    regist_us("guest", "", "guest", "", "", 10000);

    regist_us("weibo", "", "", "", "");
    regist_us("QQ", "", "", "", "");

    std::string t2000 = ini.get<string>("tok2000","4300432d2ffef40a7d0");
    sql::exec(boost::format("INSERT INTO client(UserId,ATime,Token) VALUES(2000, NOW(), '%1%')") % t2000);

    // std::string p = readfile("etc/moon.d/user-protocol.txt");
    // if (!p.empty())
    //     sql::exec(boost::format("INSERT INTO protocols(protocol_name,protocol) VALUES('user_protocol','%1%')") % sql::db_escape(p));

    copyfile("etc/moon.d/admin.icon.png", "Files/admin/icon.png");

    LOG_I << "";
}

void log_myloc(UID uid)
{
    sql::exec(boost::format("INSERT INTO mylocation(UserId) VALUES('%1%')") % uid);
}

// 早上8:00
// 一个小时未上报
// 上报距离 <500 : OK
//          >500 & < 1000 : count++
//          >1000 : Leave
// 进入其他场
//
json::object Client::mylocation(http::request& req, client_ptr& cli)
{
    if (!cli)
        return json::object()("sid", "");
    UID uid = cli->user_id();
    std::string retsid;

    double longtitude = lexical_cast<double>(req.param("longtitude"));
    double latitude = lexical_cast<double>(req.param("latitude"));
    std::string sid = req.param("s", "");
    if (!sid.empty())
    {
        sid = base64_decode( urldecode(sid));
    }

    log_myloc(uid);

    LOG_I <<uid <<" "<< sid <<" "<< longtitude <<" "<< latitude;
    LOG_I <<uid <<" "<< cli->gwid();

    //if (!sid.empty())
    //{
    //    if (sid != cli->gwid() && !boost::empty(cli->gwid()))
    //        spot_main_entry(cli, bar_ptr());
    //}

    bar_ptr pbar;
    double dist = std::numeric_limits<double>::max();
    bool olds = 0;

    if (sid.empty())
    {
        sid = cli->gwid();
        if (sid.empty())
        {
            sid = cli->oldspot_;
            if (sid.empty())
                goto Pos_retsid_;
            olds = 1;
            LOG_I <<uid <<" olds "<< sid;
        }
    }
    else if (sid == cli->gwid())
    { }
    else if (!boost::empty(cli->gwid()))
    {
        spot_main_entry(cli, bar_ptr());
        cli->oldspot_.clear();
    }

    pbar = bars_mgr::inst().get_session_bar(sid);
    if (!pbar)
        goto Pos_retsid_;

    dist = GetDistance(pbar->latitude, pbar->longtitude, latitude, longtitude);
    LOG_I <<uid <<" "<< sid <<" "<< pbar->longtitude <<" "<< pbar->latitude <<" dist "<< dist;

    if (olds)
    {
        if (dist < 1.0)
        {
            spot_main_entry(cli, pbar);
            retsid = pbar->sessionid;
            goto Pos_retsid_;
        }
        goto Pos_retsid_;
    }

    if (cli->gwid() == pbar->sessionid)
    {
        bool on_edge = cli->flags_.on_spot_edge;
        cli->flags_.on_spot_edge = 0;

        if (dist < 1.0)
        {
            pbar->refresh_spot_time(cli);
            retsid = pbar->sessionid;
            goto Pos_retsid_;
        }
        if (dist > 1.5)
        {
            spot_main_entry(cli, bar_ptr());
            goto Pos_retsid_;
        }

        if (on_edge)
        {
            spot_main_entry(cli, bar_ptr());
            goto Pos_retsid_;
        }
        cli->flags_.on_spot_edge = 1;
        pbar->refresh_spot_time(cli);

        retsid = pbar->sessionid;
        goto Pos_retsid_;
    }

    if (dist > 1.0)
    {
        cli->oldspot_.clear();
        MYTHROW(EN_Distance_Invalid, client_error_category);
    }

    spot_main_entry(cli, pbar);
    retsid = pbar->sessionid;

Pos_retsid_:
    cli->oldspot_.clear();
    LOG_I <<uid <<" ret "<< retsid;
    return json::object()("sid", retsid);
}
//{
//    if (cli)
//    {
//        bar_ptr pbar;
//
//        double longtitude = lexical_cast<double>(req.param("longtitude"));
//        double latitude = lexical_cast<double>(req.param("latitude"));
//
//        string const & spot = cli->gwid();
//        if ( !spot.empty() ) {
//            pbar = bars_mgr::inst().get_session_bar(spot);
//            if (pbar)
//            {
//                double dist = GetDistance(pbar->latitude, pbar->longtitude, latitude, longtitude);
//                // if (dist < 100.0/1000) return json::object();
//                if (dist < 300.0/1000)
//                    return json::object();
//            }
//            spot_main_entry(cli, bar_ptr());
//        }
//
//        bar_ptr nearbar = bars_mgr::inst()
//            .find_nearest_bar(latitude, longtitude, cli->flags_.devtype, cli->user_id());
//        if (!nearbar)
//            return json::object();
//        if (nearbar == pbar)
//            return json::object();
//
//        double dist = GetDistance(nearbar->latitude, nearbar->longtitude, latitude, longtitude);
//        if (dist < 150.0/1000)
//            spot_main_entry(cli, nearbar);
//    }
//
//    return json::object();
//}

Client::initializer::~initializer()
{
    LOG_I << __FILE__;

    xindex::clear(index_key<int,socket_ptr>(0));

    xindex::clear(UID());
    xindex::clear(clientid_type());

    LOG_I << __FILE__;
}

Client::initializer::initializer(const boost::property_tree::ptree & ini, boost::asio::io_service& ios)
{
    // * -> A
    urlmap_["login"] = &Client::sign_in;
    urlmap_["regist"] = &Client::user_regist;
    urlmap_["applyIdentify"] = &Client::guest_regist;
    urlmap_["login-fwd"] = &Client::login_fwd;
    //version 1.1
    urlmap_["phoneRegist"] = &Client::phoneRegist;
    urlmap_["otherLogin"] = &Client::otherLogin;
    urlmap_["firstLoginToOtherPlatform"] = &Client::firstLoginToOtherPlatform;

    // A -> A / N -> N / Z -> N
    urlmap_["validCode"] = &Client::getauth;
    urlmap_["mobileMailFindPwd"] = &Client::resetpwd;
    //version 1.1
    urlmap_["confirmValidCode"] = &Client::confirmValidCode;

    // * -> Z
    urlmap_["logout"] = &Client::sign_out;
    urlmap_["unregister"] = &Client::unregist;

    //  urlmap_["charges"] = &charges;
    //  urlmap_["appleCharges"] = &appleCharges;

    // A!
    urlmap_["bills"] = &bills;
    urlmap_["bind"] = &Client::bindaccount;
    urlmap_["updatePassword"] = &Client::chpwd;
    urlmap_["pushNotification"] = &Client::pushNotification;
    urlmap_["pushSetting"] = &Client::pushSetting;
    urlmap_["app_state"] = &Client::app_state;
    urlmap_["mylocation"] = &Client::mylocation;

    // * -> &
    // urlmap_["my-gwid"] = &Client::gwNotification;
    urlmap_["echo"] = &Client::echo;

    // xindex::set(UID(1000), sysadmin_client_ptr());

    if ( (aps_push_disabled_ = ini.get<bool>("aps_push_disabled",false)) == true)
        LOG_I << "aps push disabled";

    web_url_ = ini.get<string>("web_url",web_url_);
    htm_dir_ = ini.get<string>("htm_dir",htm_dir_.string());

    sql::datas datas(format("SELECT 1 FROM users WHERE UserId=%1%") % SYSADMIN_UID);
    if (datas.count() == 0)
        first_run(ini);

    //! load_ipaddr_spot_binds();
    load_public_messages();

    wan_ipaddress_ = ip::address::from_string("58.67.160.243");
}

void Client::serve(const std::string& path, service_fn_type fn)
{
    service_fn_ = fn;
}

ServiceEntry::ServiceEntry(boost::function<send_fn_type (socket_ptr)> a)
{
    assoc_g_ = a;
}

bool ServiceEntry::work(socket_ptr soc, http::request& req)
{
    LOG_I << "===http=" << soc <<" "<< req;

    if (req.method() != "GET" && req.method() != "POST")
        { return false; }

    http::response rsp(req.method(), req.keep_alive());

    // if (boost::asio::ip::is_private(soc->remote_endpoint().address())){
    //     req.set_header("web", "1");
    // }

    // TODO: clear
    // if (bar_ptr pbar = get_bar_byip(soc->remote_endpoint().address().to_v4()))
    // {
    //     req.set_header("gwid", pbar->sessionid);
    //     // auto i = ip_spot_map_.find( soc->remote_endpoint().address().to_v4().to_ulong() );
    //     // if (i != ip_spot_map_.end())
    //     //     req.set_header("gwid", i->second.name);
    // }

    client_ptr c = Client::http(soc, req, rsp);
    if (c)
        c->tp_.active = time(0);

    if (!rsp.empty())
    {
        send_fn_type fn = assoc_g_(soc);
        if (fn.empty())
            MYTHROW(EN_Server_InternalError,client_error_category);

        fn(rsp.str());
    }

    return rsp.keep_alive();
}

bool ServiceEntry::imessage(socket_ptr soc, std::pair<std::string,std::string>& mp)
{
    LOG_I << "===socket=" << soc << " " << mp.first; // << " " << mp.second;

    json::object h, body;

    try{
        if (!mp.first.empty())
        {
            // LOG_I << mp;
            h = json::decode(mp.first);
        }
        if (!mp.second.empty())
        {
            body = json::decode(mp.second);
        }
        if (!h.empty())
            ; //monitor_socket_(soc, time(0), "decode");
    }
    catch(...)
    {
        return false;
    }

    return bool(Client::socket(soc, h, body));
}

struct Tab_sessions { static const char *db_table_name() { return "sessions"; } };
string make_chatgroup_id() { return "_" + boost::lexical_cast<string>(max_index<Tab_sessions>(1)); }

struct Tab_IndividualAlbum { static const char *db_table_name() { return "IndividualAlbum"; } };
int IndividualAlbum_message_id() { return max_index<Tab_IndividualAlbum>(1); }

struct Tab_mailq { static const char *db_table_name() { return "mailq"; } };
int mailq_idx() { return max_index<Tab_mailq>(1); }

struct Tab_bindMail { static const char *db_table_name() { return "bindMail"; } };
int bindMail_idx() { return max_index<Tab_bindMail>(1); }

struct Tab_mailPwd { static const char *db_table_name() { return "mailPwd"; } };
int mailPwd_idx() { return max_index<Tab_mailPwd>(1); }

struct Tab_bills { static const char *db_table_name() { return "bills"; } };
int alloc_bills_id() { return max_index<Tab_bills>(1); }

struct Tab_ExchangerAddress { static const char *db_table_name() { return "ExchangerAddress"; } };
int alloc_ExchangerAddress_id() { return max_index<Tab_ExchangerAddress>(1); }

string make_p2pchat_id(UID u1, UID u2)
{
    char cg[64];
    if (u1 > u2)
        swap(u1,u2);
    snprintf(cg, sizeof(cg), ".%u.%u", u1, u2);
    return cg;
}

std::string get_head_icon(UID uid)
{
    client_ptr c = xindex::get(uid);
    if (c)
        return c->head_icon();
    return std::string();
}

template <typename Rng>
static void print_msgs(Rng const & rng, const char* tag)
{
    LOG_I << tag;
    BOOST_FOREACH(auto const & x , rng)
        LOG_I << x;
}

json::object Client::echo(http::request & req, client_ptr& cli)
{
    static std::ofstream echof("/tmp/yx-GET.echo.HTTP.headers");
    static std::string echostr(readfile("/tmp/yx-echo.txt"));

    if (cli && !boost::empty(cli->messages_))
    {
        LOG_I << cli;
        print_msgs( boost::get<TagMsgId>(cli->messages_) ,"MsgId");
        print_msgs( boost::get<TagExpire>(cli->messages_) ,"Expire");
        print_msgs( boost::get<TagGMId>(cli->messages_) ,"GroupMid");
    }

    echof << req.headers() << "\n";

    {
        record_msgtime.flush();
        auto_cpu_timer_helper::out_file::inst().flush();
        record_connect.flush();
    }

    return json::object()("echo", echostr);
}

namespace imessage {

//UInt message::alloc_message_id(int x)
//{
//    static int idx_ = 0;
//    if (idx_ == 0) {
//        idx_ = 1;
//
//        sql::datas datas(std::string("SELECT MAX(id) FROM messages"));
//        if (sql::datas::row_type row = datas.next()) {
//            if (row[0]) {
//                idx_ = std::atoi(row[0]) + 1;
//            }
//        }
//    }
//    UInt ret = idx_;
//    idx_ += x;
//
//    LOG_I << "messages / " << " idx_: "<< idx_<< " ret: "<< ret;//<< (int) pthread_self();
//    return ret;
//}

int expire_interval(std::string const & gid, std::string const & msgtype)
{
    if (is_barchat(gid))
        return (60*60*1);
    if (imessage::is_usermsg(msgtype))
    {
        if (is_groupchat(gid))
            return (60*60*24*3);
        if (is_p2pchat(gid))
            return (60*60*24*7);
    }
    return (60*60*24*90);
}

}

void store_public_messages( const imessage::message& msg )
{
    if ( public_messages.size() >= 10 ) {
        redis::command("LPOP", LKEY_GLOBAL_MESSAGE);
        public_messages.pop_front();
    }

    store_msg( msg );
    redis::command("RPUSH", LKEY_GLOBAL_MESSAGE, msg.id());
    public_messages.push_back(msg);
}

void load_public_messages()
{
    auto reply = redis::command("LRANGE", LKEY_GLOBAL_MESSAGE, 0, -1);
    if (!reply || reply->type != REDIS_REPLY_ARRAY)
        return;

    std::set<std::string> mids;
    for (unsigned int i = 0; i < reply->elements; i++) {
        mids.insert(reply->element[i]->str);
    }

    if (!mids.empty())
    {
        string cond_ids = boost::algorithm::join(mids, ",");
        load_msg(str(format(" where id in (%1%) ")%cond_ids), std::back_inserter(public_messages));
    }
}

