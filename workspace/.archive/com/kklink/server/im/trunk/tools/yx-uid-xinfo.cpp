#include <vector>
#include <set>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/range.hpp>
#include "dbc.h"

template <typename R> std::string sql_s(R const& r)
{
    if (boost::empty(r))
        return "";

    std::vector<std::string> cond;
    BOOST_FOREACH(auto &x, r) {
        cond.push_back(str(boost::format("t1.userid=%1%") % x));
    }
    //auto it = boost::begin(r), end = --boost::end(r);
    //for (; it != end; ++it)
    //    cond += str(boost::format("t1.userid=%1% OR ") % *it);
    //cond += str(boost::format("t1.userid=%1%") % *it);

    return "SELECT t1.userid,nick,app_version,app_type"
        " FROM token t1 LEFT JOIN users t2 on t1.userid=t2.userid"
        " WHERE " + boost::algorithm::join(cond, " OR ");
}

std::set<unsigned int> read_uids(std::istream& in_f)
{
    std::set<unsigned int> us;
    std::string l;
    while (std::getline(in_f, l)) {
        if (unsigned int uid = atoi(l.c_str()))
            us.insert(uid);
    }
    return us;
}

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

template <typename Uids> void Main(Uids uids)
{
    std::cout<< "(int,'uid')	(str,'nick')	(str,'ver')	(int,'x')\n";
    sql::datas datas(sql_s(uids));
    while (sql::datas::row_type row = datas.next()) {
        std::cout << row[0]
            << "\t" << boost::algorithm::replace_all_copy(std::string(row[1]), "\t", " ")
            << "\t" << row[2]
            << "\t" << row[3]
            << "\n";
    }
    //SELECT t1.userid,nick,app_version,app_type FROM ;
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

    Main( read_uids(std::cin) );
    return 0;
}

