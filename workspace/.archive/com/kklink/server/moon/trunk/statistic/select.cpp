#include <stdlib.h>
#include <iostream>
#include <string>
#include "dbc.h"
using namespace std;
using namespace boost;
const char *conf = "whisper.conf";
static void initcfg()
{
    boost::property_tree::ptree ini;
    string path(getenv("HOME"));
    path += "/";
    path += conf;
    std::ifstream ifs(path.c_str());
    boost::property_tree::ini_parser::read_ini(ifs, ini);

    //database
    sql::dbc::init(ini.get_child("database"));
}
int main(int argc, char *argv[])
{
    if( argc < 2) cout<<"usage: no less than 2 parameters needed!"<<endl;
    cout<<"statement:"<<argv[1]<<endl;
    initcfg();
    sql::datas data(argv[1]);
    cout<<data<<endl;
    return 0;
}
