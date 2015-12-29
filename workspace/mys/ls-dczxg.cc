#include <stdio.h>
#include <vector>
#include <string>
#include <boost/spirit/include/qi.hpp>

int main(int argc, char* const argv[])
{
    static const char kZXG[] = "自选股"; //UTF-16//{ '\xea', '\x81', '\x09', '\x90', '\xa1', '\x80' }
    auto print = [](int x){ printf("%06d\n", x); };

    char linebuf[1024*8-8];
    while (fgets(linebuf, sizeof(linebuf), stdin)) {
        char const* end = &linebuf[strlen(linebuf)];
        char const* it = linebuf;
        it = std::search(it, end, &kZXG[0], &kZXG[sizeof(kZXG)-1]);
        if (it < end) {
            it += sizeof(kZXG)-1;
            if ( *it++ == '=' /*(it = std::find(it, end, '=')) < end*/) {
                using namespace boost::spirit;
                qi::phrase_parse(it, end, (ascii::digit >> '.' >> qi::int_[print]) % ',', ascii::space);

                return 0; //break; // Only first group
            }
        }
    }
    return 1;
}

// zxg-diff "StockwayStock.ini" "99/lis"

//#include <boost/function_output_iterator.hpp>
//#include <set>
    //std::set<int> dfzxg = east::get_ZXG(argv[1]);
    //std::set<int> ss = read_ints(argv[2]);
    //std::set_difference(dfzxg.begin(), dfzxg.end(), ss.begin(), ss.end()
    //        , boost::make_function_output_iterator(print()));

//#include <boost/locale/encoding_utf.hpp>
        //std::string utf8 = boost::locale::conv::utf_to_utf<char>
        //    (reinterpret_cast<const char16_t*>(it), reinterpret_cast<const char16_t*>(end));
        //std::cerr << utf8 <<"\t[]\n";
        //it = &utf8[0];
        //end = &utf8[utf8.size()];

