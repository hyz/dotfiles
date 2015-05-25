#include <vector>
#include <set>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/range.hpp>
#include "dbc.h"

boost::property_tree::ptree getconf(boost::filesystem::ifstream &cf, const char* sec)
{
    boost::property_tree::ptree conf;
    boost::property_tree::ini_parser::read_ini(cf, conf);
    if (sec)
    {
        // boost::property_tree::ptree empty;
        return conf.get_child(sec);
    }
    return conf;
}

void Main(std::istream& in_f)
{
    std::string l;
    while (std::getline(in_f, l))
    {
        sql::datas datas( l );
        while (sql::datas::row_type row = datas.next()) {
            if (size_t nf = datas.count_fields()) {
                for (size_t i = 1; i < nf; ++i) {
                    std::cout << row[i-1] <<"\t";
                }
                std::cout << row[nf-1] << "\n";
            }
        }
    }
}

int main(int argc, char const* argv[])
{
    if (argc != 2) {
        exit(1);
    }
    boost::filesystem::ifstream conf(argv[1]);
    if (!conf) {
        exit(2);
    }

    boost::property_tree::ptree cfs = getconf(conf, "mysql");
    sql::dbc::init( sql::dbc::config(cfs) );

    Main( std::cin );
    return 0;
}

