#ifndef __IM_PROXY_H__
#define __IM_PROXY_H__ 

#include <netinet/in.h>

#include <boost/asio.hpp>
#include <boost/function.hpp>

#include "json.h"

struct Protocol
{
    union Len {
        uint32_t len;
        char s[sizeof(uint32_t)];
    } u;

    std::string data;

    std::string pack() const
    {
        return std::string( u.s, sizeof( u.s ) ) + data;
    }

    Protocol() { u.len = 0; }

    Protocol( const std::string& d ) 
    {
        data = d;
        u.len = htonl( data.size() );
    }

    template <typename Val> Protocol( Val const& jv )
    {
        data = json::encode(jv);
        u.len = htonl( data.size() );
    }
};

class Virtual_User_Proxy
{
    public:
        static Virtual_User_Proxy& init( uint32_t uid=0, const std::string& token="", 
                boost::asio::io_service* io_sev=NULL );
        static Virtual_User_Proxy& inst( uint32_t uid=0, const std::string& token="",
                boost::asio::io_service* io_sev=NULL );

        void verify();
        void start_timer();
        void proxy_msg( const std::string& );

    private:
        void hello( const boost::system::error_code& ec );
        Virtual_User_Proxy( uint32_t uid, const std::string& token, boost::asio::io_service* io_sev )
            : timer_( *io_sev )
        {
            uid_ = uid;
            token_ = token;
        }

        uint32_t uid_;
        std::string token_;
        boost::asio::deadline_timer timer_;
};

class Proxy_Handle_Mgr
{
    public:
        typedef boost::function< void( const std::string& ) > Pfunc;
        enum Handle_Key { VERIFY=99, PROXY_MSG=200 };

        static Proxy_Handle_Mgr& inst( Pfunc f=0 );
        static Proxy_Handle_Mgr& init( Pfunc f=0 );

        void dispatch( const std::string& in );
        void send( const std::string& result )
        {
            func_( result );
        }

        void hello()
        {
            Virtual_User_Proxy::inst().verify();
            Virtual_User_Proxy::inst().start_timer();
        }

    private:
        Proxy_Handle_Mgr( Pfunc f ) : func_( f ) {}
        Pfunc func_;
};
#endif
