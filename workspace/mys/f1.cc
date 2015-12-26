#include <string>
#include <set>
#include <vector>
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
struct VStock
{
    int code;
    float gbActive, gbTotal;

    friend std::ostream& operator<<(std::ostream& out, VStock const& a)
    {
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

struct Days : std::vector<VDay>
{
    Days(int c, std::pair<gregorian::date,gregorian::date> dr)
    {
        using namespace boost;
        std::ifstream ifs(str(format("D:\\home\\wood\\stock\\tdx\\%06d") % c));
        std::string line;
        while(getline(ifs, line)) {
            VDay a;
            std::istringstream iss(line);
            iss >> a;

            if (a.date < dr.first)
                continue;
            if (a.date > dr.second)
                break;

            this->push_back(a);
        }
    }
};

struct Stocks : boost::multi_index::multi_index_container
                <VStock,indexed_by<ordered_unique<member<VStock,int,&VStock::code>>>>
{
    Stocks() { init(std::set<int>()); }
    Stocks(std::set<int> const& s) { init(s); }
    void init(std::set<int> const& s)
    {
        std::ifstream ifs("D:\\home\\wood\\stock\\tdx\\lis");
        std::string line;
        while(getline(ifs, line)) {
            VStock a;
            std::istringstream iss(line);
            iss >> a;
            if (!s.empty() && s.find(a.code) == s.end())
                continue;
            this->insert(a);
        }
    }
};

template <typename Iter>
float ma(Iter it, Iter end)
{
    std::pair<float,float> a = {0.0f,0.0f};
    a = std::accumulate(it,end, a, [](decltype(a) const& x, VDay const& e){
                return x; //std::make_pair(x.first + e.a.Amount, x.second + e.fVolume);
            });
    return a.first/a.second;
}

std::pair<gregorian::date,gregorian::date> get_date_range(int argc, char* const argv[])
{
    auto dr = std::make_pair(gregorian::date(2000,1,1), gregorian::day_clock::local_day());
    if (argc >= 3) {
        dr.first = gregorian::from_simple_string(argv[1]);
        dr.second = gregorian::from_simple_string(argv[2]);
    } else if (argc >= 2) {
        dr.first = gregorian::from_simple_string(argv[1]);
    }
    return dr;
}

std::set<int> get_codes(int argc, char* const argv[])
{
    return std::set<int>();
}

float calc(VStock const& s, Days const& ds);

int main(int argc, char* const argv[])
{
    struct SRank {
        int code;
        float rank;
    };

    try {
        boost::multi_index::multi_index_container<SRank,indexed_by<ordered_unique<member<SRank,float,&SRank::rank>>>>
            result;

        Stocks ss(get_codes(argc, argv));
        std::pair<gregorian::date,gregorian::date> dp = get_date_range(argc, argv);
        clog << dp.first <<" "<< dp.second <<"\n";// << ss.front() <<" "<< ss.back() <<"\n";

        for (auto && s : ss) {
            Days ds(s.code, dp);
            if (ds.empty())
                continue;
            // clog << ds.front() <<" "<< ds.back() <<"\n";
            result.insert(SRank{s.code, calc(s, ds)});
        }

        int n=40;
        for (auto i=result.begin(); i != result.end(); ++i) {
            std::cout << i->code <<' '<< i->rank << '\n';
            if (--n == 0)
                break;
        }

    } catch (std::exception const& e) {
        clog << e.what();
    }
}

float calc(VStock const& s, Days const& ds)
{
	auto hi = std::max_element(ds.begin(), ds.end()
		, [](VDay const& lhs, VDay const& rhs){
			return (lhs.amount/lhs.volume) < (rhs.amount/rhs.volume);
		});
	auto lo = std::min_element(hi, ds.end()
		, [](VDay const& lhs, VDay const& rhs){
			return (lhs.amount/lhs.volume) < (rhs.amount/rhs.volume);
		});

    if (hi >= lo)
        return 13;
    //float lx = lo->amount/lo->volume;
    //if ((ds.back().close - lx)/lx > 0.10) return 10;

    struct Sta {
        float px;
        struct AV {
            float amount;
            float volume;
        } up, dn;
    } a = {};

    a.px = ds.front().amount/ds.front().volume;
    a = std::accumulate(ds.begin(),ds.end(), a, [](Sta x, VDay const& e){
                float px = e.amount/e.volume;
                if (px < x.px) {
                    x.dn.amount += e.amount;
                    x.dn.volume += e.volume;
                } else {
                    x.up.amount += e.amount;
                    x.up.volume += e.volume;
                }
                x.px = px;
                return x;
            });

    return a.dn.amount/a.up.amount;

	//int n;

	////REPORTDAT2 smx = {};
	////n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, t0, t1, nTQ, 0);
	////if (n < 0 || smx.Volume*100 < 1) {
	////	LOG << Code << "Ig" << "Vol" << smx.Volume <<"Price"<< smx.Now << smx.Close;
	////	return FALSE;
	////}
	//std::vector<HISDAT> his(30);
	//n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	//if (n < (int)his.size()) {
	//	LOG << Code << "Few"<< n;
	//	return FALSE;
	//}
    //n = (boost::gregorian::day_clock::local_day() - his.back().Time.date()).days();
	//if (n > 3 || his.back().fVolume < 1) {
    //    LOG << Code << boost::gregorian::day_clock::local_day() << his.back().Time << n << "StopEx";
    //    return 0;
    //}

	//STOCKINFO sinf = {};
	//if ( (n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0)) < 0)
	//	return 0;
	//if ((his.back().Close * sinf.ActiveCapital)/100000000 > 400)
	//	return 0;
	//if (his.back().fVolume / sinf.ActiveCapital < 3.5*tvolume(his.back().Time))
	//	return 0;

	//auto beg = his.rbegin() + args[3];
    //float a = ma(beg, beg+3, Code);
    //for (int i=1; i < (args[2] ? args[2] : 6); ++i) {
    //    float x = ma(beg+i, beg+i+3, Code);
	//	if (args[1]) {
	//		if (x > a && (x - a) > 0.05)
	//			return 0;
	//	} else {
	//		if (x < a && (a - x) > 0.05)
	//			return 0;
	//	}
    //    a = x;
	//}
    //return 1;
}

