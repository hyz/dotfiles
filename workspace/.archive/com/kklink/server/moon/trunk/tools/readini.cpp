#include <string>
#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace std;
using namespace boost;

int main(int argc, char* const argv[])
{
    if (argc != 3)
        return 3;

    property_tree::ptree pt;
    ifstream f(argv[1]);
    property_tree::ini_parser::read_ini(f, pt);

    cout << pt.get<string>(argv[2], "");

    return 0;
}

