
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/adapted/boost_array.hpp>
//#include <boost/spirit/include/phoenix.hpp>
//namespace phoenix = boost::phoenix;
#include <boost/array.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <string>

namespace filesystem = boost::filesystem;
namespace gregorian = boost::gregorian;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
template <typename T,size_t N> using array = boost::array<T,N>;

namespace boost { namespace spirit { namespace traits
{
    //template <typename T, size_t N> struct is_container<array<T, N>, void> : mpl::false_ { };

    template<>
    struct transform_attribute<boost::gregorian::date, fusion::vector<int, int, int>, qi::domain>
    {
        typedef fusion::vector<int, int, int> date_parts;
        typedef date_parts type;

        static date_parts pre(boost::gregorian::date) { 
            return date_parts(); 
        }
        static void post(boost::gregorian::date& d, date_parts const& v) {
            d = boost::gregorian::date(fusion::at_c<0>(v), fusion::at_c<1>(v), fusion::at_c<2>(v));
        }
        static void fail(boost::gregorian::date&) {}
    };
}}}
typedef gregorian::date::ymd_type ymd_type;
BOOST_FUSION_ADAPT_STRUCT(ymd_type, (ymd_type::year_type,year)(ymd_type::month_type,month)(ymd_type::day_type,day))

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
    char bsflag;
};
BOOST_FUSION_ADAPT_STRUCT(Av, (float,amount)(float,volume))
BOOST_FUSION_ADAPT_STRUCT(RecBS, (int,sec)(float,amount)(char,bsflag)(float,volume))

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

gregorian::date to_date(std::string const& s) // xdetail/20151221/SZ002280.csv
{
    static const qi::int_parser<unsigned, 10, 4, 4> _4digit = {};
    static const qi::int_parser<unsigned, 10, 2, 2> _2digit = {};
    //using boost::phoenix::ref; using qi::_1;
    using qi::omit;
    using qi::lit;
    using ascii::char_;

    gregorian::date d; // ymd_type ymd = {}; // unsigned y, m, d;
    auto it = s.cbegin();
    return qi::parse(it, s.cend()
            , omit[*(*~char_("/\\") >> (lit('/')|'\\'))]
                  >> _4digit >> _2digit >> _2digit
            , d)
        ? d : gregorian::date();
}
int to_code(std::string const& s)
{
    static const qi::int_parser<int, 10, 6, 6> _6digit = {};
    using ascii::char_;
    using qi::lit;

    int y;

    auto it = s.cbegin();
    return qi::parse(it, s.cend(), lit('S') >> (lit('Z')|'H') >> _6digit >>'.'>>ascii::no_case["csv"], y)
        ? y : 0;
}

#include <iostream>

int generate(std::istream& ifs, gregorian::date const& date, std::string const& fn)
{
    struct BSum : array<Av,12>, Av {} buys ={}, sells ={};
    float ochl[4] = {};

    typedef std::string::const_iterator iterator_type;
    BSParser<iterator_type> parser;

    std::string str; // 092500,48.40,B,200
    while (getline(ifs, str)) {
        RecBS rec;
        iterator_type iter = str.begin();
        iterator_type end = str.end();
        bool r = qi::parse(iter, end, parser, rec); // phrase_parse(iter, end, parser, ascii::space, rec);
        if (r /*&& iter == end*/) {
            //std::cout << boost::fusion::tuple_open('[');
            //std::cout << boost::fusion::tuple_close(']');
            //std::cout << boost::fusion::tuple_delimiter(", ");
            //std::cout << "got: " << boost::fusion::as_vector(rec) << std::endl;
            if (ochl[0] < 0.0001)
                ochl[0] = ochl[1] = ochl[2] = rec.amount;
            ochl[3] = rec.amount;
            if (rec.amount > ochl[1]) {
                ochl[1] = rec.amount;
            } else if (rec.amount < ochl[2]) {
                ochl[2] = rec.amount;
            }

            rec.amount *= rec.volume;

            auto& bs = (rec.bsflag=='B') ? buys : sells;
            auto& w = bs[rec.sec < 3600*13 ? abs(rec.sec - 60*(60*9+30))/20%6 : 6+(abs(rec.sec - 3600*13)/20%6)];
            w.amount += rec.amount;
            w.volume += rec.volume;

            bs.amount += rec.amount;
            bs.volume += rec.volume;
            //bs.push_back(rec);

        } else {
            std::cerr << "Parsing failed\n";
            return 2;
        }
    }

    gregorian::date::ymd_type ymd = date.year_month_day();

    if (FILE* fp = fopen(fn.c_str(), "a")) {
        fprintf(fp, "%u%02u%02u\t""%+.3f\t%+.3f\t%+.3f\t%+.3f""\t%+.3f\t%+.3f\t%+.3f\t%+.3f"
                , unsigned(ymd.year), unsigned(ymd.month), unsigned(ymd.day)
                , ochl[0], ochl[1], ochl[2], ochl[3]
                , buys.amount, buys.volume
                , sells.amount, sells.volume
              );
        for (int i = 0; i<12; ++i) {
            fprintf(fp, "\t%+.3f\t%+.3f\t%+.3f\t%+.3f"
                    , buys[i].amount , buys[i].volume
                    , sells[i].amount , sells[i].volume
                    );
        }
        fprintf(fp, "\n");
        fclose(fp);
    } else {
        std::cerr << "fopen fail:" << fn <<'\n';
        return 3;
    }

    return 0;
    //printf("[  ]\t%.2f\t%+.3f\t%+.2f\t%.2f\t%.2f""\t%.2f\t%.2f\t%.2f""\t%.2f,\t%.2f,\t%.2f,\t%.2f""\n"
    //        , (buys.amount / sells.amount)
    //        , (buys.amount - sells.amount)/(buys.amount + sells.amount)
    //        , (buys.amount - sells.amount)/10000
    //        , (buys.amount + sells.amount)/10000
    //        , (buys.volume + sells.volume)/100
    //        //
    //        , (sells.amount+buys.amount)/(sells.volume+buys.volume)
    //        , buys.amount/buys.volume
    //        , sells.amount/sells.volume
    //        , ochl[0], ochl[1] , ochl[2], ochl[3]
    //        );
    //for (int i = 0; i<12; ++i) {
    //    printf("[%02d]\t%.2f\t%+.2f\t%+.2f\t%.2f\n", i+1
    //            , (buys[i].amount / sells[i].amount)
    //            , (buys[i].amount - sells[i].amount)/(buys[i].amount + sells[i].amount)
    //            , (buys[i].amount - sells[i].amount)/10000
    //            , (buys[i].volume + sells[i].volume)/(buys.volume + sells.volume)
    //            );
    //}
}

struct XRec {
    gregorian::date date; typedef boost::fusion::vector<int, int, int> date_parts;
    std::vector<float> ochl; typedef std::vector<float> ochl_type;
    std::vector<Av> s; typedef std::vector<Av> sum_type;
    std::vector<std::vector<Av>> v; typedef std::vector<std::vector<Av>> xdetail_type;
};
BOOST_FUSION_ADAPT_STRUCT(XRec, date, ochl, s, v)

void print(std::istream& ifs, gregorian::date const& date0, gregorian::date const& date1)
{
    static const qi::int_parser<unsigned, 10, 4, 4> _4digit = {};
    static const qi::int_parser<unsigned, 10, 2, 2> _2digit = {};
    using qi::float_; //using qi::_val; using qi::_1; using ascii::char_; using qi::int_; using phoenix::ref;

    typedef std::string::const_iterator iterator_type;

    qi::rule<iterator_type,Av(),ascii::space_type> rule_av = (float_ >> float_);
    qi::rule<iterator_type,XRec::ochl_type(),ascii::space_type> rule_ochl = (float_ >> float_ >> float_ >> float_);
    qi::rule<iterator_type,XRec::date_parts(),ascii::space_type> rule_date = (_4digit >> _2digit >> _2digit);
    qi::rule<iterator_type,XRec::xdetail_type(),ascii::space_type> rule_xdetail = qi::repeat(12)[rule_av];
    qi::rule<iterator_type,XRec::sum_type(),ascii::space_type> rule_sum = rule_av >> rule_av;
    qi::rule<iterator_type,XRec(),ascii::space_type> parser =
        rule_date
        >> rule_ochl
        >> rule_sum
        >> rule_xdetail
        ;

    std::string str;
    while (getline(ifs, str)) {
        XRec xr = {};

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        bool r = qi::phrase_parse(iter, end, parser, ascii::space, xr);//(xr.date, xr.s, volume, amount, xr.ochl, xr.v);
        if (r /*&& iter == end*/) {
            //std::cout << boost::fusion::tuple_open('[');
            //std::cout << boost::fusion::tuple_close(']');
            //std::cout << boost::fusion::tuple_delimiter(", ");
            //std::cout << "got: " << boost::fusion::as_vector(xr) << std::endl;

            float volume = xr.s[0].volume + xr.s[1].volume;
            float amount = xr.s[0].amount + xr.s[1].amount;

            printf("[  ]\t%.2f\t%+.3f\t%+.2f\t%.2f\t%.2f""\t%.2f\t%.2f\t%.2f""\t%.2f,\t%.2f,\t%.2f,\t%.2f""\n"
                    , (xr.s[1].amount / xr.s[0].amount)
                    , (xr.s[1].amount - xr.s[0].amount)/(xr.s[1].amount + xr.s[0].amount)
                    , (xr.s[1].amount - xr.s[0].amount)/10000
                    , (xr.s[1].amount + xr.s[0].amount)/10000
                    , (xr.s[1].volume + xr.s[0].volume)/100
                    //
                    , amount/volume
                    , xr.s[1].amount/xr.s[1].volume
                    , xr.s[0].amount/xr.s[0].volume
                    , xr.ochl[0], xr.ochl[1], xr.ochl[2], xr.ochl[3]
                    );
            for (unsigned i = 0; i<xr.v.size(); ++i) {
                printf("[%02d]\t%.2f\t%+.2f\t%+.2f\t%.2f\n", i+1
                        , (xr.v[i][1].amount / xr.v[i][0].amount)
                        , (xr.v[i][1].amount - xr.v[i][0].amount)/(xr.v[i][1].amount + xr.v[i][0].amount)
                        , (xr.v[i][1].amount - xr.v[i][0].amount)/10000
                        , (xr.v[i][1].volume + xr.v[i][0].volume)/(volume)
                        );
            }

        } else {
            std::cerr << "Parsing failed\n";
            break;
        }
    }
}

int main(int argc, char* const argv[])
{
    if (argc != 3) {
        return 1; //return print();
    }
    if (!filesystem::is_directory(argv[1]) || !filesystem::is_directory(argv[2])) {
        std::cerr << "!filesystem::is_directory\n";
        return 2;
    }

    gregorian::date date = to_date(argv[1]);
    std::cout << argv[1] <<" "<< date <<"\n";

    filesystem::path odir(argv[2]);
    for (auto& x : filesystem::directory_iterator(argv[1])) {
        auto& p = x.path();
        int code = to_code(p.leaf().string());
        if (!code || !filesystem::is_regular_file(p))
            continue;
        //std::cout << (p / "LEAF").generic_string() <<'\t'<<code<<"\n";

        filesystem::ifstream ifs(p);
        if (!ifs)
            continue;
        using boost::format;
        generate(ifs, date, (odir / str(format("%06d") % code)).generic_string());
    }

    return 0;
}

