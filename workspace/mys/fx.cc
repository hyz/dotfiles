#include <string.h> // For strlen, strcmp, memcpy
#include <string>
#include <set>
#include <vector>
#include <array>
#include <sstream>
#include <fstream>
#include <iterator>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/date_formatting.hpp>
#include <boost/format.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/static_assert.hpp>
using namespace std;
using boost::format;
using std::array;
//namespace multi_index = boost::multi_index;
namespace gregorian = boost::gregorian;

using namespace boost::multi_index;

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

struct Av
{
    float amount = 0;
    float volume = 0;

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


//600570	617805120.00	617805120.00	0.00	0.00	0.35	0.00	24	1
struct VDay : Av
{
    gregorian::date date; //float amount; float volume;
    float open, high, low, close;

    friend std::ostream& operator<<(std::ostream& out, VDay const& a) {
        return out << a.date << std::fixed << std::setprecision(2)
            <<'\t'<< a.amount 
            <<'\t'<< a.volume
            <<'\t'<< a.open
            <<'\t'<< a.high
            <<'\t'<< a.low
            <<'\t'<< a.close
            ;
    }

    friend std::istream& operator>>(std::istream& in, VDay & a) {
        std::string tmp;
        in >> tmp;
        a.date = boost::gregorian::from_simple_string(tmp); //auto x = a.date.year_month_day();
        return in >> a.amount >> a.volume
            >> a.open >> a.high >> a.low >> a.close
            ;
    }
};

struct VStock : std::vector<VDay>
{
    int code;
    float gbActive, gbTotal, eov;
    float volume = 0;
    float amount = 0;
    //VDay maxvols[6];

    void init_(std::array<gregorian::date,2> dr)
    {
        using namespace boost;
        std::ifstream ifs(str(format("%06d") % this->code));
        std::string line;
        //std::array<VDay,2> dp = {};
        while(getline(ifs, line)) {
            VDay a = {};
            std::istringstream iss(line);
            iss >> a;

            if (a.date < dr[0]) {
            } else if (a.date > dr[1]) {
                break;
            } else {
                amount += a.amount;
                volume += a.volume;
                this->push_back(a);
            }
        }
    }


    friend std::ostream& operator<<(std::ostream& out, VStock const& a)
    {
        return out << a.code << std::fixed << std::setprecision(2)
            <<'\t'<< a.gbActive
            <<'\t'<< a.gbTotal
            ;
    }
    friend std::istream& operator>>(std::istream& in, VStock & a)
    {
        float tmp;
        int i;
        return in >> a.code >> a.gbActive >> a.gbTotal
            >> tmp >> tmp
            >> a.eov >> tmp >> i >> i
            ;
    }
};

struct Stocks : boost::multi_index::multi_index_container
                <VStock,indexed_by<ordered_unique<member<VStock,int,&VStock::code>>>>
{
    static Stocks init(int argc, char* const argv[])
    {
        Stocks ret;
        std::set<int> s = get_codes(argc, argv);
        std::array<gregorian::date,2> dp = get_date_range(argc, argv);
        clog << dp[0] <<" "<< dp[1] <<"\n";

        std::ifstream ifs("lis");
        std::string line;
        while(getline(ifs, line)) {
            VStock a;
            std::istringstream iss(line);
            iss >> a;
            if (!s.empty() && s.find(a.code) == s.end())
                continue;
            a.init_(dp);
            ret.insert( std::move(a) );
        }
        return std::move(ret);
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
        std::string s;
        std::ifstream fp(fn, std::ios::binary);
        while (getline(fp, s)) {
            ints.insert(atoi(s.c_str()));
        }
        return std::move(ints);
    }
    static std::set<int> get_codes(int argc, char* const argv[])
    {
        if (argc >= 2) {
            const char* fn = argv[1];
            if (strcmp(fn, "-") == 0)
                fn = "input.lis";
            std::ifstream ifs(fn);
            if (ifs) {
                std::set<int> ret;
                std::set<int> s0 = read_ints(fn);
                std::set<int> s1 = read_ints("../cyb.lis");
                std::set_difference(s0.begin(), s0.end(), s1.begin(), s1.end()
                        , std::inserter(ret,ret.end()));
                return std::move(ret);
            }
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
        clog << e.what();
    }
}

void Main(int argc, char* const argv[])
{
//boost::multi_index::multi_index_container<SVal,indexed_by<ordered_non_unique<member<SVal,float,&SVal::val>, std::greater<float>>>> result;
    //struct Av { float amount; float volume; };
    typedef VStock::const_iterator c_iterator;

    auto valr = [](c_iterator it, c_iterator end) {
        --end;
        it = std::min_element(it, end, [](auto& l, auto& r){ return Mv(&l)<Mv(&r); });
        auto x = Mv(it);
        return (Mv(end) - x)/x;
    };

    auto isred = [](c_iterator it, c_iterator begin){
        return it->close > it->open
                && (it==begin || it->close > (it-1)->close);
    };

    auto greater = [](c_iterator j, c_iterator i){ return !(j->volume < i->volume); };

    auto print_date = [](auto it) {
        printf("\t%02d%02d", (int)it->date.month(), (int)it->date.day());
    };

    auto stocks = Stocks::init(argc, argv);

    for (auto && sk : stocks) {
        if (sk.empty() || sk.back().volume<1 || sk.size() < 10)
            continue;

        float vol = Ma<5>(sk.end()-5, sk.end(), Av{}, [&vol](Av){}).volume/5;
        if (vol < 1)
            continue;

        auto i0 = sk.end() - 4;
        auto i1 = i0+1;
        auto i2 = i1+1;
        auto i3 = i2+1;

        printf("%06d", sk.code);
        printf("\t%+.2f\t%+.2f\t%+.2f", Vx(i0->close,i1->close), Vx(i1->close,i2->close), Vx(i2->close,i3->close));
        printf("\t%+.2f\t%+.2f\t%+.2f", Vx(vol,i1->volume), Vx(vol,i2->volume), Vx(vol,i3->volume));
        printf("\t%.2f\t%.2f""\t%d\n", sk.back().close/sk.eov, sk.back().close*sk.gbActive/100000000, (int)sk.size());
    }
    //for (auto & v : result) { printf("%06d\t%.2f\t%.2f\t%.2f\n", v.code, v.val, v[0], v[1]); }
}

