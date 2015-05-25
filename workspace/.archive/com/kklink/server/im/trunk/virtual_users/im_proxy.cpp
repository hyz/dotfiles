#include<iostream>

#include<boost/bind.hpp>
#include<boost/format.hpp>
#include <boost/timer/timer.hpp>

#include "im_proxy.h"
#include"mycurl.h"
#include"log.h"

Proxy_Handle_Mgr& Proxy_Handle_Mgr::inst( Pfunc f ) 
{ 
    static Proxy_Handle_Mgr inst_( f ); 
    return inst_; 
}

Proxy_Handle_Mgr& Proxy_Handle_Mgr::init( Pfunc f ) 
{ 
    return inst( f );
}

void Proxy_Handle_Mgr::dispatch( const std::string& in )
{
    if ( std::string::npos == in.find( "\"cmd\":99" ) 
            && std::string::npos == in.find( "\"cmd\":199" ) ) {
        Virtual_User_Proxy::inst().proxy_msg( in );
    }
} 

Virtual_User_Proxy& Virtual_User_Proxy::init( uint32_t uid, const std::string& token,
        boost::asio::io_service* io_sev )
{
    return Virtual_User_Proxy::inst( uid, token, io_sev );
}

Virtual_User_Proxy& Virtual_User_Proxy::inst( uint32_t uid, const std::string& token, 
        boost::asio::io_service* io_serv )
{
    static Virtual_User_Proxy inst_( uid, token, io_serv );
    return inst_;
}

void Virtual_User_Proxy::verify()
{
    const char* pverify = "{\"version\":\"1.0.0\",\"cmd\":99,\"body\":{\"appid\":\"yx\",\"uid\":%1%,\"token\":\"%2%\"}}";
    Proxy_Handle_Mgr::inst().send( Protocol( ( boost::format( pverify )%uid_ %token_ ).str() ).pack() );
}

void Virtual_User_Proxy::proxy_msg( const std::string& msg )
{
    LOG_I<< msg;
    boost::timer::auto_cpu_timer t;
    transmit( msg );
}
void Virtual_User_Proxy::start_timer()
{
    timer_.expires_from_now( boost::posix_time::seconds( 50 ) );
    timer_.async_wait( boost::bind( &Virtual_User_Proxy::hello, this, boost::asio::placeholders::error ) ); 
}

void Virtual_User_Proxy::hello( const boost::system::error_code& ec )
{
    if ( ec ) {
        LOG_I<< ec.message();
        if ( boost::asio::error::operation_aborted != ec ) {
            start_timer();
        }
        return;
    }

    const char* phello = "{\"body\":\"\",\"uid\":100,\"cmd\":91,\"version\":\"1.0.0\"}";
    Proxy_Handle_Mgr::inst().send( Protocol( std::string ( phello ) ).pack() );
    start_timer();
}

