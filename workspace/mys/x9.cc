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
#include <boost/container/static_vector.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <string>

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}

namespace filesystem = boost::filesystem;
namespace gregorian = boost::gregorian;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
template <typename T,size_t N> using array = boost::array<T,N>;

namespace boost { namespace spirit { namespace traits
{
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
    //template <typename T, size_t N> struct is_container<array<T, N>, void> : mpl::false_ { };
}}}
//typedef gregorian::date::ymd_type ymd_type;
//BOOST_FUSION_ADAPT_STRUCT(ymd_type, (ymd_type::year_type,year)(ymd_type::month_type,month)(ymd_type::day_type,day))

int main(int argc, char* const argv[])
{
    try {
        int Main(int argc, char* const argv[]);
        return Main(argc, argv);
    } catch (std::exception const& e) {
        fprintf(stderr, "%s\n", e.what());
    }
    return 1;
}

struct Av {
    long amount = 0;
    long volume = 0;
    Av& operator+=(Av const& lhs) {
        amount += lhs.amount;
        volume += lhs.volume;
        return *this;
    }
    Av operator+(Av const& lhs) {
        Av x = *this;
        return (x+=lhs);
    }
};
BOOST_FUSION_ADAPT_STRUCT(Av, (long,amount)(long,volume))
//struct RecBS : Av { char bsflag; }; BOOST_FUSION_ADAPT_STRUCT(RecBS, (float,amount)(char,bsflag)(float,volume))

gregorian::date to_date(std::string const& s) // ./tmp/20151221
{
    static const qi::int_parser<unsigned, 10, 4, 4> _4digit = {};
    static const qi::int_parser<unsigned, 10, 2, 2> _2digit = {};
    using qi::lit;
    using ascii::char_;

    unsigned y, m, d;
    auto it = s.cbegin();
    return qi::parse(it, s.cend()
            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
                 >> _4digit >> _2digit >> _2digit
            , y, m, d)
        ? gregorian::date(y,m,d) : gregorian::date();
}
static int _code(std::string const& s)
{
    static const qi::int_parser<int, 10, 6, 6> _6digit = {};
    using ascii::char_;
    using qi::lit;

    int y;
    auto it = s.cbegin();
    return qi::parse(it, s.cend()
            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
                 >> lit('S') >> (lit('Z')|'H') >> _6digit >>'.'>>ascii::no_case["csv"]
            , y)
        ? y : 0;
}

extern int Fn(FILE* ifp, int code, FILE* ofp);
int Main(int argc, char* const argv[])
{
    if (argc != 2) {
        ERR_EXIT("%s", argv[0]);
    }
    filesystem::path path(argv[1]);

    auto lfn = [](int code, filesystem::path const& cp) {
        if (FILE* fp = fopen(cp.generic_string().c_str(), "r")) {
            Fn(fp, code, stdout);
            fclose(fp);
        }
    };

    if (!filesystem::is_directory(path)) {
        if (!filesystem::is_regular_file(path))
            ERR_EXIT("%s", argv[1]);
        lfn(_code(path.generic_string()), path);
        return 0;
    }

    for (auto& di : filesystem::directory_iterator(path)) {
        if (!filesystem::is_regular_file(di.path()))
            continue;
        auto & p = di.path();
        lfn(_code(p.generic_string()), p);
    }
    return 0;
}

int Fn(FILE* ifp, int code, FILE* ofp)
{
    auto xread = [](bool& bsflag, Av& av, FILE* fp) {
        typedef char* iterator_type; // typedef std::string::const_iterator iterator_type;

        static const qi::int_parser<unsigned, 10, 2, 2> _2digit = {};
        //static const qi::int_parser<unsigned, 10, 1, 2> _12digit = {};
        qi::rule<iterator_type, unsigned> rule_sec
            = _2digit[qi::_val=3600*qi::_1] >> _2digit[qi::_val+=60*qi::_1] >> _2digit[qi::_val+=qi::_1] ;
        using qi::int_;
        using qi::float_;
        using ascii::char_;
        //qi::rule<iterator_type, RecBS> rule_recbs = (float_ >> ',' >> char_ >> ',' >> float_);

        char linebuf[256];
        if (!fgets(linebuf, sizeof(linebuf), fp)) {
            if (!feof(fp))
                ERR_EXIT("fget");
            return 0;
        }
        int sec;
        float price;
        char c;
        int vol;

        char* pos = linebuf; //str.cbegin();
        if (!qi::phrase_parse(pos, &linebuf[256]
                    , rule_sec >> float_ >> char_ >> int_, ',', sec, price, c, vol) /*&& pos == end*/) {
            ERR_EXIT("qi::parse");
        }
        int val = int(price*100);
        av.amount = val * vol;
        av.volume = vol;
        bsflag = (c == 'B');
        return sec/60;
    };

    std::vector<array<Av,2>> vols;
    vols.reserve(60*5);

    array<Av,2> avminute1 = {};
    bool bsflag;
    Av av;
    unsigned minutex = xread(bsflag, av, ifp);
    avminute1[bsflag] = av;
    while (unsigned minx = xread(bsflag, av, ifp)) {
        if (minutex != minx) {
            vols.emplace_back( avminute1 );
            avminute1 = array<Av,2>{};
            minutex = minx;
        }
        avminute1[bsflag] += av;
    }
    if (avminute1[0].volume || avminute1[0].volume)
        vols.emplace_back( avminute1 );

    fprintf(ofp, "%06d", code);
    for (auto& bs : vols) {
        for (auto& x : bs) {
            fprintf(ofp, "\t%ld\t%ld", x.volume, x.amount);
        }
    }
    fprintf(ofp, "\n");

    return int(vols.size());
}

