#include <arpa/inet.h>
#include <iostream>
#include <boost/range.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include "json.h"

std::string readtxt(std::istream & in)
{
    std::string cont;
    std::string l;
    while (getline(in, l))
        cont += l;
    return cont;
}

namespace json {
    extern void pretty_print(FILE* out_f, json::variant const & jv);
}

std::istream& skip_n(std::istream& in_f, int bytes)
{
    std::vector<char> tmp(bytes);
    in_f.read(&tmp[0], bytes);
    return in_f;
}

int Main(std::istream& in_f)
{
    std::string cont = readtxt(in_f);

    //std::cout << cont << "\n";
    json::variant var;
    if (json::_decode(var, cont.data(), cont.data() + cont.length())) {
        json::pretty_print(stdout, var);
        fprintf(stdout, "\n");
    }
	return 0;
}

int main(int argc, char* const argv[])
{
    int skip = 4;
    std::string filename;

    {
        namespace opt = boost::program_options;

        opt::options_description opt_desc("Options");
        opt_desc.add_options()
            ("skip,s", opt::value<int>(&skip)->default_value(skip), "skip bytes")
            ("file,f", opt::value<std::string>(&filename), "filename")
            ;
        opt::positional_options_description pos_desc;
        pos_desc.add("file", -1);

        opt::variables_map vm;
        opt::store(opt::command_line_parser(argc, argv).options(opt_desc).positional(pos_desc).run(), vm);
        opt::notify(vm);
    }

    if (!filename.empty()) {
        boost::filesystem::ifstream in_f(filename);
        return Main(skip_n(in_f, skip));
    }
    return Main(skip_n(std::cin, skip));
}

