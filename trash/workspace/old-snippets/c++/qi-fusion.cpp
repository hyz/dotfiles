#include <iostream>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/adapted/struct.hpp>
namespace qi = boost::spirit::qi;

struct TestStruct {
    int myint;
    double mydouble;
};

BOOST_FUSION_ADAPT_STRUCT(TestStruct, (int, myint)(double, mydouble));

template <typename Iterator, typename Skipper>
struct MyGrammar : qi::grammar<Iterator, TestStruct(), Skipper> {
    MyGrammar() : MyGrammar::base_type(mystruct) {
        mystruct = qi::int_ >> ":" >> qi::double_;
    }
    qi::rule<Iterator, TestStruct(), Skipper> mystruct;
};

int main() {
    typedef std::string::const_iterator It;
    const std::string input("2: 3.4");
    It it(input.begin()), end(input.end());

    MyGrammar<It, qi::space_type> gr;
    TestStruct ts;

    if (qi::phrase_parse(it, end, gr, qi::space, ts) && it == end)
        std::cout << ts.myint << ", " << ts.mydouble << std::endl;

    return 0;
}
///http://stackoverflow.com/questions/10457605/how-to-parse-text-into-a-struct-using-boostspirit?rq=1
