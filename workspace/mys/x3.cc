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
    long volume = 0;
    long amount = 0;
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
BOOST_FUSION_ADAPT_STRUCT(Av, (long,volume)(long,amount))
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

struct Elem : std::vector<long>
{
    int code = 0;
    long volume = 0;
};

struct Main : boost::multi_index::multi_index_container< Elem, indexed_by <
              sequenced<>
            , hashed_unique<member<Elem,int,&Elem::code>> >>
{
    iterator setdefault(int code) {
        auto & idc = this->get<1>();
        Elem tmp = {};
        tmp.code = code;
        return project<0>(idc.insert(tmp).first);
    }

    Main(int argc, char* const argv[])
    {}
    ~Main();
    int run(int argc, char* const argv[]);

    void step1(int code, filesystem::path const& path, gregorian::date);
    template <typename F> int step2(F reader, gregorian::date);
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

void Main::step1(int code, filesystem::path const& path, gregorian::date d)
{
    if (FILE* fp = fopen(path.generic_string().c_str(), "r")) {
        auto reader = [fp](std::vector<fusion::vector<Av,Av>>& vec) {
            //using qi::long_; //using qi::_val; using qi::_1;
            qi::rule<char*, Av(), ascii::space_type> R_Av = qi::long_ >> qi::long_;
            qi::rule<char*, fusion::vector<Av,Av>(), ascii::space_type> R_ =  R_Av >> R_Av;

            char linebuf[1024*8]; //[(60*4+15)*16+256];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("fgets");
                return 0;
            }
            int code = 0;
            char* pos = linebuf; //str.cbegin();
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , qi::int_ >> *R_
                        , ascii::space, code, vec) /*&& pos == end*/) {
                ERR_EXIT("qi::parse: %s", pos);
            }
            return code;
        };
        step2(reader, d);
        fclose(fp);
    }
}

template <typename F> int Main::step2(F read, gregorian::date d)
{
    std::vector<fusion::vector<Av,Av>> vec; // std::vector<array<Av,2>> vec;
    vec.reserve(60*4+30);
    while (int code = read(vec)) {
        Elem& els = const_cast<Elem&>(*setdefault(code));
        if (vec.size() > 60*3) {
            std::vector<long> v;
            v.reserve(vec.size());
            for (auto& a : vec) {
                v.push_back(fusion::at_c<0>(a).volume + fusion::at_c<1>(a).volume);
                els.volume += v.back();
            }
            std::nth_element(v.begin(), v.begin()+90, v.end());
            els.push_back( std::accumulate(v.begin(), v.begin()+90, 1l) );
        }
        vec.clear();
    }
    return int(size());
}

Main::~Main()
{
    for (Elem const & sk : *this) {
        fprintf(stdout, "%06d %lu %03ld", sk.code, sk.size(), std::accumulate(sk.begin(),sk.end(), 0l));
        long vb = sk.volume / sk.size();
        for (auto & v : sk) {
            fprintf(stdout, " \t%03d", int(float(v)/vb*1000));
        }
        fprintf(stdout, "\n");
    }
}

