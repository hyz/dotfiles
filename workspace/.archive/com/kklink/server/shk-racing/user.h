#ifndef EVENTS_H__
#define EVENTS_H__

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include "globals.h"
#include "singleton.h"

struct user_mgr : singleton<user_mgr>
{
    boost::signals2::signal<void (UInt,int, int)> ev_online;

    boost::signals2::signal<void (UInt,int)> ev_regist;

};

#endif // EVENTS_H__

