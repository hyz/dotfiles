#include "myconfig.h"
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <boost/throw_exception.hpp>
#include <boost/format.hpp>
#include "log.h"
#include "myerror.h"

using namespace std;
using namespace boost;

// void mythrow(int ln, const char* fn, int c, const boost::system::error_category & cat)
// {
//     mythrow(ln, fn, boost::system::error_code(c,cat));
// }

void mythrow(int ln, const char* fn, const boost::system::error_code & ec)
{
#define SIZE_ 4
    void *buffer[SIZE_];
    int nptrs;

    nptrs = backtrace(buffer, SIZE_);
    // backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);

    char ** strings = backtrace_symbols(buffer, nptrs);
    if (strings)
    {
        logging::ostream_fwd log = LOG_E;
        log << "symbols: ";
        for (int i = 0; i < nptrs; i++)
            log << strings[i] << "\t";
        free(strings);
    }

    boost::throw_exception(
        my_exception(ec) << errinfo_pos(boost::make_tuple(ln,fn))
    );
}

Error_category::Error_category()
{
    node_ = 0;
}

std::string Error_category::message( int c ) const
{
    for (Node *pn = node_; pn; pn = pn->next)
    {
        for (code_string *p = pn->begin; p != pn->end; ++p)
        {
            if (p->first == c)
            {
                return p->second;
            }
        }
    }
    return "未知错误";
}

myerror_category::myerror_category()
{
    static code_string ecl[] = {
        { EN_HTTPRequestPath_NotFound, "请求路径不存在" },

        { EN_HTTPParam_NotFound , "缺少HTTP请求参数" },
        { EN_HTTPHeader_NotFound , "缺少HTTP头部字段" },

        { EN_DBConnect_Fail , "连接数据库失败" },
        { EN_SQL_Fail , "数据库查询失败" },

        { EN_JsonName_NotFound , "缺少JSON数据" },
        { EN_JsonValue_TypeError , "JSON数据类型错误" },

        { EN_Server_InternalError, "服务器内部异常" },
    };

    Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])]);
}

// const system::error_category & myerror_category()
// {
//     static const My_error_category ecat;
//     return ecat;
// }

my_exception::my_exception(const boost::system::error_code & ec)
    : ec_(ec)
{}

my_exception::my_exception(int n, const boost::system::error_category & cat)
    : ec_(n,cat)
{}

Error_category::Node::Node(const char* nm, Node *nd, code_string *b, code_string *e)
{
    name=(nm?nm:"");
    next=nd;
    begin=b;
    end=e;

    // std::ofstream ofs;
    // ofs.open ("/tmp/error.txt", std::ofstream::out | std::ofstream::app);
    // ofs<<name<<std::endl;
    // for (code_string *p = b; p != e; ++p)
    // {
    //     ofs<<p->first<<"    "<<p->second<<std::endl;
    // }
    // ofs.close();
}

