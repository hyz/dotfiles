#ifndef __PUSH_H__
#define __PUSH_H__

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bimap.hpp>

#include <ctime>
#include <string>
#include <istream>
#include <utility>

#include "singleton.h"
#include "dbc.h"
#include "json.h"
#include "client.h"

static const int MAX_PUSH_BYTES = 140;

struct TagMsgId {};
struct TagMyId {};
struct TagUserId {};
struct TagExpiredTime {};

class push_mgr : public singleton<push_mgr>
{
    public:
        typedef unsigned int UInt;

    private:
        enum channel { IOS_RELEASE=1, IOS_ENTERPRISE, ANDROID, CH_MAX};
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
                return out << m.userid <<"/"<< m.msgid <<"/"<< m.expired<< "/"<<m.content;
            }
        };

    public:
        typedef boost::multi_index_container<
            s_push_msg,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagMyId>, BOOST_MULTI_INDEX_MEMBER(s_push_msg, UInt, myid)
                >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagMsgId>, BOOST_MULTI_INDEX_MEMBER(s_push_msg, UInt, msgid)
                >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagUserId>, BOOST_MULTI_INDEX_MEMBER(s_push_msg, UInt, userid)
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

        // bool j_push( const s_push_msg& );
        // bool jpush_get( std::string&, s_push_msg&);
        // void jpush_del(UInt uid, UInt msgid);

        // bool a_push( const s_push_msg& );
        // bool apush_get( std::string&, s_push_msg&);
        // void apush_del(UInt uid, UInt msgid);

    public:
        push_mgr()
            // : redis_ctx_(redis::context::make())
        {}

        void init(const boost::property_tree::ptree& ini);

    private:
        friend class jpush_msgr;
        friend class apush_msgr;
        friend class ios_push_client;
        friend class ios_push_mgr;

    private:
        void load_all_msgs();
        // void load_usr_msgs( UInt uid, UInt devtype, const std::string& token, UInt begid=0);
        void load_usr_msgs( UInt uid, channel ch, UInt begid );
        void load_usr_msgs( const client_info& , UInt begid=0 );
        // void jpush_remove_usr( UInt uid);
        // void apush_remove_usr( UInt uid);

        bool get_one_msg( channel ch, std::string& tok, s_push_msg& m, UInt& num );
        bool get_one_msg( UInt mid, std::string& tok, s_push_msg& m, UInt& num);
            bool insert_msg( const s_push_msg& m );
        // void del_one_msg(UInt uid, UInt msgid, channel ch);
        // void del_one_msg(UInt myid, channel ch);
        void del_one_msg(const s_push_msg& m, channel ch);

        std::string push_msg_key( UInt uid ) 
        { 
            return boost::lexical_cast<std::string>(uid) + "_push_msg"; 
        }

        // msg_type jpushs_, apushs_;
        // boost::mutex j_mutex_,a_mutex_;

        // std::map<UInt, std::string> juid_toks, auid_toks;

        // redis::context redis_ctx_;

        void fev_regist(UInt uid, int online);
        void fev_online(UInt uid, int regist, int aclose);

        msg_type notifications_;
        boost::mutex  mutex_;
        // std::map<UInt, std::string> uid_toks;
        typedef boost::bimap< UInt, std::string > b_uid_tokens_type;
        typedef b_uid_tokens_type::value_type uid_token_type;
        b_uid_tokens_type uid_toks;

        std::set<UInt> android_ids, ios_release_ids, ios_enterprise_ids;
        std::map<UInt, UInt> pushed_nums;
};

#endif

