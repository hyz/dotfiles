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
    long amount = 0;
    long volume = 0;
};
struct Pv {
    int price = 0;
    int volume = 0;
};
struct Pa // : std::pair<int,long>
{
    int price = 0;
    long amount = 0;
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
    void prep(std::string const& path, code_t code);
    void fun(std::vector<Pa> v, code_t code);
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
        m_->date = _date(argv[1]); //std::min(m_->date, _date(p));
        for (auto& di : filesystem::directory_iterator(argv[1])) {
            if (!filesystem::is_regular_file(di.path()))
                continue;
            auto && p = di.path().generic_string();
            prep( p, _code(p) );
        }
    } else {
        m_->date = _date(argv[1]);
        for (int i=2; i<argc; ++i) {
            if (!filesystem::is_regular_file(argv[i]))
                continue; // ERR_EXIT("%s: is_directory|is_regular_file", argv[i]);
            prep( argv[i], _code(argv[i]) );
        }
    }
}

void Main::init_::prep(std::string const& path, code_t code)
{
#ifndef NDEBUG
    printf("%06d %s\n", numb(code), path.c_str());
#endif
    if (FILE* fp = fopen(path.c_str(), "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
        auto xcsv = [fp,&path](Pv& pv, int& sec) {
            using qi::int_;
            using qi::float_;
            using qi::char_;

            char linebuf[256];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("%s", path.c_str());
                return 0;
            }
            float price;
            int vol;
            signed char c;

            char *pos = linebuf, *const end = &linebuf[sizeof(linebuf)];
            if (!qi::phrase_parse(pos, end, int_ >> float_ >> char_ >> int_, ','
                        , sec, price, c, vol)) {
                ERR_EXIT("%s %s", path.c_str(), pos);
            }
            pv.price = int(price*100);
            pv.volume = vol;
            return int('J') - c;
        };
        auto xtxt = [fp,&path](Pv& pv, int& sec) {
            using qi::int_;
            using qi::char_;

            char linebuf[256];
            if (!fgets(linebuf, sizeof(linebuf), fp)) {
                if (!feof(fp))
                    ERR_EXIT("%s", path.c_str());
                return 0;
            }
            int price;
            int vol;

            char *pos = linebuf, *const end = &linebuf[sizeof(linebuf)];
            if (!qi::phrase_parse(pos, end, int_ >> int_ >> int_, qi::space
                        , sec, price, vol)) {
                ERR_EXIT("%s %s", path.c_str(), pos);
            }
            pv.price = price;
            pv.volume = abs(vol);
            return vol;
        };
        auto readv = [](auto read1) {
            static const auto index = [](int xt) {
                int m = xt/100*60 + xt%100;
                return (m < 60*13-30)
                    ? std::min(std::max(m-(60*9+30), 0), 60*2)
                    : 60*2+1+std::min(std::max(m-60*13, 0), 60*4);
            };
            std::vector<Pa> vec(60*4+2);
            Av av = {};
            Pv pv;
            int price = 0;
            int xt0=0, xt;
            while ( read1(pv, xt)) {
                xt /= 100;
                if (pv.price == price || xt == xt0) {
                    av.amount += pv.price*pv.volume;
                    av.volume += pv.volume;
                } else {
                    if (av.volume) {
                        auto& e = vec[index(xt0)];
                        e.amount = av.amount/100;
                        e.price = av.amount/av.volume;
                    }
                    xt0 = xt;
                    price = pv.price;
                    av.amount = pv.price*pv.volume;
                    av.volume = pv.volume;
                }
            }
            return  std::move(vec);
        };

        if (boost::algorithm::iends_with(path, ".txt"))
            fun(readv(xtxt), code);
        else
            fun(readv(xcsv), code);
    }
}

struct XClear { template<typename T> void operator()(T*v)const{ *v=T{}; } };

template <typename I, typename F>
void walk(I b, I end, F&& fn)
{
    auto beg =b= std::find_if(b, end, [](auto&x){return x.amount;});
    if (b == end)
        return;
    auto p = b;
    ++b;
    for (; b!=end && b->price <= p->price; ++b)
        if (b->amount)
            p = b;

    if (b != end) {
        auto dnp = std::make_pair(beg, p+1);

        beg = p = b;
        ++b;
        for (; b!=end && b->price >= p->price; ++b)
            p = b;

        fn(dnp, std::make_pair(beg, b));

        if (b != end)
            walk(p, end, fn);
    }
}

auto find_last(auto& r) {
    auto it = std::find_if(r.rbegin(), r.rend(), [](auto&x){return x.amount;});
    if (it==r.rend())
        ERR_EXIT("find-last");
    return it.base();
}

void Main::init_::fun(std::vector<Pa> vpa, code_t code)
{
    const auto Idx = [&vpa](auto it) {
        int x = it-vpa.begin();
        int y = (x <= 60*2 ? 60*9+30 : 60*11-1)+x;
        return y/60*100 + y%60;
    };
    const auto rindex = [](int x) {
        int y = (x <= 60*2 ? 60*9+30 : 60*11-1)+x;
        return y/60*100 + y %60;
    };

    typedef std::vector<Pa>::iterator iterator;
    // 600712  161     5.43    002     68 936,4,017 1048,13,018 1354,16,060 1447,5,018 953,30,048

    std::vector<std::pair<iterator,iterator>> ps;//, gs;

    auto psx = [&ps,&Idx](auto&& p0, auto&&p1) {
        //BOOST_ASSERT(p0.first);
#ifndef NDEBUG
        printf("R %d-%d,%d-%d\n", Idx(p0.first), Idx(p0.second), Idx(p1.first), Idx(p1.second));
#endif
        if (ps.empty())
            ps.push_back(p1);
        else {
            auto last = std::prev(ps.end());
            int x = p0.first->price - std::prev(p0.second)->price;
            int y = std::prev(p1.second)->price - last->first->price;
            int z = std::prev(p1.second)->price - p0.first->price;
            if ((3*x < z)&&(10*x < y || 10*x < z || 15*x < y+z)) {
                last->second = p1.second;
#ifndef NDEBUG
                printf("M %d-%d %d %d %d\n", Idx(last->first), Idx(last->second), x, y, z);
#endif
            } else {
                ps.push_back(p1);
            }
        }
    };

    walk(vpa.begin(),vpa.end(), psx);

    if (ps.empty())
        return;

    auto it = ps.begin() + ps.size() - std::min(5lu,ps.size());

    auto lt1 = [](auto&p1, auto&p2) {
        return (std::prev(p1.second)->price - p1.first->price)
             < (std::prev(p2.second)->price - p2.first->price);
    };
    std::nth_element(ps.begin(), it, ps.end(), lt1);

    auto add = [](Pa& pa, iterator it, iterator end) {
        auto Chr = [](int b, int d) { return 1000*(d-b)/b; };
        pa.price += Chr(it->price, std::prev(end)->price);
        pa.amount = std::accumulate(it,end, pa.amount, [](long x, auto& y) { return x+y.amount; });
    };

    Pa pa = {};
    int m = 0;
    printf("%06d", numb(code));
    for (auto i=it; i != ps.end(); ++i) {
        m += int(i->second - i->first);
        add(pa, i->first, i->second);
    }

    constexpr int W=10000;
    printf("\t%03d\t%.2f\t%03d\t%d"
            , pa.price, 10.0*pa.amount/(100*W)/pa.price , pa.price/m, m);

    for (auto i=it; i != ps.end(); ++i)
        if (i->first->price)
        printf(" %d,%d,%03d"
            , Idx(i->first), int(i->second - i->first)
            , (std::prev(i->second)->price - i->first->price)*1000/i->first->price
            );
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

