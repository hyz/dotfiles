#include <stdio.h>
#include <string>

std::string decode64(const std::string &val);
std::string encode64(const std::string &val);

int main(int ac, char* const av[])
{
    std::string s = encode64(av[1]);
    printf("%s\n", s.c_str());
    s = decode64(s);
    printf("%s\n", s.c_str());
}

