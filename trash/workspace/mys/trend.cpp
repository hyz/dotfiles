#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <array>
#include <vector>
#include <algorithm>
#include <numeric>
#include <memory>
#include <unordered_map>
#include <string>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/operators.hpp>
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
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
//#include <boost/multi_index/ordered_index.hpp>

typedef long Long;

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...);
    fputc('\n', stderr);
    //fflush(stderr);
}
#define ERR_MSG(...) err_msg_(__LINE__, "E%d: " __VA_ARGS__)
#define DBG_MSG(...) err_msg_(__LINE__, "D%d: " __VA_ARGS__)

template <typename Iter>
static boost::iterator_range<Iter> iter_trim(Iter h, Iter end)
{
    while (end > h && isspace(*(end-1)))
        --end;
    while (h < end && isspace(*h))
        ++h;
    return boost::make_iterator_range(h,end);
}
template <typename Iter>
static boost::iterator_range<Iter> iter_trim_right(Iter h, Iter end, const char* cs)
{
    while (end > h && strchr(cs, *(end-1)))
        --end;
    //while (h < end && strchr(cs, *h))
    //    ++h;
    return boost::make_iterator_range(h,end);
}
static boost::iterator_range<char*> iter_trim_right(char* s, const char* cs) {
    return iter_trim_right(s, s + strlen(s), cs);
}

template <typename... V> struct Path_join ;
template <typename T, typename... V>
struct Path_join<T,V...> { static void concat(char*beg,char*end, T const& a, V const&... v) {
    Path_join<typename std::decay<T>::type>::concat(beg,end, a);
    Path_join<typename std::decay<V>::type...>::concat(beg,end, v...);
}};
template <> struct Path_join<char*> { static void concat(char*beg,char*end, char const* src) {
    char* p = beg + strlen(beg); // char* src = a;
    if (p != beg && p != end)
        *p++ = '/';
    while (p != end && (*p++ = *src++))
        ;
    *iter_trim_right(beg, "/\\").end() = '\0';
}}; 
template <> struct Path_join<char const*> { static void concat(char*beg,char*end, char const* src) {
    Path_join<char*>::concat(beg,end, src);
}};
template <> struct Path_join<std::string> { static void concat(char*beg,char*end, std::string const& src) {
    Path_join<char*>::concat(beg,end, src.c_str());
}};
template <> struct Path_join<boost::format> { static void concat(char*beg,char*end, boost::format const& src) {
    auto s = str(src);
    Path_join<char*>::concat(beg,end, s.c_str());
}};

template <int L=128>
struct joinp : std::array<char,L> {
    template <typename ...V>
    joinp(V&&... v) {
        this->front() = this->back() = '\0';
        char* end = &this->back(); //+1;
        char* beg = c_str();
        Path_join<typename std::decay<V>::type...>::concat(beg,end, v...);
    }
    char* c_str() const { return const_cast<char*>(this->data()); }
};

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

struct Av : boost::addable<Av,boost::subtractable<Av,boost::dividable2<Av,int>>> {
    Long volume = 0;
    Long amount = 0;

    Av& operator+=(Av const& lhs) { amount += lhs.amount; volume += lhs.volume; return *this; }
    Av& operator-=(Av const& lhs) { amount -= lhs.amount; volume -= lhs.volume; return *this; }
    Av& operator/=(int x) { volume /= x; amount /= x; return *this; }
};
BOOST_FUSION_ADAPT_STRUCT(Av, (Long,volume)(Long,amount))

//typedef fusion::vector<Av,Av> Avsb;
//auto First  = [](auto&& x) -> auto& { return fusion::at_c<0>(x); };
//auto Second = [](auto&& x) -> auto& { return fusion::at_c<1>(x); };
//
//static const auto Sum = [](Avsb const& avsb) { return fusion::at_c<0>(avsb) + fusion::at_c<1>(avsb); };
//static const auto Plus = [](Avsb const& x, Avsb const& y) {
//    Av const a = fusion::at_c<0>(x) + fusion::at_c<0>(y);
//    Av const b = fusion::at_c<1>(x) + fusion::at_c<1>(y);
//    return fusion::make_vector(a, b);
//};
//[](auto&& x) -> decltype(fusion::at_c<0>(x))& { return fusion::at_c<0>(x); };
//static auto Lohi = [](auto& rng) {
//    auto cheap = [](auto&x, auto&y) {
//        Av a = Sum(x);
//        Av b = Sum(y);
//        return (a.amount/a.volume) < (b.amount/b.volume);
//    };
//    return std::minmax_element(std::begin(rng), std::end(rng), cheap);
//};
//
//inline int Price(Av const& v) {
//    if (!v.volume)
//        return 1;
//    return int(v.amount*100/v.volume);
//};
//inline int Price(Avsb const& x) { return Price(Sum(x)); };

//struct RecBS : Av { char bsflag; }; BOOST_FUSION_ADAPT_STRUCT(RecBS, (float,amount)(char,bsflag)(float,volume))

struct code2_t { unsigned szsh : 4, numb : 28; };

typedef unsigned code_t;
inline code_t Make_code(int szsh, int numb) { return ((szsh<<24)|numb); }
inline int Numb(code_t v_) { return v_&0x0ffffff; }
inline int Szsh(code_t v_) { return (v_>>24)&0xff; }
//static code_t _code(std::string const& s)
//{
//    static const qi::int_parser<int,10,6,6> _6digit = {};
//    using qi::char_;
//    using qi::lit;
//    struct SZSH_ : qi::symbols<char, int> { SZSH_() { add ("SZ",0) ("SH",1); } } SZSH;
//
//    int x, y;
//    auto it = s.cbegin();
//    if (!qi::parse(it, s.cend()
//            , -lit('/') >> *qi::omit[+(char_-'/') >> '/']
//                 >> SZSH >> _6digit // >>'.'>>qi::no_case[lit("csv")|"txt"]
//            , x, y))
//        ERR_EXIT("%s: not-a-code", s.c_str());
//    return Make_code(x,y);
//}

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

template <typename I, typename BinaryOperation> // boost::make_function_output_iterator
void for2(I first, I second, I end, BinaryOperation&& op) // -> array<decltype(*it),2>
{
    op(first, second);
    while (++second != end) {
        op(++first, second);
    }
}

template <typename It, typename Iter, typename Cmp> // boost::make_function_output_iterator
auto Ma(unsigned n, It it, It end, Iter iter, Cmp&& cmp) // -> array<decltype(*it),2>
{
    typename It::value_type a{}; // decltype(*it) a{};
    std::vector<It> ar(n);
    unsigned x=0;
    for (; x < ar.size() && it != end; ++x, ++it) {
        a += *it;
        ar[x] = it;
    }
    auto b = a/ar.size();
    array<decltype(b),2> ret={{b,b}};

    if (x == ar.size()) {
        *iter++ = b;

        for (; it != end; ++it) {
            unsigned i = x++ % n;
            a -= *ar[i];
            a += *it;
            ar[i] = it;
            *iter++ = b = a/n;
            if (cmp(b, ret[0])) {
                ret[0] = b;
            } else if (!cmp(b, ret[1])) {
                ret[1] = b;
            }
        }
    }
    return ret;
}

struct Opstatus {
    int mll = 0;
    int incoming = 0;
    std::string detail;
};
struct SInfo {
    Long capital1 = 0; // capital stock in circulation
    Long capital0 = 0; // general capital
    int eps = 0; // earnings per share(EPS)
};
BOOST_FUSION_ADAPT_STRUCT(SInfo, (Long,capital1)(Long,capital0)(int,eps))

struct Unit : Av {
    array<int,2> oc = {}, lohi = {};
};

struct Elem : std::vector<Unit>, Unit, SInfo, Opstatus {
    unsigned code = 0;
    static Elem make(int szsh, int numb) { Elem e; e.code=Make_code(szsh,numb); return e; }
    int numb() const { return Numb(code); }
};

typedef boost::multi_index::multi_index_container<Elem, indexed_by<
            random_access<>, hashed_unique<member<Elem,unsigned,&Elem::code>>
        >> multi_index_t;

struct Main : multi_index_t
{
    Main(int argc, char* const argv[]);
    int run(int argc, char* const argv[]);

    gregorian::date date;
    // short n_ign_ = 0, n_day_ = 15;
    int NDays_Average = 5, NDays_Minimal=15, NDays_Skip=0;
    int DebugNumb = 0;

    struct initializer; friend struct initializer;
};

struct Main::initializer : multi_index_t //std::unordered_map<int,SInfo>, boost::noncopyable
{
    initializer(Main* p, int argc, char* const argv[]);

    iterator find(int szsh, int code) {
        auto & idc = get<1>();
        auto it = idc.find(Make_code(szsh,code));
        if (it == idc.end()) {
            return end(); //ERR_EXIT("add: %d", Numb(code));
        }
        return project<0>(it);
    }

    void loadsi(char const* dir, char const* fn);
    void loadops(char const* dir, char const* fn);
    void loadx(int nskip, char const* path);
};

int main(int argc, char* const argv[])
{
    BOOST_STATIC_ASSERT(sizeof(Long)==8);
    BOOST_STATIC_ASSERT(sizeof(code2_t)==sizeof(unsigned));
    try {
        Main a(argc, argv);
        return a.run(argc, argv);
    } catch (std::exception const& e) {
        fprintf(stderr, "%s\n", e.what());
    }
    return 1;
}

//Elem* Main::initializer::add(int code, Elem&& el)
//{
//    //auto & idc = a_->get<1>();
//    //auto it = idc.find(code);
//    //if (it != idc.end()) {
//    //    ERR_EXIT("add: %d", Numb(code));
//    //}
//    auto si = this->find(code);
//    if (si == this->end()) {
//        ERR_MSG("%06d: sinfo not found", Numb(code));
//        return 0;
//    }
//    static_cast<SInfo&>(el) = si->second;
//    el.code = code;
//    auto p = a_->push_back( std::move(el) );
//    if (!p.second)
//        ERR_EXIT("%06d: push_back", Numb(code));
//    return &const_cast<Elem&>(*p.first);
//}

Main::initializer::initializer(Main* m, int argc, char* const argv[])
{
    if (argc < 2) {
        ERR_EXIT("%s argc: [-dN] [-iN] <X.MMDD-N>", argv[0], argc);
    }
    
    int opt;
    while ( (opt = getopt(argc, argv, "n:m:c:")) != -1) {
        switch (opt) {
            case 'n': m->NDays_Average = atoi(optarg); break;
            case 'm': m->NDays_Minimal = atoi(optarg); break;
            case 's': m->NDays_Skip = atoi(optarg); break;
            case 'c': m->DebugNumb = atoi(optarg); break;
        }
    }

    loadsi(getenv("HOME"), "_/_sinfo");
    loadops(getenv("HOME"), "_/_opstatus");

    if (filesystem::is_directory(argv[optind])) {
        for (auto& di : filesystem::directory_iterator(argv[optind])) {
            if (!filesystem::is_regular_file(di.path()))
                continue;
            auto & p = di.path();
            loadx(m->NDays_Skip, p.generic_string().c_str() );
            //m->date = std::min(m->date, _date(p.generic_string()));
        }
    } else for (int i=optind; i<argc; ++i) {
        if (!filesystem::is_regular_file(argv[i]))
            continue; // ERR_EXIT("%s: is_directory|is_regular_file", argv[i]);
        loadx(m->NDays_Skip, argv[i]);
        //m->date = std::min(m->date, _date(argv[i]));
    }

    //this->remove_if([this](auto&e){return e.size()<unsigned(n_day_+n_ign_);});
    this->swap(*m);
    //DBG_MSG("rec %d %d", int(size()), int(m->size()));
}

void Main::initializer::loadx(int nskip, char const* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp)
        ERR_EXIT("fopen: %s", path);
    std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
    //qi::rule<char*, Av(), qi::space_type> R_Av = qi::long_ >> qi::long_;
    //qi::rule<char*, Avsb(), qi::space_type> R_ =  R_Av >> R_Av;

    char linebuf[1024*16]; //[(60*4+15)*16+256];
    while (fgets(linebuf, sizeof(linebuf), fp)) {
        int szsh = 0, numb = 0;
        char*const end = &linebuf[sizeof(linebuf)];
        char* pos = linebuf;
        if (qi::phrase_parse(pos,end, qi::int_>>qi::int_, qi::space, numb, szsh)) {
            auto iter = this->find(szsh,numb);
            if (iter == this->end()) {
                DBG_MSG("%u: not-found", numb);
                continue;
            }
            std::vector<Unit> uvec;// = elem;;
            Unit usum = {}, ulas = {};
            int nrec = 0;
            auto parse = [&pos,end](Unit& u) {
                using qi::int_;
                using qi::long_;
                return qi::phrase_parse(pos,end
                        , long_>>long_>>int_>>int_>>int_>>int_, qi::space
                        , u.volume, u.amount, u.oc[0], u.oc[1], u.lohi[0], u.lohi[1]);
            };

            while (nskip-- > 0 && parse(usum))
                ++nrec;
            while (usum.volume == 0 && parse(usum))
                ++nrec;
            while (parse(ulas)) {
                ++nrec;
                if (ulas.volume) {
                    if (usum.lohi[0] > ulas.lohi[0])
                        usum.lohi[0] = ulas.lohi[0];
                    if (usum.lohi[1] < ulas.lohi[1])
                        usum.lohi[1] = ulas.lohi[1];
                    usum.oc[1] = ulas.oc[1];
                    static_cast<Av&>(usum) += static_cast<Av&>(ulas);
                    uvec.push_back(ulas);
                }
            }

            static int nrec0 = nrec;
            if (nrec != nrec0)
                ERR_EXIT("%d %d!=%d", numb, nrec, nrec0);

            if (ulas.volume <= 0) {
                fprintf(stderr, "%06d stopped\n",numb);
                continue;
            }
            auto & el = const_cast<Elem&>(*iter);
            static_cast<std::vector<Unit>&>(el) = std::move(uvec);
            static_cast<Unit&>(el) = usum;
            // if (uvec.empty()) { ERR_EXIT("%u: empty", numb); }
        } else {
            ERR_EXIT("qi::parse: %s", pos);
        }
    }
    if (!feof(fp)) {
        ERR_EXIT("feof: %s", path);
    }
}
void Main::initializer::loadops(char const* dir, char const* fn)
{
    if (FILE* fp = fopen(joinp<>(dir,fn).c_str(), "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
        using qi::long_; //using qi::_val; using qi::_1;
        using qi::int_;
        qi::rule<char*, SInfo(), qi::space_type>
            R_ = long_ >> long_ >> qi::omit[long_]>>qi::omit[long_] >> int_;

        char linebuf[1024];
        while (fgets(linebuf, sizeof(linebuf), fp)) {
            int szsh=0, code, cat;
            char* pos = linebuf;
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , int_ >> int_ >> int_
                        , qi::space, code, szsh, cat)) {
                ERR_EXIT("qi::parse: %s %s", fn, pos);
            }
            if (cat != 3) {
                continue;
            }
    
            auto iter = this->find(szsh,code);
            if (iter == this->end())
                continue;

            Opstatus& ops = const_cast<Elem&>( *iter );
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , int_ >> int_
                        , qi::space, ops.mll, ops.incoming)) {
                ERR_EXIT("qi::parse: %s %s", fn, pos);
            }
            auto r = iter_trim(pos, pos+strlen(pos));
            ops.detail.assign(r.begin(),r.end());
        }
    } else
        ERR_EXIT("fopen: %s %s", dir, fn);
}
void Main::initializer::loadsi(char const* dir, char const* fn)
{
    joinp<> path(dir,fn);
    if (FILE* fp = fopen(path.c_str(), "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
        using qi::long_; //using qi::_val; using qi::_1;
        using qi::int_;
        qi::rule<char*, SInfo(), qi::space_type>
            R_ = long_ >> long_ >> qi::omit[long_]>>qi::omit[long_] >> int_;

        char linebuf[1024];
        while (fgets(linebuf, sizeof(linebuf), fp)) {
            int szsh=0, numb;
            SInfo si = {};
            char* pos = linebuf;
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , int_ >> int_ >> R_
                        , qi::space, numb, szsh, si)) {
                ERR_EXIT("qi::parse: %s %s", fn, pos);
            }
            if (si.capital1 > 0) {
                auto p = insert(end(), Elem::make(szsh,numb));
                Elem& el = const_cast<Elem&>( *p.first );
                static_cast<SInfo&>(el) = si;
            } else
                ERR_MSG("%06d capital1 %ld", numb, si.capital1);
            //if (numb==00570) DBG_MSG("%d %ld", numb, si.capital1);
        }
    } else
        ERR_EXIT("fopen: %s %s: %s", dir, fn, path.c_str());
}

Main::Main(int argc, char* const argv[])
    : date(gregorian::day_clock::local_day())
{
    initializer ini(this, argc, argv);
    (void)ini;
}

int Main::run(int argc, char* const argv[])
{
    // auto null_iter = boost::make_function_output_iterator([](auto&){});
    constexpr int Wn=10000;
    constexpr int Yi=Wn * Wn;

    for (Elem const & el : *this) {
        if ((int)el.size() <= NDays_Minimal+5)
            continue;
        if (DebugNumb > 0 && el.numb() != DebugNumb)
            continue;

        std::vector<int> pcs; {
            std::vector<int> values;
            auto first = el.rbegin(), second = el.rbegin() + NDays_Average;
            for2(first,second, el.rend(), [&values](auto first, auto second) {
                        Av a = std::accumulate(first,second, Av{}
                                , [](auto const&x0, auto const&x){ return x0 + x; });
                        values.push_back( a.amount*100/a.volume );
                    });
            if (el.numb() == DebugNumb) { //debug
                printf("%d", DebugNumb);
                for (int v : values)
                    printf(" %d", v);
                printf("\n");
            }
            int xs[2] = {0,0};
            for2(values.begin(), values.begin()+1, values.end(), [&pcs,&xs](auto first, auto second) {
                        xs[0]++;
                        if (*first >= *second)
                            xs[1]++;
                        pcs.push_back(xs[1]*1000/xs[0]);
                    });
        }
        if ((int)pcs.size() < NDays_Minimal) {
            continue;
        }
        printf("%06d %7.2f", Numb(el.code), el.capital0*el.oc[1]/100.0/Yi);
        {
            auto maxp = std::max_element(pcs.begin()+NDays_Minimal, pcs.end());
            printf("\t% .2d %- .3d", int(maxp-pcs.begin()), *maxp);
        } {
            typedef std::vector<unsigned>::iterator iterator;
            std::vector<unsigned> vols;
            vols.reserve(el.size()); // vols5.reserve(el.size());
            for (auto i = el.rbegin(), end = el.rend(); i != end; ++i) { // boost::adaptors::reversed(el)
                vols.push_back(i->volume);
            }

            unsigned vold = 0;
            for2(vols.begin(), vols.begin()+1, vols.end(), [&vold](auto first, auto second) {
                vold += std::abs(int(*first) - int(*second));
            });

            std::sort(vols.begin(), vols.end());
            int vlen = (int)vols.size();

            struct { iterator pos; int ri; } p = {};
            for2(vols.begin()+vlen/3, vols.begin()+vlen/3+1, vols.end(), [&p](auto first,auto second) {
                int x = (*second - *first)*100 / *first;
                if (x > p.ri) {
                    p.ri = x;
                    p.pos = second;
                }
            });
            int ln = p.pos - vols.begin();
            int hn = vols.end() - p.pos;
            unsigned ls = std::accumulate(vols.begin(),p.pos, 0u, [](unsigned s, unsigned x){ return s+x; });
            unsigned hs = std::accumulate(p.pos,  vols.end(), 0u, [](unsigned s, unsigned x){ return s+x; });
            unsigned la = ls / ln;
            unsigned ha = hs / hn;
            unsigned l0 = vols.front(), l1 = *(vols.begin()+1);
            unsigned h0 = vols.back(), h1 = *(vols.rbegin()+1);

            vold = vold/(ln+hn-1) *100 / ((ls+hs)/(ln+hn));
            unsigned vol0 = el.back().volume;
            unsigned vola5 = std::accumulate(el.rbegin()+1,el.rbegin()+6, 0u, [](unsigned x0, Av const&x){ return x0+x.volume; }) / 5;

            printf("\t% .3d % .3d % .3d % .3d % .3d % .3d", vol0*100/vola5, vold, ha*100/la, h0*100/ha, h0*100/h1, l0*100/la);
        }
        printf("\t%2d\n", (int)el.size());

        //. //if (el.size() < unsigned(n_day_+n_ign_)) {
        //. //    DBG_MSG("%u: %u < %u + %u", Numb(el.code), el.size(), unsigned(n_day_), n_ign_);
        //. //    continue;
        //. //}
        //. typedef Elem::const_iterator iterator;
        //. //iterator const begin0 = el.begin();
        //. iterator const end0 = el.end() - n_ign_;
        //. iterator const beg0 = end0 - n_day_;
        //. iterator const last = end0-1;
        //. iterator const lasp = last-1;;
        //. // boost::reverse_iterator<iterator> rend(beg0), rbegin(end0);

        //. std::vector<iterator> ivec(end0 - beg0);
        //. //boost::generator_iterator_generator<my_generator>::type it = boost::make_generator_iterator(gen);
        //. //boost::make_generator_iterator([b=beg0]()mutable{return b++;});
        //. //make_function_input_iterator(f,0), make_function_input_iterator(f,10),
        //. //std::generate(ivec.begin(),ivec.end(), [b=beg0]()mutable{return b++;});

        //. std::copy(boost::make_counting_iterator(beg0), boost::make_counting_iterator(end0), ivec.begin());
        //. auto const eb1 = std::partition(ivec.begin(),ivec.end(), [](auto&i){return i->oc[0]<i->oc[1];});
        //. auto const beg1 = ivec.begin();
        //. auto const end1 = ivec.end();

        //. std::sort(beg1,eb1, [](auto&l,auto&r){return l->volume>r->volume;});
        //. std::sort(eb1,end1, [](auto&l,auto&r){return l->volume>r->volume;});

        //. //std::nth_element(ivec.begin(), ivec.begin()+3, ivec.end(), [](auto&l,auto&r){return l->volume<r->volume;});
        //. //std::nth_element(ivec.begin()+3, ivec.begin()+3+5, ivec.end(), [](auto&l,auto&r){return l->volume>r->volume;});
        //. //ivec.resize(3+5);
        //. //iterator near0 = *std::min_element(ivec.begin(),ivec.begin()+3, [end](auto&l,auto&r){return (end-l) < (end-r);});
        //. //ivec.erase(std::remove_if(ivec.begin()+3, ivec.end(), [near0](auto&x){return x>near0;}), ivec.end());
        //. //auto const beg0 = ivec.begin();

        //. auto const beg2 = boost::make_indirect_iterator(beg1); //ivec.begin()+3;
        //. auto const eb2 = boost::make_indirect_iterator(eb1); //ivec.begin()+3;
        //. auto const end2 = boost::make_indirect_iterator(end1); //ivec.begin()+3;

        //. // auto const end2 = ivec.end();
        //. //iterator near1 = *std::max_element(beg2,end2, [end](auto&l,auto&r){return (end-l) < (end-r);});
        //. //std::sort(beg2, end2);
        //. //Long volRa = std::accumulate(beg2,end2, Long{},[](Long a,auto&x){return a+x->volume;});
        //. Long volR1 = std::max_element(beg2,eb2, [](auto&l,auto&r){return l.volume<r.volume;})->volume;
        //. //Long volG1 = std::max_element(eb2,end2, [](auto&l,auto&r){return l.volume<r.volume;})->volume;
        //. //Long volR0 = std::min_element(beg2,eb2, [](auto&l,auto&r){return l.volume<r.volume;})->volume;
        //. //Long volG0 = std::min_element(eb2,end2, [](auto&l,auto&r){return l.volume<r.volume;})->volume;
        //. Long volRa = std::accumulate(beg2,eb2, Av{}).volume;
        //. Long volGa = std::accumulate(eb2,end2, Av{}).volume;
        //. int nR = eb1 - beg1;
        //. int nG = end1 - eb1;
        //. //Long vol0 = std::min(volR0, volG0);
        //. //Long volMr = std::max_element(near0,end0, [](auto&l,auto&r){return l.volume<r.volume;})->volume;
        //. //Long volm = near0->volume; //std::accumulate(beg0,beg2, Av{}).volume;

        //. //Long vola = std::accumulate(beg0,end0, Av{}).volume/el.size();
        //. //Long volx = std::accumulate(beg0,end0, Long{}, [vola](Long a,auto&x){return a+labs(x.volume-vola);})/15;
        //. //auto vlohi = Ma(3, rbegin,rend, null_iter, [](auto&l,auto&r){return l.volume<r.volume;});

        //. printf("%06d %7.2f", Numb(el.code), el.capital0*el.oc[1]/100.0/Yi);
        //. {
        //.     //printf(" %03d %6.2f"" %6.2f %5.2f %.3ld %3d", el.mll, el.incoming/double(Wn), lsz/double(Yi), last->amount/double(Yi), 1000*last->amount/lsz, el.eps?last->oc[1]/el.eps:-1);
        //.     //if (Numb(el.code)==570) DBG_MSG("%d %ld %d %ld %6.2f", Numb(el.code), el.capital1, el.oc[1], lsz, lsz/double(Yi));
        //. } {
        //.     //printf("\t%.2lu %.2d %.2d %.3ld %.3ld", el.size(), (nG+nR), (volRa+volGa), last->volume, lasp->volume); fflush(stdout);
        //.     printf("\t%.2d %.2d", 100*nR/(nG+nR), 100*volRa/(volRa+volGa));
        //.     printf("\t% .2ld % .3ld", volR1*10/last->volume, volRa*10/last->volume);
        //.     printf( " % .2ld % .3ld", volR1*10/lasp->volume, volRa*10/lasp->volume);
        //.     //printf("\t%d %d %.3ld %.3ld", int(end0-near0), int(end2-beg2), 100*volRa/near0->volume, 100*volRa/volR1);
        //.     //printf("\t%.3ld %.3ld %.3ld %.3ld"
        //.     //        , 100*lasp->volume/last->volume
        //.     //        , 100*vlohi[0].volume/last->volume, 100*vola/last->volume, 100*vlohi[1].volume/last->volume);
        //.     //printf(" %.3ld", 1000*volx/vola);
        //. } {
        //.     //auto Chr = [](int b, int lastv) { return 1000*(lastv-b)/b; };
        //.     //auto lh = std::minmax_element(beg0,end0, [](auto&l,auto&r){return l.oc[1]<r.oc[1];});
        //.     //if (lh.first > lh.second)
        //.     //    std::swap(lh.first,lh.second);
        //.     //printf("\t% .3d % .3d", Chr(lasp->oc[1],last->oc[1]), Chr(lh.first->oc[1],lh.second->oc[1]));
        //. }
        //. //printf("\t%d\t%s\n", (int)el.size(), el.detail.c_str());
        //. printf("\t%-2d %2d\n", int(end0-beg0), (int)el.size());

    }
    return 0;
}
//     for (auto tup : boost::combine(a, b, c, a)) {}    // <---
//     i   = boost::make_zip_iterator( keys+0, values+0  ),
//     end = boost::make_zip_iterator( keys+9, values+9  ),

