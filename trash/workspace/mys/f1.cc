#include <string.h> // For strlen, strcmp, memcpy
#include <string>
#include <set>
#include <vector>
#include <array>
#include <sstream>
#include <fstream>
#include <iterator>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/date_time/date_formatting.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/hashed_index.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/static_assert.hpp>

namespace gregorian = boost::gregorian;
namespace fusion = boost::fusion;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

using namespace boost::multi_index;

#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
//#include <boost/convert.hpp> #include <boost/convert/strtol.hpp>
//struct boost::cnv::by_default : public boost::cnv::strtol {};
//template <unsigned size_=16> struct xstr
//{
//    typedef xstr              this_type;
//    typedef char                  value_type;
//    typedef value_type*             iterator;
//    typedef value_type const* const_iterator;
//
//    xstr (const_iterator beg, const_iterator end =0) {
//        std::size_t const sz = end ? (end - beg) : strlen(beg);
//        BOOST_ASSERT(sz < size_);
//        memcpy(buf, beg, sz); buf[sz] = 0;
//    }
//    xstr () { buf[0] = 0; }
//
//    char const*    c_str () const { return buf; }
//    const_iterator begin () const { return buf; }
//    const_iterator   end () const { return buf + strlen(buf); }
//    this_type& operator= (char const* str) {
//        BOOST_ASSERT(strlen(str) < size_);
//        strcpy(buf, str);
//        return *this;
//    }
//
//    char buf[size_];
//};
//template <unsigned N> inline bool operator==(char const* s1, xstr<N> const& s2) { return strcmp(s1, s2.buf) == 0; }
//template <unsigned N> inline bool operator==(xstr<N> const& s1, char const* s2) { return strcmp(s2, s1.buf) == 0; }

namespace boost { namespace spirit { namespace traits
{
    template<>
    struct transform_attribute<gregorian::date, fusion::vector<int,int,int>, qi::domain>
    {
        typedef fusion::vector<int,int,int> type; //typedef type ymd_type;

        static void post(gregorian::date& d, type const& v) {
            d = gregorian::date(fusion::at_c<0>(v), fusion::at_c<1>(v), fusion::at_c<2>(v));
        }
        static type pre(gregorian::date&) { return type(); }
        static void fail(gregorian::date&) {}
    };
    //template <typename T, size_t N> struct is_container<array<T, N>, void> : mpl::false_ { };
}}}
//typedef gregorian::date::ymd_type ymd_type;
//BOOST_FUSION_ADAPT_STRUCT(ymd_type, (ymd_type::year_type,year)(ymd_type::month_type,month)(ymd_type::day_type,day))

struct Av
{
    float volume = 0;
    float amount = 0;

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
BOOST_FUSION_ADAPT_STRUCT(Av, (float,volume)(float,amount))

//600570	617805120.00	617805120.00	0.00	0.00	0.35	0.00	24	1
struct VDay : Av
{
    //gregorian::date date; // float volume, amount;
    float open, high, low, close;

    //    a.date = boost::gregorian::from_simple_string(tmp); //auto x = a.date.year_month_day();
    //    return in >> a.amount >> a.volume
    //        >> a.open >> a.high >> a.low >> a.close
};
BOOST_FUSION_ADAPT_STRUCT(VDay, (float,amount)(float,volume) (float,open)(float,high)(float,low)(float,close))

struct Elem
{
    std::vector<VDay> days;
    int code;
    float gbActive, gbTotal, eov;

    float volume = 0;
    float amount = 0;
    //    return in >> a.code >> a.gbActive >> a.gbTotal
    //        >> tmp >> tmp >> a.eov >> tmp >> i >> i

    void init_days(std::array<gregorian::date,2> dr);
};
BOOST_FUSION_ADAPT_STRUCT(Elem, (int,code)(float,gbActive)(float,gbTotal)(float,eov))

void Elem::init_days(std::array<gregorian::date,2> dr)
{ // using namespace boost;
    char fn[12];
    snprintf(fn,sizeof(fn), "%06d", this->code);

    if (FILE* fp = fopen(fn, "r")) {
        std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
        static const qi::int_parser<int,10,4,4> _4digit = {};
        static const qi::int_parser<int,10,2,2> _2digit = {};
        qi::rule<char*, fusion::vector<int,int,int>()> Rdate = _4digit >> _2digit >> _2digit;

        char linebuf[1024*4];
        while (fgets(linebuf, sizeof(linebuf), fp)) {
            char* pos = linebuf;

            gregorian::date date;
            if (!qi::parse(pos, &linebuf[sizeof(linebuf)], Rdate, date))
                ERR_EXIT("qi::parse: %s", pos);
            if (date > dr[1])
                break;
            if (date < dr[0])
                continue;

            VDay vd = {};
            using qi::float_;
            if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                        , float_>>float_>>float_>>float_>>float_>>float_, ascii::space, vd))
                ERR_EXIT("qi::parse: %s", pos);
            this->amount += vd.amount;
            this->volume += vd.volume;
            days.push_back(vd);
        }
    }
}

struct SLis : boost::multi_index::multi_index_container< Elem, indexed_by<
              sequenced<>
              , ordered_unique<member<Elem,int,&Elem::code>> >>
{
    SLis(int argc, char* const argv[])
    {
        std::set<int> ics = get_codes(argc, argv);
        std::array<gregorian::date,2> dp = get_date_range(argc, argv);
        //clog << dp[0] <<" "<< dp[1] <<"\n";

        if (FILE* fp = fopen("lis", "r")) {
            std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
            char linebuf[1024*4];
            while (fgets(linebuf, sizeof(linebuf), fp)) {
                char* pos = linebuf;
                int c = atoi(linebuf);
                if (ics.find(c) == ics.end())
                    continue;
                Elem el = {};
                using qi::float_;
                if (!qi::phrase_parse(pos, &linebuf[sizeof(linebuf)]
                            , qi::int_ >> float_ >> float_ >> qi::omit[float_ >> float_] >> float_
                            , ascii::space, el))
                    ERR_EXIT("qi::parse: %s", pos);
                el.init_days(dp);
                this->push_back( std::move(el) );
            }
        }
    }

    static std::array<gregorian::date,2> get_date_range(int argc, char* const argv[])
    {
        std::array<gregorian::date,2> dr{gregorian::date(2000,1,1), gregorian::day_clock::local_day()};
        if (argc >= 4) {
            dr[0] = gregorian::from_simple_string(argv[2]);
            dr[1] = gregorian::from_simple_string(argv[3]);
        } else if (argc >= 3) {
            dr[0] = gregorian::from_simple_string(argv[2]);
        }
        return dr;
    }

    static std::set<int> read_ints(char const* fn)
    {
        std::set<int> ints;
        if (FILE* fp = fopen(fn, "r")) {
            std::unique_ptr<FILE,decltype(&fclose)> auto_c(fp, fclose);
            char linebuf[1024*8];
            while (fgets(linebuf, sizeof(linebuf), fp)) {
                ints.insert( atoi(linebuf) );
            }
        }
        return std::move(ints);
    }
    static std::set<int> get_codes(int argc, char* const argv[])
    {
        if (argc >= 2) {
            const char* fn = argv[1];
            if (strcmp(fn, "-") == 0)
                fn = "input.lis";
            return read_ints(fn);
            //std::set<int> ret;
            //std::set<int> s0 = read_ints(fn);
            //std::set<int> s1 = read_ints("../cyb.lis");
            //std::set_difference(s0.begin(), s0.end(), s1.begin(), s1.end()
            //        , std::inserter(ret,ret.end()));
        }
        return std::set<int>();
    }
};

// boost::make_function_output_iterator(print));
template <unsigned N, typename I, typename A, typename Op>
auto Ma(I it, I end, A a, Op && op) -> A
{
    std::array<I,N> ar = {};
    unsigned x = 0;
    for (; x < N && it != end; ++x) {
        a += *it;
        ar[x] = it;
        ++it;
    }
    if (x == N)
        op(a);
    for (; it != end; ++it) {
        unsigned i = x++ % N;
        a -= *ar[i];
        a += *it;
        ar[i] = it;
        op(a);
    }
    return a;
}

//template <typename I> inline float Ma3(I it) { return Ma<3>(it); }
template <typename I> inline float Mv(I it) { return it->amount/it->volume; }
inline float Vx(float x, float y) { return (y-x)/x * 100; }

int main(int argc, char* const argv[])
{
    try {
        void Main(int argc, char* const argv[]);
        Main(argc, argv);
    } catch (std::exception const& e) {
        fprintf(stderr, "%s", e.what());
    }
    return 0;
}

void Main(int argc, char* const argv[])
{
//boost::multi_index::multi_index_container<SVal,indexed_by<ordered_non_unique<member<SVal,float,&SVal::val>, std::greater<float>>>> result;
    //struct Av { float amount; float volume; };
    //typedef Elem::const_iterator c_iterator;

    //auto valr = [](c_iterator it, c_iterator end) {
    //    --end;
    //    it = std::min_element(it, end, [](auto& l, auto& r){ return Mv(&l)<Mv(&r); });
    //    auto x = Mv(it);
    //    return (Mv(end) - x)/x;
    //};
    //auto isred = [](c_iterator it, c_iterator begin){
    //    return it->close > it->open
    //            && (it==begin || it->close > (it-1)->close);
    //};
    //auto greater = [](c_iterator j, c_iterator i){ return !(j->volume < i->volume); };
    //auto print_date = [](auto it) {
    //    printf("\t%02d%02d", (int)it->date.month(), (int)it->date.day());
    //};

    SLis slis(argc, argv);

    for (auto && sk : slis) {
        auto & days = sk.days;
        if (days.empty() || days.back().volume<1 || days.size() < 10)
            continue;
        auto last = days.cend()-1;

        float vol = Ma<5>(days.end()-5, days.end(), Av{}, [&vol](Av){}).volume/5;
        if (vol < 1)
            continue;

        auto i0 = days.end() - 4;
        auto i1 = i0+1;
        auto i2 = i1+1;
        auto i3 = i2+1;

        printf("%06d", sk.code);
        printf("\t%.2f\t%.2f\t%.2f", Vx(i0->close,i1->close), Vx(i1->close,i2->close), Vx(i2->close,i3->close));
        printf("\t%.2f\t%.2f\t%.2f", Vx(vol,i1->volume), Vx(vol,i2->volume), Vx(vol,i3->volume));
        printf("\t%.2f\t%.2f""\t%d\n", last->close/sk.eov, last->close*sk.gbActive/100000000, (int)days.size());
    }
    //for (auto & v : result) { printf("%06d\t%.2f\t%.2f\t%.2f\n", v.code, v.val, v[0], v[1]); }
}

