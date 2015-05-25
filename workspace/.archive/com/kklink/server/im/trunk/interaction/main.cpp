#include <boost/asio.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "dbc.h"
#include "log.h"
#include "mycurl.h"
#include "users.h"
#include "task.h"
#include "interaction.h"

boost::property_tree::ptree getconf(boost::filesystem::ifstream &cf, const char* sec)
{
    boost::property_tree::ptree conf;
    boost::property_tree::ini_parser::read_ini(cf, conf);
    if (sec) { return conf.get_child(sec); }
    return conf;
}

int main( int argc, char* argv[] )
{
    boost::filesystem::ifstream cf("/etc/yxim.conf");
    if ( !cf ) {
        LOG_I<< "invalid config file";
    }

    auto confs = getconf( cf, 0 );
    sql::dbc::init( sql::dbc::config( confs.get_child( "mysql" ) ) );
    CurlHandle ch;
    UsrMgr um;

    // interaction( 1125490, 10014, 0, " + 林 + 都+ " );

    boost::asio::io_service io_serv;
    BasicTask bt( io_serv );
    DailyTask dt( io_serv );
    io_serv.run();

    return 0;
}
