
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

#include <string>
#include <iostream>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct Av
{
    float amount;
    float volume;
};
struct Rec : Av
{
    int sec;
};
struct RecBS : Rec
{
    char bs;
};
BOOST_FUSION_ADAPT_STRUCT(RecBS, (int,sec)(float,amount)(char,bs)(float,volume))

template <typename Iterator>
struct BSParser : qi::grammar<Iterator, RecBS()/*, ascii::space_type*/> //092500,48.40,B,200
{
    BSParser() : BSParser::base_type(start) {
        using qi::_val;
        using qi::_1;
        using qi::int_;
        using qi::float_;
        using ascii::char_;
        static const qi::int_parser<unsigned, 10, 2, 2> _2digit = {};

        seconds = _2digit[_val=3600*_1] >> _2digit[_val+=60*_1] >> _2digit[_val+=_1];
        start %= seconds >> ',' >> float_ >> ',' >> char_ >> ',' >> int_;
    }

    qi::rule<Iterator, int/*, ascii::space_type*/> seconds;
    qi::rule<Iterator, RecBS()/*, ascii::space_type*/> start;
    //struct BuySell_ : qi::symbols<char, bool> { BuySell_() { add ("B",true) ("S",false) ; } };
};

int main(int argc, char* const argv[])
{
    typedef std::string::const_iterator iterator_type;

    struct BSum : std::array<Av,8>, Av {} buys ={}, sells ={};

    BSParser<iterator_type> parser;
    std::string str; //092500,48.40,B,200
    while (getline(std::cin, str)) {
        RecBS rec;
        iterator_type iter = str.begin();
        iterator_type end = str.end();
        bool r = parse(iter, end, parser, rec); // phrase_parse(iter, end, parser, ascii::space, rec);
        if (r && iter == end) {
            //std::cout << boost::fusion::tuple_open('[');
            //std::cout << boost::fusion::tuple_close(']');
            //std::cout << boost::fusion::tuple_delimiter(", ");
            //std::cout << "got: " << boost::fusion::as_vector(rec) << std::endl;

            rec.amount *= rec.volume;

            auto& bs = (rec.bs=='B') ? buys : sells;
            auto& w = bs[rec.sec < 3600*13 ? rec.sec > 3600*10+1800 : 2+(rec.sec > 3600*14)];
            w.amount += rec.amount;
            w.volume += rec.volume;

            bs.amount += rec.amount;
            bs.volume += rec.volume;
            //bs.push_back(rec);

        } else {
            std::cerr << "Parsing failed\n";
            break;
        }
    }

    printf("[0]\t%.3f\t%.2f\t%.2f\t%.2f\n"
            , (buys.amount - sells.amount)/(buys.amount + sells.amount)
            , (buys.amount - sells.amount)/10000
            , (buys.amount + sells.amount)/10000
            , (buys.volume + sells.volume)/100
            );
    for (int i = 0; i<4; ++i) {
        printf("[%d]\t%.2f\t%.2f\t%.2f\n", i+1
                , (buys[i].amount - sells[i].amount)/(buys[i].amount + sells[i].amount)
                , (buys[i].amount - sells[i].amount)/10000
                , (buys[i].volume + sells[i].volume)/(buys.volume + sells.volume)
                );
    }
    //printf("[ ]\t%.2f\t%.2f\t%.2f", (buys.amount - sells.amount)/10000
    //        , abs(buys.amount - sells.amount)/(buys.amount + sells.amount) // , (buys.volume + sells.volume)/100
    //        , (buys.amount + sells.amount)/10000
    //        );
    //printf("\t%.2f\t%.2f\t%.2f"
    //        , (sells.amount+buys.amount)/(sells.volume+buys.volume)
    //        , buys.amount/buys.volume
    //        , sells.amount/sells.volume
    //        );
    //printf("\n");

    return 0;
}


