#include <sys/types.h>
#include <linux/unistd.h>
#define gettid() syscall(__NR_gettid)  
// #include <poll.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <curl/curl.h>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/function/function0.hpp>  
#include <boost/thread/thread.hpp> 
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <istream>

#include "util.h"
#include "push.h"
#include "json.h"
#include "log.h"
#include "dbc.h"
#include "user.h"
#include "client.h"
#include "apple_push.h"

#define LG_J LOG
#define LG_A LOG

using namespace std;
using namespace boost;

UInt n_ap_ = 0;
const char* reject_push_users::reject_push_users_key = "moonv2:disable_push_user_set";
const char* push_count_record::push_record_user_key = "moonv2:valid_push_user_set";

UInt push_mgr::s_push_msg::alloc_id( UInt base ) 
{ 
    static UInt id = 0; 
    id += base; 
    return id; 
}

class jpush_msgr
{
    public:
        typedef push_mgr::UInt UInt;

        static jpush_msgr& inst();

        void init_push()
        {
            LG_J << "initing jpush_msgr....";
            boost::function0<void> f = boost::bind(&jpush_msgr::loop_catch_ex, this); 
            boost::thread t( f );
            t.detach();
        }
    private:
        void loop_catch_ex();
        void loop_send();
};

jpush_msgr& jpush_msgr::inst()
{
    static jpush_msgr ins;
    return ins;
}

void jpush_msgr::loop_catch_ex()
{
    while (1) {
        try {
            loop_send();
        } catch (std::exception const& ex) {
            LG_J << "=except:" << ex.what();
        }
    }
    LOG << "exit thread tid" << gettid();
}

void jpush_msgr::loop_send()
{
    static const char* push_url = "https://api.jpush.cn/v3/push";
    static const char* user_passwd = "7a09f670a6ad0e5ecd1e0251:fe69151208f26d124b1281b3";
    static const char* content = "{\"platform\":[\"android\"],\"audience\":{\"alias\":[\"%1%\"]},\"notification\":{\"alert\":\"%2%\"}}";

    LOG << "tid" << gettid() << push_url;
    CURL* curl = NULL;

    while( 1 ) {
        int ec = 0, retry=3;

        string tok;
        push_mgr::s_push_msg m;
        UInt num = 1;

        // if ( !push_mgr::instance().jpush_get(tok, m) ) {
        if ( !push_mgr::instance().get_one_msg(push_mgr::ANDROID, tok, m, num) ) {
            // LG_J << "jpush message queue empty.";
            sleep(1); //usleep(500);
            ec = -1;
            retry = 4;
            continue;
        }
        BOOST_ASSERT ( !tok.empty() );

        do {
            if ( NULL == curl ) {
                if ( NULL == (curl = curl_easy_init())) {
                    LG_J<<"failed to curl_easy_init";
                    sleep(1); //usleep(500);
                    continue;
                }

                LG_J<< "set url"<< push_url;
                curl_easy_setopt(curl, CURLOPT_URL, push_url);
                curl_easy_setopt(curl, CURLOPT_POST, 1); 
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
                LG_J<< "set user:password"<< user_passwd;
                curl_easy_setopt(curl, CURLOPT_USERPWD, user_passwd); 
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30); 
                curl_easy_setopt(curl, CURLOPT_HEADER, 0); 
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); 
            }

            string pmsg = (format(content)% tok %m.content).str();

            LG_J << tok << " " <<m;
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pmsg.c_str());
            ec = curl_easy_perform(curl);

            if (ec) {
                LG_J<<"ec:"<<ec<<" msgid:"<< m.msgid;
                curl_easy_cleanup(curl);
                curl = NULL;
                sleep(1); 
            } else {
                // push_mgr::instance().jpush_del( m.userid, m.msgid );
                push_mgr::instance().del_one_msg( m, push_mgr::ANDROID );
            }
        } while ( ec && --retry );
    }
}

void push_mgr::init( boost::asio::io_service& io_service, const boost::property_tree::ptree& ini) 
{
    ios_push_mgr::init( io_service, ini );
    jpush_msgr::inst().init_push();

    load_all_msgs();
    rejects_.load_all_rejects();
    //this; // TODO
    user_mgr::instance().ev_online.connect(boost::bind(&push_mgr::fev_online, this, _1, _2, _3));
    user_mgr::instance().ev_regist.connect(boost::bind(&push_mgr::fev_regist, this, _1, _2));

}

void push_mgr::remove_dev( const std::string& devtoken )
{
    auto it = uid_toks.right.find( devtoken );
    if ( it != uid_toks.right.end() ) {
        LOG<<"devtoken is repeated:"<<it->first;
        uid_toks.right.erase( it );
    }
}

void push_mgr::fev_regist(UInt uid, int regist)
{
    AUTO_CPU_TIMER("push_mgr:fev_regist");
    // if (!regist)
    LOG << uid << regist << "push_mgr";
    pause(uid);
}

void push_mgr::fev_online(UInt uid, int online, int aclose)
{
    AUTO_CPU_TIMER("push_mgr:fev_online");
    // time_t tp0 = time(0);
    if (online)
    {
        LOG << uid << online << aclose << "push_mgr";
        pause(uid);
        // if (time(0) - tp0 > 3) LOG << uid << "time-killer";
        return;
    }

    LOG << "uid" << uid << online << aclose << "push_mgr";

    char k_mq[64];
    snprintf(k_mq,sizeof(k_mq), "msg/q/%u", uid);

    UInt begid = -1;

    auto reply = redis::command("LINDEX", k_mq, 0);
    if (reply && reply->type == REDIS_REPLY_STRING) {
        begid = atoi(reply->str);
    }
    // if (time(0) - tp0 > 3) LOG << uid << "time-killer";

    restart(uid, begid);

    // if (time(0) - tp0 > 3) LOG << uid << "time-killer";
}

bool push_mgr::get_one_msg( channel ch, string& tok, s_push_msg& m, UInt& num )
{
    bool ret = false;
    if ( ch<IOS_RELEASE || ch>= CH_MAX ) {
        LG_A <<" Invalid push channel "<< ch;
        return ret;
    }

    boost::unique_lock<boost::mutex> lock( mutex_ );
    while ( !ids_arry[ ch ].empty() ) {
        auto it = ids_arry[ ch ].begin();
        BOOST_ASSERT( 0 != *it );
        auto& ises = boost::get<TagMyId>( notifications_ );
        auto p = ises.find( *it );
        if ( p == ises.end() ) {
            LG_A << "Failed to find msg of msgid:" << *it;
            ids_arry[ ch ].erase( it );
            continue;
        }

        auto ptok = uid_toks.left.find( p->userid );
        if ( ptok == uid_toks.left.end() ) {
            LG_A << "failed to find token of userid:" << p->userid;
            ids_arry[ ch ].erase( it );
            ises.erase( p );
            continue;
        }

        if ( ch == IOS_ENTERPRISE || ch == IOS_RELEASE ) {
            num = ++pushed_nums[p->userid];
        }

        m = *p;
        tok = ptok->second;
        ret = true;
        break;
    }
    
    return ret;
}

void push_mgr::timed_check()
{
    static time_t tp_=0;
    if (tp_ == 0)
        tp_ = time(0);

    //UInt n_ap = 0; bool empty = 0;
    {
        // boost::unique_lock<boost::mutex> lock(this->mutex_);
        //empty = notifications_.empty();
        //n_ap = n_ap_;
    }

    tp_ = time(0);

    ;
}

template <typename T>
struct loop_this : boost::noncopyable
{
    ~loop_this() {
        LOG << "end tid" << gettid();
    }
    loop_this(T* t) { thiz = t; }
    T* thiz;

    template <typename F> void forever(F fp) const
    {
        LOG << "thread" << boost::this_thread::get_id();
        while (1) {
            try {
                (thiz->*fp)();
            } catch (std::exception const& ex) {
                LG_J << "=except:" << ex.what();
            }
        }
    }
};
//loop_this<jpush_msgr> loop(this);
//loop.forever(&jpush_msgr::loop_send);

bool push_mgr::push( const s_push_msg& msg )
{
    bool ret = true;
    LOG << msg;

    auto cli = clients::inst().find_client( msg.userid, true );
    if ( rejects_.check( msg.userid ) || !check_push_user( cli ) || msg.content.empty()
            || msg.content.length() > MAX_PUSH_BYTES ) {
        LOG<< "rejected or invalid push user:"<< msg.userid<< " content length:"<< msg.content.length();
        return false;
    }

    if ( msg.expired > time( NULL ) ) {
        boost::unique_lock<boost::mutex> lock(mutex_);
        redis::command( "HSET", push_msg_key( msg.userid ), msg.msgid, msg.expired );
        bool is_insert = false, is_new_push_user = false;
        auto it = valid_push_users_.find( msg.userid );

        if ( valid_push_users_.end() == it ){
            valid_push_users_.add( msg.userid, 1, true );
            is_insert = true;
            is_new_push_user = true;
        } else {
            if ( push_count_record::ONLINE_NOT_EMPTY != it->second ) {
                if ( push_count_record::ONLINE_EMPTY == it->second ) { 
                    valid_push_users_.add( msg.userid, push_count_record::ONLINE_NOT_EMPTY, true );
                } else {
                    valid_push_users_.incr( it );
                    is_insert = true;
                }
            }
        }

        if ( is_insert ) {
            if ( is_new_push_user ) { uid_toks.insert( uid_token_type( msg.userid, cli->second.get_devtoken() ) ); }
            auto i = insert_msg( cli, vector<s_push_msg>( 1, msg ) );
            if ( i == 0  ) { ret = false; }
        }

    } else {
        LOG << msg.msgid << "message to push has expired:"<<msg.expired<< "/"<<time(NULL);
        ret = false;
    }

    return ret;
}

void push_mgr::restart( UInt userid, UInt begid )
{
    LOG << userid << begid<< " restart....";
    auto cli = clients::inst().find_client( userid );
    if ( check_push_user( cli ) ) {
        boost::unique_lock<boost::mutex> lock(mutex_);
        uid_toks.insert( uid_token_type( userid, cli->second.get_devtoken() ) );

        auto it = valid_push_users_.find( userid );
        if ( it != valid_push_users_.end() && push_count_record::ONLINE_EMPTY != it->second ) {
            UInt num = load_usr_msgs( cli, begid );
            if ( 0 != num ) {
                it->second = num;
            } else {
                LOG << "Empyty! No push queque of userid:"<<userid;
                valid_push_users_.remove( userid, true );
            }
        } else {
            if ( it != valid_push_users_.end() ) { valid_push_users_.remove( userid ); }
            LOG<<"WARNING!! Valid push users do not contain userid:"<<userid;
        }
    } else {
        LOG << "Cannot find userid:"<<userid;
    }
}

void push_mgr::pause( UInt userid )
{
    int debug = 0;
    if ( check_push_user( userid ) ) {
        debug = 1;
        boost::unique_lock<boost::mutex> lock(mutex_);
        auto it = valid_push_users_.find( userid );
        if ( valid_push_users_.end() == it ) {
            valid_push_users_.add( userid );
        } else {
            debug = 2;
            if ( it->second != push_count_record::ONLINE_EMPTY ) {
                it->second = push_count_record::ONLINE_NOT_EMPTY;

                auto& ises = boost::get<TagUserId>( notifications_ );
                ises.erase( userid );
            }
        }
        pushed_nums.erase( userid );
        uid_toks.left.erase( userid );
    }
    LOG << userid<< "paused......" << debug;
}

void push_mgr::load_all_msgs()
{
    std::vector<UInt> u;
    boost::unique_lock<boost::mutex> lock(mutex_);
    auto reply = redis::command( "SMEMBERS", push_count_record::push_record_user_key );
    if ( reply && reply->type == REDIS_REPLY_ARRAY ) {
        for ( unsigned int i = 0; i < reply->elements; ++i ) {
            u.push_back( boost::lexical_cast<UInt>( reply->element[i]->str ) );
        }
    } else {
        LOG<< "Unrecognized type:"<< reply->type;
    }

    BOOST_FOREACH( const auto& uid , u ) {
        auto cli = clients::inst().find_client( uid, true );
        if ( check_push_user( cli ) ) {
            UInt num = load_usr_msgs( cli, 0 );
            if ( 0 != num ) { 
                valid_push_users_.add( uid, num );
                uid_toks.insert( uid_token_type( uid, cli->second.get_devtoken() ) );
            } else {
                valid_push_users_.remove( uid, true );
            }
        } else {
            valid_push_users_.remove( uid, true );
        }
    }
}

UInt push_mgr::load_usr_msgs( clients::iterator cli, UInt begid)
{
    UInt ret = 0;
    UInt uid = cli->first;
    time_t now = time(NULL);
    string key = push_msg_key( uid );
    auto reply = redis::command( "HGETALL", key );

    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        map< UInt, time_t > id_expires;
        for (unsigned int i = 0; i < reply->elements/2; i++) {
            UInt msgid = lexical_cast<UInt>(reply->element[i*2]->str);
            time_t expired = lexical_cast<time_t>(reply->element[i*2+1]->str);

            if ( expired < now || msgid < begid) {
                LG_J << msgid <<" "<< expired;
                redis::command("HDEL", key, msgid);
            } else {
                id_expires.insert( make_pair( msgid, expired ) );
            }
        }

        if ( !id_expires.empty() ) {
            string ids;
            for ( auto it = id_expires.begin(); it != id_expires.end(); ++it) {
                ids += lexical_cast<string>( it->first ) + ",";
            }
            *ids.rbegin() = ' ';

            vector<s_push_msg> msgs;
            const char* data = "select id,notification from message where id in (%1%)";
            sql::datas datas(format(data) %ids);
            while(sql::datas::row_type row = datas.next()){
                UInt id = lexical_cast<UInt> (row.at(0,"0"));
                string cont = row.at(1,"");
                if ( 0 == id || cont.empty() ) { continue; }

                msgs.push_back( s_push_msg( uid, id, id_expires[id], cont ) );
            }

            if ( !msgs.empty() ) {
                ret = insert_msg( cli, msgs );
            }
        }
    }
    return ret;
}

UInt push_mgr::insert_msg( clients::iterator cli, const std::vector<s_push_msg>& msgs )
{
    UInt ret = 0;
    channel ch  = CH_MAX;

    auto devtype = cli->second.get_devtype();
    if ( client_info::DevType_Android == devtype ) {
        ch = ANDROID;
    } else if ( client_info::DevType_iPhone == devtype ) {
        string bundleid = cli->second.get_bundleid();
        if ( ios_push_mgr::enterBid() == bundleid ) {
            ch = IOS_ENTERPRISE;
        } else if ( ios_push_mgr::relBid() == bundleid ) {
            ch = IOS_RELEASE;
        } else if ( ios_push_mgr::etBid() == bundleid ) {
            ch = IOS_ET;
        } else {
            LOG << "Invalid bundleid:" << bundleid;
            return ret;
        }

    } else {
        LOG<< "Invalid devtype of userid:" << cli->second.get_uid() << " devtype:" << devtype;
        return ret;
    }

    BOOST_FOREACH( const auto& p , msgs ) {
        if ( ids_arry[ ch ].insert( p.myid ).second ) {
            notifications_.insert( notifications_.end(), msgs.begin(), msgs.end() );
            ++ret;
        }
    }

    LOG<< ret<< " "<<ch << " "<< msgs.size();
    if ( ret > 0 && ANDROID != ch ) {
        ios_push_mgr::inst().push( ch );
    }

    return ret;
}

void push_mgr::del_one_msg(const s_push_msg& m, channel ch)
{
    ++n_ap_;
    Sql_Exec(str(
        boost::format("UPDATE message_rx SET push=%3%,time=NOW() WHERE uid=%1% AND msgid=%2%")
            % m.userid % m.msgid % ch ));

    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        ids_arry[ ch ].erase( m.myid );
        auto& ises = boost::get<TagMyId>(notifications_);
        ises.erase( m.myid );

        redis::command( "HDEL", push_msg_key( m.userid ), m.msgid );
        if ( 0 == valid_push_users_.decr( m.userid ) ) {
            valid_push_users_.remove( m.userid, true );
            redis::command( "DEL", push_msg_key( m.userid ) );
        }
    }
}
