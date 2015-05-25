#ifndef EVENTS_H__
#define EVENTS_H__

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include "globals.h"
#include "singleton.h"

enum { UID_PRESERVED_DUMMY_USER=100 };
enum { UID_PRESERVED_CHATROOM=101 };


struct user_mgr : singleton<user_mgr>
{
    boost::signals2::signal<void (UInt,int, int)> ev_online;

    boost::signals2::signal<void (UInt,int)> ev_regist;

};

#endif // EVENTS_H__

