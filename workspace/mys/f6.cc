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
typedef gregorian::date::ymd_type ymd_type;
BOOST_FUSION_ADAPT_STRUCT(ymd_type, (ymd_type::year_type,year)(ymd_type::month_type,month)(ymd_type::day_type,day))

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
    Av& operator-=(Av const& lhs) {
        amount -= lhs.amount;
        volume -= lhs.volume;
        return *this;
    }
    Av operator-(Av const& lhs) {
        Av x = *this;
        return (x-=lhs);
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

struct Elem : array<std::vector<long>,3> //std::vector<long> vols[2];
{
    int code = 0;
    long volume = 0;
    long amount = 0;

    array<int,2> lohi = {{0,0}};

    unsigned nday() const { return (*this)[0].size(); }
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

    void prepare(gregorian::date, filesystem::path const& path);
    template <typename F> int process(F reader, gregorian::date);
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
    if (argc < 2) {
        ERR_EXIT("%s argc: %d", argv[0], argc);
    }

    if (!filesystem::is_directory(argv[1])) {
        for (int i=1; i<argc; ++i) {
            if (!filesystem::is_regular_file(argv[i]))
                continue; // ERR_EXIT("%s: is_directory|is_regular_file", argv[i]);
            prepare(_date(argv[i]), argv[i]);
        }
        return 0;
    }

    for (auto& di : filesystem::directory_iterator(argv[1])) {
        if (!filesystem::is_regular_file(di.path()))
            continue;
        auto & p = di.path();
        prepare(_date(p.generic_string()), p);
    }
    return 0;
}

void Main::prepare(gregorian::date d, filesystem::path const& path)
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
        process(reader, d);
        fclose(fp);
    }
}

template <typename F> int Main::process(F read, gregorian::date d)
{
    std::vector<fusion::vector<Av,Av>> vec; // std::vector<array<Av,2>> vec;
    vec.reserve(60*4+30);
    while (int code = read(vec)) {
        if (vec.size() > 60*3) {
            Elem& vss = const_cast<Elem&>(*setdefault(code));
            Av av = {};
            std::vector<long> vols[3];
            for (auto& v: vols)
                v.reserve(vec.size());

            for (auto& a : vec) {
                av += fusion::at_c<0>(a) + fusion::at_c<1>(a);

                long sv = fusion::at_c<0>(a).volume;
                long bv = fusion::at_c<1>(a).volume;
                vss.volume += (sv + bv);

                vols[0].push_back(sv);
                vols[1].push_back(bv);
                vols[2].push_back(sv + bv);
            }

            int x = int(double(av.amount*100) / av.volume);
            if (vss.empty())/*(vss.lohi[0] == 0)*/ {
                vss.lohi[0] = vss.lohi[1] = x;
            } else if (x < vss.lohi[0]) {
                vss.lohi[0] = x;
            } else if (x > vss.lohi[1]) {
                vss.lohi[1] = x;
            }

            for (int i=0; i < 2; ++i) {
                std::nth_element(vols[i].begin(), vols[i].end()-15, vols[i].end());
                vss[i].push_back( std::accumulate(vols[i].end()-15, vols[i].end(), 0l) );
            }
            std::nth_element(vols[2].begin(), vols[2].begin()+90, vols[2].end());
            vss[2].push_back( std::accumulate(vols[2].begin(), vols[2].begin()+90, 0l) );
        }
        vec.clear();
    }
    return int(size());
}

Main::~Main()
{
    for (Elem const & vss : *this) {
        fprintf(stdout, "%06d %d", vss.code, vss.nday());
        fprintf(stdout, " %03d", int(double(abs(vss.lohi[1] - vss.lohi[0]))/vss.lohi[0] * 1000));

        //array<long,3> maxv[2] = {};
        //{
        //    auto last_n = std::min(7u, vss.nday());
        //    auto last_m = std::min(3u, last_n);
        //    for (int i=0; i < 2; ++i) {
        //        std::nth_element(vss[i].end()-last_n, vss[i].end()-last_m, vss[i].end());
        //        //std::copy(vss[i].end()-last_m, vss[i].end(), maxv[i].begin());
        //    }
        //}

        auto & vl = vss[2];
        long lov = std::accumulate(vl.begin(),vl.end(),0l);
        fprintf(stdout, "\t%03d", int(double(lov)/vss.volume * 1000));
        {
            long vb = lov/vl.size();
            long x = std::accumulate(vl.begin(),vl.end(),0l, [&vb](long o, long v){ return o+abs(v-vb); });
            fprintf(stdout, " %03d", int(double(x)/vl.size()/vb*1000));
        }
        if (long fvb = vss.volume / vss.nday()) {
            for (long v : vss[2])
                fprintf(stdout, " %03d", int(double(v)/fvb*1000));
        }

        fprintf(stdout, "\t%04d %04d", vss.lohi[0], vss.lohi[1]);
        fprintf(stdout, "\n");
    }
}

