#include <string.h> // For strlen, strcmp, memcpy
#include <string>
#include <set>
#include <vector>
#include <array>
#include <sstream>
#include <fstream>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/static_assert.hpp>
#include <boost/convert.hpp>
#include <boost/convert/strtol.hpp>

struct boost::cnv::by_default : public boost::cnv::strtol {};

using namespace std;
using boost::format;
//namespace multi_index = boost::multi_index;
namespace gregorian = boost::gregorian;

using namespace boost::multi_index;

template <int size_=12>
struct xstr
{
    typedef xstr              this_type;
    typedef char                  value_type;
    typedef value_type*             iterator;
    typedef value_type const* const_iterator;

    xstr () { storage_[0] = 0; }

    xstr (const_iterator beg, const_iterator end =0) {
        std::size_t const sz = end ? (end - beg) : strlen(beg);
        BOOST_ASSERT(sz < size_);
        memcpy(storage_, beg, sz); storage_[sz] = 0;
    }

    char const*    c_str () const { return storage_; }
    const_iterator begin () const { return storage_; }
    const_iterator   end () const { return storage_ + strlen(storage_); }
    this_type& operator= (char const* str) {
        BOOST_ASSERT(strlen(str) < size_);
        strcpy(storage_, str);
        return *this;
    }

    char storage_[size_]; //static size_t const size_ = 12;
};

template <int N> inline bool operator==(char const* s1, xstr<N> const& s2) { return strcmp(s1, s2.c_str()) == 0; }
template <int N> inline bool operator==(xstr<N> const& s1, char const* s2) { return strcmp(s2, s1.c_str()) == 0; }

//600570	617805120.00	617805120.00	0.00	0.00	0.35	0.00	24	1
struct VDay
{
    gregorian::date date;
    float amount;
    float volume;
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
    float gbActive, gbTotal;

    void init_(std::array<gregorian::date,2> dr) {
        using namespace boost;
        std::ifstream ifs(str(format("%06d"/*"D:\\home\\wood\\stock\\tdx\\%06d"*/) % this->code));
        std::string line;
        while(getline(ifs, line)) {
            VDay a;
            std::istringstream iss(line);
            iss >> a;

            if (a.date < dr[0])
                continue;
            if (a.date > dr[1])
                break;

            this->push_back(a);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, VStock const& a) {
        return out << a.code << std::fixed << std::setprecision(2)
            <<'\t'<< a.gbActive
            <<'\t'<< a.gbTotal
            ;
    }
    friend std::istream& operator>>(std::istream& in, VStock & a) {
        float tmp;
        int i;
        return in >> a.code >> a.gbActive >> a.gbTotal
            >> tmp >> tmp
            >> tmp >> tmp >> i >> i
            ;
    }
};

struct Stocks : boost::multi_index::multi_index_container
                <VStock,indexed_by<ordered_unique<member<VStock,int,&VStock::code>>>>
{
    static Stocks init(int argc, char* const argv[]) {
        Stocks ret;
        std::set<int> s = get_codes(argc, argv);
        std::array<gregorian::date,2> dp = get_date_range(argc, argv);
        clog << dp[0] <<" "<< dp[1] <<"\n";// << ret.front() <<" "<< ret.back() <<"\n";

        std::ifstream ifs("lis"/*"D:\\home\\wood\\stock\\tdx\\lis"*/);
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

    static std::array<gregorian::date,2> get_date_range(int argc, char* const argv[]) {
        std::array<gregorian::date,2> dr{gregorian::date(2000,1,1), gregorian::day_clock::local_day()};
        if (argc >= 4) {
            dr[0] = gregorian::from_simple_string(argv[2]);
            dr[1] = gregorian::from_simple_string(argv[3]);
        } else if (argc >= 3) {
            dr[0] = gregorian::from_simple_string(argv[2]);
        }
        return dr;
    }
    static std::set<int> get_codes(int argc, char* const argv[]) {
        if (argc >= 2) {
            const char* fn = argv[1];
            if (strcmp(fn, "-") == 0)
                fn = "lis";//"D:\\home\\wood\\stock\\tdx\\lis";
            std::ifstream ifs(fn);
            if (ifs) {
                std::set<int> s;
                std::string line;
                while(getline(ifs, line)) {
                    s.insert(atoi(line.c_str()));
                }
                return std::move(s);
            }
        }
        return std::set<int>();
    }
};

template <typename Iter>
float ma_price(Iter it, Iter end)
{
    std::pair<float,float> a = {0.0f,0.0f};
    a = std::accumulate(it,end, a, [](decltype(a) const& x, VDay const& e){
                return std::make_pair(x.first + e.amount, x.second + e.volume);
            });
    return a.first/a.second;
}

template <int N, typename I>
float Ma(I it)
{
    float a = 0, v = 0;
    for (int i=0; i<N; ++i) {
        a+=it[i].amount;
        v+=it[i].volume;
    }
    return a/v; //it->amount/it->volume;
}
template <typename I> inline float Ma1(I it) { return Ma<1>(it); }
template <typename I> inline float Ma3(I it) { return Ma<3>(it); }

float calc(VStock const& s);

struct SComp {
    template <typename It> bool operator()(It l, It r) const { return l->volume > r->volume; }
};

int main(int argc, char* const argv[])
{
    struct Av {
        float amount;
        float volume;
    };

    try {
        //boost::multi_index::multi_index_container<SVal,indexed_by<ordered_non_unique<member<SVal,float,&SVal::val>, std::greater<float>>>> result;

        auto stocks = Stocks::init(argc, argv); //(get_codes(argc, argv), get_date_range(argc, argv));
        // stock/tdx/999999

        for (auto && stk : stocks) {
            if (stk.empty() || stk.back().volume<1 || stk.size() < 5)
                continue;

            Av total = {};
            Av grsum[2] = {};
            Av grmx3[2] = {};

            auto it = stk.begin();
            decltype(it) grmx3_[2][4] = { {it,it,it,it}, {it,it,it,it} };
            std::vector<float> va3;

            for (float pa = it->amount/it->volume; it != stk.end(); ++it) {
                float a = it->amount/it->volume;

                total.amount += it->amount;
                total.volume += it->volume;

                auto & v = grmx3_[pa < a];
                std::pop_heap(&v[0], &v[4], SComp());
                v[3] = it;
                std::push_heap(&v[0], &v[4], SComp());

                auto & gr = grsum[pa < a];
                gr.amount += it->amount;
                gr.volume += it->volume;

                if (it < (stk.end()-2)) {
                    va3.push_back( Ma3(it) );
                }

                pa = a;
            }
            for (int x=0; x < 2; ++x) {
                auto& tmp = grmx3_[x];
                auto& gr = grmx3[x];
                for (int i=1; i<4; ++i) {
                    gr.amount = tmp[i]->amount;
                    gr.volume = tmp[i]->volume;
                }
            }
            auto hi = std::max_element(va3.begin(), va3.end(), [](float l, float r){ return l<r; });
            auto lo = std::min_element(hi, va3.end(), [](float l, float r){ return l<r; });
            if (hi >= lo)
                continue;
            auto hv = *hi, lv = *lo;
            auto zv = Ma1(stk.rbegin()); //last = stk.end()-1; // auto lasp = last-1; // if (Ma(last) < Ma(lasp)) continue;

            printf("%06d", stk.code);
            printf("\t%.2f\t%.2f\t%.2f", (zv - hv)/hv, (zv - lv)/lv, (lv - hv)/hv);
            printf("\t%.2f\t%.2f\t%.2f", grmx3[1].amount/grmx3[0].amount, grmx3[1].amount/total.amount, grmx3[0].amount/total.amount);
            printf("\t%.2f\t%.2f\t%.2f", grsum[1].amount/grsum[0].amount, grsum[1].amount/total.amount, grsum[0].amount/total.amount);
            printf("\t%d\n", (int)stk.size());
        }
        //for (auto & v : result) { printf("%06d\t%.2f\t%.2f\t%.2f\n", v.code, v.val, v[0], v[1]); }

    } catch (std::exception const& e) {
        clog << e.what();
    }
}

