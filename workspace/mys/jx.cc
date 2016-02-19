#include <stdio.h>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <boost/algorithm/string/predicate.hpp>
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
#include <boost/iterator/filter_iterator.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/range/iterator_range.hpp>

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

using namespace boost::multi_index;

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
};
BOOST_FUSION_ADAPT_STRUCT(Av, (long,volume)(long,amount))

struct Pa // : std::pair<int,long>
{
    int price ;// = 0;
    long amount ;// = 0;
};
//BOOST_FUSION_ADAPT_STRUCT(Pa, (int,price)(long,amount))

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
            , -lit('/') >> *(qi::omit[+(char_-'/')] >> '/')
                 >> _4digit >> _2digit >> _2digit // >> '/' >> qi::omit[+(char_-'/')]
            , y, m, d))
        ERR_EXIT("%s: not-a-date", s.c_str());
    return gregorian::date(y,m,d);
}

struct Elem : std::vector<Pa>
{
    int code = 0;
    long gbx = 0;
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

struct SInfo
{
    long gbx, gbtotal;
    int eov;
};
BOOST_FUSION_ADAPT_STRUCT(SInfo, (long,gbx)(long,gbtotal)(int,eov))
struct Main::init_ : std::unordered_map<int,SInfo> , boost::noncopyable
{
    Main* m_ = 0;
    Elem tmp_ = {};
    init_(Main* p, int argc, char* const argv[]);

    void loadsi(char const* fn);
    void prep(gregorian::date d, char const* fn/*, int xbeg, int xend*/);
    void fun(code_t code, std::vector<Pa>& v);
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
        std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
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
                ERR_EXIT("qi::parse: %s %s", fn, pos);
            }
            if (si.gbx > 0)
                this->emplace(make_code(szsh,code), si);
            else
                fprintf(stderr, "%d\n", code);
        }
    } else
        ERR_EXIT("fopen: %s", fn);
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
            auto && p = di.path().generic_string();
            prep( _date(p.c_str())/*_code(p)*/, p.c_str() );
        }
    } else {
        for (int i=1; i<argc; ++i) {
            if (!filesystem::is_regular_file(argv[i]))
                continue;
            prep( _date(argv[i])/*_code(argv[i])*/, argv[i] );
        }
    }
}

typedef fusion::vector<Av,Av> Avsb;
auto First  = [](auto&& x) -> auto& { return fusion::at_c<0>(x); };
auto Second = [](auto&& x) -> auto& { return fusion::at_c<1>(x); };
Avsb operator+(Avsb const& x, Avsb const& y) {
    return fusion::make_vector(First(x)+First(y), Second(x)+Second(y));
}
//static const auto Plus = [](Avsb const& x, Avsb const& y) {
//    return fusion::make_vector(First(x)+First(y), Second(x)+Second(y));
//};

struct XClear { template<typename T> void operator()(T*v)const{ v->clear(); } };

void Main::init_::prep(gregorian::date d, char const* fn/*, int xbeg, int xend*/)
{
    //ERR_MSG("info: %d,%d\n", xbeg, xend);
    if (FILE* fp = fopen(fn, "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
        //typedef boost::iterator_range<std::vector<Avsb>::iterator> rng_t;
        auto read = [fp](std::vector<Pa>& vec) {
            vec.reserve(60*4+2); //using qi::long_; //using qi::_val; using qi::_1;

            qi::rule<char*, Av(), qi::space_type> R_Av = qi::long_ >> qi::long_;
            //qi::rule<char*, Avsb(), qi::space_type> R_ =  R_Av >> R_Av;

            char linebuf[(60*4+8)*64];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("fgets");
                return 0u;
            }
            int szsh=0, code = 0;
            char*const end = &linebuf[sizeof(linebuf)];
            char* pos = linebuf;
            if (qi::phrase_parse(pos,end, qi::int_ >> qi::int_, qi::space, code, szsh)) {
                Av av0, av1; //Avsb avsb;
                while (qi::phrase_parse(pos,end, R_Av>>R_Av, qi::space, av0, av1)) {
                    Av av = av0 + av1;
                    vec.push_back( {int(av.amount*100/std::max(av.volume,1l)), av.amount} );
                }
            } else
                ERR_EXIT("qi:parse: %s", pos);
            return make_code(szsh,code);
        };

        std::vector<Pa> vec;
        while (code_t code = read(vec)) {
            std::unique_ptr<decltype(vec),XClear> xclear(&vec);
            if (vec.size() != 60*4+2) {
                ERR_EXIT("%d :size-error", numb(code));
            }
            this->fun(code, vec);
        }
    }
}

template <typename I, typename F>
void _walkimpl(I b, I end, F&& fn)
{
    auto p = b;
    ++b;
    auto beg = p;
    for (; b!=end && b->price <= p->price; ++b)
        if (b->price)
            p = b;

    if (b != end) {
        auto dnp = std::make_pair(beg, p);

        beg = p; //++b; //if ( (b = skipz(b+1, end)) == end) return;
        for (; b!=end && (b->price >= p->price || !b->price); ++b)
            if (b->price)
                p = b;

        fn(dnp, std::make_pair(beg, p));

        if (b != end)
            _walkimpl(p, end, fn);
    }
}
template <typename I, typename F>
void walk(I b, I end, F&& fn)
{
    //static const auto skipz = [](auto it, auto end) {
    //    while (it != end && !it->amount)
    //        ++it;
    //    return it;
    //};
    while (b != end && !b->amount)
        ++b;
    if (b != end)
        _walkimpl(b,end, fn);
}


void Main::init_::fun(code_t code, std::vector<Pa>& vpa)
{
    //Elem* vss = this->address(code);
    //if (!vss) {
    //    fprintf(stderr, "%d :address-fail\n", numb(code));
    //    return;
    //}

    const auto Minx = [&vpa](auto it) {
        int x = it-vpa.begin();
        int y = 60*9+30 + (x <= 60*2 ? x : x-1+90);
        return y/60*100 + y%60;
    };

    typedef std::vector<Pa>::iterator iterator;

    array<std::vector<std::pair<iterator,iterator>>,2> pv;

    auto psx = [&pv](auto&&la0, auto&&la1) {
        auto conc = [&pv]() {
            auto prev = pv[1].rbegin()+1; //std::prev(pv[1].end());
            auto& la0 = pv[0].back();
            auto& la1 = pv[1].back();

            int x = la0.first->price - la0.second->price;
            int y = la1.second->price - la1.first->price;
            int z = prev->second->price - prev->first->price;
            if ((2*x < y)&&(2*x < z)&&(5*x < y+z)&&(la0.second-la0.first < 5)) {
                prev->second = la1.second;
                pv[0].pop_back();
                pv[1].pop_back();
                //printf("M %d-%d %d %d %d\n", Minx(prev->first), Minx(prev->second), x, y, z); //Debug
            }
        };
        //BOOST_ASSERT(la0.first);
        //printf("R %d-%d,%d-%d\n", Minx(la0.first), Minx(la0.second), Minx(la1.first), Minx(la1.second)); //Debug
        if (pv[1].empty() || &pv[1].front() == &pv[1].back()) {
            pv[0].push_back(la0);
            pv[1].push_back(la1);
        } else {
            pv[0].push_back(la0);
            pv[1].push_back(la1);
            conc();
        }
    };
    walk(vpa.begin(),vpa.end(), psx);
    if (pv[1].empty())
        return;
    auto& pv1 = pv[1];

    auto it = pv1.begin() + pv1.size() - std::min(6lu,pv1.size());

    auto lt1 = [](auto&p1, auto&p2) {
        return (p1.second->price - p1.first->price)
             < (p2.second->price - p2.first->price);
    };
    std::nth_element(pv1.begin(), it, pv1.end(), lt1);

    auto Add = [](long&amount, int&chr, iterator it, iterator last) {
        auto Chr = [](int b, int d) { return 1000*(d-b)/b; };
        chr += Chr(it->price, last->price);
        amount += std::accumulate(it,last+1, 0l, [](long x, auto& y) { return x+y.amount; });
    };

    long amount0 = std::accumulate(vpa.begin(),vpa.end(), 0l, [](long x, auto&y){return x+y.amount;});
    long amount=0;
    int m=0, chr=0;
    for (auto i=it; i != pv1.end(); ++i) {
        m += int(i->second - i->first)+1;
        Add(amount, chr, i->first, i->second);
    }
    if (chr == 0)
        return;
    constexpr int W=10000;

    printf("%06d", numb(code));
    printf("\t%4.2f %4.2f %03ld\t%2d %3d\t%4ld"
            , double(amount0)/W/W, double(amount)/W/W, 1000*amount/amount0, m, chr, amount/chr/W);

    //auto Pos = [&vpa](auto&p1, auto&p2) { return (p1.first < p2.first); };
    //std::sort(it,pv1.end(), Pos);
    //for (auto i=it; i != pv1.end(); ++i) printf(" %d,%d,%03d" , Minx(i->first), int(i->second - i->first)+1 , (i->second->price - i->first->price)*1000/i->first->price);
    printf("\n");
}

Main::Main(int argc, char* const argv[])
    : date(gregorian::day_clock::local_day())
{
    init_ ini(this, argc, argv);
    (void)ini;
}

int Main::run(int argc, char* const argv[])
{
    //constexpr int Wn=10000;
    //constexpr long Yi=Wn * Wn;

    //Avsb _sum = {};

    //for (Elem const & vss : *this) {
    //    _sum = Plus(_sum, vss.sum);
    //    int n_day = vss.n_day();
    //    Av sell = First(vss.sum);
    //    Av buy = Second(vss.sum);
    //    Av total = buy + sell;
    //    Av b = buy - sell;

    //    printf("%06d", numb(vss.code));
    //    {
    //        long lsz = vss.gbx*Price(total);
    //        printf("\t%7.2f", double(lsz)/Yi);
    //        printf("\t% 6.2f %5.2f %03ld", b.amount/double(Yi), buy.amount/double(Yi), 1000*buy.amount/total.amount);
    //    } {
    //        auto& lh = vss.lohi;
    //        printf("\t%03d", (lh[1]-lh[0])*1000/lh[0]);

    //        long vola = total.volume/n_day;
    //        auto exval = [&vola](long a, Avsb const& x){
    //            return a + labs(Sum(x).volume - vola);
    //        };
    //        long ex = std::accumulate(vss.all.begin(), vss.all.end(), 0l, exval);
    //        printf("\t%03ld", 1000*ex/n_day/vola);

    //        printf("\t%03ld", 1000*total.volume/vss.gbx);
    //    } {
    //        auto& vl = vss.vless;
    //        auto v = std::accumulate(vl.begin(), vl.end(), Avsb{}, Plus);
    //        Av buy_ = Second(v);
    //        Av both = Sum(v);
    //        printf("\t%03d %03d", int(buy_.volume*1000/total.volume), int(buy_.volume*1000/both.volume));
    //    } {
    //        auto& vl = vss.vmass;
    //        auto v = std::accumulate(vl.begin(), vl.end(), Avsb{}, Plus);
    //        Av buy_ = Second(v);
    //        Av both = Sum(v);
    //        printf("\t%03d %03d", int(buy_.volume*1000/total.volume), int(buy_.volume*1000/both.volume));
    //    }
    //    printf("\t%d\n", n_day);
    //}
    //if (!empty()) {
    //    Av buy = Second(_sum);
    //    Av sell = First(_sum);
    //    Av total = buy + sell;
    //    Av b = buy - sell;
    //    gregorian::date::ymd_type ymd = date.year_month_day();

    //    fprintf(stderr,"%04d%02d%02d", int(ymd.year), int(ymd.month), int(ymd.day));
    //    fprintf(stderr,"\t%8.2f %8.2f %03ld"
    //            , total.amount/double(Yi), b.amount/double(Yi), labs(b.amount)*100/(total.amount/10));
    //    fprintf(stderr,"\t%6.2f %6.2f %03ld"
    //            , total.volume/double(Yi), b.volume/double(Yi), labs(b.volume)*100/(total.volume/10));
    //    fprintf(stderr, "\n");
    //}

    return 0;
}

