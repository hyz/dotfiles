#ifndef CHATROOM_H__
#define CHATROOM_H__

#include <string>
#include <ostream>
#include <boost/unordered_map.hpp>
//#include <boost/unordered_set.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
//#include <boost/multi_index/identity.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/circular_buffer.hpp>
#include "json.h"
#include "dbc.h"
#include "singleton.h"
#include "message.h"

// typedef unsigned int UInt;
static bool bquit_ = 0;
namespace multi_index = boost::multi_index;

struct chatr_impl // : std::set<UInt>
{
    UInt refcount_;
    UInt rid;
    UInt l_msgid;
    //boost::circular_buffer<> msglist;

    chatr_impl(UInt r, UInt lmid) // : msglist(10)
    {
        refcount_=0;
        rid = r;
        l_msgid = lmid;
    }

    UInt last_msgid() { return l_msgid; }
};
typedef boost::intrusive_ptr<chatr_impl> room_ptr;

struct chatr_user
{
    room_ptr room;
    UInt uid;

    chatr_user(UInt u, room_ptr r) {
        uid = u;
        room = r;
    }
};

typedef boost::multi_index_container<
        chatr_user,
        multi_index::indexed_by<
            multi_index::hashed_unique< multi_index::member<chatr_user, UInt const, &chatr_user::uid> >
          , multi_index::ordered_non_unique< multi_index::member<chatr_user, room_ptr const, &chatr_user::room> >
        //, multi_index::sequenced<>,
        >
    > chatr_user_mindex;

BOOST_STATIC_CONSTANT(char*, rk_chatroom_="chat/rooms");

struct chatroom_mgr : singleton<chatroom_mgr> , chatr_user_mindex
{
    boost::object_pool<chatr_impl> roompool_; 
    boost::unordered_map<UInt,room_ptr> rooms_;

    std::pair<room_ptr,bool> create_room(char const* csrid, UInt lmid, int recover)
    {
        UInt rid = atoi(csrid);
        if (rid > 0 /*&& rooms_.find(rid) == rooms_.end()*/) {
            room_ptr room( roompool_.construct(rid, lmid) );
            auto p = rooms_.emplace(rid, room);
            if (p.second) {
                if (!recover) {
                    redis::command("HSET", rk_chatroom_, rid, room->last_msgid());
                }
            } else {
                room->rid = 0;
                room = room_ptr();
            }
            return std::make_pair(room, p.second);
        }
        return std::make_pair(room_ptr(), false);
    }
    std::pair<room_ptr,bool> create_room(char const* csrid) { return create_room(csrid, 1, 0); }

    void destroy_room(char const* csrid)
    {
        UInt rid = atoi(csrid);
        if (rid > 0) {
            rooms_.erase(rid);
        }
    }

    room_ptr find_room(char const* csrid) const
    {
        UInt rid = atoi(csrid);
        if (rid > 0) {
            auto it = rooms_.find(rid);
            if (it != rooms_.end()) {
                return it->second;
            }
        }
        return room_ptr();
    }

    UInt new_message(UInt uid, room_ptr room, json::object const& m)
    {
        BOOST_ASSERT(room);
        if (uid > 1000) {
            auto& idx = boost::get<0>(*this);
            auto it = idx.find(uid);
            if (it == idx.end()) {
                LOG << uid << "not member of" << room->rid;
                return 0;
            }
        } else {
            LOG << uid;
        }
        UInt mid = message_mgr::alloc_msgid();
        room->l_msgid = mid;
        redis::command("HSET", rk_chatroom_, room->rid, mid);
        // this->relocate(end(), project<0>(it));
        return mid;
    }

    bool join_chat(room_ptr room, UInt uid)
    {
        auto p = get<0>().emplace(uid, room);
        if (!p.second) {
            if (p.first->room == room) {
                return true;
            }
            BOOST_ASSERT(p.first->uid == uid);
            // get<1>.modify_key(project<1>(p.first), );
            if (get<0>().modify(p.first, [&room](chatr_user& u){ u.room=room; })) {
                return true;
            }
        }
        return p.second;
    }

    void leave_chat(room_ptr room, UInt uid)
    {
        auto& idx = get<0>();
        auto it = idx.find(uid);
        if (it != idx.end()) {
            idx.erase(it);
        }
    }

    template <typename Us>
    void erase_users(Us const& uids)
    {
        auto& idx = boost::get<0>(*this);
        BOOST_FOREACH(UInt uid, uids) {
            idx.erase(uid);
        }
    }

    //typedef chatr_user_mindex::nth_index<1>::type::iterator iterator_R;
    //typedef chatr_user_mindex::index<TagRoom>::type::iterator iterator_R;
    //typedef multi_index::index<chatr_user_mindex,TagRoom>::type::iterator iterator_R;
    //boost::iterator_range<iterator_R> members(room_ptr room) const
    auto members(room_ptr room) const -> boost::iterator_range<nth_index<1>::type::iterator>
    {
        auto& ix = get<1>(); //boost::get<TagRoom>(*this);
        return ix.equal_range(room);
    }

   ~chatroom_mgr() { bquit_ = 1; this->clear(); }
    chatroom_mgr()
    {
        auto reply = redis::command("HGETALL", rk_chatroom_);
        if (!reply || reply->type != REDIS_REPLY_ARRAY) {
            return;
        }
        LOG << "rooms" << reply->elements;
        for (size_t i = 0; i < reply->elements; ) {
            char const* csrid = reply->element[i++]->str;
            char const* csmid = reply->element[i++]->str;
            this->create_room(csrid, atoi(csmid), 1);
        }
    }
};

inline void intrusive_ptr_add_ref(chatr_impl * m)
{
    m->refcount_++;
}
inline void intrusive_ptr_release(chatr_impl * m)
{
    if (--m->refcount_ == 0) {
        if (m->rid > 0 && !bquit_) {
            //LOG << "HDEL room" << m->rid << m->last_msgid();
            redis::command("HDEL", rk_chatroom_, m->rid);
        }
        chatroom_mgr::instance().roompool_.destroy(m);
    }
}

room_ptr find_room(chatroom_mgr& mgr, std::string const& sid);
extern const char* sid_sptr(std::string const& sid);
inline const char* chatroom_subsid(std::string const& sid) { return sid_sptr(sid); }


#endif // CHATROOM_H__

