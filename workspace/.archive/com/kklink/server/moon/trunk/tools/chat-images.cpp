#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>

#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
// #include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <mysql/mysql.h>
#include "dbc.h"
#include "jss.h"

using namespace std;
using namespace boost;

boost::property_tree::ptree getcfg(std::string cfg, const char* sec)
{
    boost::property_tree::ptree empty, ini;

    std::ifstream ifs(cfg.c_str());
    boost::property_tree::ini_parser::read_ini(ifs, ini);

    return ini.get_child(sec, empty);
}

typedef boost::tuple<std::string,std::string> url_info;

inline std::ostream & operator<<(std::ostream & out, url_info const & m)
{
    return out << get<0>(m) <<"\t"<< get<1>(m);
}

struct image_user
{
    int count;
    int uid;
    std::string nick;
    std::vector<url_info> urls;

    image_user(int uid_, std::string nick_)
    {
        count = 0;
        uid = uid_;
        nick = nick_;
    }
    image_user() { count=0; uid=0; }
};

int main(int ac, char *const av[])
{
    std::string conf("/etc/moon.conf");
    std::string tmbegin(av[1]);
    std::string tmend(av[2]);
    //std::cout << conf <<"\n"; std::cout << tmbegin <<"\n"; std::cout << tmend <<"\n";

    logging::setup(&std::cerr, LOG_PID|LOG_CONS, 0);
    sql::initializer dbc_iniobj( getcfg(conf, "database") );

    std::map<int, image_user, greater<int> > mapc;

    sql::datas datas(format("SELECT MsgTime,users.UserId,nick,content"
                " FROM messages INNER JOIN users ON messages.UserId=users.UserId"
                " WHERE SessionId='%1%' AND MsgType='%2%' AND MsgTime>='%3%' AND MsgTime<'%4%'")
            % "KK1007A" % "chat/image" % tmbegin % tmend);
    while (sql::datas::row_type row = datas.next())
    {
        std::string tm = row.at(0);
        int uid = lexical_cast<int>(row.at(1));
        std::string nick = row.at(2);
        json::object body = json::decode(row.at(3));
        std::string url = body.get<std::string>("content");

        auto itr = find_first(url, "/file/");
        url.erase(url.begin(), itr.end());

        auto p = mapc.insert(make_pair(uid, image_user()));
        if (p.second)
            p.first->second = image_user(uid, nick);
        p.first->second.count++;
        p.first->second.urls.push_back(url_info(tm, url));
    }

    std::map<int, image_user, greater<int> > sortc;

    for (auto i = mapc.begin(); i != mapc.end(); ++i)
    {
        int n = i->second.count;
        swap(sortc[n], i->second);
    }

    std::vector<url_info> urls;

    for (auto i = sortc.begin(); i != sortc.end(); ++i)
    {
        image_user & img = i->second;
        std::cout << img.uid <<"/"<< img.nick
            <<"\t"<< img.count
            <<"\n";
        urls.insert(urls.end(), img.urls.begin(), img.urls.end());
    }
    std::cout << "\n";

    for (auto i = urls.begin(); i != urls.end(); ++i)
    {
        std::cout << *i << "\n";
    }
    std::cout << "\n";

    return 0;
}

