#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_set>
//#include <algorithm>
//#include <numeric>
#include <boost/config/warning_disable.hpp>
#include <boost/format.hpp>
//#include <boost/algorithm/string.hpp>
//#include <set>
//#include <iostream>
//#include "log.h"
//#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/fstream.hpp>

#include "tdxif.h"

#define HOME "D:/home/wood"
#define DIR_OUT HOME"/_"
#define FN_EXCLS HOME"/._excls"

struct ymd_type
{
    int y,m,d;
    ymd_type(NTime const& t) {
        y = t.year;
        m = t.month;
        d = t.day;
    }
};

static char* c_trim_right(char* h, const char* cs)
{
    char* end = h + strlen(h);
    char* p = end;
    while (p > h && strchr(cs, *(p-1)))
        --p;
    *p = '\0';
    return h;
}
template <typename... V> struct Path_join ;
template <typename T, typename... V>
struct Path_join<T,V...> { static void concat(char*beg,char*end, T const& a, V const&... v) {
    Path_join<typename std::decay<T>::type>::concat(beg,end, a);
    Path_join<typename std::decay<V>::type...>::concat(beg,end, v...);
}};
template <> struct Path_join<char*> { static void concat(char*beg,char*end, char const* src) {
    char* p = beg + strlen(beg); // char* src = a;
    if (p != beg && p != end)
        *p++ = '/';
    while (p != end && *src)
        *p++ = *src++;
    *p = '\0';
    c_trim_right(beg, "/\\");
}}; 
template <> struct Path_join<char const*> { static void concat(char*beg,char*end, char const* src) {
    Path_join<char*>::concat(beg,end, src);
}};
template <> struct Path_join<std::string> { static void concat(char*beg,char*end, std::string const& src) {
    Path_join<char*>::concat(beg,end, src.c_str());
}};
template <> struct Path_join<boost::format> { static void concat(char*beg,char*end, boost::format const& src) {
    auto s = str(src);
    Path_join<char*>::concat(beg,end, s.c_str());
}};

template <int L=128>
struct makepath : std::array<char,L> {
    template <typename ...V>
    makepath(V&&... v) {
        this->front() = this->back() = '\0';
        char* end = &this->back(); //+1;
        char* beg = c_str();
        Path_join<typename std::decay<V>::type...>::concat(beg,end, v...);
    }
    char* c_str() const { return const_cast<char*>(this->data()); }
};

struct codes_set : std::unordered_set<int>
{
    template <typename ...V> codes_set(V&& ... v)
    {
        makepath<128> fn(v...);
        if (FILE* fp = fopen(fn.c_str(), "r")) {
            std::unique_ptr<FILE,decltype(&fclose)> xclose(fp, fclose);
            char linebuf[1024];
            while (fgets(linebuf, sizeof(linebuf), fp)) {
                this->insert(atoi(linebuf));
            }
        }
    }
    bool exist(int c) const { return find(c)!=end(); }
};

struct _999999 :  std::vector<HISDAT>
{
    _999999(BYTE nTQ, int len=5) {
        std::vector<HISDAT>& v = *this;
        v.resize(len<1 ? 1:len);
		int n = GDef::tdx_read("999999", 1, PER_DAY, &v[0], (int)v.size(), NTime{}, NTime{}, nTQ, 0);
        if (n != len)
            ERR_EXIT("999999: %d: %d", (int)v.size(), n);
    }
	ymd_type ymd(int x = -1) const { return ymd_type(Time(x)); }
	NTime const& Time(int x = -1) const { return (x == 0 ? front().Time : back().Time); }
};

BOOL myflt0(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static codes_set excls_(FN_EXCLS); //(FN_EXCLS);
    static codes_set s(HOME, boost::format("_%02d") % args[1]); //(str(boost::format(DIR_OUT"\\_%02d") % args[1]).c_str());
    int c = atoi(Code);
    return (s.exist(c) && !excls_.exist(c));
}

struct Out1 : _999999
{
    codes_set excls_;
    FILE* fp_ = 0;
    BYTE nTQ_;
    std::vector<HISDAT> sh_; //std::pair<NTime,NTime> time_range_ = {};

    Out1(int args[4], BYTE nTQ) : _999999(nTQ, args[1]>0 ? args[1]:30), excls_(FN_EXCLS) {
        nTQ_ = nTQ;
        sh_.resize(_999999::size());
        auto t = ymd(); //ymd_type(sh_.back().Time);
        makepath<128> fn(DIR_OUT, boost::format("%d.%02d%02d-%d") % args[3] % t.m % t.d % (int)sh_.size());
        fp_ = fopen(fn.c_str(), "w");
    }
    ~Out1() {
        if (fp_) {
            std::unique_ptr<FILE,decltype(&fclose)> xclose(fp_, fclose);
            (void)xclose;
        }
    }

    void print(char const* Code, short nSetCode)
    {
        if (!fp_ || excls_.exist(atoi(Code)))
            return;
        int len = (int)sh_.size();
        if ( (len = GDef::read(&sh_[0], len, PER_DAY, Code, nSetCode, Time(0), NTime{}, nTQ_, 0)) > 3) {
            if (sh_[len-1].fVolume < 1)
                return;
            fprintf(fp_, "%s %d", Code, nSetCode);
            for (int i=0; i < len; ++i) {
                auto& a = sh_[i];
                fprintf(fp_, "\t" "%.0f %.0f" " %.0f %.0f %.0f %.0f"
                        , a.fVolume, a.a.Amount
                        , 100*a.Open, 100*a.Close, 100*a.Low, 100*a.High);
            }
            fprintf(fp_, "\n");
        }
    }
};
BOOL myflt1(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static Out1 out(args, nTQ);
    out.print(Code, nSetCode);
    //static codes_set excls_(FN_EXCLS);

    //int icode = atoi(Code);
    //if (excls_.exist(icode)) {
    //    return 0;
    //}

	//STOCKINFO si = {};
	//REPORTDAT2 rp = {};
	//std::vector<HISDAT> his;

    //{
    //    int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, t0, t1, nTQ, 0);
    //    if (n < 0) {
    //        //LOG << Code << "STKINFO_DAT error" << n;
    //        return 0;
    //    }
    //    if (si.ActiveCapital < 1) {
    //        return 0;
    //    }
    //}
    //if (!args[2]) {
    //    his.resize(args[1] ? args[1]*10 : 50);
    //    int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
    //    if (n < 1)/*(n < (int)his.size())*/ {
    //        //LOG << Code << "PER_DAY error"<< n;
    //        return 0;
    //    }
    //    his.resize(n);
    //}
    //if(false){
    //    int n = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &rp, 1, t0, t1, nTQ, 0);
    //    if (n < 0 || rp.Volume*100 < 1) {
    //        //LOG << Code << "REPORT_DAT2 error" << n << "Vol" << rp.Volume <<"Price"<< rp.Now << rp.Close;
    //        return 0;
    //    }
    //}

	//using boost::format;
    //makepath<128> path(DIR_OUT, str(format("%02d") % args[3]).c_str()); //= boost::filesystem::path(DIR_OUT) / str(format("%02d") % args[3]);

	//if (!boost::filesystem::exists(path.c_str())) {
	//	boost::filesystem::create_directories(path.c_str());
	//}

    //{
	//	static boost::filesystem::ofstream ofs(path/"lis", std::ios::trunc);
    //    ofs << Code<<' '<<nSetCode
    //        //<<'\t'<< format("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f") % rp.Close % rp.Open % rp.Max % rp.Min % rp.Now % rp.Amount
    //        <<'\t'<< format("%.0f %.0f %.0f %.0f") % si.ActiveCapital % si.J_zgb % si.J_bg % si.J_hg
    //        <<'\t'<< format("%.0f %.0f") % (si.J_mgsy*100) % (100*si.J_mgsy2)
    //        //<<'\t'<< format("%.2f\t%.2f\t%.2f""\t""%.2f\t%.2f\t%.2f\t%.2f")
    //        //            % si.J_yysy % si.J_yycb % si.J_yyly
    //        //            % si.J_lyze % si.J_shly % si.J_jly % si.J_jyl
    //        <<'\t'<< int(si.J_hy) // <<'\t'<< int(si.J_zjhhy *100)
	//		<<'\n'<< std::flush;
    //    //float       J_yysy;			//营业收入 # 营业收入(元)
    //    //float       J_yycb;			//营业成本
    //    //float       J_yyly;			//营业利润
    //    //
    //    //float       J_lyze;			//利益总额
    //    //float       J_shly;			//税后利益
    //    //float       J_jly;			//净利益   # 归属净利润(元)
    //    //float		J_jyl;				//净益率%  # 摊薄净资产收益率(%)
    //    //
    //    //float       J_bg;				//B股
    //    //float       J_hg;				//H股
    //    //
    //    //short       J_hy;				//所属行业
    //    //float       J_zjhhy;			//证监会行业
    //    //float		J_mgsy;				//每股收益(折算成全年的)
    //    //float       J_mgsy2;			//季报每股收益 (财报中提供的每股收益,有争议的才填)
    //}
    //if (!his.empty()) {
    //    boost::filesystem::ofstream ofs(path/Code, std::ios::trunc);
    //    for (auto it=his.begin(); it!=his.end(); ++it) {
    //        auto& a = *it;
    //        ofs //    << Code <<'\t'
    //            << a.Time
    //            <<'\t'<< format("%.0f %.0f")% a.fVolume % a.a.Amount 
    //            <<'\t'<< format("%.0f %.0f %.0f %.0f") % (100*a.Open) % (100*a.Close) % (100*a.High) % (100*a.Low)
    //            <<'\n';
    //    }
    //}

    return 0;
}

inline unsigned make_code(int szsh, int numb) { return ((szsh << 24) | numb); }
inline int numb(unsigned v_) { return v_ & 0x0ffffff; }
inline int szsh(unsigned v_) { return (v_ >> 24) & 0xff; }

struct Out2
{
    std::vector<HISDAT> sh_;
    FILE* fp_ = 0;
    codes_set excls_;
    BYTE nTQ_;

    bool tdxread(std::vector<HISDAT>& vh, char const* c) {
        return (GDef::read(&vh[0], vh.size(), PER_DAY, c, 1, NTime{}, NTime{}, nTQ_, 0) == (int)vh.size());
    }
    Out2(int args[4], BYTE nTQ) : excls_(FN_EXCLS) //(FN_EXCLS)
    {
        nTQ_ = nTQ;
        sh_.resize(args[1]>0 ? args[1] : 30);
        if (tdxread(sh_, "999999")) {
			//auto & t0 = sh_.front().Time;
			auto t1 = ymd_type(sh_.back().Time);
            makepath<128> fn(DIR_OUT, boost::format("%02d%02d-%d.d") % t1.m % t1.d % (int)sh_.size()); // char fn[128]; sprintf(fn,DIR_OUT"\\%02d%02d-%d.d", t1.month,t1.day, (int)sh_.size());
            fp_ = fopen(fn.c_str(), "w");
        }
    }
    ~Out2() {
        if (fp_)
            fclose(fp_);
    }

    void print(char const* Code, short nSetCode)
    {
        if (!fp_ || excls_.exist(atoi(Code)))
            return;
		std::vector<HISDAT> ls(sh_.size());
        int n;
		if ((n = GDef::read(&ls[0], ls.size(), PER_DAY, Code, nSetCode, NTime{}, NTime{}, nTQ_, 0)) > 3) {
            if (ls.back().fVolume < 1)
                return;
            ls.resize(n);
            //if (ls.front().Time.day != sh_.front().Time.day || ls.back().Time.day != sh_.back().Time.day || ls.back().fVolume < 1) return;
            fprintf(fp_, "%s %d", Code, nSetCode);
            for (auto it = ls.begin(); it != ls.end(); ++it) {
                fprintf(fp_, "\t%.0f %.0f %.0f %.0f %.0f %.0f"
                        , it->fVolume, it->a.Amount
                        , (100*it->Open), (100*it->Close), (100*it->Low), (100*it->High) );
            }
			fprintf(fp_, "\n");
        }
    }
};

struct Out3
{
    std::vector<HISDAT> sh_;
    codes_set excls_;
    FILE* fp_ = 0;

    template <typename ...A> Out3(int args[4], A... a) : excls_(FN_EXCLS)
    {
        rd_(sh_, "999999", 1, a...);
        auto t = ymd_type(sh_.front().Time);
        makepath<128> fn(DIR_OUT, boost::format("%02d%02d.M") % t.m % t.d); // char fn[128]; sprintf(fn,DIR_OUT"\\%02d%02d.M", t.month, t.day);
        fp_ = fopen(fn.c_str(), "w");
    }

    ~Out3() {
        if (fp_)
            fclose(fp_);
    }

    template <typename ...A> void print(char const* Code, short nSetCode, A&&... a) {
        if (excls_.exist(atoi(Code)))
            return;

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
    static Out2 out(args, nTQ);
    out.print(Code, nSetCode);
	return 0;
}

BOOL myflt3(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static Out3 out(args, t0, t1, nTQ, 0);
    out.print(Code, nSetCode, t0, t1, nTQ, 0);

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

BOOL myflt4(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
	return 0;
}

BOOL myflt5(char const* Code, short nSetCode // 复p
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
	return 0;// y->Time.date() != hs[3].Time.date() || t->Time.date() != hs[4].Time.date();
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

BOOL myflt9(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    return 0;
}

struct Out6 : _999999
{
    FILE* fp_ = 0;
    BYTE nTQ_;

    Out6(int args[4], BYTE nTQ) : _999999(nTQ)
    {
        nTQ_ = nTQ;
        //char tmp[8]; sprintf(tmp, "_%02d", args[3]); // auto s = str(boost::format("_%02d") % args[3]);
        auto t = ymd();
        makepath<128> fn(DIR_OUT, boost::format("s.%02d%02d") % t.m % t.d); // using boost::format; auto fn = boost::filesystem::path(DIR_OUT) / str(format("_%02d") % args[3]);
        ERR_MSG("%s", fn.c_str());
        fp_ = fopen(fn.c_str(), "w");
    }
    ~Out6() {
        if (fp_) {
            std::unique_ptr<FILE,decltype(&fclose)> xclose(fp_, fclose);
            (void)xclose;
        }
    }

    int print(char const* Code, short nSetCode)
    {
        STOCKINFO si = {};
        int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, NTime{}, NTime{}, nTQ_, 0);
        if (n < 0) {
            return 0;
        }
        if (si.ActiveCapital < 1) {
            return 0;
        }
        fprintf(fp_, "%s %d" "\t%.0f %.0f %.0f %.0f" "\t%.0f %.0f" "\t%d" "\n"
                , Code, int(nSetCode)
                , si.ActiveCapital, si.J_zgb, si.J_bg, si.J_hg
                , si.J_mgsy*100, 100*si.J_mgsy2
                , int(si.J_hy)
               );
        return 0;
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
};
struct Out7 : _999999
{
    FILE* fp_ = 0;
    BYTE nTQ_;

    Out7(int args[4], BYTE nTQ) : _999999(nTQ) {
        nTQ_ = nTQ;
        auto t = ymd();
        makepath<128> fn(DIR_OUT, boost::format("names.%02d%02d") % t.m % t.d); //auto fn = boost::filesystem::path(DIR_OUT) / "_names";
        ERR_MSG("%s", fn.c_str());
        fp_ = fopen(fn.c_str(), "w");
    }
    ~Out7() {
        if (fp_) {
            std::unique_ptr<FILE,decltype(&fclose)> xclose(fp_, fclose);
            (void)xclose;
        }
    }
    void print(char const* Code, short nSetCode) {
        STOCKINFO si = {};
        int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, NTime{}, NTime{}, nTQ_, 0);
        if (n > 0) {
            si.Name[8]='\0';
            fprintf(fp_, "%s %d\t%s\n" , Code, nSetCode, si.Name);
        }
    }
};


BOOL myflt6(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static Out6 out(args, nTQ); //ERR_MSG("%d, %s %hd", args[0], Code, nSetCode);
    out.print(Code, nSetCode);
	return 0;
	//STOCKINFO si = {};
    //int n = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &si, 1, t0, t1, nTQ, 0);
    //if (n > 0) {
    //    static boost::filesystem::path fp = DIR_OUT"\\_sname";
    //    static boost::filesystem::ofstream ofs(fp, std::ios::trunc);
    //    si.Name[8]='\0';
    //    ofs << Code <<' '<< nSetCode <<' '<< si.Name <<'\n';
    //}
    //return 0;
}

BOOL myflt7(char const* Code, short nSetCode
	, int args[4]
	, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)  //选取区段
{
    static Out7 out(args, nTQ);
    out.print(Code, nSetCode);
	return 0;
	//std::vector<HISDAT> his(5);
	//int n = GDef::tdx_read(Code, nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
	//if (n < (int)his.size())
	//	return 0;
	//auto last = his.end() - 1;
	//auto lasp = last - 1;

    //float afvol = 0;
    //for (auto it=his.begin(); it != last; ++it)
    //    afvol += it->fVolume;
    //afvol /= (his.size()-1);
    //float fvol = last->fVolume/tvolume(last->Time.date());

    //return last->Close/lasp->Close > (1-0.02) && fvol/afvol < (1+0.1);
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

//static float tvolume(boost::gregorian::date const& date)
//{
//	using namespace boost::posix_time;
//
//	auto lt = second_clock::local_time();
//	if (lt.date() != date)
//		return 1;
//
//	auto tod = lt.time_of_day();
//    if (tod < hours(12))
//        tod += hours(1) + minutes(30);
//    return float(tod.total_seconds() - 3600*11) / 3600*4;
//}

