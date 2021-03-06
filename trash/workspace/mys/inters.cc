#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <boost/function_output_iterator.hpp>
//#include <string>
//#include <iostream>

static std::map<int,char*> read_ints(FILE* fp, unsigned count)
{
    std::map<int,char*> ints;
    //if (FILE* fp = fopen(fn, "r")) {
        char linebuf[1024*16];
        while (fgets(linebuf, sizeof(linebuf), fp) && count-- > 0) {
            ints.emplace(atoi(linebuf), strdup(linebuf));
        }
        //fclose(fp);
    //}
    return std::move(ints);
}

int main(int argc, char* const argv[])
{
    try {
        auto comp = [](auto& x,auto& y){ return x.first<y.first; };
        auto print = [](auto& x){ fputs(x.second, stdout); };

        unsigned count=-1;
        switch (argc) {
        case 1: return 1;
        case 2: break;
        case 3:
            count = atoi(argv[2]);
        }

        auto s0 = read_ints(stdin, unsigned(-1));
        auto s1 = read_ints(fopen(argv[1], "r"), count);
        std::set_intersection(
                s0.begin(), s0.end()
                , s1.begin(), s1.end()
                , boost::make_function_output_iterator(print), comp);

        return 0;
    } catch (std::exception const& e) {
        fprintf(stderr,"%s",e.what());
    }
    return 127;
}

