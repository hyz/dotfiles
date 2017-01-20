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
#include <boost/static_assert.hpp>
using namespace std;
using boost::format;
//namespace multi_index = boost::multi_index;
namespace gregorian = boost::gregorian;

using namespace boost::multi_index;

//#include <boost/convert.hpp> #include <boost/convert/strtol.hpp>
//struct boost::cnv::by_default : public boost::cnv::strtol {};
//template <int size_=12>
//struct xstr
//{
//    typedef xstr              this_type;
//    typedef char                  value_type;
//    typedef value_type*             iterator;
//    typedef value_type const* const_iterator;
//
//    xstr () { storage_[0] = 0; }
//
//    xstr (const_iterator beg, const_iterator end =0)
//    {
//        std::size_t const sz = end ? (end - beg) : strlen(beg);
//        BOOST_ASSERT(sz < size_);
//        memcpy(storage_, beg, sz); storage_[sz] = 0;
//    }
//
//    char const*    c_str () const { return storage_; }
//    const_iterator begin () const { return storage_; }
//    const_iterator   end () const { return storage_ + strlen(storage_); }
//    this_type& operator= (char const* str)
//    {
//        BOOST_ASSERT(strlen(str) < size_);
//        strcpy(storage_, str);
//        return *this;
//    }
//
//    char storage_[size_]; //static size_t const size_ = 12;
//};
//template <int N> inline bool operator==(char const* s1, xstr<N> const& s2) { return strcmp(s1, s2.c_str()) == 0; }
//template <int N> inline bool operator==(xstr<N> const& s1, char const* s2) { return strcmp(s2, s1.c_str()) == 0; }

//600570	617805120.00	617805120.00	0.00	0.00	0.35	0.00	24	1
struct VDay
{
    gregorian::date date;
    float amount;
    float volume;
    float open, high, low, close;

    friend std::ostream& operator<<(std::ostream& out, VDay const& a)
    {
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
    VDay maxvols[6];

    void init_(std::array<gregorian::date,2> dr)
    {
        using namespace boost;
        std::ifstream ifs(str(format("%06d") % this->code));
        std::string line;
        std::array<VDay,2> dp = {};
        while(getline(ifs, line)) {
            VDay a = {};
            std::istringstream iss(line);
            iss >> a;

            if (a.date < dr[0]) {
                if ((dr[0] - a.date).total_days() < 30) {
                    auto & v = grmx3_[pa < a];
                    std::pop_heap(&v[0], &v[4], SComp());
                    v[3] = it;
                    std::push_heap(&v[0], &v[4], SComp());
                }
            } else if (a.date > dr[1]) {
                break;
            } else {
                this->push_back(a);
            }
            dp[1] = a;
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
            >> tmp >> tmp >> i >> i
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
                fn = "lis";
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
    typedef VStock::const_iterator c_iterator;
    struct Av {
        float amount;
        float volume;
    };

    auto incr = [](c_iterator it, c_iterator end) {
        --end;
        it = std::min_element(it, end, [](auto& l, auto& r){ return Ma1(&l)<Ma1(&r); });
        auto x = Ma1(it);
        return (Ma1(end) - x)/x;
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

    for (auto && stk : stocks) {
        if (stk.empty() || stk.back().volume<1 || stk.size() < 10)
            continue;

        struct GRIs : std::array<std::vector<c_iterator>,4> {
            //std::vector<c_iterator> reds; std::vector<c_iterator> gres;
        } gris = {};
        struct Vols : std::array<float,4> {
            float volume; //float amount;
        } vols = {};

        for (auto it=stk.begin(), l5=stk.end()-5; it != stk.end(); ++it) {
            if (isred(it,stk.begin())) {
                gris[1].push_back(it); // reds.push_back(it);
                vols[1] += it->volume; // a.amount += it->amount;
                if (it >= l5) {
                    gris[3].push_back(it); // reds.push_back(it);
                    vols[3] += it->volume;
                }
            } else {
                gris[0].push_back(it);// gres.push_back(it);
                vols[0] += it->volume;
                if (it >= l5) {
                    gris[2].push_back(it);
                    vols[2] += it->volume;
                }
            }
            vols.volume += it->volume; //vols.amount += it->amount;
        }
        if (gris[1].empty() || gris[0].empty() || gris[3].empty() || gris[4].empty())
            continue;
        vols[0] /= gris[0].size();
        vols[2] /= gris[2].size();
        vols[1] /= gris[1].size();
        vols[3] /= gris[3].size();
        vols.volume /= stk.size(); //vols.amount += it->amount;

        float l5x[4] = {};
        if (!gris[3].empty()) {
            for (unsigned i=0; i < gris[3].size(); ++i) {
                l5x[3] += abs(gris[3][i]->volume - vols[3]);
            }
            l5x[3] /= gris[3].size();
        }
        if (!gris[2].empty()) {
            for (unsigned i=0; i < gris[2].size(); ++i) {
                l5x[2] += abs(gris[2][i]->volume - vols[3]);
            }
            l5x[2] /= gris[2].size();
        }

        float m2x[2] = {};
        std::nth_element(gris[1].begin(), gris[1].begin()+2, gris[1].end(), greater);
        std::nth_element(gris[0].begin(), gris[0].begin()+2, gris[0].end(), greater);
        m2x[0] = (gris[1][0]->volume + gris[1][1]->volume)/2;
        m2x[1] = (gris[0][0]->volume + gris[0][1]->volume)/2;

        //if ((m2x[1] - vols[0])/vols[0] > 0.5) continue;
        printf("%06d", stk.code);
        printf("\t%.2f", l5x[2]/vols[3], l5x[3]/vols[3]);
        printf("\t%.2f\t%.2f", (m2x[0]-vols[1])/vols[1], (m2x[1]-vols[1])/vols[1]);
        printf("\t%.2f", float(gris[1].size())/gris[0].size());
        printf("\t%.2f", incr(stk.begin(),stk.end()));
        print_date(gris[0][0]); print_date(gris[0][1]);
        print_date(gris[1][0]); print_date(gris[1][1]);
        printf("\t%d\n", (int)stk.size());
    }
    //for (auto & v : result) { printf("%06d\t%.2f\t%.2f\t%.2f\n", v.code, v.val, v[0], v[1]); }
}

