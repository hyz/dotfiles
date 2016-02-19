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

inline unsigned make_code(int szsh, int numb) { return ((szsh << 24) | numb); }
inline int numb(unsigned v_) { return v_ & 0x0ffffff; }
inline int szsh(unsigned v_) { return (v_ >> 24) & 0xff; }

struct Print1
{
    std::vector<HISDAT> sh_;
    FILE* fp_;

    template <typename ...A> Print1(int args[4], A... a)
    {
        rd_(sh_, "999999", 1, a...);
        auto fn = "D:\\home\\wood\\workspace\\mys\\TDX.1";
        fp_ = fopen(fn, "w");
    }

    ~Print1() {
        if (fp_)
            fclose(fp_);
    }

    template <typename ...A> void operator()(char const* Code, short nSetCode, A&&... a) {
        std::vector<HISDAT> ls;
        if (fp_ && rd_(ls, Code, nSetCode, a...) > 0) {
            fprintf(fp_, "%s %d", Code, nSetCode);
            auto it = ls.begin();
            auto i0 = sh_.begin();
            for (; i0 != sh_.end(); ++i0) {
                if (i0->Time.minute < it->Time.minute) {
                    fprintf(fp_, "\t0 0 0 0 0 0");
                    continue;
                }
                fprintf(fp_, "\t%.0f %.0f %.0f %.0f %.0f %.0f"
                        , it->fVolume, it->a.Amount
                        , (100*it->Open), (100*it->Close), (100*it->High), (100*it->Low) );
                ++it;
            }
			fprintf(fp_, "\n");
        }
    }

    template<typename ...A> int rd_(std::vector<HISDAT>& ls, A&&... a)
    {
        ls.resize(30*10); //((args[2] ? args[2] : 9)*30);
        int n = GDef::read(&ls[0], ls.size(), PER_MIN1, a...);
        if (n < 1) {
            ls.resize(0);
            return 0;
        }
        ls.resize(n);
        if (&ls == &sh_) {
            auto& back = ls.back();
            int i = 0;
            while (i < n && ls[i].Time.day != back.Time.day) {
                ++i;
            }
            if (i > 0) {
                ::memmove(&ls[0], &ls[i], (n-i)*sizeof(ls[0]));
                ls.resize(n-i);
            }
            return n;
        } else if (!sh_.empty()) {
            auto& back = sh_.back();
            while (n > 0 && ls[n-1].Time.day != back.Time.day) {
                --n;
            }
            int i = 0;
            while (i < n && ls[i].Time.day != back.Time.day) {
                ++i;
            }
            if (i > 0) {
                ::memmove(&ls[0], &ls[i], (n-i)*sizeof(ls[0]));
                ls.resize(n-i);
            }
            return n;
        }
        ls.resize(0);
        return 0;
    }
};

BOOL myflt2(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static codes_set excls("D:\\home\\wood\\._sexcl");
    int icode = atoi(Code);
    if (excls.exist(icode))
        return 0;

    static Print1 print(args, t0, t1, nTQ, 0);
    print(Code, nSetCode, t0, t1, nTQ, 0);

    //if (args[1]==0)
    //    args[1] = PER_MIN1;
    //static bool once = 0;

    //if (!once && icode != 999999) {
    //    once = 1;
    //    std::vector<HISDAT> his;
    //    his.resize((args[2] ? args[2] : 9)*30);
    //    int n = GDef::tdx_read("999999", 1, args[1], &his[0], his.size(), t0, t1, nTQ, 0);
    //    if (n > 1)/*(n < (int)his.size())*/ {
    //        his.resize(n);
    //        print(make_code(nSetCode, icode), his);
    //    }
    //}

    return 0;
}

BOOL myflt3(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt9(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    return 0;
}


BOOL myflt4(char const* Code, short nSetCode
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

BOOL myflt5(char const* Code, short nSetCode // 复牌
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

BOOL myflt6(char const* Code, short nSetCode
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

static float tvolume(boost::gregorian::date const& date)
{
	using namespace boost::posix_time;

	auto lt = second_clock::local_time();
	if (lt.date() != date)
		return 1;

	auto tod = lt.time_of_day();
    if (tod < hours(12))
        tod += hours(1) + minutes(30);
    return float(tod.total_seconds() - 3600*11) / 3600*4;
}

BOOL myflt7(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	std::vector<HISDAT> his(5);
	int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	if (n < (int)his.size())
		return 0;
	auto last = his.end() - 1;
	auto lasp = last - 1;

    float afvol = 0;
    for (auto it=his.begin(); it != last; ++it)
        afvol += it->fVolume;
    afvol /= (his.size()-1);
    float fvol = last->fVolume/tvolume(last->Time.date());

    return last->Close/lasp->Close > (1-0.02) && fvol/afvol < (1+0.1);
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

