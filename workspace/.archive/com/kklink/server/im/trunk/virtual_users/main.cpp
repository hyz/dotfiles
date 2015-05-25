#include <string>
#include <vector>
#include <iostream>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "log.h"
#include "async.h"
#include "mycurl.h"
#include "im_proxy.h"

struct config
{
    std::string im_host;
    std::string im_port;
    std::vector<std::string> bs_header;
    std::string bs_url;

    unsigned int user;
    std::string token;
    friend std::ostream& operator<< ( std::ostream& out, const config& cf );
} conf_;

std::ostream& operator<< ( std::ostream& out, const config& cf )
{
    out<< cf.im_port<< " / "<< cf.im_host<< " / "
        << " / "<< cf.bs_url<< " / "<< cf.user<< " / "<< cf.token;
    out<< " / header:";
    for ( std::vector<std::string>::const_iterator it = cf.bs_header.begin();
        it != cf.bs_header.end(); ++it ) {
        out<< *it<< ",";
    }

    return out;
}

boost::property_tree::ptree getconf( const std::string& fn, const std::string& sec )
{
    boost::property_tree::ptree conf;
    LOG<< "filename:"<< fn<< " section:"<< sec;
    boost::filesystem::ifstream confs( fn );
    if ( confs ) {
        boost::property_tree::ini_parser::read_ini( confs, conf );
        if ( !sec.empty() ) {
            return conf.get_child( sec );
        }
    }

    return conf;
}

void init_conf( const std::string& fn, const std::string& sec="" )
{
    auto cf = getconf( fn, sec );

#ifdef DEBUG
    conf_.im_host = cf.get<std::string>("IM_DEBUG_HOST");
    conf_.im_port = cf.get<std::string>("IM_DEBUG_PORT");
    conf_.bs_url = cf.get<std::string>("BS_DEBUG_URL");
    std::string headers = cf.get<std::string>("BS_DEBUG_HEADER");
    boost::split( conf_.bs_header, headers, boost::is_any_of(",") );
#else
    conf_.im_host = cf.get<std::string>("IM_HOST");
    conf_.im_port = cf.get<std::string>("IM_PORT");
    conf_.bs_url = cf.get<std::string>("BS_URL");
    std::string headers = cf.get<std::string>("BS_HEADER");
    boost::split( conf_.bs_header, headers, boost::is_any_of(",") );
#endif

    conf_.user = cf.get<unsigned int>("user");
    conf_.token = cf.get<std::string>("token");
}

int main( int argc, char* argv[] )
{
    boost::asio::io_service io_service;

    boost::program_options::options_description generic( "Generic options" );
    generic.add_options()
        ( "help,h", "print this help message" )
        ( "daemon,d", "execute as daemon" )
        ( "config,c", boost::program_options::value<std::string>(), "name of a file of a configuration." );

    boost::program_options::variables_map vm;
    boost::program_options::store( 
            boost::program_options::command_line_parser( argc, argv ).options( generic ).run(), vm );
    boost::program_options::notify( vm );

    if ( vm.count( "help" ) ) {
        LOG<< generic;
        return 0;
    }

    if ( !vm.count( "config" ) ) {
        LOG<< "file of configuration not specified";
        LOG<< "Usage: init <filename> [ --daemon ]";
        return 1;
    }

    init_conf( vm["config"].as<std::string>() );

    if ( vm.count( "daemon" ) ) {
        LOG<< "to be executed as daemon";
        logging::syslog( LOG_PID|LOG_CONS, 0 );
        if ( 0 != daemon( 1, 0 ) ) {
            LOG << "daemon" << strerror(errno);
        }
    }

    LOG_I << conf_;
    IM_Conn im_connection( io_service, conf_.im_host, conf_.im_port );
    Proxy_Handle_Mgr::init( boost::bind( &IM_Conn::send, &im_connection, _1 ) );
    Virtual_User_Proxy::init( conf_.user, conf_.token, &io_service );
    Curl_Handle::init( conf_.bs_url, conf_.bs_header );

    im_connection.start();

    io_service.run();

    return 0;
}
