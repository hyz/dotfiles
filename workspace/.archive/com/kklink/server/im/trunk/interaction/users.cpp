#include <set>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "dbc.h"
#include "users.h"

std::vector<UInt> UsrMgr::im_usr( time_t gap ) 
{
    std::set<UInt> actives;
    const char* sql = "SELECT DISTINCT( %1% ) from message_rx WHERE time >FROM_UNIXTIME( UNIX_TIMESTAMP()-%2% )";
    sql::datas datas1( boost::format( sql ) % "uid" %gap );
    while( sql::datas::row_type row = datas1.next() ) { 
        UInt u = boost::lexical_cast<UInt>( row.at(0,"0") );
        if ( 0 != u ) { actives.insert( u ); }
    }  

    sql::datas datas2( boost::format( sql ) %"from_uid" %gap );
    while( sql::datas::row_type row = datas2.next() ) { 
        UInt u = boost::lexical_cast<UInt>( row.at(0,"0") );
        if ( 0 != u ) { actives.insert( u ); }
    }  

    LOG_I<< actives;
    return std::vector<UInt>( actives.begin(), actives.end() );
}

std::vector<UInt> UsrMgr::getnewuser( time_t bt, time_t et, int sex )
{
    std::vector<UInt> users;
    const char* url = "http://192.168.10.245/im/getNewUser?btime=%1%&etime=%2%&sex=%3%";
    auto rp = php_get( ( boost::format( url ) % bt %et %sex ).str() );
    json::array us = json::as<json::array>( rp, "users" ).value_or( json::array() );

    BOOST_FOREACH( const auto& a, us ) {
        users.push_back( json::value<UInt>( a ) );
    }

    return users;
}

std::vector<UInt> UsrMgr::getactiveuser( int num, int sex )
{
    std::vector<UInt> users;
    const char* url = "http://192.168.10.245/im/getActiveUser?n=%1%&sex=%2%";
    auto rp = php_get( ( boost::format( url ) % num %sex ).str() );
    json::array us = json::as<json::array>( rp, "users" ).value_or( json::array() );

    BOOST_FOREACH( const auto& a, us ) {
        users.push_back( json::value<UInt>( a ) );
    }

    return users;
}
