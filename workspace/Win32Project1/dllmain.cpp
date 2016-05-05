//#include <boost/filesystem/path.hpp>
// #include "log.h"
#include "tdxif.h"

FILE* logfile_ = stderr;
//static std::pair<int, char const*> DataTypes_[] = {
//	{ PER_MIN5, "5分钟数据" }
//	, { PER_MIN15, "15分钟数据" }
//	, { PER_MIN30, "30分钟数据" }
//	, { PER_HOUR, "1小时数据" }
//	, { PER_DAY, "日线数据" }
//	, { PER_WEEK, "周线数据" }
//	, { PER_MONTH, "月线数据" }
//	, { PER_MIN1, "1分钟数据" }
//	, { PER_MINN, "多分析数据(10)" }
//	, { PER_DAYN, "多天线数据(45)" }
//	, { PER_SEASON, "季线数据" }
//	, { PER_YEAR, "年线数据" }
//	, { PER_SEC5, "5秒线" }
//	, { PER_SECN, "多秒线(15)" }
//	, { PER_PRD_DIY0, "DIY周期" }
//	, { PER_PRD_DIY10, "DIY周期" }
//	, { REPORT_DAT2, "行情数据(第二版)" }
//	, { GBINFO_DAT, "股本信息" }
//	, { STKINFO_DAT, "股票相关数据 STOCKINFO" }
//	, { TPPRICE_DAT, "涨跌停数据" }
//};

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	GDef::ref();
	//LOG << hModule << ul_reason_for_call << lpReserved;

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
            // for (auto& f : at_exists_) f();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

PLUGIN_API void RegisterDataInterface(PDATAIOFUNC pfn)
{
	//LOG << pfn;
	GDef::ref().tdx_read_ = pfn;
}

PLUGIN_API void GetCopyRightInfo(PluginInfo* info)
{
	strcpy(info->Name,"ooo"); //此信息会显示在通达信插件选股对话框中“名称”栏
	strcpy(info->Dy,"0");
	strcpy(info->Author,"wood");	//此信息会显示在通达信插件选股对话框中“设计人”栏
	strcpy(info->Period,"x");		//还不清楚有什么用
	strcpy(info->Descript,"x");	//此信息会显示在通达信插件选股对话框中“选股对象”栏
	strcpy(info->OtherInfo, "x");  //strcpy(info->OtherInfo, "无其它信息描述");

	info->ParamNum = 4;	//定义插件参数数量，必须是[0~4]之间整数；

	strcpy(info->ParamInfo[0].acParaName, "Fx"); // strcpy(info->ParamInfo[0].acParaName, "MA短天数"); //第一个参数的名称
	info->ParamInfo[0].nMin=0;	//这部分通达信官方模板错误写成nMax，应该为nMin，参数最小值；
	info->ParamInfo[0].nMax=100;	//定义参数最大值
	info->ParamInfo[0].nDefault=0;	//定义参数默认值
	
	strcpy(info->ParamInfo[1].acParaName, "[1]"); // strcpy(info->ParamInfo[1].acParaName, "MA长天数"); //第二个参数的名称
	info->ParamInfo[1].nMin=0;
	info->ParamInfo[1].nMax=100;
	info->ParamInfo[1].nDefault=0;

	strcpy(info->ParamInfo[2].acParaName, "[2]"); // strcpy(info->ParamInfo[1].acParaName, "MA长天数"); //第二个参数的名称
	info->ParamInfo[2].nMin=0;
	info->ParamInfo[2].nMax=100;
	info->ParamInfo[2].nDefault=0;

	strcpy(info->ParamInfo[3].acParaName, "[3]"); // strcpy(info->ParamInfo[1].acParaName, "MA长天数"); //第二个参数的名称
	info->ParamInfo[3].nMin=0;
	info->ParamInfo[3].nMax=100;
	info->ParamInfo[3].nDefault=0;
}

static WORD   AfxRightData(float *pData,WORD nMaxData)	//获取有效数据位置
{	
	for (WORD nIndex=0; nIndex<nMaxData; nIndex++) {
        static const	BYTE	g_nAvoidMask[]={0xF8,0xF8,0xF8,0xF8};	// 无效数据标志(通达信系统定义)
        if (memcmp(&pData[nIndex],g_nAvoidMask,4) == 0)
            return nIndex;
    }
	return(nMaxData);
}

//当按照时间段选股时会自动调用此函数
PLUGIN_API BOOL InputInfoThenCalc2(char* Code, short nSetCode
        , int Value[4]
		, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long nDataNum)  //选取区段
{
	//if (t0 != NTime{ 0 })
	//	nDataNum = 8;
	//return (*GDef::ref().funcs_[Value[0]]) (Code, nSetCode, Value, DataType, t0, t1, nTQ, nDataNum);
    return 0;
}

//用通达信本地保存的所有数据进行选股，函数原型必须保持不变，系统会自动将需要的参数传递到本函数名字空间内
//Code:股票代码；
//nSetCode:所属市场（0为深市，1为沪市）；
//Value存参数值（通达信插件选股时用户设定值）
//DataType:由通达信行情软件传递过来的数据，4为日线，5为周线，参见OutStruct.h中宏定义。
//    默认为日线数据。如申请行情数据则赋值为REPORT_DAT2，其他相关类型参见OutStruct.h
//nDataNum为申请数据个数，
//    红宝书上讲，若为-1且pData为NULL则函数返回历史数据个数，
//    然而取到的数据必须存储到pData中，参数pData不允许为NULL
//    所以以上说法还需要继续研究，本程序中取nDataNum=2000
//nTQ:复权模式，0：不复权；1：前复权

static void test_hisdat(char const* Code, short nSetCode, short DataType, BYTE nTQ, const char* sdt="")
{
//    //static std::set<int> s;
//    std::vector<HISDAT> his(60*5);
//
//    LOG << Code << nSetCode << "--- --- ---" << sdt;
//
//	int n = GDef::tdx_read(Code, nSetCode, DataType, &his[0], his.size(), NTime{0}, NTime{0}, nTQ, 0);  //利用回调函数申请数据，返回得到的数据个数
//    if (n < 1 || n > (int)his.size()) {
//        LOG << Code << nSetCode << DataType << (int)nTQ << his.size() << "Fail:"<< n;
//
//    } else {
//        LOG << Code << DataType << his.size() << n <<"\nfront"<< his[0] <<"\nback"<< his[n-1];
//
//        REPORTDAT2 smx = {};
//        STOCKINFO sinf = {};
//        int n1 = GDef::tdx_read(Code, nSetCode, REPORT_DAT2, &smx, 1, NTime{}, NTime{}, nTQ, 0);
//        if (n1 < 0) {
//            LOG << Code << "REPORT_DAT2"<< n1;
//        } else {
//            LOG << Code  << n1<< smx;
//        }
//        int n2 = GDef::tdx_read(Code, nSetCode, STKINFO_DAT, &sinf, 1, NTime{}, NTime{}, nTQ, 0);
//        if (n2 < 0) {
//            LOG << Code << "STKINFO_DAT"<< n1;
//        } else {
//            LOG << Code  << n2<< sinf;
//        }
//	}
//
//    LOG << Code << nSetCode << "=== === ===";
//    //int n3 = GDef::tdx_read("002550", nSetCode, PER_DAY, &his[0], his.size(), t0, t1, nTQ, 0);
}

static BOOL help_info_dump(char const* Code, short nSetCode
		, int args[4]
		, short DataType, NTime t0, NTime t1, BYTE nTQ, unsigned long)
{
//    static bool showtime = 0;
//    if (!showtime) { //(sz.empty() && hz.empty())  // test time
//        showtime = 1;
//		boost::gregorian::from_simple_string("2000-01-01");
//        LOG << "posix_time:second_clock:local_time" <<boost::posix_time::second_clock::local_time();
//        LOG << "posix_time:from_time_t" <<boost::posix_time::from_time_t(time(0));
//        LOG << "gregorian:day_clock:local_day"<< boost::gregorian::day_clock::local_day();
//        //int doy = boost::gregorian::day_clock::local_day().day_of_year();
//
//        boost::posix_time::ptime pt(boost::gregorian::date(2015, 11, 22)
//            , boost::posix_time::hours(17) + boost::posix_time::minutes(30));
//        boost::gregorian::date date = pt.date();
//        boost::posix_time::time_duration tod = pt.time_of_day();
//        // auto x = tod.hours();
//        LOG << "ptime"<< pt << "\ttod" << tod
//            << "\ndate" << date
//            << "\ntod hms" << tod.hours() << tod.minutes() << tod.seconds()
//            ;
//
//		LOG << Code << nSetCode << DataType << (int)nTQ << t0 << t1 << boost::make_iterator_range(&args[0], &args[4]);
//
//		test_hisdat("999999", 1, DataType, nTQ, "PER_DAY"); // more
//    }
//
//	static const std::set<int> cset = {
//		399001, 399005, 399006, 2550 // 0
//		, 999999, 600570 // 1
//	};
//
//	if (cset.find(atoi(Code)) != cset.end()) {
//		test_hisdat(Code, nSetCode, DataType, nTQ, "PER_DAY");
//		test_hisdat(Code, nSetCode, PER_MIN1, nTQ, "PER_MIN1");
//		return 1;
//	}
//
//    LOG << Code << nSetCode << "......";
    return 0;
}

PLUGIN_API BOOL InputInfoThenCalc1(char* Code, short nSetCode
		, int args[4]
		, short DataType, short nDataNum, BYTE nTQ, unsigned long unused) //按最近数据计算
{
	return (*GDef::ref().funcs_[args[0]]) (Code, nSetCode, args, DataType, NTime{}, NTime{}, nTQ, nDataNum);
	//return InputInfoThenCalc2(const_cast<char*>(Code), nSetCode, args, DataType, NTime{}, NTime{}, nTQ, nDataNum);
}

static BOOL defaf(char const* Code, short nSetCode
	, int Value[4]
	, short DataType, NTime time1, NTime time2, BYTE nTQ, unsigned long unused)  //选取区段
{
	return atoi(Code) == 600570;
}

BOOL myflt0(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt1(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt2(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt3(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt4(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt5(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt6(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt7(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt8(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);
BOOL myflt9(char const* Code, short, int Value[4] , short DataType, NTime, NTime, BYTE nTQ, unsigned long);

GDef::GDef()
{
#define PATH_PREFIX "C:\\cygwin64\\home\\wood"
	logfile_ = fopen(PATH_PREFIX"\\xg.log", "w");
    ERR_MSG( "%s", ctime(0) );

	for (auto& f : funcs_)
		f = defaf;
	funcs_[0] = myflt0; // Default
	funcs_[1] = myflt1;
	funcs_[2] = myflt2;
	funcs_[3] = myflt3;
	funcs_[4] = myflt4;
	funcs_[5] = myflt5;
	funcs_[6] = myflt6;
	funcs_[7] = myflt7;
	funcs_[8] = myflt8;
	funcs_[9] = myflt9;
	//funcs_[22] = dfcf_read_ZXG; //dfcf::read_dfcf_ZXG;
	funcs_[23] = help_info_dump; //log helper
    ERR_MSG( "%s", PATH_PREFIX );
}
GDef::~GDef()
{
    //LOG << "UNINIT";
    ERR_MSG( "End" );
    if (logfile_ != stderr)
        fclose(logfile_); //(logging::logfile());
}

GDef& GDef::ref() {
	static GDef def;
	return def;
}

#include <boost/regex/pending/unicode_iterator.hpp>

