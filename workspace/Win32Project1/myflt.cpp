#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string>
#include <vector>
//#include <algorithm>
//#include <numeric>
#include <fstream>
#include <boost/config/warning_disable.hpp>
//#include <boost/algorithm/string.hpp>
//#include <set>
#include <unordered_set>
//#include <iostream>
//#include "log.h"
#include "tdxif.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

struct codes_set : std::unordered_set<int>
{
    codes_set(boost::filesystem::path fp)
    {
        if (boost::filesystem::exists(fp)) {
            boost::filesystem::ifstream ifs(fp);
            std::string s;
            while (getline(ifs, s)) {
                this->insert(atoi(s.c_str()));
            }
        }
    }
    bool exist(int c) const { return find(c)!=end(); }
};

BOOL myflt0(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static codes_set excls("D:\\home\\wood\\._sexcl");
    static codes_set s(str(boost::format("D:\\home\\wood\\_%02d") % args[1]));
    int c = atoi(Code);
    return (s.exist(c) && !excls.exist(c));
}

BOOL myflt1(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static codes_set excls("D:\\home\\wood\\._sexcl");

    int icode = atoi(Code);
    if (excls.exist(icode)) {
        return 0;
    }

	STOCKINFO si = {};
	REPORTDAT2 rp = {};
	std::vector<HISDAT> his;

    {
        int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, t0, t1, nTQ, 0);
        if (n < 0) {
            //LOG << Code << "STKINFO_DAT error" << n;
            return 0;
        }
        if (si.ActiveCapital < 1) {
            return 0;
        }
    }
    if (!args[2]) {
        his.resize(args[1] ? args[1]*20 : 50);
        int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
        if (n < 1)/*(n < (int)his.size())*/ {
            //LOG << Code << "PER_DAY error"<< n;
            return 0;
        }
        his.resize(n);
    }
    if(false){
        int n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &rp, 1, t0, t1, nTQ, 0);
        if (n < 0 || rp.Volume*100 < 1) {
            //LOG << Code << "REPORT_DAT2 error" << n << "Vol" << rp.Volume <<"Price"<< rp.Now << rp.Close;
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
        ofs << Code<<' '<<nSetCode
            //<<'\t'<< format("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f") % rp.Close % rp.Open % rp.Max % rp.Min % rp.Now % rp.Amount
            <<'\t'<< format("%.0f %.0f %.0f %.0f") % si.ActiveCapital % si.J_zgb % si.J_bg % si.J_hg
            <<'\t'<< format("%.0f %.0f") % (si.J_mgsy*100) % (100*si.J_mgsy2)
            //<<'\t'<< format("%.2f\t%.2f\t%.2f""\t""%.2f\t%.2f\t%.2f\t%.2f")
            //            % si.J_yysy % si.J_yycb % si.J_yyly
            //            % si.J_lyze % si.J_shly % si.J_jly % si.J_jyl
            <<'\t'<< int(si.J_hy) // <<'\t'<< int(si.J_zjhhy *100)
			<<'\n'<< std::flush;
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

    }
    if (!his.empty()) {
        boost::filesystem::ofstream ofs(fp/Code, std::ios::trunc);
        for (auto it=his.begin(); it!=his.end(); ++it) {
            auto& a = *it;
            ofs //    << Code <<'\t'
                << a.Time
                <<'\t'<< format("%.0f %.0f")% a.fVolume % a.a.Amount 
                <<'\t'<< format("%.0f %.0f %.0f %.0f") % (100*a.Open) % (100*a.Close) % (100*a.High) % (100*a.Low)
                <<'\n';
        }
    }

    return 0;
}

BOOL myflt9(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    return 0;
}


BOOL myflt2(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	STOCKINFO si = {};
    int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, t0, t1, nTQ, 0);
    if (n > 0) {
        static boost::filesystem::path fp = "D:\\home\\wood\\._sname";
        static boost::filesystem::ofstream ofs(fp, std::ios::trunc);
        ofs << Code <<' '<< nSetCode <<' '<< si.Name <<'\n';
    }
    return 0;
}

BOOL myflt3(char const* Code, short nSetCode // 扫描复牌
	, int Value[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	static std::vector<HISDAT> hs; // typedef std::vector<HISDAT>::reverse_iterator itertype;
	if (hs.empty()) {
		hs.resize(5);
		int n = GDef::tdx_read("999999", 1, PER_DAY, &hs[0], hs.size(), t0, t1, nTQ, 0);
		if (n < 5) {
			hs.clear();
			return 0;
		}
	}

	std::vector<HISDAT> his(5); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < (int)his.size())
		return 1;
	auto t = his.end()-1;
	auto y = t-1;
	return y->Time.date() != hs[3].Time.date() || t->Time.date() != hs[4].Time.date();
	//auto rnext = rbeg + 1;
	////if (atoi(Code) < 40) { // help info
	////	LOG << Code
	////		<< rbeg->Time << rnext->Time
	////		<< "\n" << rbeg->Time.date() << rnext->Time.date();
	////}
	//return (rbeg->Time.date() - rnext->Time.date()).days() > 3 || rbeg->fVolume*100 < 1;
	////return rbeg->fVolume * 100 > 1 && rnext->fVolume * 100 < 1;
	////return rbeg->Time.date().day_of_year() - rnext->Time.date().day_of_year() > 3;
}

BOOL myflt4(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	std::vector<HISDAT> his(5); // typedef std::vector<HISDAT>::reverse_iterator itertype;
	int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < 3)
		return 0;
	auto last = his.end() - 1;
	auto lasp = last - 1;
	auto laspp = lasp - 1;
	//auto inr = [](float x, float y){ return (y - x) / x;  };
	if (int(lasp->Close * 100) < int(laspp->Close * 1.1 * 100 + 0.5))
		return 0;
	return int(lasp->Close*1.1 * 100 + 0.5) > int(last->Close * 100);
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
    static std::ofstream ofs("D:\\home\\wood\\codes_", std::ios::trunc);
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

