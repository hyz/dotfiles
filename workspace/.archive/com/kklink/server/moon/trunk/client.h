#ifndef CLIENT_H__
#define CLIENT_H__

#include <stdint.h>
#include <time.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <boost/function.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>

#include "util.h"
#include "dbc.h"
#include "jss.h"
#include "async.h"
#include "proto_http.h"
#include "proto_im.h"
#include "user.h"

#include <cstring>
#include <pthread.h>
// #include "dynamic_state.h"
class chat_group;

struct Client;
typedef boost::shared_ptr<Client> client_ptr;
struct s_bar_info;
typedef boost::shared_ptr<s_bar_info> bar_ptr;

typedef std::string groupid_type;
typedef std::string clientid_type;

std::string make_p2pchat_id(UID u1, UID u2);
std::string make_chatgroup_id();

inline bool is_p2pchat(const std::string & g) { return !g.empty() && strchr(".!",*g.begin()); }
inline bool is_bottlechat(const std::string & g) { return !g.empty() && ( '!'==*g.begin()); }
//inline bool is_p2pchat(const std::string & g) { return !g.empty() && (*g.begin()=='.'||*g.begin()=='!'); }
inline bool is_groupchat(const std::string & g) { return !g.empty() && *g.begin()=='_'; }
inline bool is_barchat(const groupid_type& g) { return !g.empty() && !is_p2pchat(g) && !is_groupchat(g); }

// int alloc_message_id();
enum { Seconds_PDay = (60*60*24) };

namespace imessage {

inline bool is_control(std::string const & type)
{
    return 0; //(type == "chat/image" || type == "chat/audio" || type == "chat/text" || type == "chat/cartoon");
}

inline bool is_usermsg(std::string const & type)
{
    return (type == "chat/image" || type == "chat/audio" || type == "chat/text" || type == "chat/cartoon");
}

int expire_interval(std::string const & gid, std::string const & msgtype);

inline int max_nlimit(std::string const & gid)
{
    if (is_barchat(gid))
        return 20+5;
    return 99;
}

struct message
{
    std::string type;
    UID from; // sender id
    std::string gid; // chat group id
    UID a;

    json::object body;

    explicit message(std::string const & ty, UID fr, std::string const & g)
        : type(ty) , gid(g)
    {
        id_ = alloc_message_id();
        from = fr;
        time_ = std::time(0);
        expire_ = time_ + expire_interval(gid, type);
        a = INVALID_UID;
    }

    explicit message(MSGID id, std::string const & ty, UID fr, std::string const & g, time_t xt)
        : type(ty) , gid(g)
    {
        id_ = id;
        from = fr;
        time_ = xt;
        expire_ = time_ + expire_interval(gid, type);
        a = INVALID_UID;
    }

    time_t time() const { return time_; }
    MSGID id() const { return id_; }
// private:

    MSGID id_;   // message id
    time_t time_, expire_;

private:
    // static UInt alloc_message_id(int x=1);
    struct Tab_messages { static const char *db_table_name() { return "messages"; } };
    static UInt alloc_message_id() { return max_index<Tab_messages>(1); }
};

inline std::ostream & operator<<(std::ostream& out, const message & m)
{
    return out << m.id() <<"/"<< m.from <<"/"<< m.gid <<"/"<< m.type; // <<" "<< json::encode(m.body);
}

} //

struct xindex
{
    template <typename T> static client_ptr get(T const & id, bool memonly=0);
    template <typename T> static client_ptr set(T const & id, client_ptr const & c);
    template <typename T> static void clear(T const & id);
};

struct signals
{
    static boost::signals2::signal<void (client_ptr, bool)> & online();
    static boost::signals2::signal<void (client_ptr, bool)> & sign();
};

// typedef std::map<std::string, std::string> cache_type;
typedef json::object cache_type;

typedef boost::function<json::object (http::request&, client_ptr& )> service_fn_type;

enum enum_client_error
{
    EN_Client_Base = 200,

    EN_Login_Other = 215,
    EN_Unauthorized,
    EN_User_NotFound,
    EN_Login_Fail,

    EN_Password_Incorrect = 225,
    EN_Password_Invalid,
    EN_AuthCode_Invalid,

    EN_UserName_NotAvail,
    EN_UserName_Invalid,
    EN_AuthCode_Timeout,

    EN_NickName_Invalid,

    EN_Phone_Invalid = 235,
    EN_Phone_Incorrect,
    EN_Phone_Not_Bind,
    EN_Phone_Bind_Already,

    EN_Mail_Not_Bind = 245,
    EN_Mail_Invalid,
    EN_Mail_Bind_Already,

    EN_Input_Form = 255,
    EN_Input_Data,

    EN_Operation_NotPermited = 265,

    EN_Distance_Invalid = 615
};
struct client_error_category : ::myerror_category
{
    client_error_category();
};

template <typename Val,typename Tag>
struct index_key
{
    explicit index_key() : val_() {}
    explicit index_key(Val const & val) : val_(val) {}
    bool operator<(const index_key & rhs) const { return val_ < rhs.val_; }
    Val val_;
};

struct tag_aps_tok;

struct TagMsgId {};
struct TagGMId {};

struct ck_gid_mid : boost::multi_index::composite_key<
    imessage::message,
    BOOST_MULTI_INDEX_MEMBER(imessage::message,std::string,gid),
    BOOST_MULTI_INDEX_MEMBER(imessage::message,MSGID,id_)
> {};

struct TagExpire {};

struct Client
{
    static client_ptr http(socket_ptr & soc, http::request& req, http::response& rsp);
    static client_ptr socket(socket_ptr soc, json::object & jso, json::object & body);

    static void serve(const std::string& path, service_fn_type fn);

    cache_type & cache() { return cache_; }

    UID user_id() const { return uid_; }
    clientid_type const& client_id() const { return cid_; }
    void client_id(clientid_type const& cid) { cid_=cid; }

    bool is_authorized() const { return uid_>0 && !cid_.empty(); }
    bool is_guest() const { return flags_.is_guest>0; }
    bool is_third_part() const { return flags_.is_third_part>0; }

private:
    static inline bool is_token_back(service_fn_type fn);
    static json::object getauth(http::request& req, client_ptr&);
    // static json::object checkauth(http::request& req, client_ptr&) ;
    static json::object resetpwd(http::request& req, client_ptr&) ;

    static json::object sign_in(http::request& req, client_ptr&) ;
    static json::object user_regist(http::request& req, client_ptr&) ;
    static json::object guest_regist(http::request& req, client_ptr& c);
    static json::object login_fwd(http::request & q, client_ptr& cli);

    static json::object sign_out(http::request& req, client_ptr&);
    static void record_sign_out(client_ptr&, bool is_forced=false);
    static json::object unregist(http::request& req, client_ptr&) ;
    static json::object pushNotification(http::request & req, client_ptr& client);
    static json::object bindaccount(http::request & req, client_ptr& client);

    static json::object pushSetting(http::request & req, client_ptr& cli);

    static json::object chpwd(http::request & req, client_ptr& client);

    static json::object favicon(http::request& req, client_ptr& c);
    
    //version 1.1
    static json::object confirmValidCode(http::request& req, client_ptr&) ;
    static json::object phoneRegist(http::request& req, client_ptr&) ;
    static json::object otherLogin(http::request& req, client_ptr&) ;
    static json::object firstLoginToOtherPlatform(http::request& req, client_ptr&) ;

    static json::object echo(http::request & req, client_ptr& cli);
    static json::object app_state(http::request & req, client_ptr& cli);
    static json::object mylocation(http::request & req, client_ptr& cli);

public:
    void uptime() {  }

    json::object brief_user_info() const;
    json::object const & user_info() const { return user_info_; }
    json::object & user_info() { return user_info_; }

    const std::string user_nick() const { return user_info_.get<std::string>("nick"); }
    const std::string user_name() const;
    std::string user_mail() const { return user_info_.get<std::string>("mail",""); }
    const std::string user_gender() const { return user_info_.get<std::string>("gender"); }

    const std::string & gwid() const { return spot_; }

    std::string head_icon() const { return user_info_.get<std::string>("icon",""); }

public:
    std::string spot_, roomsid_;

    std::string get_spot_or(std::string const & x) const;

    static void check_join_chat(client_ptr & cli, const std::string & bsid);

    static void spot_main_entry(client_ptr & cli, bar_ptr pbar);
private:

    static void ack(client_ptr & cli, MSGID x);
    static void write_socket(client_ptr & cli);
    static void close_socket(client_ptr & cli);
    static void check_deadline(const boost::system::error_code & err, client_ptr cli);

    void keepalive(json::object & jso);

    void db_save(bool logout=0);
    static void set_apple_devtoken(client_ptr & cli, std::string const & tok);

    void clear_message(std::string const & gid, int n_max);
    template <typename I> int db_delete_message(I beg, I end);
    void db_save_message(imessage::message const & m, int n_max);
    void GetUnpushedMessage();

    void barhis(std::string const & gid, chat_group const & cg);

    std::map<std::string, MSGID> lastack_;
public:
    MSGID get_lastack(std::string const& gid) const
    {
        auto i = lastack_.find(gid);
        return (i==lastack_.end() ? 0 : i->second);
    }

    static void pushmsg(UID rcpt, const imessage::message & m);
    static void pushmsg(client_ptr & cli, const imessage::message & m);

    UInt user_info_ver() const { return user_info_ver_; }
    UInt user_info_ver(int) { return (user_info_ver_ = incr_user_info_ver()); }

    void clear_oldspot() { oldspot_.clear(); }
private:
    // std::list<imessage::message> messages_;
    typedef boost::multi_index_container<
        imessage::message,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagMsgId>, BOOST_MULTI_INDEX_MEMBER(imessage::message,MSGID,id_)>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagGMId>, ck_gid_mid>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagExpire>, BOOST_MULTI_INDEX_MEMBER(imessage::message,time_t,expire_)>
        >
    > message_list_t;
    message_list_t messages_;

    std::string cid_; // client-id
    UID uid_; // user-id
    std::string apple_devtoken_;

    std::string macaddr_;

    boost::function<void (const std::string&)> writer_; // boost::function<> socket_;
    int native_fd_;
    boost::asio::ip::tcp::endpoint endpoint_r;


    UInt user_info_ver_;
    json::object user_info_; // TODO: db cache
    cache_type cache_;
    cache_type temps_;
    struct wrapflags
    {
        // bool location_by_gps;
        bool is_iosrel;
        bool is_app_background;
        // bool is_lanip;
        bool is_guest;
        bool is_third_part;
        bool imsg_login_other;
        bool on_spot_edge;
        unsigned char devtype;
        wrapflags() { memset(this, 0, sizeof(*this)); }
    };
    wrapflags flags_;
    std::string oldspot_;

    static UInt incr_user_info_ver();

    boost::shared_ptr<boost::asio::deadline_timer> timer_;

    static void timer_init(client_ptr & cli, boost::asio::io_service & ios);

public:
    boost::posix_time::ptime pt_sock_write;
    struct _ptimes
    {
        struct _sock
        {
            boost::posix_time::ptime open, close;
            boost::posix_time::ptime read;
            boost::posix_time::ptime write;
        } sock;
    } pt_;

    struct time_points
    {
        // time_t sock_open, sock_close, sock_read;
        time_t active;
        time_t today;
        time_t expire_hello;
        time_t expire_ack;
        time_t spot_changed;
        time_t app_background;
        time_t mylocation;

        time_points() { memset(this, 0, sizeof(*this)); }
    } tp_;

    template <typename X> struct zero_inited { zero_inited() { memset(this, 0, sizeof(X)); } };

    struct zdcounters : zero_inited<zdcounters>
    {
        struct Sx{ MSGID id; short write; short _; } msg;
        struct Sy{ short bg, fg; } connect;
    } count_;

    int lucky_shake_count, whisper_love_count, pic_charm_count;
    int charms, richs;

    ~Client();
    explicit Client(UID uid=0);
    explicit Client(UID uid, const std::string& cid);

    static boost::asio::ip::address wan_ipaddress();

private:
    enum { Closed_By_Local=0, Closed_By_Remote=1 } closed_by_;

    void connection_notify(int y);

    // explicit Client(std::string cid, UID uid, const json::object & jso);
    static client_ptr load(const clientid_type& cid);
    static client_ptr load(UID uid);
    static client_ptr load(const index_key<std::string,tag_aps_tok>& idx);
    static client_ptr sqload(boost::format & sql);
    template <typename T> static client_ptr load(T const&) { BOOST_ASSERT(0); return client_ptr(); }

    friend struct account;
    friend struct xindex;
    friend std::ostream& operator<<(std::ostream& out, const client_ptr& c);

public:
    struct initializer : boost::noncopyable
    {
        initializer(const boost::property_tree::ptree & ini, boost::asio::io_service& ios);
        ~initializer();
    };
    friend struct initializer;

public:
    //version 1.1
    typedef std::map<UID, std::pair<std::string, std::string> > client_admires_type;
    typedef std::map<UID, std::pair<std::string, std::string> >::iterator client_admires_iterator;
    typedef std::map<UID, std::pair<std::string, std::string> >::const_iterator client_admires_con_iterator;
    typedef std::map<UID, std::string> client_fans_type;
    typedef std::map<UID, std::string>::iterator client_fans_iterator;
    typedef std::map<UID, std::string>::const_iterator client_fans_con_iterator;

    bool check_black(UID uid)
    {
        if(!load_blacks_flag) load_black();
        return black_list.find(uid) != black_list.end();
    }
    bool check_black(client_ptr cli){return check_black(cli->user_id());}
    bool check_black(Client& client){return check_black(client.user_id());}
    void erase_black(UID uid)
    {
        if(black_list.empty()) return;
        black_list.erase(uid);
    }
    void erase_black(client_ptr cli){erase_black(cli->user_id());}
    void erase_black(Client& client){erase_black(client.user_id());};
    void insert_black(UID uid) { black_list.insert(uid); }
    void insert_black(client_ptr cli){insert_black(cli->user_id());}
    void insert_black(Client& client){insert_black(client.user_id());}

    //version 1.1
    client_fans_type& get_fans() {if(!load_fans_flag) load_fans(); return fans;}
    bool get_fan(UID uid, client_fans_iterator& itr) 
    {
        if(!load_fans_flag) load_fans();
        itr=fans.find(uid);
        return itr != fans.end();
    }
    void insert_fan(UID uid, const std::string& admire_time) 
    {
        client_fans_iterator itr=fans.find(uid);
        if(itr != fans.end()){
            itr->second=admire_time;
        }
        else{
            fans.insert(std::make_pair(uid, admire_time));
        }
    }
    void delete_fan(UID uid)
    {
        fans.erase(uid);
    }
    client_admires_type& get_admires() {if(!load_admires_flag) load_admires(); return admires;}
    bool get_admire(UID uid, client_admires_iterator& itr) 
    {
        if(!load_admires_flag) load_admires(); 
        itr=admires.find(uid);
        return itr != admires.end();
    }
    void insert_admire(UID uid, const std::string& admire_time, const std::string remark="") 
    {
        client_admires_iterator itr=admires.find(uid);
        if(itr != admires.end()){
            itr->second=std::make_pair(admire_time, remark);
        }
        else{
            admires.insert(std::make_pair(uid, std::make_pair(admire_time, remark)));
        }
    }
    void delete_admire(UID uid)
    {
        admires.erase(uid);
    }
    void remark_admire(UID uid, const std::string remark="")
    {
        client_admires_iterator itr=admires.find(uid);
        if(itr != admires.end()){
            itr->second.second = remark;
        }
    }

    // int get_bar_charm( std::string sessionId )
    // {
    //     if ( !load_charm_flag ) { load_charm(); }
    //     std::map<std::string, int>::iterator it = bar_charms_.find( sessionId );
    //     if ( it != bar_charms_.end() ) {
    //         return it->second;
    //     } else {
    //         return 0;
    //     }
    // }

    // void set_bar_charm( std::string sessionId ,int charm)
    // {
    //     if ( !load_charm_flag ) { load_charm(); }
    //     std::map<std::string, int>::iterator it = bar_charms_.find( sessionId );
    //     if ( it != bar_charms_.end() ) {
    //         it->second += charm;
    //     } else {
    //         bar_charms_[sessionId] = charm;
    //     }

    //     charms += charm;
    // }

    int get_charm() { return charms; }
    void set_charms(int n) { charms += n; }

    int get_rich() { return richs; }
    void set_rich( UID fromuid, UID touid, int rich ) 
    { 
        std::string sessionid("");
        if (spot_.empty()) { sessionid=spot_;}
        const char* INSERT_STATISTICS_RICH = "INSERT INTO statistics_rich(UserId,DestId,money,SessionId) "
            " VALUES(%1%, %2%, %3%, '%4%')";
        sql::exec( boost::format(INSERT_STATISTICS_RICH) %fromuid %touid %rich %sessionid );
        richs += rich; 
    }

    void add_attention_bar(std::string sessionid)
    {
        std::pair< std::set<std::string>::iterator, bool> ret;
        ret = this->attention_bars.insert(sessionid);
        if(ret.second){
            update_attention_bar();
        }
    }

    void delete_attention_bar(std::string sessionid)
    {
        if(this->attention_bars.erase(sessionid)){
            update_attention_bar();
        }
    }

    void update_attention_bar()
    {
#define UPDATE_ATTENTION_BARS ("update IndividualDatas set attention_bars='%1%' WHERE UserId=%2%")
        std::ostringstream out;
        BOOST_FOREACH(auto const & p , this->attention_bars)
            out << p << " ";
        sql::exec(boost::format(UPDATE_ATTENTION_BARS) % out.str() %this->uid_);
    }
    void load_attention_bars(const std::string& att_bars)
    {
        std::istringstream ins(att_bars);
        std::istream_iterator<std::string> i(ins), end;
        this->attention_bars.insert( i,end );
    }
    std::set<std::string> const& get_attention_bars() const
    {
        return attention_bars;
    }

private:
    void load_black();
    void load_fans();
    void load_admires();
    // void load_charm();

    std::set<UID> black_list;
    client_fans_type fans;
    client_admires_type admires;
    bool load_fans_flag, load_admires_flag, load_blacks_flag; //, load_charm_flag;
    std::set<std::string> attention_bars;
    std::map<std::string, int> bar_charms_;
private:
    MSGID pub_msg_id;

public:
    static boost::filesystem::path Files_Dir;
    static std::string Files_Url;
};

void incr_charms(client_ptr & cli, int charm, UID fromuid=0);

inline void incr_charms(UID uid, int charm, UID fromuid=0)
{
    if (client_ptr c = xindex::get(uid))
        incr_charms(c, charm, fromuid);
}

client_ptr & sysadmin_client_ptr();

std::ostream& operator<<(std::ostream& out, const client_ptr& c);

typedef boost::function<void (const std::string&)> send_fn_type;

struct ServiceEntry //struct http::responser
{
    ServiceEntry(boost::function<send_fn_type (Socket_ptr)> a);

    bool work(Socket_ptr soc, http::request& req);
    bool imessage(Socket_ptr soc, std::pair<std::string,std::string>& msg);

private:
};

std::string read_file(std::string const & fnp, bool not_throw=false);
std::string get_file(boost::filesystem::path const & fnp, bool no_throw=false);
std::string uniq_relfilepath(boost::filesystem::path fnp, std::string dir);
std::string write_file(boost::filesystem::path fnp, const std::string & cont);


bool is_match(const std::string& value, const std::string &re);

#define REG_GENDER "[FM]?"
#define REG_USER "[a-zA-Z][0-9a-zA-Z_\\-\\.]{4,24}"
#define REG_USER_GUEST "#\\d{4,}"
#define REG_NICK ".{1,80}"
#define REG_PWD ".{6,64}"
#define REG_MACADDR ".{8,64}"
#define REG_MOBILE_PHONE "(\\+?86)?(13[0-9]|147|15[^4]|18[^4])\\d{8}"
#define REG_PLAT ".{3,20}"
#define REG_DOMAIN "([0-9a-zA-Z][0-9a-zA-Z_\\-]{1,24}\\.){1,3}" "[a-zA-Z]{2,3}"
#define REG_EMAIL "[0-9a-zA-Z][0-9a-zA-Z_\\-\\.]{1,24}" "@" REG_DOMAIN
#define REG_URL ".{16,512}"
#define REG_OPENID ".{8,32}"

inline std::string complete_url(const std::string& file)
{
    if(!file.empty() && !boost::starts_with(file, "http:")){
        return Client::Files_Url + "/" + file;
    }
    else{
        return file;
    }
}

inline std::string complete_url(const char* fnp)
{
    return complete_url(std::string(fnp));
}

inline std::string complete_url(const boost::filesystem::path & fnp)
{
    return complete_url(fnp.string());
}

template <typename Val>
UID passwd_verify(const std::string & pwd, const std::string & k, const Val & val)
{
    sql::datas datas(boost::format("SELECT password,UserId FROM users WHERE %1%='%2%' LIMIT 1") % k % val);
    sql::datas::row_type row = datas.next();
    if (!row || pwd != row.at(0))
    {
        return INVALID_UID;
    }

    return boost::lexical_cast<UID>(row.at(1));
}

UID _user_regist(std::string const & user, std::string const & pwd, std::string icon_path);

inline bool operator<(imessage::message const & lhs, imessage::message const & rhs)
{
    // return (lhs.gid < rhs.gid || (lhs.gid==rhs.gid && lhs.id() < rhs.id()));
    return (lhs.id() < rhs.id());
}

struct message_gid_cmp_t
{
    bool operator()(imessage::message const & lhs, imessage::message const & rhs) const
    {
        return (lhs.gid < rhs.gid || (lhs.gid==rhs.gid && lhs.id() < rhs.id())) ;
    }
};

// struct ck_gid_mid : boost::multi_index::composite_key<
//     imessage::message,
//     BOOST_MULTI_INDEX_MEMBER(imessage::message,std::string,gid),
//     BOOST_MULTI_INDEX_MEMBER(imessage::message,MSGID,id_)
// > {};

typedef boost::multi_index_container<
    imessage::message,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<ck_gid_mid>
    >
> message_idx_t;

typedef boost::multi_index_container<
    imessage::message,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::ordered_unique<ck_gid_mid>,
        boost::multi_index::ordered_non_unique<BOOST_MULTI_INDEX_MEMBER(imessage::message,time_t,expire_)>
    >
> message_idx_expire_t;


extern std::ofstream timer_log;
struct my_auto_cpu_timer 
{
    explicit my_auto_cpu_timer(std::ofstream& fs, const std::string& file, 
            const std::string& func, int line) : fs_(fs), x_time(fs), 
            file_(file), func_(func), line_(line){}
    ~my_auto_cpu_timer()
    {
        fs_<<file_<<":"<<func_<<":"<<line_<<std::endl;
    }
    private:
    std::ofstream& fs_;
    boost::timer::auto_cpu_timer x_time;
    std::string file_;
    std::string func_;
    int         line_;
};

inline int GetStatistic(boost::format& fmt)
{
    sql::datas datas(fmt);
    if(sql::datas::row_type row = datas.next()){
        return boost::lexical_cast<int>(row.at(0,"0"));
    }
    return 0;
}

extern std::list<imessage::message> public_messages;
void store_public_messages( const imessage::message& msg );
void load_public_messages();

#define RECORD_TIMER( fs ) my_auto_cpu_timer a(fs, __FILE__, __FUNCTION__, __LINE__)
#endif

