// MyPlugin.cpp : 通达信行情软件插件选股代码示例，根据通达信官方模板改编。
//template <typename X,typename Y>
//std::ostream& operator<<(std::ostream& out, std::pair<X, Y> const& rhs)
//{
//	return out << rhs.first << ' ' << rhs.second <<'\t';
//}

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <map>
#include <istream>
#include <fstream>
#include <boost/config/warning_disable.hpp>
//#include <boost/algorithm/string.hpp>
//#include <fstream>
//#include <iostream>
#include "log.h"
#include "tdxif.h"

static float tvolume(tag_NTime const& nt)
{
	using namespace boost::posix_time;

	auto lt = second_clock::local_time();

	if (lt.date() != nt.date())
		return 1;

	auto tpc = lt.time_of_day().total_seconds();
	auto tp0 = (hours(9) + minutes(30)).total_seconds();
	auto tp1 = hours(15).total_seconds();
	LOG << lt << tpc << tp0 << tp1;

    if (tpc < tp0 || tpc > tp1)
        return 1;

    int s = int(tpc - tp0);
    if (s > 60*30*4) {
        int x = (s - 60*30*4);
        s -= (x < 60*30*3 ? x : 60*30*3);
    }
    return s / float(60*30*8);
}

template <typename I> bool vola(const char* Code, I it, I end, int nlast=3)
{
    if (end - it < nlast+1)
        return 0;

    std::vector<boost::reference_wrapper<HISDAT>> vols_r, vols_g;
    float volume_s0=0, volume_s1=0;

    for (auto e1 = it+nlast; it != e1; ++it) {
        volume_s0 += it->fVolume;
    }
    volume_s0 /= nlast;
    nlast = end - it;
    for (; it != end; ++it) {
        volume_s1 += it->fVolume;
    }
    volume_s1 /= nlast;

    return ((volume_s1 - volume_s0) / volume_s0 > 0.1);
    // 61780.51 6005620736.00/85917488.00 11671623680.00/162795984.00 27870810112.00/374703648.00

    float volume_s, volume_sr, volume_sg;
    float amount_s, amount_sr, amount_sg;
    volume_s = volume_sr = volume_sg = 0;
    amount_s = amount_sr = amount_sg = 0;

    for (auto i = it; i != end; ++i) {
        volume_s += i->fVolume;
        amount_s += i->a.Amount;
        if (i->Close < i->a.Amount/i->fVolume) {
            volume_sg += i->fVolume;
            amount_sg += i->a.Amount;
            vols_g.push_back(boost::ref(*i));
        } else {
            volume_sr += i->fVolume;
            amount_sr += i->a.Amount;
            vols_r.push_back(boost::ref(*i));
        }
    }

    LOG << Code
        << std::distance(it, end) << vols_r.size() << vols_g.size()
		<< std::fixed << std::setprecision(2)
        << boost::format("%.2f/%.2f") % amount_s % volume_s
        << boost::format("%.2f/%.2f") % amount_sr % volume_sr
        << boost::format("%.2f/%.2f") % amount_sg % volume_sg
        ;
    if (vols_r.size() > vols_g.size()) {
    }

    return 1;
}

template <typename Iter>
static float ma(Iter it, Iter end, char const* Code="0")
{
    std::pair<float,float> a = {0.0f,0.0f};

    a = std::accumulate(it,end, a, [](decltype(a) const& x, HISDAT const& e){
                return std::make_pair(x.first + e.a.Amount, x.second + e.fVolume);
            });
	if (600570 == atoi(Code)) {
		LOG << Code << it->Time
			<< std::fixed << std::setprecision(2)
			<< a.first << a.second << a.first/a.second;
	}
    return a.first/a.second;
}

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

BOOL myflt9(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	STOCKINFO si = {};
	REPORTDAT2 rp = {};
	std::vector<HISDAT> his(args[1] ? args[1]*20 : 50);

    {
        int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, t0, t1, nTQ, 0);
        if (n < 0) {
            LOG << Code << "STKINFO_DAT error" << n;
            return 0;
        }
        if (si.ActiveCapital < 1) {
            return 0;
        }
    } {
        int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
        if (n < 1)/*(n < (int)his.size())*/ {
            LOG << Code << "PER_DAY error"<< n;
            return 0;
        }
        his.resize(n);
    } if(false){
        int n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &rp, 1, t0, t1, nTQ, 0);
        if (n < 0 || rp.Volume*100 < 1) {
            LOG << Code << "REPORT_DAT2 error" << n << "Vol" << rp.Volume <<"Price"<< rp.Now << rp.Close;
            return 0;
        }
    }
	using boost::format;
	boost::filesystem::path fp = "D:\\home\\wood\\workspace\\mys";
	fp /= str(format("%02d") % args[3]);

	if (!boost::filesystem::exists(fp)) {
		boost::filesystem::create_directories(fp);
	}

    {
		static boost::filesystem::ofstream ofs(fp/"lis", std::ios::trunc);
        ofs << Code
            //<<'\t'<< format("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f") % rp.Close % rp.Open % rp.Max % rp.Min % rp.Now % rp.Amount
            <<'\t'<< format("%.2f\t%.2f\t%.2f\t%.2f") % si.ActiveCapital % si.J_zgb % si.J_bg % si.J_hg
            <<'\t'<< format("%.2f\t%.2f") % si.J_mgsy % si.J_mgsy2
            //<<'\t'<< format("%.2f\t%.2f\t%.2f""\t""%.2f\t%.2f\t%.2f\t%.2f")
            //            % si.J_yysy % si.J_yycb % si.J_yyly
            //            % si.J_lyze % si.J_shly % si.J_jly % si.J_jyl
            <<'\t'<< int(si.J_hy) // <<'\t'<< int(si.J_zjhhy *100)
			<<'\t'<< nSetCode << '\n' << std::flush;
        //float       J_yysy;			//营业收入 # 营业收入(元)
        //float       J_yycb;			//营业成本
        //float       J_yyly;			//营业利润
        //
        //float       J_lyze;			//利益总额
        //float       J_shly;			//税后利益
        //float       J_jly;			//净利益   # 归属净利润(元)
        //float		J_jyl;				//净益率%  # 摊薄净资产收益率(%)
        //
        //float       J_bg;				//B股
        //float       J_hg;				//H股
        //
        //short       J_hy;				//所属行业
        //float       J_zjhhy;			//证监会行业
        //float		J_mgsy;				//每股收益(折算成全年的)
        //float       J_mgsy2;			//季报每股收益 (财报中提供的每股收益,有争议的才填)

    } {
        boost::filesystem::ofstream ofs(fp/Code, std::ios::trunc);
        for (auto it=his.begin(); it!=his.end(); ++it) {
            auto& a = *it;
            ofs //    << Code <<'\t'
                << a.Time
                <<'\t'<< format("%.2f\t%.2f") % a.a.Amount % a.fVolume
                <<'\t'<< format("%.2f\t%.2f\t%.2f\t%.2f") % a.Open % a.High % a.Low % a.Close
                <<'\n';
        }
    }

    return 0;
}

BOOL myflt0(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static std::set<int> s;
    if (s.empty()) {
		s.insert(0);
        std::ifstream ifs(str(boost::format("D:\\home\\wood\\%02d") % args[1]));
        std::string line;
        while (getline(ifs, line)) {
            s.insert(atoi(line.c_str()));
        }
    }

    return s.find(atoi(Code)) != s.end();
}

BOOL myflt1(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	int n;

	//REPORTDAT2 smx = {};
	//n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, t0, t1, nTQ, 0);
	//if (n < 0 || smx.Volume*100 < 1) {
	//	LOG << Code << "Ig" << "Vol" << smx.Volume <<"Price"<< smx.Now << smx.Close;
	//	return FALSE;
	//}
	std::vector<HISDAT> his(30);
	n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < (int)his.size()) {
		LOG << Code << "Few"<< n;
		return FALSE;
	}
    n = (boost::gregorian::day_clock::local_day() - his.back().Time.date()).days();
	if (n > 3 || his.back().fVolume < 1) {
        LOG << Code << boost::gregorian::day_clock::local_day() << his.back().Time << n << "StopEx";
        return 0;
    }

	STOCKINFO sinf = {};
	if ( (n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0)) < 0)
		return 0;
	if ((his.back().Close * sinf.ActiveCapital)/100000000 > 500)
		return 0;
	if (his.back().fVolume / sinf.ActiveCapital < 0.03*tvolume(his.back().Time))
		return 0;

	auto beg = his.rbegin() + args[3];
    float a = ma(beg, beg+3, Code);
    for (int i=1; i < (args[2] ? args[2] : 6); ++i) {
        float x = ma(beg+i, beg+i+3, Code);
		if (args[1]) {
			if (x > a && (x - a) > 0.05)
				return 0;
		} else {
			if (x < a && (a - x) > 0.05)
				return 0;
		}
        a = x;
	}
    return 1;
}

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

/* tags for accessing both sides of a bidirectional map */

using namespace boost::multi_index;

template <typename Cmp = std::less<float>>
struct CodeSet //: bimap_type //std::map<int,float>
{
    struct Value {
        int code;
        float val;
        boost::gregorian::date d;

		friend std::istream& operator>>(std::istream& in, Value& rhs) {
			return in >> rhs.code >> rhs.val >> rhs.d;
		}
		friend std::ostream& operator<<(std::ostream& out, Value const& rhs) {
			return out << rhs.code <<'\t'<< rhs.val <<'\t' << rhs.d;
		}
    };
    typedef boost::multi_index::multi_index_container<
      Value, //std::pair<int,float>,
      indexed_by<
        ordered_unique<member<Value,int,  &Value::code>>,
		ordered_non_unique<member<Value, float, &Value::val>,Cmp> //std::greater<float>
      >
    > type;

    type bimap;
    char const* tempf = "D:\\home\\wood\\stock\\.temp.1";

    CodeSet(int step, int nelem=0)
    {
		LOG << "INIT";
        if (step == 0) {
            ;
		} else {
            load(nelem);
			LOG << "load" << nelem << bimap.size();
            tempf = nullptr;
		}
    }
	~CodeSet() {
        if (tempf)
            save();
		LOG << "UNINIT";
    }

	void add(int x, float y, boost::gregorian::date const& d) { bimap.insert( Value{ x, y, d } ); }
    bool exist(int x) const { return bimap.find(x) != bimap.end(); }

private:
    void load(int nelem);
    void save() const;
};

//namespace std {
//}

template <typename Cmp> void CodeSet<Cmp>::load(int nelem)
{
    std::ifstream ifs(tempf);
    if (ifs) {
        //std::istream_iterator<std::pair<int, float> > it(ifs), end;
        Value val; //std::pair<int, float> p;
        for (int x = 0; x < nelem; ++x) {
            if (ifs >> val) {
                bimap.insert(val); //LOG << p <<"std:pair" <<x << nelem;
            } else
                break;
        }
    }
}
template <typename Cmp> void CodeSet<Cmp>::save() const
{
    std::ofstream ofs(tempf, std::ios::trunc|std::ios::out);
    if (ofs) {
        std::ostream_iterator<Value> it(ofs, "\n"); //, end;
        auto& m = get<1>(bimap);
        std::copy(m.begin(), m.end(), it);
    }
}

BOOL myflt2(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static CodeSet<> codes(args[1], args[2] ? args[2] : 50);
    int code = atoi(Code);
	int n;

    if (args[1] != 0) {
        return codes.exist(code);
    }

	//REPORTDAT2 smx = {};
	//n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, t0, t1, nTQ, 0);
	//if (n < 0 || smx.Volume*100 < 1) {
	//	LOG << Code << "Ig" << "Vol" << smx.Volume <<"Price"<< smx.Now << smx.Close;
	//	return FALSE;
	//}
	std::vector<HISDAT> his(args[2]<1 ? 20 : args[2]);
	n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < (int)his.size()) {
		LOG << Code << "Few"<< n;
		return FALSE;
	}
    n = (boost::gregorian::day_clock::local_day() - his.back().Time.date()).days();
	if (n > 3 || his.back().fVolume < 1) {
        LOG << Code << boost::gregorian::day_clock::local_day() << his.back().Time << n << "StopEx";
        return 0;
    }
	
	STOCKINFO sinf = {};
	if ( (n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0)) < 0)
		return 0;
	if ((his.back().Close * sinf.ActiveCapital)/100000000 > 500)
		return 0;
	if (his.back().fVolume / sinf.ActiveCapital < 0.035*tvolume(his.back().Time))
		return 0;

    auto last = his.rbegin();
	auto high = std::max_element(his.rbegin(), his.rbegin() + (args[2]<1 ? 20 : args[2])
		, [](HISDAT const& lhs, HISDAT const& rhs){
			return (lhs.a.Amount/lhs.fVolume) < (rhs.a.Amount/rhs.fVolume);
		});
    auto lowp = std::min_element(his.rbegin(), his.rbegin() + (args[2]<1 ? 20 : args[2])
        , [](HISDAT const& lhs, HISDAT const& rhs){
            return (lhs.a.Amount/lhs.fVolume) < (rhs.a.Amount/rhs.fVolume);
        });
    float a0 = last->a.Amount/last->fVolume;
    float al = lowp->a.Amount/lowp->fVolume;
    float ah = high->a.Amount/high->fVolume;
	float ax = al;
	auto px = lowp;
	if ((a0 - al) < (ah - a0)) {
        ax = ah;
		px = high;
	}
    codes.add(code, (a0 - ax) / ax, px->Time.date()); //codes.save();

    return 0;
}

BOOL myflt3(char const* Code, short nSetCode // 扫描复牌
	, int Value[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	std::vector<HISDAT> his(5); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < 1)
		return 0;
	if (n == 1)
		return 1;
	if (n < (int)his.size())
		return 1;
	auto rbeg = his.rbegin();
	auto rnext = rbeg + 1;
	//if (atoi(Code) < 40) { // help info
	//	LOG << Code
	//		<< rbeg->Time << rnext->Time
	//		<< "\n" << rbeg->Time.date() << rnext->Time.date();
	//}
	return (rbeg->Time.date() - rnext->Time.date()).days() > 3 || rbeg->fVolume*100 < 1;
	//return rbeg->Time.date().day_of_year() - rnext->Time.date().day_of_year() > 3;
}

BOOL myflt4(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	t0 = NTime::init(2015,8,1);
	t1 = NTime::init(2015,9,30);

	std::vector<HISDAT> his(150); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < 1)
		return 0;
	if (n == 1)
		return 1;
	his.resize(n);

	STOCKINFO sinf = {};
	if ( (n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0)) < 0)
		return 0;

	for (auto it = his.begin() + 1, p = his.begin(); it != his.end(); ++it, ++p) {
		if ((it->Open - p->Close) / p->Close < -0.095) {
			if (it->High > it->Open+0.01) {
				if (it->fVolume / sinf.ActiveCapital > 0.12) {
					LOG << Code << *p <<"\n"<< *it << sinf.ActiveCapital;
					return 1;
				}
			}
		}
    }
	return 0;
}

BOOL myflt5(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt6(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt7(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static std::ofstream ofs("D:\\home\\wood\\stock\\tdx\\codes", std::ios::trunc);
    ofs << Code <<'\t'<< nSetCode << '\n';
	return 0;
}

BOOL myflt8(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt9_x(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt10(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

