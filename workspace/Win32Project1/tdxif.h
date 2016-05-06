/**************************************************/
/*		PLUGIN动态连接库导出头文件				  */
/**************************************************/

#ifndef PLUGIN_H_
#define PLUGIN_H_

#ifdef PLUGIN_EXPORTS
#define PLUGIN_API extern "C"  __declspec(dllexport)
#else
#define PLUGIN_API extern "C" __declspec(dllimport)
#endif

#include <stdlib.h>
#include <string.h>
#include "tdxdef.h"

#pragma pack(push,1)
extern FILE* logfile_;
template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a) {
    fprintf(logfile_, fmt, lin_, a...);
    fflush(logfile_);
    exit(127);
}
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a) {
    fprintf(logfile_, fmt, lin_, a...);
    fputc('\n', logfile_);
    fflush(logfile_);
}
#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)

typedef struct PluginInfo_Param_Type //参数信息的结构定义
{
	char acParaName[14];			//参数的中文名称
	int  nMin;				//参数最小取值范围
	int  nMax;				//参数最大取值范围
	int  nDefault;				//系统推荐的缺省值
	int  nValue;				//用户定义的值
} PluginInfo_Param;

typedef struct PluginInfo_Type
{
	char  Name[50];				//指标名称与版本
	char  Dy[30];				//指标编写人所在地，不重要
	char  Author[30];			//设计人姓名
	char  Descript[100];			//选股指标描述
	char  Period[30];			//适应周期
	char  OtherInfo[300];
	short ParamNum;				//0<=参数个数<=4
	PluginInfo_Param ParamInfo[4];		//参数信息，见上struct tag_PluginPara定义
} PluginInfo;

//回调函数,取数据接口
typedef long (CALLBACK * PDATAIOFUNC)
    (char* Code,short nSetCode,short DataType,void* pData,short nDataNum,NTime,NTime,BYTE nTQ,unsigned long);

extern "C" {
//注册回调函数
PLUGIN_API void  RegisterDataInterface(PDATAIOFUNC pfn);

//得到版权信息
PLUGIN_API void	 GetCopyRightInfo(PluginInfo* info);

//按最近数据计算(nDataNum为ASK_ALL表示所有数据)
PLUGIN_API BOOL	 InputInfoThenCalc1(char* Code,short nSetCode,int Value[4],short DataType,short nDataNum,BYTE nTQ,unsigned long unused);

//选取区段计算
PLUGIN_API BOOL	 InputInfoThenCalc2(char* Code,short nSetCode,int Value[4],short DataType,NTime time1,NTime time2,BYTE nTQ,unsigned long unused); 

} // extern "C"

//#include <unordered_set>

//struct codes_set : std::unordered_set<int>
//{
//    codes_set(boost::filesystem::path fp);
//    bool exist(int c) const { return find(c)!=end(); }
//};
//
//codes_set::codes_set(boost::filesystem::path fp)
//{
//    if (boost::filesystem::exists(fp)) {
//        boost::filesystem::ifstream ifs(fp);
//        std::string s;
//        while (getline(ifs, s)) {
//            this->insert(atoi(s.c_str()));
//        }
//    }
//}

//#include <vector>
//#include <ptr_container/vector.hpp>

struct GDef
{
	template <typename ...T> static long tdx_read(const char* c, T&&... a) { return (*ref().tdx_read_)(const_cast<char*>(c), a...); }
	template <typename ...T> static long read(HISDAT* buf, unsigned bufsiz, int w, const char* c, short szsh, T&&... a) {
        return (*ref().tdx_read_)(const_cast<char*>(c), szsh, w, buf, bufsiz, a...);
    }

	static GDef& ref();

	PDATAIOFUNC	 tdx_read_;

	typedef BOOL(*FuncType)(char const* Code, short nSetCode
		, int Value[4]
		, short DataType, NTime time1, NTime time2, BYTE nTQ, unsigned long unused);

	FuncType funcs_[24];

    //std::vector<std::function<void()>> at_exists_;
	
	~GDef();
private:
	GDef();
};


#pragma pack(pop)
#endif

