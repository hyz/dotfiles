#include "chatroom.h"

const char* sid_sptr(std::string const& sid)
{
    if (atoi(sid.c_str()) != 23) {
        return 0;
    }
    auto pos = sid.find('/');
    if (pos == std::string::npos) {
        return 0;
    }
    return sid.c_str()+(pos+1);
}

room_ptr find_room(chatroom_mgr& mgr, std::string const& sid)
{
    char const* subsid = sid_sptr(sid);
    if (subsid) {
        if (room_ptr room = mgr.find_room(subsid)) {
            return room;
        }
    }
    LOG << "room not-found" << sid;
    return room_ptr();
}

