#include <arpa/inet.h>
#include <iostream>
#include <boost/range.hpp>
#include <boost/filesystem/fstream.hpp>
#include "json.h"

std::string readtxt(std::istream & in)
{
    std::string cont;
    std::string l;
    while (getline(in, l))
        cont += l;
    return cont;
}

std::string read_cont(int argc, char* const argv[])
{
    if (argc == 1)
        return readtxt(std::cin);

    boost::filesystem::ifstream in(argv[1]);
    if (!in)
        exit(3);
    return readtxt(in);
}

int main(int argc, char* const argv[])
{
    std::string cont = read_cont(argc, argv);
    //std::istream_iterator<char> it(in), iend;
    //std::vector<char> cont(it, iend);
    json::object jo = json::decode<json::object>(cont).value();

    std::string ret = json::encode(jo);
    union {
        uint32_t len;
        char s[sizeof(uint32_t)];
    } u;
    u.len = ntohl(ret.size());
    std::cout.write(u.s, sizeof(u));
    std::cout.write(ret.data(), ret.size());
    return 0;
}

