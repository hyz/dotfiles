#include "client.h"
#include "json.h"
#include "log.h"
#include "push.h"

const char* clients::red_key = "users"; // "h_json_clients"
const char* virtual_user_mgr::virtual_user_set_key_ = "moonv2:virtual_user_set";
const char* PuppetAgent::puppet_agent_set_key_ = "moonv2:puppet_agent_set";

using namespace std;

clients& clients::inst()
{
    static clients cli;
    return cli;
}

client_info::client_info( const json::object& jo )
{
    uid_ = json::value<UID>(jo,"uid");
    token_ = json::value<string>(jo,"token"); 
    devtoken_ = json::value<string>(jo,"devtoken"); 
    devtype_ = json::value<int>(jo,"devtype");
    bundleid_ = json::as<string>(jo,"bundleid").value_or(bundleid_);
}

clients::client_type::iterator clients::add_client( UID uid, const std::string& token, const std::string& bundleid )
{
    client_info cli(uid, token, bundleid);

    auto i = clients_.insert( make_pair(uid,  cli) );
    if ( !i.second ) {
        i.first->second = cli;
    }

    redis_add( i.first->second );
    return i.first;
}

void clients::delete_client( UID uid )
{
    LOG << uid<<" is been deleting.....";
    if (uid > 0) {
        clients_.erase( uid );
        redis_remove( uid );
    }
}

clients::iterator clients::find_client( UID uid, bool load )
{
    auto it = clients_.find( uid );
    if ( it == clients_.end() && load ) {
        it = load_user( uid );
    }

    return it;
}

clients::iterator clients::load_user( UID uid )
{
    auto reply = redis::command( "HMGET", red_key, uid );
    if ( !reply || reply->type != REDIS_REPLY_ARRAY 
            || 0 == reply->elements || !( reply->element[0]->str ) ) {
        LOG_I << "Unable to find info of userid:"<< uid;
        return clients_.end();
    }
    
    client_info cli( json::decode<json::object>( reply->element[0]->str ).value() );

    return clients_.insert(make_pair(cli.uid_, cli)).first;
}

bool clients::set_device( UID uid, int devtype, const std::string& devtok, const std::string& bundleid)
{
    bool ret = false;
    if ( devtype != client_info::DevType_iPhone 
            && devtype != client_info::DevType_iPad
            && devtype != client_info::DevType_Android ) { 
        LOG << "devtype fail";
        return false; 
    }

    if ( devtype == client_info::DevType_iPhone
            || devtype == client_info::DevType_iPad ) {
        if ( devtok.length() != 64 ) {
            LOG << devtok.length()<<" token invalid";
            return false;
        }

        push_mgr::instance().remove_dev( devtok );
    }

    auto i = find_client( uid );
    if ( i != clients_.end() ) { 
        auto& cli = i->second;
        if ( cli.devtype_ != devtype 
                || cli.devtoken_ != devtok 
                || cli.bundleid_ != bundleid ) {
            LOG_I << "uid:"<< uid<< " devtype:"<<devtype<<" devtok:"<<devtok<<" bundleid:"<<bundleid;
            cli.devtype_ = devtype ;
            cli.devtoken_ = devtok ;
            cli.bundleid_ = bundleid;

            redis_add( cli );
        }

        ret = true;
    } else {
        LOG_I<<"uid:"<<uid<<" has not been registed";
    }

    return ret;
}
