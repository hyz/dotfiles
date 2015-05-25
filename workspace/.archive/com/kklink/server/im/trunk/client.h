#ifndef  __CLIENT_H__
#define  __CLIENT_H__

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <string>
#include <ostream>

#include "json.h"
#include "dbc.h"
#include "singleton.h"

typedef unsigned int UID;

struct client_info
{
    enum { DevType_Invalid=0, DevType_iPhone=1, DevType_iPad=2, DevType_Android=3 };
    // enum Socket_Status{ NON_CONN=0, CONN, _MAX };

    // int socket_flag() const { return socket_flag_; }
    int get_devtype() const { return devtype_; }

    std::string const&  get_devtoken() const { return devtoken_; }
    std::string const&  get_token() const { return token_; }
    std::string const&  get_bundleid() const { return bundleid_; }
    UID get_uid() const { return uid_; }

    json::object to_json() const 
    {
        json::object ret;
        json::insert(ret)
            ("uid", uid_)("token", token_)
            ("devtoken", devtoken_)
            ("bundleid", bundleid_)
            ("devtype", devtype_)
            ;
        return ret;
    }

    private:
    friend class clients;

    client_info( UID userid, std::string const& tok, const std::string& bundleid )
    {
        uid_ = userid;
        token_ = tok;
        bundleid_ = bundleid;

        devtoken_ = "";
        // socket_flag_ = NON_CONN;
        devtype_ = DevType_Invalid;
    }

    client_info( const json::object& obj );

    UID uid_;
    std::string token_;

    std::string devtoken_;
    std::string bundleid_;
    signed char devtype_;
    // Socket_Status socket_flag_;   // 0 out of connect, 1 connected;

    friend std::ostream& operator<<(std::ostream& out, client_info const& ci)
    {
        return out << ci.uid_
            <<" "<< ci.token_
            <<" "<< int(ci.devtype_)
            <<" "<< ci.devtoken_
            <<" "<< ci.bundleid_
            // <<" "<< int( ci.socket_flag_ )
            ;
    }
};

class clients
{
    public:
        typedef boost::unordered_map<UID, client_info> client_type;
        static clients& inst();

        typedef client_type::iterator iterator;

        void delete_client( UID uid  );
        client_type::iterator add_client( UID uid, const std::string& token ,const std::string& bundleid);
        bool set_device( UID uid, int devtype, const std::string& devtok, const std::string& bundleid="" );

        client_type::iterator find_client( UID uid, bool load = false );
        // client_type::iterator find_client( UID uid )
        // {
        //     return clients_.find( uid );
        // }

        client_type::iterator begin() { return clients_.begin(); }
        client_type::iterator end() { return clients_.end(); }

        //  bool set_socket_flag(UID uid, client_info::Socket_Status flag )
        //  {
        //      auto i = find_client( uid );
        //      return set_socket_flag ( i, flag );
        //  }

        //  bool set_socket_flag(iterator it, client_info::Socket_Status flag )
        //  {
        //      bool ret = false;
        //      if ( it != clients_.end() ) {
        //          it->second.socket_flag_ = flag;
        //          ret = true;
        //      }

        //      return ret;
        //  }

    private:
        client_type clients_;
        static const char* red_key;

        void load();
        clients::iterator load_user( UID uid );

        void redis_add( const client_info& cli )
        {
            redis::command("HSET", red_key, cli.uid_, json::encode(cli.to_json()) ); 
        }

        void redis_remove( UID uid )
        {
            redis::command("HDEL", red_key, uid);
        }
};

class virtual_user_mgr : public singleton<virtual_user_mgr>
{
    public:
        typedef boost::unordered_set<UID>::iterator iterator;
        typedef boost::unordered_set<UID>::const_iterator const_iterator;

        virtual_user_mgr() { load(); }
        bool is_exist( UID uid ) const
        {
            return ( virtuals_.end() != virtuals_.find( uid ) );
        }

        iterator add( UID uid, bool store=true )
        {
            auto ret = virtuals_.insert( uid );
            if ( store && ret.second ) {
                redis::command("SADD", virtual_user_set_key_, uid);
            }

            return ret.first;
        }

        void remove( UID uid )
        {
            if ( virtuals_.erase( uid ) ) {
                redis::command("SREM", virtual_user_set_key_, uid);
            }
        }
    private:
        void load() 
        {
            auto reply = redis::command("SMEMBERS", virtual_user_set_key_);
            if (!reply || reply->type != REDIS_REPLY_ARRAY)
                return;

            LOG << reply->elements;
            for ( size_t i = 0; i< reply->elements; ++i ) {
                virtuals_.insert( boost::lexical_cast< UID>( reply->element[i]->str ) );
            }

        }

        boost::unordered_set<UID> virtuals_;
        static const char* virtual_user_set_key_;
};

class PuppetAgent : public singleton<PuppetAgent>
{
    public:
        PuppetAgent() { load(); }

        bool is_empty() const { return agents_.empty(); }

        UID get_agent( UID uid ) {
            if ( agents_.empty() ) { return 0; }
            if ( agents_.end() != std::find( agents_.begin(), agents_.end(), uid ) ) {
                return uid;
            }

            int s = agents_.size();
            int index = uid % s;
            return agents_[index];
        }

    private:
        void load() {
            auto reply = redis::command( "SMEMBERS", puppet_agent_set_key_ );
            if ( !reply || reply->type != REDIS_REPLY_ARRAY ) { return; }

            LOG << reply->elements;
            for ( size_t i = 0; i< reply->elements; ++i ) {
                auto v = boost::lexical_cast< UID>( reply->element[i]->str );
                agents_.push_back( v );
                virtual_user_mgr::instance().add( v, false );
            }

        }

        std::vector<UID> agents_;
        static const char* puppet_agent_set_key_;
};
#endif
