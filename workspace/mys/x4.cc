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
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
//#include <boost/fusion/include/io.hpp>
//#include <boost/fusion/adapted/boost_array.hpp>
//#include <boost/spirit/include/phoenix.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_object.hpp>
//#include <boost/spirit/include/phoenix_core.hpp>
//#include <boost/spirit/include/phoenix_operator.hpp>
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
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    //fflush(stderr);
}


//namespace phoenix = boost::phoenix;
namespace filesystem = boost::filesystem;
namespace gregorian = boost::gregorian;
namespace fusion = boost::fusion;
namespace qi = boost::spirit::qi;
//namespace ascii = boost::spirit::ascii;
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

typedef fusion::vector<Av,Av> Avsb;

static const auto Sum = [](Avsb const& avsb) {
    return fusion::at_c<0>(avsb) + fusion::at_c<1>(avsb);
};
static const auto Plus = [](Avsb const& x, Avsb const& y) {
    Av const a = fusion::at_c<0>(x) + fusion::at_c<0>(y);
    Av const b = fusion::at_c<1>(x) + fusion::at_c<1>(y);
    return fusion::make_vector(a, b);
};
//[](auto&& x) -> decltype(fusion::at_c<0>(x))& { return fusion::at_c<0>(x); };
auto First  = [](auto&& x) -> auto& { return fusion::at_c<0>(x); };
auto Second = [](auto&& x) -> auto& { return fusion::at_c<1>(x); };

//struct RecBS : Av { char bsflag; }; BOOST_FUSION_ADAPT_STRUCT(RecBS, (float,amount)(char,bsflag)(float,volume))

typedef unsigned code_t;
inline code_t make_code(int szsh, int numb) { return ((szsh<<24)|numb); }
inline int numb(code_t v_) { return v_&0x0ffffff; }
inline int szsh(code_t v_) { return (v_>>24)&0xff; }
static code_t _code(std::string const& s)
{
    static const qi::int_parser<int,10,6,6> _6digit = {};
    using qi::char_;
    using qi::lit;
    struct SZSH_ : qi::symbols<char, int> { SZSH_() { add ("SZ",0) ("SH",1); } } SZSH;

    int x, y;
    auto it = s.cbegin();
    if (!qi::parse(it, s.cend()
            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
                 >> SZSH >> _6digit // >>'.'>>qi::no_case[lit("csv")|"txt"]
            , x, y))
        ERR_EXIT("%s: not-a-code", s.c_str());
    return make_code(x,y);
}

gregorian::date _date(std::string const& s) // ./20151221
{
    static const qi::int_parser<unsigned,10,4,4> _4digit = {};
    static const qi::int_parser<unsigned,10,2,2> _2digit = {};
    using qi::lit;
    using qi::char_;

    unsigned y, m, d;
    auto it = s.cbegin();
    if (!qi::parse(it, s.cend()
            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
                 >> _4digit >> _2digit >> _2digit
            , y, m, d))
        ERR_EXIT("%s: not-a-date", s.c_str());
    return gregorian::date(y,m,d);
}

using namespace boost::multi_index;

struct SInfo
{
    long gbx, gbtotal;
    int eov;
    //000001	11804054528	14308676608	0	0	165	0	1	0
    //int_ >> long_ >>long_ >> omit[long_]>>omit[long_] >> int_
};
BOOST_FUSION_ADAPT_STRUCT(SInfo, (long,gbx)(long,gbtotal)(int,eov))

struct Elem : SInfo
{
    int code = 0;

    std::vector<Avsb> all = {};
    std::vector<Avsb> vless = {};
    std::vector<Avsb> vmass = {};
    std::vector<Avsb> voths = {};

    Avsb sum = {}; //Av av = {}; //long volume = 0; long amount = 0;
    array<int,2> lohi = {};

    int n_day() const { return int(all.size()); }
};

struct Main : boost::multi_index::multi_index_container< Elem, indexed_by <
              sequenced<>
            , hashed_unique<member<Elem,int,&Elem::code>> >>
{
    Main(int argc, char* const argv[]);
    int run(int argc, char* const argv[]);

    gregorian::date date;
    struct init_;
};

struct Main::init_ : std::unordered_map<int,SInfo> , boost::noncopyable
{
    Main* m_ = 0;
    Elem tmp_ = {};
    init_(Main* p, int argc, char* const argv[]);

    void loadsi(char const* fn);
    Elem* address(int code);
    void process1(filesystem::path const& path);
    template <typename F> int proc1(F reader);
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

void Main::init_::loadsi(char const* fn)
{
    if (FILE* fp = fopen(fn, "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
        using qi::long_; //using qi::_val; using qi::_1;
        using qi::int_;
        qi::rule<char*, SInfo(), qi::space_type> R_
            = long_ >> long_ >> qi::omit[long_]>>qi::omit[long_] >> int_;

        char linebuf[1024];
        while (fgets(linebuf, sizeof(linebuf), fp)) {
            int szsh=0, code;
            SInfo si = {};
            char* pos = linebuf;
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , int_ >> int_ >> R_
                        , qi::space, code, szsh, si)) {
                ERR_EXIT("qi::parse: %s", pos);
            }
            if (si.gbx > 0)
                this->emplace(make_code(szsh,code), si);
            else
                fprintf(stderr, "%d\n", code);
        }
    } else
        ERR_EXIT("fopen: %s", fn);
}

Elem* Main::init_::address(int code) //-> Main::iterator
{
    tmp_.code = code;
    auto & idc = m_->get<1>();
    auto p = idc.insert(tmp_);
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
    loadsi("/cygdrive/d/home/wood/._sinfo");

    if (filesystem::is_directory(argv[1])) {
        for (auto& di : filesystem::directory_iterator(argv[1])) {
            if (!filesystem::is_regular_file(di.path()))
                continue;
            auto & p = di.path();
            process1( p);
            m_->date = std::min(m_->date, _date(p.generic_string()));
        }
    } else for (int i=1; i<argc; ++i) {
        if (!filesystem::is_regular_file(argv[i]))
            continue; // ERR_EXIT("%s: is_directory|is_regular_file", argv[i]);
        process1( argv[i]);
        m_->date = std::min(m_->date, _date(argv[i]));
    }
}

void Main::init_::process1(filesystem::path const& path)
{
    if (FILE* fp = fopen(path.generic_string().c_str(), "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
        auto reader = [fp](std::vector<Avsb>& vec, int* nonx) {
            vec.reserve(60*4); //using qi::long_; //using qi::_val; using qi::_1;
            qi::rule<char*, Av(), qi::space_type> R_Av = qi::long_ >> qi::long_;
            //qi::rule<char*, Avsb(), qi::space_type> R_ =  R_Av >> R_Av;

            char linebuf[1024*16]; //[(60*4+15)*16+256];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("fgets");
                return 0u;
            }
            int szsh=0, code = 0;
            char*const end = &linebuf[sizeof(linebuf)];
            char* pos = linebuf;
            if (qi::phrase_parse(pos,end, qi::int_ >> qi::int_, qi::space, code, szsh)) {
                Avsb avsb;
                while (qi::phrase_parse(pos,end, R_Av>>R_Av, qi::space, avsb)) {
                    if (Sum(avsb).volume == 0) {
                        if(nonx) ++*nonx;
                    } else {
                        First(avsb).amount /= 100;
                        Second(avsb).amount /= 100;
                        vec.push_back(avsb);
                    }
                }
                //if (nonx && *nonx) ERR_MSG("%d %d: volume=0\n", code, nonx);
            } else
                ERR_EXIT("qi::parse: %s", pos);
            return make_code(szsh,code);
        };
        proc1(reader);
    }
}

template <typename F> int Main::init_::proc1(F read)
{
    std::vector<Avsb> vec;
    while (code_t code = read(vec, 0)) {
        if (vec.size() < 100)
            continue;
        Elem* vss = this->address(code);
        if (!vss) {
            fprintf(stderr, "%d address-fail\n", code);
            continue;
        }

        auto avsb = std::accumulate(vec.begin(), vec.end(), Avsb{}, Plus);
        vss->sum = Plus(vss->sum, avsb);
        vss->all.push_back(avsb);

        {
            auto cheap = [](auto&x, auto&y) {
                auto a = Sum(x);
                auto b = Sum(y);
                return (a.amount*100/a.volume) < (b.amount*100/b.volume);
            };
            auto p = std::minmax_element(vec.begin(), vec.end(), cheap);
            Av a = Sum(*p.first );
            Av b = Sum(*p.second);
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
            return Sum(x).volume < Sum(y).volume;
        };

        int n = int(3/10.0 * (float)vec.size());
        int m = int(3/10.0 * float(vec.size()-n));

        std::nth_element(vec.begin(), vec.end()-n, vec.end(), fvcmp);
        auto vl = std::accumulate(vec.begin(), vec.end()-n, Avsb{}, Plus);
        vss->vless.push_back(vl);

        std::nth_element(vec.end()-n, vec.end()-m, vec.end(), fvcmp);
        auto vm = std::accumulate(vec.end()-m, vec.end(), Avsb{}, Plus);
        vss->vmass.push_back(vm);

        auto vk = std::accumulate(vec.end()-n, vec.end()-m, Avsb{}, Plus);
        vss->voths.push_back(vk);

        vec.clear();
    }
    return int(size());
}

Main::Main(int argc, char* const argv[])
    : date(gregorian::day_clock::local_day())
{
    init_ ini(this, argc, argv);
    (void)ini;
}

int Main::run(int argc, char* const argv[])
{
    Avsb _sum = {};

    for (Elem const & vss : *this) {
        _sum = Plus(_sum, vss.sum);
        int n_day = vss.n_day();
        Av sell = First(vss.sum);
        Av buy = Second(vss.sum);
        Av total = buy + sell;

        printf("%06d %9.2f", numb(vss.code), (buy-sell).amount/10000.0);
        {
            long vola = total.volume/n_day;
            auto xps = [&vola](long a, Avsb const& x){
                return a + labs(Sum(x).volume - vola);
            };
            long vx = std::accumulate(vss.all.begin(), vss.all.end(), 0l, xps);
            printf("\t%03ld %04ld", 1000*vx/n_day/vola, 1000*total.volume/vss.gbx);
            printf("\t%03ld", 1000*buy.volume/total.volume);
        } {
            auto& vl = vss.vless;
            auto v = std::accumulate(vl.begin(), vl.end(), Avsb{}, Plus);
            Av buy_ = Second(v);
            Av both = Sum(v);
            printf("\t%03d %03d", int(buy_.volume*1000/total.volume), int(buy_.volume*1000/both.volume));
        } {
            auto& vl = vss.vmass;
            auto v = std::accumulate(vl.begin(), vl.end(), Avsb{}, Plus);
            Av buy_ = Second(v);
            Av both = Sum(v);
            printf("\t%03d %03d", int(buy_.volume*1000/total.volume), int(buy_.volume*1000/both.volume));
        } {
            auto& lh = vss.lohi;
            printf("\t%03d", (lh[1]-lh[0])*100/lh[0]);
            printf("\t%ld %ld", buy.amount*100/std::max(buy.volume,1l), total.amount*100/std::max(total.volume,1l));
        }
        fprintf(stdout, "\t%d\n", n_day);
    }
    if (!empty()) {
        constexpr int W_=10000;
        constexpr int Y_=10000*10000;
        Av buy = Second(_sum);
        Av sell = First(_sum);
        Av total = Sum(_sum);
        long a = (buy.amount-sell.amount);
        long v = (buy.volume-sell.volume);
        gregorian::date::ymd_type ymd = date.year_month_day();

        fprintf(stderr,"%04d%02d%02d", int(ymd.year), int(ymd.month), int(ymd.day));
        fprintf(stderr,"\t%8.2f %8.2f %03ld"
                , total.amount/double(Y_), a/double(Y_), labs(a*100)/(total.amount/10));
        fprintf(stderr,"\t%6.2f %6.2f %03ld"
                , total.volume/double(Y_), v/double(Y_), labs(v*100)/(total.volume/10));
        fprintf(stderr, "\n");
    }

    return 0;
}

