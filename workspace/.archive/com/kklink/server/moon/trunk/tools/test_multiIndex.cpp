#include "test_multiIndex.h"

boost::property_tree::ptree getcfg(std::string cfg, const char* sec)
{
    boost::property_tree::ptree empty, ini;

    std::ifstream ifs(cfg.c_str());
    boost::property_tree::ini_parser::read_ini(ifs, ini);

    return ini.get_child(sec, empty);
}

int main(int argc, char* argv[])
{
    logging::setup( &std::cout, LOG_PID|LOG_CONS, 0);
    sql::dbc::init( getcfg("/etc/moon.conf", "database") );

    global_mgr inst;
    inst.get_advertising_bycity("南昌市");
    inst.print_advers_bycity("南昌市");
    return 0;
}
