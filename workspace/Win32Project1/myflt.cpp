// MyPlugin.cpp : 通达信行情软件插件选股代码示例，根据通达信官方模板改编。

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <boost/config/warning_disable.hpp>
//#include <boost/algorithm/string.hpp>
//#include <fstream>
//#include <iostream>
#include "log.h"
#include "Plugin.h"

static float getsubs(tag_NTime const& nt)
{
	using namespace boost::posix_time;

	auto lt = second_clock::local_time();

	if (lt.date() != nt.ptime().date())
		return 1;

	auto tpc = lt.time_of_day().total_seconds();
	auto tp0 = (hours(9) + minutes(30)).total_seconds();
	auto tp1 = hours(15).total_seconds();
	LOG << tpc << tp0 << tp1;
    //time_t tpc = time(0);
    //time_t tp0 = nt.time() + 60*60*9 + 60*30;
    //time_t tp1 = tp0 + 60*30*11;

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
BOOL myflt0(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static const NTime sdate = NTime::init(2015,9,15);
	int n;
	const int nskip = 0; // args[1];
    t0 = NTime::init(args[3]/10000+2000, args[3]/100%100, args[3]%100);
	t1 = NTime::init(boost::posix_time::second_clock::local_time().date());
    if (t0 > t1 || t0 < sdate)
        t0 = sdate;

	REPORTDAT2 smx = {};
	n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, t0, t1, nTQ, 0);
	if (n < 1 || smx.Volume*100 < 1 /*|| (smx.Now - smx.Close)/smx.Close < -0.008*/) {
		LOG << Code << "Ig" << "Vol" << smx.Volume <<"Price"<< smx.Now << smx.Close;
		return FALSE;
	}
	STOCKINFO sinf = {};
	n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0);

	std::vector<HISDAT> his(100); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	if ((n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0)) < 1) {
		return FALSE;
	}
    his.resize(n);
    if (strcmp(Code,"600570") == 0) { // help info
		LOG << Code << nSetCode << DataType << (int)nTQ << t0 << t1 << boost::make_iterator_range(&args[0], &args[4]);
        LOG << Code << his.front() <<"\n"<< his.back();
        LOG << smx;
        LOG << sinf;
		//print_his(Code, nSetCode, PER_MIN1, 4, nTQ);
		//time_t t = time(0);
		//auto nt = NTime::init(t);
		//LOG << t <<"<>"<< nt <<"<>"<< nt.time();
		//int doy = boost::gregorian::day_clock::local_day().day_of_year();
        boost::posix_time::ptime pt(boost::gregorian::date(2015,11,22)
                , boost::posix_time::hours(17) + boost::posix_time::minutes(30));
		boost::gregorian::date date = pt.date();
		boost::posix_time::time_duration tod = pt.time_of_day();
		auto x = tod.hours();
		LOG << pt << "\t" << tod
			<<"\n"<< date.year() << date.month() << date.day()
			<<"\n"<< tod.hours() << tod.minutes() << tod.seconds()
			;
        LOG << boost::posix_time::second_clock::local_time();
        LOG << boost::posix_time::from_time_t(time(0));
    }
	if (his.size() < 20) {
		LOG << Code << his.size() << t0 << t1
	        <<"\n"<< his.front()
			<<"\n"<< his.back();
        return FALSE;
	}

    auto it = his.rbegin() + nskip;
    vola(Code, it, it+3);
    vola(Code, it, it+5);
    vola(Code, it, it+10);
    vola(Code, it, it+15);
    vola(Code, it, his.rend());
    return 0;
}
BOOL myflt0_(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	int n;

	REPORTDAT2 smx = {};
	if ((n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, t0, t1, nTQ, 0)) < 1) {
		return FALSE;
	}
	STOCKINFO sinf = {};
	if ((n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0)) < 1) {
		return FALSE;
	}
	if (smx.Volume*100 < 1 /*|| (smx.Now - smx.Close)/smx.Close < -0.008*/) {
		LOG << Code << "Ig" << "Vol" << smx.Volume <<"Price"<< smx.Now << smx.Close;
		return FALSE;
	}
	//if (smx.Volume*100 / sinf.ActiveCapital < 0.04) { return FALSE; }

	std::vector<HISDAT> his(10); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	if ((n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0)) < his.size()) {
		LOG << Code << "Few"<< n;
		return FALSE;
	}
    if (strcmp(Code,"600570") == 0) { // help info
		LOG << Code << nSetCode << DataType << (int)nTQ << t0 << t1 << boost::make_iterator_range(&args[0], &args[4]);
        LOG << Code << his.front() <<"\n"<< his.back();
        LOG << smx;
        LOG << sinf;
		////print_his(Code, nSetCode, PER_MIN1, 4, nTQ);
		//time_t t = time(0);
		//auto nt = NTime::init(t);
		//LOG << t <<"<>"<< nt <<"<>"<< nt.time();
        boost::posix_time::ptime pt(boost::gregorian::date(2015,11,22)
                , boost::posix_time::hours(17) + boost::posix_time::minutes(30));
		boost::gregorian::date date = pt.date();
		boost::posix_time::time_duration tod = pt.time_of_day();
		auto x = tod.hours();
		LOG << pt << "\t" << tod
			<<"\n"<< date.year() << date.month() << date.day()
			<<"\n"<< tod.hours() << tod.minutes() << tod.seconds()
			;
        LOG << boost::posix_time::second_clock::local_time();
        LOG << boost::posix_time::from_time_t(time(0));
    }
    if (strcmp(Code,"600570") == 0) { // help info
		LOG << Code << nSetCode << DataType << (int)nTQ << t0 << t1 << boost::make_iterator_range(&args[0], &args[4]);
        LOG << his.back();
        LOG << smx;
        LOG << sinf;
		//print_his(Code, nSetCode, PER_MIN30, 4, nTQ);
    }

	auto rbeg = his.rbegin();
	for (; rbeg != his.rend(); ++rbeg) {
		if (rbeg->fVolume / sinf.ActiveCapital > 0.05 && rbeg->Close > rbeg->Open) {
			rbeg = his.rbegin();
            break;
		}
	}
    if (rbeg != his.rbegin()) {
        return FALSE;
    }
	auto rnext = rbeg + 1;
	if ((rbeg->Close - rnext->Close)/rnext->Close < -0.01) {
		return FALSE;
	}

	auto lowp = std::min_element(his.rbegin(), his.rend()
		, [](HISDAT const& lhs, HISDAT const& rhs){
			return std::min(lhs.Open, lhs.Close) < std::min(rhs.Open,rhs.Close);
		});
	auto topp = std::max_element(lowp, his.rend()
		, [](HISDAT const& lhs, HISDAT const& rhs){
			return std::max(lhs.Open,lhs.Close) < std::max(rhs.Open,rhs.Close);
		});
	if (topp - lowp < 4) {
		return FALSE;
	}
	if (lowp - rbeg > 2) {
		return FALSE;
	}
	if (lowp == rbeg) {
		float upr = (rbeg->Close - rnext->Close) / rnext->Close;
		return (upr > -0.01);
	}
	float upr = (rbeg->Close - lowp->Close) / lowp->Close;
	if (upr < -0.01 || upr > 0.11) {
		return FALSE;
	}
	float exr = float(smx.Volume*100 / sinf.ActiveCapital);
	if (exr < 0.04 || exr > 0.11) {
        //LOG <<Code << "Volume" << int(rbeg->fVolume) << smx.Volume << smx.NowVol << int(sinf.ActiveCapital);
		return FALSE;
	}
	if ((rbeg->fVolume - rnext->fVolume) / sinf.ActiveCapital < 0.009) {
		return FALSE;
	}
	//if ((rbeg->fVolume - lowp->fVolume) * 100 / sinf.ActiveCapital < (lowp - rbeg) * 0.004 + 0.0025) { return FALSE; }
	return TRUE;
}

BOOL myflt1(char* Code, short nSetCode
	, int Value[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	int n;

	REPORTDAT2 smx = {};
	n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, t0, t1, nTQ, 0);
	if (n < 1 || smx.Volume*100 < 1) {
		LOG << Code << "Ig" << "Vol" << smx.Volume <<"Price"<< smx.Now << smx.Close;
		return FALSE;
	}
	STOCKINFO sinf = {};
	n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0);
	if (n<0)
		return FALSE;
	std::vector<HISDAT> his(10); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < (int)his.size()) {
		LOG << Code << "Few"<< n;
		return FALSE;
	}
	auto rbeg = his.rbegin();
	for (; rbeg != his.rend(); ++rbeg) {
		if (rbeg->fVolume / sinf.ActiveCapital > 0.05 && rbeg->Close > rbeg->Open) {
			rbeg = his.rbegin();
            break;
		}
	}
    if (rbeg != his.rbegin()) {
        return FALSE;
    }
	auto rnext = rbeg + 1;
    const float Ss = getsubs(his.back().Time); //(Value[1]<1 ? 4 : Value[1])/4.0;

	// float upr = (rbeg->Close - rnext->Close) / rnext->Close;
	if (rbeg->Close < rbeg->Open) {
		return FALSE;
	}
	//float exr = (rbeg->fVolume - rnext->fVolume) / sinf.ActiveCapital;
	//if (exr < (0.06*Ss)) {
		//return FALSE;
	//}
	if (rbeg->fVolume * 0.8 < rnext->fVolume *Ss) {
		return FALSE;
	}

	LOG << Code << "(" << int(rbeg->fVolume) << "-" << int(rnext->fVolume) << ")/" << int(sinf.ActiveCapital)
		<< rbeg->Time;

	return TRUE;
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

BOOL myflt2(char* Code, short nSetCode
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
	//STOCKINFO sinf = {};
	//n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, t0, t1, nTQ, 0);
	//if (n<0)
	//	return FALSE;
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
	if (atoi(Code) == 600259) {
		LOG << Code << boost::gregorian::day_clock::local_day() << his.back() << n;
	}

	auto beg = his.rbegin() + args[3];
    float a = ma(beg, beg+3, Code);
    for (int i=1; i < (args[2] ? args[2] : 6); ++i) {
        float x = ma(beg+i, beg+i+3, Code);
		if (args[1]) {
			if (x > a && (x - a) > 0.06)
				return 0;
		} else {
			if (x < a && (a - x) > 0.06)
				return 0;
		}
        a = x;
	}
    return 1;
}
BOOL myflt3(char* Code, short nSetCode // 扫描复牌
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
	return (rbeg->Time.date() - rnext->Time.date()).days() > 3;
	//return rbeg->Time.date().day_of_year() - rnext->Time.date().day_of_year() > 3;
}

BOOL myflt4(char* Code, short nSetCode
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

BOOL myflt5(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt6(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt7(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt8(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt9(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt10(char* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

