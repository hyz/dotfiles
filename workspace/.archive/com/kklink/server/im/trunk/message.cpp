#include <sys/types.h>
#include <linux/unistd.h>
// #include <poll.h>
#define gettid() syscall(__NR_gettid)  
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/algorithm/string/join.hpp>

#include "dbc.h"
#include "async.h"
#include "message.h"
#include "push.h"
#include "statis.h"

namespace bmi = boost::multi_index;

static boost::object_pool<message> mpool_; 

struct xkey_expire_tp {
    typedef time_t result_type; 
    result_type operator()(message_ptr mp) const { return mp->expire_at(); }
};

struct xkey_id {
    typedef UInt result_type; 
    result_type operator()(message* m) const { return m->id; }
};

boost::multi_index_container<
    message*,
    bmi::indexed_by<
        // bmi::hashed_non_unique<BOOST_MULTI_INDEX_MEMBER(message,UInt,id)>
        bmi::hashed_non_unique<xkey_id>
        , bmi::ordered_non_unique<xkey_expire_tp>
    >
> msgl_;

struct msgrcpt_t : destination
{
    msgrcpt_t(destination const& dst, message_ptr mp)
        : destination(dst)
        , mptr(mp)
    {}

    message_ptr mptr;
};

//struct um_idp : std::pair<UInt,UInt>
//{
//    UInt uid() const { return first; }
//    UInt msgid() const { return second; }
//    um_idp(UInt uid, UInt mid) : std::pair<UInt,std::string>(uid,mid) {}
//};
//
//struct xkey_um {
//    typedef um_idp result_type; 
//    result_type operator()(msgrcpt_t const& r) const { return um_idp(r.uid, r.mptr->id); }
//};

struct xkey_msgid {
    typedef UInt result_type; 
    result_type operator()(msgrcpt_t const& r) const { return r.mptr->id; }
};
struct xkey_uid {
    typedef UInt result_type; 
    result_type operator()(msgrcpt_t const& r) const { return r.uid; }
};

boost::multi_index_container<
    msgrcpt_t,
    bmi::indexed_by<
        bmi::ordered_non_unique<xkey_uid>//<BOOST_MULTI_INDEX_MEMBER(msgrcpt_t,UInt,uid)>
        // , bmi::ordered_non_unique<xkey_um>
        // , bmi::hashed_non_unique<BOOST_MULTI_INDEX_MEMBER(msgrcpt_t,message_ptr,mptr)>
    >
> msgqueue_;

struct online_t
{
    UInt uid;
    time_t online;
    time_t offline;

    // UInt msg_count;

    explicit online_t(UInt u)
    {
        uid = u;
        online = time(0);
        offline = std::numeric_limits<time_t>::max();
        // msg_count = 0;
    }

    friend std::ostream& operator<<(std::ostream& out, online_t const& ol)
    {
        return out << ol.uid <<"/"<< ol.online <<"/"<< ol.offline;
    }
};

boost::multi_index_container<
    online_t,
    bmi::indexed_by<
        bmi::ordered_non_unique<BOOST_MULTI_INDEX_MEMBER(online_t,UInt,uid)>
        // , bmi::sequenced_index<>
        // , bmi::ordered_non_unique<xkey_um>
        // , bmi::hashed_non_unique<BOOST_MULTI_INDEX_MEMBER(msgrcpt_t,message_ptr,mptr)>
    >
> online_;
boost::multi_index_container<
    online_t,
    bmi::indexed_by<
        bmi::sequenced<>
        , bmi::ordered_non_unique<BOOST_MULTI_INDEX_MEMBER(online_t,UInt,uid)>
    >
> post_online_;
// boost::unordered_map<UInt, online_t > online_;
// std::deque<online_t> post_online_;

void intrusive_ptr_add_ref(message * m)
{
    m->reference_count_++;
    //    .relocate(idxs.end(), );
}

void intrusive_ptr_release(message * m)
{
    switch (--m->reference_count_)
    {
        case 0:
            LOG << "free" << *m;
            msgl_.erase(m->id);
            mpool_.destroy(m);
            break;
    }
}

// static void message_save(message_ptr mp, UInt fr_uid)
// {
//     redis::command("ZADD", "msg/expire", mp->expire_at(), mp->id)
//         ;
// 
//     boost::format fmt("INSERT INTO message(id,content,notification,uid) VALUES(%1%,'%2%','%3%',%4%)");
//     Sql_Exec(fmt % mp->id
//             % sql::escape(json::encode(mp->jdata))
//             % sql::escape(mp->notification)
//             % fr_uid
//         );
// }

// static UInt max_msgid = UInt(-1);

UInt public_message_save(UInt uid, const std::string& sid, const json::object& body)
{
    AUTO_CPU_TIMER("sql:public_message");
    UInt id = message_mgr::alloc_msgid();

    boost::format fmt("INSERT INTO public_message(id, uid, sid,content) VALUES (%1%,%2%,'%3%','%4%')");
    Sql_Exec(fmt % id % uid
            % sql::escape(sid)
            % sql::escape(json::encode(body)))
        ;
    // max_msgid = id;

    return id;
}

UInt public_message_get(UInt bid, json::array& msgs )
{
    LOG << bid;

    // if (bid >= max_msgid) { return bid; }

    UInt retid = bid;
    boost::format fmt("SELECT id,sid,content,UNIX_TIMESTAMP(ctime) from public_message where id>%1% ");
    sql::datas datas(fmt % bid);
    while (sql::datas::row_type row = datas.next()) {
        UInt id = boost::lexical_cast<UInt>(row[0]);
        auto *cont = row.at(2);
        json::object jo = json::decode<json::object>(cont).value();

        retid = std::max(retid,id); //retid < id ? id : retid;

        json::object obj;
        json::insert( obj )
                ("sid", std::string(row[1]))
                ("data", jo)
                ("ctime", boost::lexical_cast<time_t>(row[3]))
                ;
        msgs.push_back( obj );
    }

    return retid; // (max_msgid = retid);
}

//static message_ptr msgl_remake(UInt msgid
//        , time_t tp_exp, time_t tp_creat
//        , json::object const& body, std::string const& notification)
//{
//    message* m = mpool_.construct(msgid, tp_exp, tp_creat);
//    message_ptr mp( m );
//
//    auto p = msgl_.insert(m);
//    BOOST_ASSERT(p.second);
//
//    mp->jdata = body;
//    mp->notification = notification;
//
//    return mp;
//}

static message_ptr message_get(UInt msgid)
{
    // LOG << msgid;
    auto it = msgl_.find(msgid);
    if (it != msgl_.end()) {
        return message_ptr(*it);
    }

    time_t tpc = time(0);

    auto reply = redis::command("ZSCORE", "msg/expire", msgid);
    if (!reply || reply->type != REDIS_REPLY_STRING) {
        LOG << msgid << "expired";
        return message_ptr();
    }

    time_t tp_exp = boost::lexical_cast<time_t>(reply->str);
    if (tp_exp < tpc) {
        LOG << msgid << "expired" << tp_exp;
        return message_ptr();
    }

    message_ptr mp;

    boost::format fmt("SELECT UNIX_TIMESTAMP(ctime),content,notification FROM message WHERE id=%1%");
    sql::datas datas(fmt % msgid);
    if (sql::datas::row_type row = datas.next())
    {
        // std::string sid(row[0]);
        // time_t tp_exp = boost::lexical_cast<time_t>(row[1]);
        time_t tp_creat = boost::lexical_cast<time_t>(row[0]);
        auto cont = row.at(1);
        json::object jo = json::decode<json::object>(cont).value();
        std::string notification(row[2]);

        mp = message_mgr::construct(msgid, tp_exp, tp_creat, jo, notification); // msgl_remake
    }

    return mp;
}

static bool msgq_unpack(UInt& msgid, std::string& sid, std::string& fr_name, char const* hs)
{
    char const *end = hs + strlen(hs);
    char const *t1, *t2;

    t1 = std::find(hs, end, '\t');
    if (t1 == end) {
        return 0;
    }

    t2 = std::find(t1+1, end, '\t');
    if (t2 != end) {
        sid.assign(t1+1, t2);
        fr_name.assign(t2+1, end);
    } else {
        fr_name.assign(t1+1, end);
    }

    msgid = atoi(hs);
    return msgid > 0;
}

static msgrcpt_t const* usermsg_load(UInt uid)
{
    typedef boost::circular_buffer<UInt> cirbuffer_t;
    boost::unordered_map<std::string, cirbuffer_t> sidc_map;
    boost::multi_index_container<
        msgrcpt_t,
        bmi::indexed_by< bmi::ordered_unique<xkey_msgid> >
    > mq;

    char k_mq[64];
    snprintf(k_mq,sizeof(k_mq), "msg/q/%u", uid);

    auto reply = redis::command("LRANGE", k_mq, 0, -1);
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        return 0;
    }
    LOG << uid << reply->elements;

    for (size_t i = 0; i < reply->elements; i++)
    {
        UInt msgid; // = atoi(reply->element[i]->str);
        std::string sid;
        std::string fr_name;

        if (msgq_unpack(msgid, sid, fr_name, reply->element[i]->str))
        {
            if (message_ptr mp = message_get(msgid))
            {
                auto p = sidc_map.insert(std::make_pair(sid, cirbuffer_t()));
                auto & cirl = p.first->second;
                if (p.second) {
                    cirl.set_capacity( stype_limits(atoi(sid.c_str())).second );
                }
                if (cirl.full()) {
                    LOG << uid << "msgq full" << msgid;
                    UInt x = *cirl.begin();
    statis::instance().up_message_drop(uid, x);
                    mq.erase(x);
                }

                cirl.push_back(mp->id);
                mq.insert( mq.end(), msgrcpt_t(destination(uid, sid, fr_name), mp) );
            } else {
                LOG << uid << msgid << sid << "msg drop";
    statis::instance().up_message_drop(uid, msgid);
            }
        }
    }

    if (mq.empty()) {
        return 0;
    }

    {
        auto pos = msgqueue_.upper_bound(uid);
        BOOST_FOREACH(auto & rcpt, mq) {
            msgqueue_.insert(pos, rcpt);
        }
        // TODO BOOST_ASSERT(is_ordered(msgqueue_));
    }

    auto lowest = mq.begin();

    for (size_t i = 0; i < reply->elements; i++)
    {
        UInt msgid = atoi(reply->element[i]->str);
        if (msgid < lowest->mptr->id) {
            redis::command("LPOP", k_mq);
            //Sql_Exec(str( boost::format("UPDATE message_rx SET stat=2,time=NOW() WHERE uid=%1% AND msgid=%2%") % uid % msgid))
        }
    }

    destination const & dst = *lowest;
    emit(message_mgr::instance().ev_newmsg, dst, lowest->mptr);

    return &*lowest;
}

static void usermsg_delete(UInt uid, UInt msgid)
{
    char k_mq[64];
    snprintf(k_mq,sizeof(k_mq), "msg/q/%u", uid);

    while ( 1 )
    {
        auto reply = redis::command("LPOP", k_mq);
        if (!reply || reply->type != REDIS_REPLY_STRING) {
            return;
        }

        UInt mid = atoi(reply->str);
        if (mid <= msgid) {
            Sql_Exec(str(
                boost::format("UPDATE message_rx SET stat=1,time=NOW() WHERE uid=%1% AND msgid=%2%")
                    % uid % mid));
        } else {
            redis::command("LPUSH", k_mq, reply->str);
            LOG << "should not reach?";
        }

        if (mid >= msgid) {
            break;
        }
    }
}

//static void usermsg_save(destination const& dst, UInt msgid)
//{
//    char k[64];
//    snprintf(k,sizeof(k), "msg/q/%u", dst.uid);
//
//    redis::command("RPUSH", k, str(boost::format("%1%\t%2%\t%3%") % msgid % dst.sid % dst.fr_name));
//}

static void fn_offline(UInt uid, int aclose)
{
    auto it_ol = online_.find(uid);
    if (it_ol != online_.end())
    {
        auto & idx = boost::get<1>( post_online_ );
        auto it = idx.find(uid);
        if (it == idx.end()) { // (ustat_ref(uid, &mp->sid, &st))
            post_online_.push_back(*it_ol);
        }
    }
}

static void fev_online(message_mgr*self, UInt uid, int onl, int aclose)
{
    AUTO_CPU_TIMER("message_mgr:fev_online");
    if (!onl) {
        return fn_offline(uid, aclose);
    }

    auto it = online_.find(uid);
    if (it == online_.end()) // (ustat_ref(uid, &mp->sid, &st))
    {
        online_.insert(online_t(uid));

        if (auto mrp = usermsg_load(uid)) {
            destination const & dst = *mrp;
            emit(self->ev_newmsg, dst, mrp->mptr);
        }
    }
    else
    {
        auto & idx = boost::get<1>( post_online_ );
        idx.erase(uid);
    }
}

namespace placeholders = boost::asio::placeholders;

void message_queue::tl_watch(boost::system::error_code ec)
{
    LOG << "tid"<< gettid() << "message queue" << mq_.empty();
    if (ec) {
        LOG << ec << ec.message();
        return;
    }
    //expires_at(boost::posix_time::pos_infin);

    deadline_.expires_from_now(boost::posix_time::minutes(3) + boost::posix_time::seconds(30));
    deadline_.async_wait(boost::bind(&message_queue::tl_watch, this, placeholders::error));
}

message_queue::~message_queue()
{
    deadline_.cancel(); // io_s_.post(boost::bind(&boost::asio::io_service::stop, &io_s_)) ;
    thread_.join();

    if (!mq_.empty()) {
        _save(mq_);
    }
    LOG << "message_queue destructor";
}

message_queue::message_queue()
    : io_s_()
    , deadline_(io_s_)
{
    boost::system::error_code ec;
    io_s_.post(boost::bind(&message_queue::tl_watch, this, ec));
    thread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &io_s_)) ;
}

void message_queue::lock_post(message_data && md)
{
    boost::unique_lock<boost::mutex> lock(mq_mutex_);
    auto empty = mq_.empty();

    mq_.push_back( boost::move(md) );

    if (empty) {
        io_s_.post(boost::bind(&self_t::thread_save, this));
    } else {
        LOG << mq_.size() << "tid" << gettid();
    }
}

void message_queue::thread_save()
{
    AUTO_CPU_TIMER("message_mgr:thread_save");

    try
    {
        std::vector<message_data> cpy; // = boost::move(mq_);
        {
            boost::unique_lock<boost::mutex> lock(mq_mutex_);
            if (mq_.empty()) {
                return;
            }
            cpy.swap(mq_);
        }
        LOG << cpy.size() <<"tid"<< gettid();

        _save(cpy);

        auto thiz = static_cast<message_mgr*>(this); { message_queue* q=thiz; q=q; }
        thiz->thread_saved( boost::move(cpy) );

    } catch(...) {
        LOG << "save fail";
    }
}

void message_queue::_save(std::vector<message_data>& cpy)
{
    if (cpy.empty()) {
        return;
    }

    { // sql, message
        // AUTO_CPU_TIMER("sql:message");
        std::vector<std::string> vals;
        boost::format fmt("(%1%,%2%,'%3%','%4%')");
        BOOST_FOREACH(auto& m, cpy) {
            vals.push_back(str(fmt % m.msgid % m.fr_uid
                        % sql::escape(json::encode(m.content)) % sql::escape(m.notification)));
        }
        if (!vals.empty()) {
            Sql_Exec("INSERT INTO message(id,uid,content,notification) VALUES " + boost::algorithm::join(vals,","));
        }
    }
    { // sql, message_rx
        // AUTO_CPU_TIMER("sql:message_rx");
        std::vector<std::string> vals;
        boost::format fmt("(%1%,%2%,%3%,'%4%','%5%')");
        BOOST_FOREACH(auto& m, cpy) {
            BOOST_FOREACH(auto& dst, m.dests) {
                vals.push_back(str(fmt % dst.uid % m.msgid % m.fr_uid % sql::escape(dst.fr_name) % sql::escape(dst.sid)));
            }
        }
        if (!vals.empty()) {
            Sql_Exec("INSERT INTO message_rx(uid,msgid,from_uid,from_name,sid) VALUES " + boost::algorithm::join(vals,","));
        }
    }
    //{ // sql, message_stat
    //    AUTO_CPU_TIMER("sql:message_stat");
    //    std::vector<std::string> vals;
    //    boost::format fmt("(%1%,%2%)");
    //    BOOST_FOREACH(auto& m, cpy) {
    //        BOOST_FOREACH(auto& dst, m.dests) {
    //            vals.push_back(str(fmt % dst.uid % m.msgid));
    //        }
    //    }
    //    if (!vals.empty()) {
    //        Sql_Exec("INSERT INTO message_stat(uid,msgid) VALUES " + boost::algorithm::join(vals,","));
    //    }
    //}
    { // redis
        // AUTO_CPU_TIMER("redis:msg/q _save");
        redis::context redx; //( redis::context::make() );
        int n = 0;
        BOOST_FOREACH(auto& m, cpy) {
            BOOST_FOREACH(auto& dst, m.dests) {
                char k[64];
                snprintf(k,sizeof(k), "msg/q/%u", dst.uid);
                redx.append("RPUSH", k, str(boost::format("%1%\t%2%\t%3%") % m.msgid % dst.sid % dst.fr_name));
                ++n;
            }
            redx.append("ZADD", "msg/expire", (m.tp_create+m.seconds), m.msgid);
        }
        if (n > 16) {
            LOG << "long dests" << n;
        }
    }
}

void message_mgr::on_saved(std::vector<message_data> cpy)
{
    // AUTO_CPU_TIMER("message_mgr:on_saved");
    boost::unordered_map<UInt, int> um_c;
    std::pair<UInt, int> max_c(0,0);

    BOOST_FOREACH(auto& md, cpy)
    {
        message_ptr mp = construct(md);

        std::vector<UInt> uids;
        uids.reserve(md.dests.size());

        BOOST_FOREACH(auto& dst, md.dests)
        {
            this->_post(dst, mp);

            uids.push_back(dst.uid);

            auto i = um_c.emplace(dst.uid, 0).first;
            if (++i->second > max_c.second) {
                max_c = *i;
            }
        }

        statis::instance().up_message_send(md.fr_uid, md.msgid, uids);
    }
    if (max_c.second > 1) {
        LOG << max_c.first << "n" << max_c.second;
    }
}

message_ptr message_mgr::construct(UInt msgid, time_t tp_exp, time_t tp_creat, json::object const& jo, std::string const& noti)
{
    message* rp = mpool_.construct(msgid, tp_exp, tp_creat);
    message_ptr mp(rp);

    auto suc = msgl_.insert(rp).second;
    BOOST_ASSERT(suc);

    mp->jdata = jo;
    mp->notification = noti;
    return mp;
}

message_mgr::message_mgr(boost::asio::io_service & io_s)
    : message_queue()
    , io_service_(io_s)
{
    user_mgr::instance().ev_online.connect(boost::bind(&fev_online, this, _1, _2, _3));
}

message_mgr::~message_mgr()
{
    // disconnect
}

void message_mgr::check_post_online()
{
    AUTO_CPU_TIMER("message_mgr:check_post_online");
    int n = 0;
    time_t tpc = time(0);
    for (;;)
    {
        auto it = post_online_.begin();
        if (it->offline + (60*3) < tpc) {
            break;
        }
        LOG << *it;
        ++n;

        UInt uid = it->uid;

        online_.erase(uid);
        size_t n = msgqueue_.erase(uid);
        if (n > 0) {
            ;
        } else {
            ;
        }

        post_online_.pop_front();
    }
    LOG << tpc << n;
}

void message_mgr::check_expire()
{
    AUTO_CPU_TIMER("message_mgr:check_expire");
    int nxd = 0;
    auto & idx = boost::get<1>( msgl_ );
    time_t tpc = time(0);
    for (;;)
    {
        auto it = idx.begin();
        if (it == idx.end()) {
            break;
        }

        auto * m = *it;

        if (m->expire_at() > tpc) {
            break;
        }

        ++nxd;
        LOG << *m << "expired";

        redis::command("ZREM", "msg/expire", m->id);
        // msg/q/%uid% // TODO ...

        idx.erase(it);
    }

    if (nxd > 0) {
        LOG << tpc << nxd << "mmgr";
    }
}

UInt message_mgr::alloc_msgid()
{
    auto reply = redis::command("INCR", "msg/id");
    if (!reply || reply->type != REDIS_REPLY_INTEGER) {
        LOG << "should not reach";
        return 0x001; // TODO throw
    }
    return reply->integer;
}

void message_mgr::_post(destination const& dst, message_ptr mp)
{
    // statis::instance().up_message_add(dst.uid, mp->id);
    // usermsg_save(dst, mp->id); // TODO: cleanup

    if (!mp->notification.empty()) {
        push_mgr::instance().notice(dst.uid, mp->id
                , mp->expire_at()
                , dst.fr_name + (dst.fr_name.empty() ? "" : ":") + mp->notification);
    }

    auto it_ol = online_.find(dst.uid);
    if (it_ol != online_.end())
    {
        auto pos = msgqueue_.upper_bound(dst.uid);
        msgqueue_.insert(pos, msgrcpt_t(dst, mp));

        // it_ol->msg_count++;
        {
            emit(ev_newmsg, dst, mp);
        }

    }
}

std::pair<destination,message_ptr> message_mgr::front(UInt uid)
{
    // AUTO_CPU_TIMER("message_mgr:front");
    std::pair<destination,message_ptr> ret;

    auto it = msgqueue_.lower_bound(uid);
    if (it == msgqueue_.end() || it->uid != uid) {
        return ret;
    }
    ret.second = it->mptr;
    ret.first = destination(*it);
    return ret;
}

std::pair<destination,message_ptr> message_mgr::completed(UInt uid, UInt msgid)
{
    // AUTO_CPU_TIMER("message_mgr:completed");
    LOG << uid << msgid;
    std::pair<destination,message_ptr> ret;

    auto it_ol = online_.find(uid);
    if (it_ol == online_.end()) {
        LOG << uid << msgid;
        return ret;
    }

    auto it = msgqueue_.lower_bound(uid);
    auto end = msgqueue_.end();
    auto it0 = it;
    if (it == end) {
        return ret;
    }

    for (; it != end && it->uid == uid && it->mptr->id <= msgid; ++it) {
        usermsg_delete(uid, it->mptr->id);
        // redis::command("HSET", "msg/last", sk.idx_, msgid);
    }

    if (it0 != it) {
        msgqueue_.erase(it0, it);
    }

    if (it == end || it->uid != uid) {
        return ret;
    }
    ret.second = it->mptr;
    ret.first = destination(*it);
    return ret;
}

std::pair<int,int> stype_limits(int stype)
{
    enum { Seconds_PDay=(60*60*24) };

    int expires = 60*60*8;
    int count = 99;

    switch (stype)
    {
        case SType_syscommand:
        case SType_sysnotification:
        case SType_p2pchat:
            expires = Seconds_PDay * 7;
            break;
        case SType_imgroup:
        case SType_groupchat:
            expires = Seconds_PDay * 3;
            break;
        case SType_chatroom:
            expires = 60*60;
            break;
        case SType_official:
            expires = Seconds_PDay * 3;
            break;
        default:
            break;
    }

    switch (stype)
    {
        case SType_syscommand:
        case SType_sysnotification:
        case SType_p2pchat:
        case SType_imgroup:
        case SType_groupchat:
            count = 99;
            break;
        case SType_chatroom:
            count = 20;
            break;
        case SType_official:
            count = 200;
            break;
        default:
            break;
    }

    return std::make_pair(expires, count);
}

json::object message::json(destination const& dst) const
{
    json::object js, body;
    json::insert(body)
        ("msgid", this->id)
        ("sid", dst.sid)
        ("time", this->ctime)
        ("data", this->jdata)
        // ("notification", msg.notification)
        ;
    json::insert(js)
        ("cmd", 200) // ("error", 0)
        ("body", body)
        ;
    return js; //json::encode(jmsg);
}

message::message(UInt mid, time_t tp_exp, time_t tp_creat)
    : reference_count_(0)
{
    LOG << mid << "expire@" << tp_exp;

    ctime = tp_creat;
    tp_expire = tp_exp;
    id=mid;

    flags.ack_required=1; flags.ack_required; // TODO
}

////////////////// mongo //////////////////
#if 0
const char* pub_msg_key= "moonv2.public_messages";
UInt public_message_save2(UInt uid, const std::string& sid, const json::object& body)
{
    UInt id = alloc_msgid();

    mongo::BSONObj p = BSON( "id" << id << "from_uid"<<uid 
            << "sid"<<sid << "content" << json::encode( body ) << "when" << (int)time(NULL) );

    LOG_I<< p.toString();

    MyMongo::mongo_insert( pub_msg_key, p );

    return id;
}

UInt public_message_get2(UInt bid, json::array& msgs )
{
    LOG << bid;
    UInt retid = bid;
    boost::format fmt("{\"id\":{$gt:%1%}}");

    auto cursor = MyMongo::mongo_select( pub_msg_key, (fmt %bid).str() );

    while ( cursor->more() ) {
        mongo::BSONObj bobj = cursor->next();
        int id = bobj.getIntField("id");
        if ( id > retid ) { retid = id; }
        std::string sid = bobj.getStringField("sid");
        std::string cont = bobj.getStringField("content");
        time_t ct = bobj.getIntField("when");

        json::object obj;
        json::insert( obj )
                ("sid", sid)
                ("data", json::decode<json::object>(cont).value())
                ("ctime", ct)
                ;
        msgs.push_back( obj );
    }

    return retid;
}

const char* msg_key= "moonv2.messages";
static void message_save2(message_ptr mp, UInt fr_uid)
{
    redis::command("ZADD", "msg/expire", mp->expire_at(), mp->id);

    mongo::BSONObj p = BSON( "id" << mp->id << "sid"<< mp->sid << "content" << json::encode( mp->jdata ) 
            << "notification" << mp->notification << "when" << (int)time(NULL) );

    LOG_I<< p.toString();

    MyMongo::mongo_insert( msg_key, p );
}

static message_ptr message_get2(UInt msgid)
{
    LOG << msgid;

    auto it = msgl_.find(msgid);
    if (it != msgl_.end())
    {
        return message_ptr(*it);
    }

    time_t tpc = time(0);

    auto reply = redis::command("ZSCORE", "msg/expire", msgid);
    if (!reply || reply->type != REDIS_REPLY_STRING)
    {
        LOG << msgid << "expired";
        return message_ptr();
    }

    time_t tp = boost::lexical_cast<time_t>(reply->str);
    if (tp > tpc)
    {
        LOG << msgid << "expired" << tp;
        return message_ptr();
    }

    message_ptr mp;


    boost::format fmt("{\"id\":%1%}");
    auto cursor = MyMongo::mongo_select( msg_key, (fmt %msgid).str() );

    while( cursor->more() ) {
        mongo::BSONObj obj = cursor->next();
        std::string sid = obj.getStringField("sid");
        time_t ct = obj.getIntField("when");
        std::string notification = obj.getStringField("notification");
        std::string cont = obj.getStringField("content");
        json::object jo = json::decode<json::object>(cont).value();
        mp = msgl_remake(msgid, sid, ct, jo, notification);
    }

    return mp;
}
#endif // mongo

