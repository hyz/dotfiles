#include <stdio.h>
#include <algorithm>
#include <set>
#include <boost/function_output_iterator.hpp>
//#include <string>
//#include <iostream>

static std::set<int> read_ints(char const* fn)
{
    std::set<int> ints;
    if (FILE* fp = fopen(fn, "r")) {
        char linebuf[1024*8];
        while (fgets(linebuf, sizeof(linebuf), fp)) {
            ints.insert(atoi(linebuf));
        }
        fclose(fp);
    }
    return std::move(ints);
}

int main(int argc, char* const argv[])
{
    try {
        auto print = [](int x){ printf("%06d\n", x); };

        if (argc == 3) {
            std::set<int> s0 = read_ints(argv[1]);
            std::set<int> s1 = read_ints(argv[2]);
            std::set_difference(s0.begin(), s0.end(), s1.begin(), s1.end()
                , boost::make_function_output_iterator(print));
            return 0;
        }
    } catch (std::exception const& e) {
        fprintf(stderr,e.what());
    }
    return 1;
}

