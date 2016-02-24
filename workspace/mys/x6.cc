#include <stdio.h>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <string>
#include <boost/signals2/detail/null_output_iterator.hpp>
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
auto First  = [](auto&& x) -> auto& { return fusion::at_c<0>(x); };
auto Second = [](auto&& x) -> auto& { return fusion::at_c<1>(x); };

static const auto Sum = [](Avsb const& avsb) { return fusion::at_c<0>(avsb) + fusion::at_c<1>(avsb); };
static const auto Plus = [](Avsb const& x, Avsb const& y) {
    Av const a = fusion::at_c<0>(x) + fusion::at_c<0>(y);
    Av const b = fusion::at_c<1>(x) + fusion::at_c<1>(y);
    return fusion::make_vector(a, b);
};
//[](auto&& x) -> decltype(fusion::at_c<0>(x))& { return fusion::at_c<0>(x); };

static auto Lohi = [](auto& rng) {
    auto cheap = [](auto&x, auto&y) {
        Av a = Sum(x);
        Av b = Sum(y);
        return (a.amount/a.volume) < (b.amount/b.volume);
    };
    return std::minmax_element(std::begin(rng), std::end(rng), cheap);
};

inline int Price(Av const& v) {
    if (!v.volume)
        return 1;
    return int(v.amount*100/v.volume);
};
inline int Price(Avsb const& x) { return Price(Sum(x)); };

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

template <typename It, typename Iter> // boost::make_function_output_iterator
auto Ma(unsigned n, It it, It end, Iter iter) // -> array<decltype(*it),2>
{
    decltype(*it) a{};
    std::vector<It> ar(n);
    unsigned x=0;
    for (; x < ar.size() && it != end; ++x, ++it) {
        a += *it;
        ar[x] = it;
    }
    auto b = a/ar.size();
    array<decltype(*it),2> ret={{b,b}};

    if (x == ar.size()) {
        *iter++ = b;

        for (; it != end; ++it) {
            unsigned i = x++ % n;
            a -= *ar[i];
            a += *it;
            ar[i] = it;
            *iter++ = b = a/n;
            if (b < ret[0]) {
                ret[0] = b;
            } else if (b > ret[1]) {
                ret[1] = b;
            }
        }
    }
    return ret;
}

struct SInfo
{
    long gbx, gbtotal;
    int eov;
};
BOOST_FUSION_ADAPT_STRUCT(SInfo, (long,gbx)(long,gbtotal)(int,eov))

struct Summary : AV , array<int,2>
{
    array<int,2> lohi= {}; //, oc= {};
    //Avsb sums = {}, vless = {} , vmass = {} , voths = {};
};

struct Elem : SInfo, Summary, std::vector<Summary>
{
    int code = 0;

    //Avsb sum = {}; //Av av = {}; //long volume = 0; long amount = 0;
    //array<int,2> lohi= {}, oc= {};

    int n_day() const { return int(this->size()); }
    void extend(Elem const& o) {}
};

struct Main : boost::multi_index::multi_index_container< Elem, indexed_by <
              sequenced<>
            , hashed_unique<member<Elem,int,&Elem::code>> >>
{
    struct init_;

    Main(int argc, char* const argv[]);
    int run(int argc, char* const argv[]);

    gregorian::date date;
};

struct Main::init_ : std::unordered_map<int,SInfo> , boost::noncopyable
{
    Main* m_ = 0;
    Elem tmp_ = {};
    init_(Main* p, int argc, char* const argv[]);

    void loadsi(char const* fn);
    Elem* add(int code, Elem&& d);
    void prep(filesystem::path const& path);
    template <typename F> void fun (F read, int, int);
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

Elem* Main::init_::add(int code, Elem&& el)
{
    auto & idc = m_->get<1>();
    auto it = idc.find(code);
    if (it == idc.end()) {
        auto si = this->find(code);
        if (si == this->end())
            return 0;
        static_cast<SInfo&>(el) = si->second;
        el.code = code;
        auto p = m_->push_back( std::move(el) );
        if (!p.second)
            ERR_EXIT("%d", numb(code));
        return &const_cast<Elem&>(*p.first);
    }
    Elem& o = const_cast<Elem&>(*it);
    o.extend(el);
    return &o;
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
            prep( p);
            //m_->date = std::min(m_->date, _date(p.generic_string()));
        }
    } else for (int i=1; i<argc; ++i) {
        if (!filesystem::is_regular_file(argv[i]))
            continue; // ERR_EXIT("%s: is_directory|is_regular_file", argv[i]);
        prep( argv[i]);
        //m_->date = std::min(m_->date, _date(argv[i]));
    }
}

void Main::init_::prep(filesystem::path const& path)
{
    if (FILE* fp = fopen(path.generic_string().c_str(), "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
        auto reader = [fp](std::vector<Summary>& vec, Summary& sa) {
            // vec.reserve(60*4+2); //using qi::long_; //using qi::_val; using qi::_1;
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
                Summary sm = {};
                Av & av = sm;

                using qi::int_;
                while (qi::phrase_parse(pos,end, R_Av>>int_>>int_>>int_>>int_, qi::space
                            , av, sm[0], sm[1], sm.lohi[0], sm.lohi[1])) {
                    if (av.volume) {
                        if (vec.empty()) {
                            sa = sm;
                        } else { 
                            if (sa.lohi[0] > sm.lohi[0])
                                sa.lohi[0] = sm.lohi[0];
                            if (sa.lohi[1] < sm.lohi[1])
                                sa.lohi[1] = sm.lohi[1];
                            sa[1] = sm[1];
                            static_cast<Av&>(sa) += av;
                        }
                        vec.push_back(sm);
                    }
                }
            } else
                ERR_EXIT("qi::parse: %s", pos);
            return make_code(szsh,code);
        };
        fun(reader, 0,0);
    }
}

struct XClear { template<typename T> void operator()(T*v)const{v->clear();} };

template <typename F> void Main::init_::fun(F read, int, int)
{
    Elem el = {}; //Summary sa = {};
    while (code_t code = read(el, el)) {
        std::unique_ptr<decltype(vec),XClear> xclear(&vec);
        if (vec.size() < 3) {
            ERR_MSG("%d :size<3\n", numb(code));
            continue;
        }
        //std::vector<Summary>& rng = el; // array<int,2> lohi;

        //auto fvcmp = [](auto& x, auto& y){
        //    return Sum(x).volume < Sum(y).volume;
        //};

        //int n = int(3/10.0 * (float)boost::size(rng));
        //int m = int(3/10.0 * float(boost::size(rng)-n));

        //std::nth_element(std::begin(rng), std::end(rng)-n, std::end(rng), fvcmp);
        //o.vless = std::accumulate(std::begin(rng), std::end(rng)-n, Avsb{}, Plus);
        //vss->vless.push_back(vl);
        //vss->vless = Plus(vss->vless, o.vless);

        //std::nth_element(std::end(rng)-n, std::end(rng)-m, std::end(rng), fvcmp);
        //o.vmass = std::accumulate(std::end(rng)-m, std::end(rng), Avsb{}, Plus);
        //vss->vmass.push_back(vm);

        //o.voths = std::accumulate(std::end(rng)-n, std::end(rng)-m, Avsb{}, Plus);
        //vss->voths.push_back(vk);

        if (!this->add(code, std::move(el))) {
            ERR_MSG("%d :add-fail\n", numb(code));
        }
    }
}

Main::Main(int argc, char* const argv[])
    : date(gregorian::day_clock::local_day())
{
    init_ ini(this, argc, argv);
    (void)ini;
}

int Main::run(int argc, char* const argv[])
{
    constexpr int Wn=10000;
    constexpr int Yi=Wn * Wn;

    for (Elem const & el : *this) {
        Av last = el.back();
        long volx = 0;
        long vola = (av.volume+last.volume)/el.size();

        Av av = std::accumulate(el.rbegin()+1, el.rend(), Av{});
        for (auto& x : el) {
            volx += ::labs(x.volume - vola);
        }
        volx /= el.size();

        auto vlh = Ma(2, el.rbegin()+1, el.rend(), boost::signals2::detail::null_output_iterator());
                //, boost::make_function_output_iterator( [&last](Av const& a) { if (a.volume < last->volume) last->volume; } )

        printf("%06d", numb(el.code));
        {
            long lsz = el.gbx*Price(total)/100;
            printf("\t%6.2f %5.2f %03ld", lsz/double(Yi), last.amount/double(Yi), 1000*last.amount/lsz);
        } {
            printf("\t%.3ld %.3ld", 100*last.volume/vlh[0], 100*vlh[1]/last.volume);
            printf("\t%.3ld", 1000*volx/vola);
        } {
            auto Chr = [](int b, int lastv) { return 1000*(lastv-b)/b; };
            printf("\t% .4d %.3d", Chr(el.lohi[0],el.lohi[1]), Chr(el[0],el[1]));
        }
        printf("\t%d\n", el.size());
    }
    return 0;
}

