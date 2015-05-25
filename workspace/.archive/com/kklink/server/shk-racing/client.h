#ifndef  __CLIENT_H__
#define  __CLIENT_H__

#include <boost/unordered_map.hpp>

#include <string>
#include <ostream>

#include "json.h"
#include "dbc.h"

typedef unsigned int UID;

struct client_info
{
    enum { DevType_Invalid=0, DevType_iPhone=1, DevType_iPad=2, DevType_Android=3 };
    enum PUSH_STATUS{PUSH_DISABLE=0, PUSH_ENABLE };

    int get_flag() const { return push_flag_; }

    int get_devtype() const { return devtype_; }

    std::string const&  get_devtoken() const { return devtoken_; }

    std::string const&  get_token() const { return token_; }

    std::string const&  get_bundleid() const { return bundleid_; }

    int get_uid() const { return uid_; }

    json::object to_json() const 
    {
        json::object ret;
        json::insert(ret)
            ("uid", uid_)("token", token_)
            ("devtoken", devtoken_)
            ("bundleid", bundleid_)
            ("devtype", devtype_)
            ("push_flag", push_flag_)
            ;
        return ret;
    }

    private:
    friend class clients;

    client_info( UID userid, std::string const& tok, const std::string& bundleid )
    {
        uid_ = userid;
        token_ = tok;
        devtype_ = DevType_Invalid;
        devtoken_ = "";
        bundleid_ = bundleid;
        push_flag_ = PUSH_DISABLE;
    }

    client_info( json::object& obj );

    UID uid_;
    std::string token_;

    std::string devtoken_;
    std::string bundleid_;
    signed char devtype_;
    char push_flag_;   // 0 push, 1 im;

    friend std::ostream& operator<<(std::ostream& out, client_info const& ci)
    {
        return out << ci.uid_
            <<" "<< ci.token_
            <<" "<< int(ci.devtype_)
            <<" "<< ci.devtoken_
            <<" "<< ci.bundleid_
            <<" "<< int(ci.push_flag_)
            ;
    }
};

class clients
{
    public:
        typedef boost::unordered_map<UID, client_info> client_type;
        static clients& inst();

        typedef client_type::iterator iterator;

        client_type::iterator add_client( UID uid, const std::string& token ,const std::string& bundleid);

        void delete_client( UID uid  );
        bool set_device( UID uid, int devtype, const std::string& devtok, const std::string& bundleid="" );
        bool set_push(UID uid, int flag );
        bool set_push( iterator it, int flag );

        client_type::iterator find_client( UID uid );

        client_type::iterator begin() { return clients_.begin(); }
        client_type::iterator end() { return clients_.end(); }

        bool isexist( UID uid )
        {
            return find_client( uid ) != clients_.end();
        }

    private:
        clients(){ isload = false; }
        client_type clients_;
        bool isload;
        static const char* red_key;

        void load();

        void redis_add( const client_info& cli )
        {
            // std::ostringstream os;
            // os << cli.to_json();
            // redis::command("HSET", red_key, cli.uid_, os.str() ); 
            redis::command("HSET", red_key, cli.uid_, json::encode(cli.to_json()) ); 
        }

        void redis_remove( UID uid )
        {
            redis::command("HDEL", red_key, uid);
        }
};

#endif
