#ifndef DYNAMIC_STATE_H__
#define DYNAMIC_STATE_H__

#include <time.h>
#include <string>
#include <vector>
#include <set>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/multi_index_container.hpp>
// #include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include "user.h"
#include "dbc.h"

typedef unsigned int UInt;

struct dynamic_message // : private primary_id_allocator<dynamic_message>
{
    UInt id; //, dynid;
    UID userid;
    std::string barsid;
    int devtype;
    time_t xtime;

    std::string words;
    std::vector<std::string> imgs;

    explicit dynamic_message(UID uid)
    {
        userid = uid;
        if (uid) {
            xtime = ::time(0);
            id = alloc_id();
        }
    }

    bool is_like() const { return words.empty() && imgs.empty(); }

public:
    static const char *db_table_name() { return "dynamic_message"; }
    static int alloc_id() { return max_index<dynamic_message>(+1); }

    template <typename OutIter> static void Load(std::string const & cond, OutIter iter)
    {
        boost::format fmt("SELECT"
                " id,DynamicId,UserId,UNIX_TIMESTAMP(xtime),BarId,DevType,words,imgs"
                " FROM dynamic_message WHERE %1%");
        sql::datas datas(fmt % cond);
        while (sql::datas::row_type row = datas.next())
        {
            dynamic_message m;
            m.id = boost::lexical_cast<UInt>(row.at(0));
            // m.dynid = boost::lexical_cast<UID>(row.at(1));
            m.userid = boost::lexical_cast<UID>(row.at(2));
            m.xtime = boost::lexical_cast<time_t>(row.at(3));
            m.barsid = row.at(4,"");
            m.devtype = boost::lexical_cast<int>(row.at(5));
            m.words = row.at(6,"");
            if (row.at(7,NULL)!=NULL)
            {
                std::istringstream iss(row.at(7));
                std::istream_iterator<std::string> i(iss), end;
                std::copy(i, end, std::back_inserter(m.imgs));
            }
            *iter++ = m;
        }
    }

    void Save(UInt dynid) const
    {
        boost::format fmt("INSERT INTO dynamic_message"
                " (id,DynamicId,UserId,BarId,DevType,words,imgs)"
                " VALUES(%1%,%2%,%3%,'%4%',%5%,'%6%',%7%)");
        std::string simgs = "NULL";
        if (!imgs.empty())
        {
            std::ostringstream oss;
            std::copy(imgs.begin(), imgs.end(), std::ostream_iterator<std::string>(oss, " "));
            simgs = "'" + sql::db_escape(oss.str()) + "'";
        }
        sql::exec(fmt % id % dynid % userid % sql::db_escape(barsid) % devtype % sql::db_escape(words) % simgs);
    }

    static void Delete(UInt did)
    {
        boost::format fmt("DELETE FROM dynamic_message WHERE id=%1%");
        sql::exec(fmt % did);
    }

private:
    dynamic_message()
    {}
};
template <typename OutS>
OutS & operator<<(OutS & out, dynamic_message const & m)
{
    return out << m.id <<"/"<< m.userid <<"/"<< m.barsid;
}

struct TagDynPtr {};
struct TagId {};
struct TagUsrId {};
struct TagRcpt {};
struct TagBar {};

struct dynamic_state
    : dynamic_message, std::vector<dynamic_message>
{
    std::set<UID> supports;
    UInt latest_cid_, n_support_;

    explicit dynamic_state(dynamic_message const & m)
        : dynamic_message(m)
    {
        latest_cid_ = m.id;
        n_support_ = 0;
    }

    bool add_comment(dynamic_message const & m);

    UInt n_support() const { return n_support_; }
    UInt n_comment() const { return (UInt)size(); }
    // UInt max_id() const { return latest_cid_; }

    iterator find(UInt cid) const { return const_cast<dynamic_state*>(this)->find(cid); }
    iterator find(UInt cid)
    {
        auto i = begin();
        for (; i != end(); ++i)
            if (i->id == cid)
                break;
        return i;
    }

    void delete_comment(UInt cid)
    {
        auto i = find(cid);
        if (i != end())
            erase(i);
    }

    iterator latest_comment() const { return find(latest_cid_); }
};
template <typename OutS>
OutS & operator<<(OutS & out, dynamic_state const & ds)
{
    dynamic_message const & m = ds;
    out << m << " dynamic-state\n";
    BOOST_FOREACH(auto const & x, ds)
        out << x << "\n";
    return out;
}

typedef boost::shared_ptr<dynamic_state> dynamic_state_ptr;

typedef boost::multi_index_container<
    dynamic_state_ptr,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<TagId>, BOOST_MULTI_INDEX_MEMBER(dynamic_message,UInt,id)
        >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagUsrId>, BOOST_MULTI_INDEX_MEMBER(dynamic_message,UID,userid)
        >
    >
> dynamic_state_index_type;

struct dynamic_newrsp_t
{
    dynamic_state_ptr dyn;
    dynamic_message rsp;

    dynamic_newrsp_t(dynamic_state_ptr dptr, dynamic_message const & m) : dyn(dptr), rsp(m) {}

    bool operator<(dynamic_newrsp_t const & rhs) const
    {
        return dyn->userid<rhs.dyn->userid || (dyn->userid==rhs.dyn->userid && dyn->id<rhs.dyn->id);
    }

    struct EqUsrId {
        bool operator()(dynamic_newrsp_t const &item, UID userid) const { return item.dyn->userid<userid; }
        bool operator()(UID userid, dynamic_newrsp_t const &item) const { return userid<item.dyn->userid; }
    };
};

struct IdExtractor
{
  typedef UInt result_type;
  result_type operator()(const dynamic_newrsp_t& e)const{return e.rsp.id;}
  // result_type operator()(dynamic_newrsp_t* e)const{return e->rsp.id;}
};

typedef boost::multi_index_container<
    dynamic_newrsp_t,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagUsrId>, boost::multi_index::identity<dynamic_newrsp_t>
        >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagDynPtr>, BOOST_MULTI_INDEX_MEMBER(dynamic_newrsp_t,dynamic_state_ptr,dyn)
        >,
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<TagId>, IdExtractor
        >
    >
> dynamic_newrsp_index_t;

struct dynamic_rcpt_t
{
    dynamic_state_ptr dyn;
    UID rcpt;

    bool operator<(dynamic_rcpt_t const & rhs) const
    {
        return rcpt<rhs.rcpt || (rcpt==rhs.rcpt && dyn->id<rhs.dyn->id);
    }

    struct EqCmp {
        bool operator()(dynamic_rcpt_t const &d, UID rcpt) const { return d.rcpt<rcpt; }
        bool operator()(UID rcpt, dynamic_rcpt_t const &d) const { return rcpt<d.rcpt; }
    };
};

typedef boost::multi_index_container<
    dynamic_rcpt_t,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagDynPtr>, BOOST_MULTI_INDEX_MEMBER(dynamic_rcpt_t,dynamic_state_ptr,dyn)
        >,
        boost::multi_index::ordered_non_unique<
            // boost::multi_index::tag<TagRcpt>, BOOST_MULTI_INDEX_MEMBER(dynamic_rcpt_t,UID,rcpt)
            boost::multi_index::tag<TagRcpt>, boost::multi_index::identity<dynamic_rcpt_t>
        >
    >
> dynamic_rcpt_index_t;

struct dynamic_bar_t
{
    dynamic_state_ptr dyn;
    std::string barsid;

    bool operator<(dynamic_bar_t const & rhs) const
    {
        return barsid<rhs.barsid || (barsid==rhs.barsid && dyn->id<rhs.dyn->id);
    }
    struct EqCmp {
        bool operator()(dynamic_bar_t const &d, std::string const & b) const { return d.barsid<b; }
        bool operator()(std::string const & b, dynamic_bar_t const &d) const { return b<d.barsid; }
    };
};

typedef boost::multi_index_container<
    dynamic_bar_t,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagDynPtr>, BOOST_MULTI_INDEX_MEMBER(dynamic_bar_t,dynamic_state_ptr,dyn)
        >,
        boost::multi_index::ordered_non_unique<
        //    boost::multi_index::tag<TagBar>, BOOST_MULTI_INDEX_MEMBER(dynamic_bar_t,std::string,barsid)
            boost::multi_index::tag<TagBar>, boost::multi_index::identity<dynamic_bar_t>
        >
    >
> dynamic_bar_index_t;

struct dynamic_userstat
{
    UInt mylast_moment_id;

    struct unread_t
    {
        UInt count;
        UID last_userid;
        void clear() { last_userid=0; count=0; }
    } unread;

    UInt last_dynshare_id;
    UInt last_dynshare_userid;
    UInt synced_dynshare_id;

    dynamic_userstat()
    {
        memset(this, 0, sizeof(*this));
    }

    bool load_self, load_friends;

public:
    void trace_last_sync(UID uid, dynamic_rcpt_t const & rcpt)
    {
        last_dynshare_id = synced_dynshare_id = rcpt.dyn->id;
        redis::command("HSET", "moments/sync/last", uid, rcpt.dyn->id);
    }

    void load_trace_last_sync(UID uid)
    {
        auto reply = redis::command("HGET", "moments/sync/last", uid);
        if (reply)
        {
            if (reply->type == REDIS_REPLY_STRING)
                synced_dynshare_id = boost::lexical_cast<UInt>(reply->str);
        }
    }
};

struct dynamic_datas
{
    dynamic_state_index_type users;
    dynamic_newrsp_index_t newrsps;

    dynamic_rcpt_index_t rcpts;
    dynamic_bar_index_t bars;

    std::map<UID,dynamic_userstat> stat;

    static dynamic_datas & inst(UID uid=0);

private:
    static dynamic_datas & instb(std::string const & barsid);
    static void init_dynamic_user(dynamic_datas & dyns, dynamic_userstat& stat, UID uid, int recur);
};

inline dynamic_userstat & get_dynamic_stat(UID uid)
{
    auto & dyns = dynamic_datas::inst(uid);
    return dyns.stat.insert( std::make_pair(uid,dynamic_userstat()) ).first->second;
}

inline dynamic_state_ptr get_dynamic(UInt dynid, UID uid)
{
    auto & dyns = dynamic_datas::inst(uid);
    auto & idxs = boost::get<TagId>(dyns.users);
    auto i = idxs.find(dynid);
    if (i == idxs.end())
        return dynamic_state_ptr();
    return *i;
}

dynamic_state_ptr new_dynamic(dynamic_message const & m);
dynamic_state_ptr new_dynamic_comment(UInt id, dynamic_message const & m);
dynamic_state_ptr like_dynamic(UInt dynid, UID userid);

void delete_dynamic(UID userid, UInt did);
void delete_dynamic_comment(UID userid, UInt did, UInt cid);

template <typename T>
T dyn_filter_range(T const & rng, UInt offid, int count)
{
    if (offid == 0)
    {
        auto i = boost::rbegin(rng);
        while (count-- > 0 && i != boost::rend(rng))
            ++i;
        return std::make_pair(i.base(), boost::end(rng));
    }

    auto end = boost::begin(rng);

    for (; end != boost::end(rng); ++end)
        if (end->dyn->id >= offid)
            break;

    auto beg = end;
    while (count-- > 0 && beg != boost::begin(rng))
        --beg;
    return std::make_pair(beg, end);
}
template <typename T>
T dyn_filter_range_2(T const & rng, UInt offid, int count)
{
    if (offid == 0)
    {
        auto i = boost::rbegin(rng);
        while (count-- > 0 && i != boost::rend(rng))
            ++i;
        return std::make_pair(i.base(), boost::end(rng));
    }

    auto end = boost::begin(rng);
    
    for (; end != boost::end(rng); ++end)
        if (end->id >= offid)
            break;

    auto beg = end;
    while (count-- > 0 && beg != boost::begin(rng))
        --beg;
    return std::make_pair(beg, end);
}
template <typename T>
T dyn_filter_range_3(T const & rng, UInt offid, int count)
{
    if (offid == 0)
    {
        auto i = boost::rbegin(rng);
        while (count-- > 0 && i != boost::rend(rng))
            ++i;
        return std::make_pair(i.base(), boost::end(rng));
    }

    auto end = boost::begin(rng);

    for (; end != boost::end(rng); ++end)
        if ((*end)->id >= offid)
            break;

    auto beg = end;
    while (count-- > 0 && beg != boost::begin(rng))
        --beg;
    return std::make_pair(beg, end);
}
template <typename T>
T dyn_filter_range_4(T const & rng, UInt offid, int count)
{
    if (offid == 0)
    {
        auto i = boost::rbegin(rng);
        while (count-- > 0 && i != boost::rend(rng))
            ++i;
        return std::make_pair(i.base(), boost::end(rng));
    }

    auto end = boost::begin(rng);

    for (; end != boost::end(rng); ++end)
        //if (end->rsp.id >= offid)
            break;

    auto beg = end;
    while (count-- > 0 && beg != boost::begin(rng))
        --beg;
    return std::make_pair(beg, end);
}

//typedef boost::multi_index::index<dynamic_rcpt_index_t, TagRcpt>::type rcpt_index_t;
//inline std::pair<rcpt_index_t::iterator,rcpt_index_t::iterator>
//list_rcpt_dynamic(UID rcpt, UInt offid, int count)
//{
//    auto & dyns = dynamic_datas::inst(rcpt);
//    auto & idxs = boost::get<TagRcpt>(dyns.rcpts);
//    return idxs.equal_range(rcpt);
//}
typedef boost::multi_index::index<dynamic_rcpt_index_t, TagRcpt>::type rcpt_index_t;
inline std::pair<rcpt_index_t::iterator,rcpt_index_t::iterator>
list_rcpt_dynamic(UID rcpt, UInt offid, int count)
{
    auto & dyns = dynamic_datas::inst(rcpt);
    auto & idxs = boost::get<TagRcpt>(dyns.rcpts);
    // return idxs.equal_range(rcpt, dynamic_rcpt_t::EqCmp());
    auto rng = idxs.equal_range(rcpt, dynamic_rcpt_t::EqCmp());
    return dyn_filter_range(rng, offid, count);
}

//typedef boost::multi_index::index<dynamic_bar_index_t, TagBar>::type bar_index_t;
//inline std::pair<bar_index_t::iterator,bar_index_t::iterator>
//list_bar_dynamic(std::string const & bar, UInt offid, int count)
//{
//    auto & dyns = dynamic_datas::instb(bar);
//    auto & idxs = boost::get<TagBar>(dyns.bars);
//    // return idxs.equal_range(bar, dynamic_bar_t::EqCmp());
//    auto rng = idxs.equal_range(bar, dynamic_bar_t::EqCmp());
//    return dyn_filter_range(rng, offid, count);
//}

typedef boost::multi_index::index<dynamic_state_index_type, TagUsrId>::type user_index_t;
inline std::pair<user_index_t::iterator,user_index_t::iterator>
list_user_dynamic(UID userid, UInt offid, int count)
{
    auto & dyns = dynamic_datas::inst(userid);
    auto & idxs = boost::get<TagUsrId>(dyns.users);
    auto rng = idxs.equal_range(userid);
    return dyn_filter_range_3(rng, offid, count);
}

typedef dynamic_state::iterator comment_iterator;
inline std::pair<comment_iterator,comment_iterator>
list_dyncomments(dynamic_state_ptr dyn, UInt offid, int count)
{
    auto rng = std::make_pair(dyn->begin(),dyn->end());
    return dyn_filter_range_2(rng, offid, count);
}

//typedef boost::multi_index::index<dynamic_newrsp_index_t, TagUsrId>::type newrsp_usrid_index_t;
//inline std::pair<newrsp_usrid_index_t::iterator,newrsp_usrid_index_t::iterator>
inline std::vector<dynamic_newrsp_t> fetch_dynrsps(UID userid, UInt offid, int count)
{
    std::vector<dynamic_newrsp_t> ret;
    auto & dyns = dynamic_datas::inst(userid);
    auto & idxs = boost::get<TagUsrId>(dyns.newrsps);

    auto rng = idxs.equal_range(userid, dynamic_newrsp_t::EqUsrId());
    BOOST_FOREACH(auto const & x, dyn_filter_range_4(rng, offid, count))
        ret.push_back(x);

    idxs.erase(boost::begin(rng), boost::end(rng));

    auto & st = get_dynamic_stat(userid);
    st.unread.count = 0;
    st.unread.last_userid = 0;
    //st.unread.icon.clear();

    boost::format fmt("DELETE FROM dynamic_userstat WHERE UserId=%1%");
    sql::exec(fmt % userid);

    return ret;
}

#endif // DYNAMIC_STATE_H__


