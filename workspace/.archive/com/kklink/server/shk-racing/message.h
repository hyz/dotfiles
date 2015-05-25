#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/thread/thread.hpp>
#include "util.h"
#include "user.h"
#include "json.h"

typedef unsigned int UInt;
enum { ACCEPT_MSGS=200 };
enum { DevType_iPhone=1, DevType_Android=3 };
enum {
    SType_syscommand=11,
    SType_sysnotification=12,
    SType_p2pchat=21,
    SType_groupchat=22,
    SType_chatroom=23,
    SType_official=31
};

std::pair<int,int> stype_limits(int stype); //return std::make_pair(expires, count);

struct destination
{
    UInt uid;
    std::string sid;
    std::string fr_name;

    destination(UInt u, std::string const& s, std::string const& fr) : sid(s), fr_name(fr) { uid=u; }
    destination() : uid(0) {}
    operator bool() const { return uid>0; }

    friend std::ostream & operator<<(std::ostream& out, destination const& x)
    {
        return out<<x.uid<<"/"<<x.sid<<"/"<<x.fr_name;
    }
};

struct message
{
    UInt reference_count_;

    // UInt fr_uid;
    UInt id;
    time_t ctime;
    time_t tp_expire;

    unsigned char _stype;
    unsigned char _[3];

    struct _Flags {
        bool ack_required;
        bool _[3];
        _Flags() { memset(this, 0, sizeof(_Flags)); }
    } flags;

    message(UInt mid, time_t tp_exp, time_t tp_creat);

    time_t expire_at() const { return tp_expire; }//{ return ctime + stype_limits(stype).first; }

    json::object jdata;
    std::string notification;

    json::object json(destination const& dst) const;

    friend std::ostream & operator<<(std::ostream& out, message const& m)
    {
        return out << m.id; // <<"/"<< m.sid;
    }
};


void intrusive_ptr_add_ref(message * m);
void intrusive_ptr_release(message * m);

typedef boost::intrusive_ptr<message> message_ptr;

inline int avail_seconds(int stype)
{
    return stype_limits(stype).first;
}
// inline int avail_seconds(std::string const& sid) { return avail_seconds(atoi(sid.c_str())); }

struct message_data
{
    UInt msgid;
    UInt fr_uid;
    time_t tp_create;
    UInt seconds;
    json::object content;
    std::string notification;
    std::vector<destination> dests;

    template <typename Dsts>
    message_data(Dsts& dsts, UInt mid, UInt fr
            , time_t tp_creat, UInt secs, json::object const& cont, std::string const& noti)
        : content(cont)
        , notification(noti)
        , dests(dsts.begin(), dsts.end())
    {
        fr_uid = fr;
        msgid = mid;
        tp_create = tp_creat;
        seconds = secs;
    }
};

struct message_queue : boost::noncopyable
{
    typedef message_queue self_t;
protected:
    ~message_queue();
    message_queue(); // (boost::asio::io_service & io_s);

    void lock_post(message_data && md);

private:
    void thread_save();
    void _save(std::vector<message_data>& cpy);

    void tl_watch(boost::system::error_code ec);

    std::vector<message_data> mq_;
    boost::mutex mq_mutex_;

    boost::asio::io_service io_s_;
    boost::asio::deadline_timer deadline_;
    boost::thread thread_;
};

struct message_mgr : singleton<message_mgr>, message_queue
{
    boost::signals2::signal<void (destination,message_ptr)> ev_newmsg;

    template <typename Dsts>
    UInt post(Dsts& dsts, UInt fr_uid, int stype, json::object const& data, std::string const& noti);

    static message_ptr construct(UInt msgid, time_t tp_exp, time_t tp_creat, json::object const& jo, std::string const& noti);
    static message_ptr construct(message_data const& md) //(UInt fr_uid, int seconds, json::object const& body, std::string const& notification)
    {
        return construct(md.msgid, md.tp_create+md.seconds, md.tp_create, md.content, md.notification);
    }

    static UInt alloc_msgid();
public:
    std::pair<destination,message_ptr> front(UInt uid);
    std::pair<destination,message_ptr> completed(UInt uid, UInt msgid);

private:
    void _post(destination const& dst, message_ptr mp);

    friend message_queue;

    void thread_saved(std::vector<message_data>&& cpy)
    {
        io_service_.post(boost::bind(&message_mgr::on_saved, this, boost::move(cpy)));
    }

    void on_saved(std::vector<message_data> cpy);

    boost::asio::io_service & io_service_;

public:
    void check_post_online();
    void check_expire();

    message_mgr(boost::asio::io_service & io_s);
    ~message_mgr();
};

template <typename Dsts>
UInt message_mgr::post(Dsts& dsts, UInt fr_uid, int stype, json::object const& data, std::string const& noti)
{
    AUTO_CPU_TIMER("message_mgr:post");
    {
        auto n = boost::size(dsts);
        if (n > 10) {
            LOG << fr_uid << n << stype <<"long rcpts";
        }
    }

    UInt msgid = alloc_msgid();
    time_t tp_creat = time(0);
    UInt seconds = avail_seconds(stype);

    message_queue::lock_post( message_data(dsts, msgid, fr_uid, tp_creat, seconds, data, noti) );

    return msgid;
}

UInt public_message_save(UInt uid, const std::string& sid, const json::object& body);
UInt public_message_get(UInt bid, json::array& msgs );

//UInt public_message_save2(UInt uid, const std::string& sid, const json::object& body);
//UInt public_message_get2(UInt bid, json::array& msgs );

#endif

