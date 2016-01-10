#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
//#include <boost/fusion/include/io.hpp>
//#include <boost/fusion/adapted/boost_array.hpp>
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
namespace fusion = boost::fusion;
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

gregorian::date _date(std::string const& s) // ./20151221
{
    static const qi::int_parser<unsigned,10,4,4> _4digit = {};
    static const qi::int_parser<unsigned,10,2,2> _2digit = {};
    using qi::lit;
    using ascii::char_;

    unsigned y, m, d;
    auto it = s.cbegin();
    if (!qi::parse(it, s.cend()
            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
                 >> _4digit >> _2digit >> _2digit
            , y, m, d))
        ERR_EXIT("%s: not-a-date", s.c_str());
    return gregorian::date(y,m,d);
}
static int _code(std::string const& s)
{
    static const qi::int_parser<int,10,6,6> _6digit = {};
    using ascii::char_;
    using qi::lit;

    int y;
    auto it = s.cbegin();
    if (!qi::parse(it, s.cend()
            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
                 >> lit('S') >> (lit('Z')|'H') >> _6digit >>'.'>>ascii::no_case[lit("csv")|"txt"]
            , y))
        ERR_EXIT("%s: not-a-code", s.c_str());
    return y;
}

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
//#include <boost/multi_index/random_access_index.hpp>
//#include <boost/multi_index/ordered_index.hpp>
using namespace boost::multi_index;

struct Vols : std::vector<long>
{
    int code = 0;
    long volume = 0;
};

struct Main : boost::multi_index::multi_index_container< Vols, indexed_by <
              sequenced<>
            , hashed_unique<member<Vols,int,&Vols::code>> >>
{
    iterator setdefault(int code) {
        auto & idc = this->get<1>();
        Vols tmp;
        tmp.code = code;
        return project<0>(idc.insert(tmp).first);
    }

    Main(int argc, char* const argv[])
    {}
    ~Main();
    int run(int argc, char* const argv[]);

    void step1(int code, filesystem::path const& path, gregorian::date);
    template <typename F> int step2(F reader, int code);
};

int main(int argc, char* const argv[])
{
    try {
        Main a(argc, argv);
        return a.run(argc, argv);
    } catch (std::exception const& e) {
        fprintf(stderr, "%s\n", e.what());
    }
    return 1;
}

int Main::run(int argc, char* const argv[])
{
    if (argc != 2) {
        ERR_EXIT("%s: argc>1", argv[0]);
    }
    filesystem::path path(argv[1]);

    if (!filesystem::is_directory(path)) {
        if (!filesystem::is_regular_file(path))
            ERR_EXIT("%s: is_directory|is_regular_file", argv[1]);
        step1(_code(path.generic_string()), path, _date(path.generic_string()));
        return 0;
    }

    for (auto& di : filesystem::directory_iterator(path)) {
        if (!filesystem::is_regular_file(di.path()))
            continue;
        auto & p = di.path();
        step1(_code(p.generic_string()), p, _date(p.generic_string()));
    }
    return 0;
}

void Main::step1(int code, filesystem::path const& path, gregorian::date)
{
    if (FILE* fp = fopen(path.generic_string().c_str(), "r")) {
        auto xcsv = [fp](bool& bsflag, Av& av) {
            static const qi::int_parser<int, 10, 2, 2> _2digit = {};
            qi::rule<char*, int> rule_sec = _2digit[qi::_val=3600*qi::_1] >> _2digit[qi::_val+=60*qi::_1] >> _2digit[qi::_val+=qi::_1] ;
            using qi::int_;
            using qi::float_;
            using ascii::char_;

            char linebuf[256];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("fgets");
                return 0;
            }
            int sec;
            float price;
            char c;
            int vol;

            char* pos = linebuf; //str.cbegin();
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , rule_sec >> float_ >> char_ >> int_, ',', sec, price, c, vol) /*&& pos == end*/) {
                ERR_EXIT("qi::parse %s", pos);
            }
            bsflag = (c == 'B');
            av.volume = vol;
            av.amount = int(price*100) * av.volume;
            return sec/60;
        };
        auto xtxt = [fp](bool& bsflag, Av& av) {
            //static const qi::int_parser<int, 10, 2, 2> _2digit = {};
            //qi::rule<char*, int> rule_sec = _2digit[qi::_val=3600*qi::_1] >> _2digit[qi::_val+=60*qi::_1] >> _2digit[qi::_val+=qi::_1] ;
            using qi::int_;
            using ascii::char_;

            char linebuf[256];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("fgets");
                return 0;
            }
            int sec;
            int price;
            int vol;

            char* pos = linebuf; //str.cbegin();
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , int_ >> int_ >> int_, ascii::space, sec, price, vol) /*&& pos == end*/) {
                ERR_EXIT("qi::parse: %s", pos);
            }
            bsflag = (vol > 0);
            av.volume = abs(vol);
            av.amount = price * av.volume;
            return 60*(sec/10000) + sec/100;
        };
        if (path.extension() == ".txt")
            step2(xtxt, code);
        else
            step2(xcsv, code);
        fclose(fp);
    }
}

template <typename F> int Main::step2(F read, int code)
{
    std::vector<array<Av,2>> vols;
    vols.reserve(60*5);

    array<Av,2> avminute1 = {};
    bool bsflag = 0;
    Av av;
    unsigned minutex = read(bsflag, av);
    avminute1[bsflag] = av;
    while (unsigned minx = read(bsflag, av)) {
        if (minutex != minx) {
            vols.emplace_back( avminute1 );
            avminute1 = array<Av,2>{};
            minutex = minx;
        }
        avminute1[bsflag] += av;
    }
    if (avminute1[0].volume || avminute1[0].volume)
        vols.emplace_back( avminute1 );

    fprintf(stdout, "%06d", code);
    for (auto& bs : vols) {
        for (auto& x : bs) {
            fprintf(stdout, "\t%ld\t%ld", x.volume, x.amount);
        }
    }
    fprintf(stdout, "\n");

    return int(vols.size());
}

Main::~Main()
{}

