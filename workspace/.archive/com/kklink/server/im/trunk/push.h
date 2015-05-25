#ifndef __PUSH_H__
#define __PUSH_H__

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bimap.hpp>

#include <boost/unordered_map.hpp>

#include <ctime>
#include <string>
#include <istream>
#include <utility>
#include <set>

#include "singleton.h"
#include "dbc.h"
#include "json.h"
#include "client.h"

static const int MAX_PUSH_BYTES = 140;

struct TagMsgId {};
struct TagMyId {};
struct TagUserId {};
struct TagExpiredTime {};

typedef unsigned int UInt;
class reject_push_users
{
    public:
        void add( UInt uid ) { if( users_.insert( uid ).second ) { redis::command("SADD", reject_push_users_key, uid); } }
        void remove( UInt uid ) { if( users_.erase( uid ) ) { redis::command("SREM", reject_push_users_key, uid); } }
        bool check( UInt uid ) { return users_.end() != users_.find( uid ); }
        void load_all_rejects() 
        {
            auto reply = redis::command( "SMEMBERS", reject_push_users_key );
            if (reply && reply->type == REDIS_REPLY_ARRAY) {
                for ( unsigned int i = 0; i < reply->elements; ++i ) {
                    if ( reply->element[i]->str ) {
                        users_.insert( boost::lexical_cast<UInt>( reply->element[i]->str ) );
                    }
                }
            }
        }

    private:
        static const char* reject_push_users_key; 
        std::set<UInt> users_;
};

class push_count_record : public boost::unordered_map<UInt, int>
{
    public:
        enum SPECIAL_COUNT { INVALID=-2, ONLINE_NOT_EMPTY=-1, ONLINE_EMPTY=0 };
        typedef boost::unordered_map<UInt, int>::iterator iterator;
        iterator add( UInt uid, int num = ONLINE_EMPTY, bool is_store = false )
        {
            auto it = insert( std::make_pair( uid, num ) );
            if ( !it.second ) { it.first->second = num; }
            if ( is_store ) { store( uid ); }

            return it.first;
        }

        void remove( UInt uid, bool is_unstore = false )
        {
            erase( uid );
            if ( is_unstore ) { unstore( uid ); }
        }

        int incr( UInt uid ) 
        {
            auto it = find( uid );
            return incr( it );
        }

        int incr( iterator it )
        {
            if ( end() == it ) { return INVALID; }
            return ++it->second;
        }

        int decr( UInt uid ) 
        {
            auto it = find( uid );
            return decr( it );
        }

        int decr( iterator it )
        {
            if ( end() == it ) { return INVALID; }
            return --it->second;
        }

        void set( UInt uid, UInt num ) 
        {
            auto it = find( uid );
            set( it, num );
        }

        void set( iterator it, UInt num )
        {
            if ( end() != it ) { it->second = num ; }
        }

    private:
        void store( UInt uid ) { redis::command( "SADD", push_record_user_key, uid ); }
        void unstore( UInt uid ) { redis::command( "SREM", push_record_user_key, uid ); }
    public:
        static const char* push_record_user_key;
};

class push_mgr : public singleton<push_mgr>
{
    public:
        typedef unsigned int UInt;
    private:
        enum channel { IOS_RELEASE=0, IOS_ENTERPRISE, IOS_ET, ANDROID, CH_MAX };
        struct s_push_msg
        {
            UInt myid;
            UInt userid;
            UInt msgid;
            time_t expired;
            std::string content;

            s_push_msg(){ myid=0; }
            s_push_msg(UInt uid, UInt mid, time_t ex, const std::string& cont)
            {
                myid = alloc_id();
                userid = uid;
                msgid = mid;
                expired = ex;
                content = cont;
            };

            static UInt alloc_id( UInt base=1 );

            friend std::ostream& operator<<(std::ostream& out, s_push_msg const& m)
            {
                return out << m.userid << " / " << m.myid <<" / "<< m.msgid <<" / " << m.expired << " / " << m.content;
            }
        };

    public:
        typedef boost::multi_index_container<
            s_push_msg,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagMyId>, BOOST_MULTI_INDEX_MEMBER( s_push_msg, UInt, myid )
                >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagMsgId>, BOOST_MULTI_INDEX_MEMBER( s_push_msg, UInt, msgid )
                >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagUserId>, BOOST_MULTI_INDEX_MEMBER( s_push_msg, UInt, userid )
                >
            //,boost::multi_index::ordered_non_unique<
            //    boost::multi_index::tag<TagExpiredTime>, BOOST_MULTI_INDEX_MEMBER(s_push_msg,time_t, expired)
            //    >
            >
        > msg_type;

        typedef msg_type::iterator iterator;

    public:
        bool notice(UInt uid, UInt mid, time_t expired, const std::string& cont)
        {
            return push( s_push_msg( uid, mid, expired, cont));
        }

        void remove_dev(const std::string& devtoken);

        void timed_check();
    private:
        bool push( const s_push_msg& );
        void restart( UInt userid, UInt begid );
        void pause( UInt userid );

    public:
        push_mgr()
            // : redis_ctx_(redis::context::make())
        {}

        void init( boost::asio::io_service& io_service, const boost::property_tree::ptree& ini );

    private:
        friend class jpush_msgr;
        // friend class apush_msgr;
        friend class ios_push_client;
        friend class ios_push_mgr;

    private:
        void load_all_msgs();
        UInt load_usr_msgs( UInt uid, UInt begid )
        {
            return load_usr_msgs( clients::inst().find_client( uid ), begid );
        }

        UInt load_usr_msgs( clients::iterator it  , UInt begid=0 );


        bool get_one_msg( channel ch, std::string& tok, s_push_msg& m, UInt& num );
        // bool get_one_msg( UInt mid, std::string& tok, s_push_msg& m, UInt& num );
        UInt insert_msg( clients::iterator it, const std::vector<s_push_msg>& );
        void del_one_msg( const s_push_msg& m, channel ch );

        std::string push_msg_key( UInt uid ) 
        { 
            return boost::lexical_cast<std::string>(uid) + "_push_msg"; 
        }

        void fev_regist(UInt uid, int online);
        void fev_online(UInt uid, int regist, int aclose);

        msg_type notifications_;
        boost::mutex  mutex_;
        typedef boost::bimap< UInt, std::string > b_uid_tokens_type;
        typedef b_uid_tokens_type::value_type uid_token_type;
        b_uid_tokens_type uid_toks;

        std::set<UInt> ids_arry[ CH_MAX ];
        std::map<UInt, UInt> pushed_nums;

    public:
        // bool get_one_msg( channel ch, std::string& tok, s_push_msg& m, UInt& num );
        // void del_one_msg( const s_push_msg& m, channel ch );
        void add_reject( UInt uid ) { rejects_.add( uid ); }
        void remove_reject( UInt uid ) { rejects_.remove( uid ); }
    private:
        reject_push_users rejects_;
    private:
        push_count_record valid_push_users_;
    private:
        bool check_push_user( UInt uid ) const
        {
            return check_push_user( clients::inst().find_client( uid ) );
        }
        
        bool check_push_user( clients::iterator it ) const
        {
            bool ret = true;
            if ( it == clients::inst().end() || it->second.get_devtoken().empty() ) {
                ret = false;
            }

            return ret;
        }
};

#endif
