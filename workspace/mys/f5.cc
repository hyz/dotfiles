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
using namespace std;
using boost::format;
//namespace multi_index = boost::multi_index;
namespace gregorian = boost::gregorian;

using namespace boost::multi_index;

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

    void init_(std::array<gregorian::date,2> dr)
    {
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
    static std::set<int> get_codes(int argc, char* const argv[])
    {
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
float ma(Iter it, Iter end)
{
    std::pair<float,float> a = {0.0f,0.0f};
    a = std::accumulate(it,end, a, [](decltype(a) const& x, VDay const& e){
                return std::make_pair(x.first + e.amount, x.second + e.volume);
            });
    return a.first/a.second;
}

float calc(VStock const& s);

int main(int argc, char* const argv[])
{
    struct Av {
        int count;
        float amount;
        float volume;
    };
    struct SVal {
        int code;
        float val;
        std::array<int,2> gr;
        std::array<int,2> x10;
    };

    try {
        boost::multi_index::multi_index_container<SVal
                ,indexed_by<ordered_non_unique<member<SVal,float,&SVal::val>, std::greater<float>>>
            > result;

        auto ss = Stocks::init(argc, argv); //(get_codes(argc, argv), get_date_range(argc, argv));
        // stock/tdx/999999

        for (auto const & s : ss) {
            if (s.empty())
                continue;
            // clog << ds.front() <<" "<< ds.back() <<"\n";
            SVal sv = {};
            Av av = {};

            auto it = s.begin();
            av.amount = it->amount;
            av.volume = it->volume;
            for (auto p=it++; it != s.end(); ++it, ++p) {
                if ((0.10 - abs(it->close - p->close)/p->close)*p->close < 0.01)
                    sv.x10[p->close < it->close]++;
                sv.gr[p->close < it->close]++;
                av.amount += it->amount;
                av.volume += it->volume;
            }

            it = s.begin();
            auto last = s.rbegin();

            //float a = it->amount / it->volume; // = av.amount / av.volume;
            sv.val = (last->close - it->close)/it->close; //= (last->amount/last->volume - a)/a;
            sv.code = s.code;
            //clog << sv.code << std::fixed << std::setprecision(2)
            //    <<'\t'<< sv.val <<'\t'<< last->close <<'\t'<< a <<'\t'<< it->close <<'\n';
            result.insert(sv);
        }

        for (auto & v : result) {
            printf("%06d\t%.2f\t%d\t%d\t%d\t%d\n", v.code, v.val, v.gr[1], v.gr[0], v.x10[1], v.x10[0]);
        }

    } catch (std::exception const& e) {
        clog << e.what();
    }
}

float calc(VStock const& s)
{
	//auto hp = std::max_element(s.begin(), s.end()
	//	, [](VDay const& lhs, VDay const& rhs){
	//		return (lhs.amount/lhs.volume) < (rhs.amount/rhs.volume);
	//	});
	//auto lp = std::min_element(hp, s.end()
	//	, [](VDay const& lhs, VDay const& rhs){
	//		return (lhs.amount/lhs.volume) < (rhs.amount/rhs.volume);
	//	});

    //if (hp >= lp) return 13;
    //float lx = lp->amount/lp->volume;
    //if ((s.back().close - lx)/lx > 0.10) return 10;
    return 0;
}

