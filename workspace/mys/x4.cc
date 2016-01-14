#include <stdio.h>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
//#include <boost/spirit/include/phoenix.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_object.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
//#include <boost/fusion/include/io.hpp>
//#include <boost/fusion/adapted/boost_array.hpp>
//#include <boost/container/static_vector.hpp>
#include <boost/function_output_iterator.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/fstream.hpp>
//#include <boost/format.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
//#include <boost/multi_index/random_access_index.hpp>
//#include <boost/multi_index/ordered_index.hpp>

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}

//namespace phoenix = boost::phoenix;
namespace filesystem = boost::filesystem;
namespace gregorian = boost::gregorian;
namespace fusion = boost::fusion;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
template <typename T,size_t N> using array = std::array<T,N>;

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
    Av operator+(Av const& lhs) const {
        Av x = *this;
        return (x+=lhs);
    }
    Av& operator-=(Av const& lhs) {
        amount -= lhs.amount;
        volume -= lhs.volume;
        return *this;
    }
    Av operator-(Av const& lhs) const {
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

using namespace boost::multi_index;

struct SInfo
{
    long gbx, gbtotal;
    float eov;
    //000001	11804054528.00	14308676608.00	0.00	0.00	1.65	0.00	1	0
    //int_ >> long_ >>".00">> long_ >>".00" >> omit[float_>>float_] >> float_
};
BOOST_FUSION_ADAPT_STRUCT(SInfo, (long,gbx)(long,gbtotal)(float,eov))

struct Elem : SInfo
{
    int code = 0;

    std::vector<fusion::vector<Av,Av>> all = {};
    std::vector<fusion::vector<Av,Av>> vless = {};
    std::vector<fusion::vector<Av,Av>> vmass = {};
    std::vector<fusion::vector<Av,Av>> voths = {};

    fusion::vector<Av,Av> sum = {}; //Av av = {}; //long volume = 0; long amount = 0;
    array<int,2> lohi = {};

    int n_day() const { return int(vless.size()); }
};

struct Main : boost::multi_index::multi_index_container< Elem, indexed_by <
              sequenced<>
            , hashed_unique<member<Elem,int,&Elem::code>> >>
{
    Main(int argc, char* const argv[]);
    int run(int argc, char* const argv[]);

    struct init_;
};

struct Main::init_ : std::unordered_map<int,SInfo>
{
    Main* m_;
    init_(Main* p, int argc, char* const argv[]);

    void init_sinf(char const* fn);
    Elem* address(int code);
    void process1(gregorian::date, filesystem::path const& path);
    template <typename F> int proc1(F reader, gregorian::date);
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

void Main::init_::init_sinf(char const* fn)
{
    if (FILE* fp = fopen(fn, "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
        using qi::long_; //using qi::_val; using qi::_1;
        using qi::int_;
        using qi::float_;
        qi::rule<char*, SInfo(), ascii::space_type> R_
            = long_ >>".00">> long_ >>".00" >> qi::omit[float_>>float_] >> float_;

        char linebuf[1024];
        while (!fgets(linebuf, sizeof(linebuf), fp)) {
            int code;
            SInfo a = {};
            char* pos = linebuf;
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , int_ >> R_ //long_ >>".00">> long_ >>".00" >> qi::omit[float_>>float_] >> float_
                        , ascii::space, code, a)) {
                ERR_EXIT("qi::parse: %s", pos);
            }
            this->emplace(code, a);
        }
    }
}

Elem* Main::init_::address(int code) //-> Main::iterator
{
    static Elem tmp = {};
    tmp.code = code;
    auto & idc = m_->get<1>();
    auto p = idc.insert(tmp);
    Elem& el = const_cast<Elem&>(*p.first);
    if (p.second) {
        auto it = this->find(code);
        if (it == this->end())
            return 0;
        SInfo& si = el;
        si = it->second;
    }
    return &el;
}

Main::init_::init_(Main* p, int argc, char* const argv[])
    : m_(p)
{
    if (argc < 2) {
        ERR_EXIT("%s argc: %d", argv[0], argc);
    }
    init_sinf("D:/home/wood/._sinfo");

    if (!filesystem::is_directory(argv[1])) {
        for (int i=1; i<argc; ++i) {
            if (!filesystem::is_regular_file(argv[i]))
                continue; // ERR_EXIT("%s: is_directory|is_regular_file", argv[i]);
            process1(_date(argv[i]), argv[i]);
        }
    }

    for (auto& di : filesystem::directory_iterator(argv[1])) {
        if (!filesystem::is_regular_file(di.path()))
            continue;
        auto & p = di.path();
        process1(_date(p.generic_string()), p);
    }
}

void Main::init_::process1(gregorian::date d, filesystem::path const& path)
{
    if (FILE* fp = fopen(path.generic_string().c_str(), "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
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
        proc1(reader, d);
    }
}

template <typename F> int Main::init_::proc1(F read, gregorian::date d)
{
    std::vector<fusion::vector<Av,Av>> vec; // std::vector<array<Av,2>> vec;
    vec.reserve(60*4+30);
    while (int code = read(vec)) {
        if (vec.size() > 60*3) {
            auto plus = [](fusion::vector<Av,Av> const& x, fusion::vector<Av,Av> const& y) {
                using fusion::at_c;
                return fusion::make_vector(at_c<0>(x)+at_c<0>(y), at_c<1>(x)+at_c<1>(y));
            };
            Elem* vss = this->address(code);
            if (!vss) {
                fprintf(stderr, "%d address-fail\n", code);
                continue;
            }

            auto av = std::accumulate(vec.begin(), vec.end(), fusion::vector<Av,Av>{}, plus);
            vss->sum = plus(vss->sum, av);
            vss->all.push_back(av);

            {
                auto less = [](auto&x, auto&y) {
                    auto a = fusion::at_c<0>(x) + fusion::at_c<1>(x);
                    auto b = fusion::at_c<1>(y) + fusion::at_c<0>(y);
                    return (a.amount*100/a.volume) < (b.amount*100/b.volume);
                };
                auto p = std::minmax_element(vec.begin(), vec.end(), less);
                Av a = fusion::at_c<0>(*p.first ) + fusion::at_c<1>(*p.first );
                Av b = fusion::at_c<0>(*p.second) + fusion::at_c<1>(*p.second);
                int x = int(a.amount*100 / a.volume);
                int y = int(b.amount*100 / b.volume);
                if (vss->lohi[0] == 0) {
                    vss->lohi[0] = x;
                    vss->lohi[1] = y;
                } else if (x < vss->lohi[0]) {
                    vss->lohi[0] = x;
                } else if (y > vss->lohi[1]) {
                    vss->lohi[1] = y;
                }
            }

            auto fvcmp = [](auto& x, auto& y){
                using fusion::at_c;
                return at_c<0>(x).volume+at_c<1>(x).volume < at_c<0>(y).volume+at_c<1>(y).volume;
            };

            std::nth_element(vec.begin(), vec.end()-80, vec.end(), fvcmp);
            auto vl = std::accumulate(vec.begin(), vec.end()-80, fusion::vector<Av,Av>{}, plus);
            vss->vless.push_back(vl);

            std::nth_element(vec.end()-80, vec.end()-20, vec.end(), fvcmp);
            auto vm = std::accumulate(vec.end()-20, vec.end(), fusion::vector<Av,Av>{}, plus);
            vss->vmass.push_back(vm);

            auto vk = std::accumulate(vec.end()-80, vec.end()-20, fusion::vector<Av,Av>{}, plus);
            vss->voths.push_back(vk);
        }
        vec.clear();
    }
    return int(size());
}

Main::Main(int argc, char* const argv[])
{
    init_ ini(this, argc, argv);
    (void)ini;
}

int Main::run(int argc, char* const argv[])
{
    auto plus = [](fusion::vector<Av,Av> const& x, fusion::vector<Av,Av> const& y) {
        using fusion::at_c;
        return fusion::make_vector(at_c<0>(x)+at_c<0>(y), at_c<1>(x)+at_c<1>(y));
    };

    for (Elem const & vss : *this) {
        int n_day = vss.n_day();
        Av sumv = fusion::at_c<0>(vss.sum) + fusion::at_c<1>(vss.sum);

        printf("%06d %2d", vss.code, n_day);
        {
            long vola = sumv.volume/n_day;
            auto xps = [&vola](long a, fusion::vector<Av,Av> const& x){
                return a + abs((fusion::at_c<0>(x).volume+fusion::at_c<1>(x).volume) - vola);
            };
            long vx = std::accumulate(vss.all.begin(), vss.all.end(), 0l, xps);
            Av buy = fusion::at_c<1>(vss.sum); //Av bs = buy + fusion::at_c<0>(sumv);
            printf("\t%3ld %3ld", 1000*vx/n_day/vola, 1000*buy.volume/sumv.volume);
        } {
            auto& vl = vss.vless;
            auto v = std::accumulate(vl.begin(), vl.end(), fusion::vector<Av,Av>{}, plus);
            Av buy = fusion::at_c<1>(v);
            Av both = buy + fusion::at_c<0>(v);
            printf("\t%3d %3d", int(buy.volume*1000/sumv.volume), int(buy.volume*1000/both.volume));
        } {
            auto& vl = vss.vmass;
            auto v = std::accumulate(vl.begin(), vl.end(), fusion::vector<Av,Av>{}, plus);
            Av buy = fusion::at_c<1>(v);
            Av both = buy + fusion::at_c<0>(v);
            printf("\t%3d %3d", int(buy.volume*1000/sumv.volume), int(buy.volume*1000/both.volume));
        } {
            auto& lh = vss.lohi;
            printf("\t%3d", abs(lh[1]-lh[0])*1000 / std::min(lh[0],lh[1]));

            Av buy = fusion::at_c<1>(vss.sum);
            printf("\t%ld %ld", buy.amount*100/std::max(buy.volume,1l), sumv.amount*100/std::max(sumv.volume,1l));
        }
//        array<long,3> maxv[2] = {};
//        {
//            auto last_n = std::min(7, vss.n_day());
//            auto last_m = std::min(3, last_n);
//            for (int i=0; i < 2; ++i) {
//                //std::nth_element(vss[i].end()-last_n, vss[i].end()-last_m, vss[i].end());
//                //std::copy(vss[i].end()-last_m, vss[i].end(), maxv[i].begin());
//            }
//        }
//
//        if (long fvb = vss.av.volume / vss.n_day()) {
//            auto & vl = vss.vless;
//            long sl = std::accumulate(vl.begin(),vl.end(),0l);
//            fprintf(stdout, "\t%3d", int((sl*1000)/fvb));
//            {
//                long vb = sl/vl.size();
//                long x = std::accumulate(vl.begin(),vl.end(),0l, [&vb](long o, long v){ return o+abs(v-vb); });
//                fprintf(stdout, " %3d", int((x*1000)/vl.size()/vb));
//            }
//            for (long v : vl)
//                fprintf(stdout, " %3d", int((v*1000)/fvb));
//        }
//
        fprintf(stdout, "\t%d\n", int(vss.vless.size()==vss.vmass.size() && vss.all.size()==vss.vless.size()));
    }
    return 0;
}

