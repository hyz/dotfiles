#ifndef __users_h__
#define __users_h__

#include <vector>
#include <algorithm>

#include "json.h"
#include "mycurl.h"  
#include "singleton.h"

typedef unsigned int UInt;

class UsrMgr : public singleton<UsrMgr>
{
    public:
        enum Sex{ MALE=0, FEMALE, _INVALID };
        std::vector<UInt> getactiveuser( int num, int sex );
        std::vector<UInt> getnewuser( time_t bt, time_t et, int sex );
        std::vector<UInt> im_usr( time_t gap );
};

#endif
