#include "myconfig.h"
#include <iconv.h>
#include <iostream>
#include <list>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include "MTNMsgApi.h"
#include "EUCPCInterface.h"
#include "MTNProtocol.h"
#include "log.h"
#include "util.h"
#include "asclient.h"

// using namespace std;
// using namespace boost;
// using namespace std;
using boost::asio::ip::tcp;
namespace sys = boost::system;
namespace placeholders = boost::asio::placeholders;

int sendsms(const std::string& phone, const std::string& message)
{
    const char* sn = "3SDK-EMY-0130-PEURL"; //"0SDK-EBB-0130-NEVOL";
    const char* key = "3fc2719ce96ebdf42b6dc1a1d8303e1b";
    const char* priority = "1";

    std::string gbkmsg(message.size() * 2, '\0');
    wiconv(gbkmsg, "GBK", message, "UTF-8");


    // size_t outleft = gbkmsg.size();

    // char* inbuf = const_cast<char*>(message.c_str());
    // size_t inleft = message.size();
    // iconv_t cd = iconv_open("GBK", "UTF-8");
    // if (cd == (iconv_t)-1)
    //     return -1;

    // char *p = &gbkmsg[0];
    // size_t ret = iconv(cd, &inbuf, &inleft, &p, &outleft);
    // iconv_close(cd);
    // if (ret == (size_t)-1)
    //     return -2;

    // gbkmsg.resize(gbkmsg.size() - outleft);

    LOG_I << boost::format("%s %s %s") % phone % sn % key;

    int ret1=0, ret2 = 0;
    ret1 = MTNMsgApi_NP::SetKey_NoMD5(const_cast<char*>(key));
    ret2 = MTNMsgApi_NP::SendSMS(const_cast<char*>(sn)
            , const_cast<char*>(phone.c_str())
            , const_cast<char*>(gbkmsg.c_str())
            , const_cast<char*>(priority)
            );

    LOG_I << boost::format("%d %d") % ret1 % ret2;
    return ret2;
}

// ./a.out 0SDK-EBB-0130-NEVOL 987729 123456
// 序列号：3SDK-EMY-0130-PEURL（复制的时候不要多余的空格复制进去了） 密码：438059（密码请手动输入） 特服号：186413（方便后台查询发送记录时提供）
 
