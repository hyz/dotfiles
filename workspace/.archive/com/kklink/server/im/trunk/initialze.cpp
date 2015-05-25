#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <string>

#include "initialze.h"
#include "service.h"
#include "log.h"
#include "push.h"

using namespace std;
using namespace boost;

// void test_conf( string conf )
// {
//     LOG_I<<"db:";
//     {
//         auto cf = getcfg( conf, "database");
//         string host_ = cf.get<string>("host", "192.168.1.57");
//         int port_ = cf.get<int>("port", 3307);
//         string user_ = cf.get<string>("user", "alindu");
//         string password_ = cf.get<string>("password", "alindu12345");
//         string dbname_ = cf.get<string>("db", "alindu_moon");
//         LOG_I<<" host:"<<host_<<" port:"<<port_<<" user:"<<user_<<" password:"<<password_<<" dbname:"<<dbname_;
//     }
//     LOG_I<<"network:";
//     {
//         auto cf = getcfg( conf, "network");
//         int listen_ = cf.get<int>("listen", 1);
//         int port_ = cf.get<int>("port",9000);
// 
//         LOG_I<<"listen:"<<listen_<<" " <<" port:"<<port_;
//     }
// }

initializer::initializer( const std::string& fn)
    // : dbc_(getcfg(fn, "database"))
{
    // service_mgr::inst().initialize(getcfg(fn, "network"));
}

