#include <map>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include "dbc.h"
#include "dynamic_state.h"
#include "client.h"
// #include "bars.h"

bool dynamic_state::add_comment(dynamic_message const & m)
{
    if (m.is_like())
    {
        bool ret = supports.insert(m.userid).second;
        if (ret)
            n_support_++;
        return ret;
    }

    push_back(m);
    latest_cid_ = m.id;
    return true;
}

template <typename Fans>
dynamic_state_ptr add_dynamic(dynamic_datas & dyns, dynamic_state_ptr dptr, Fans const & fans)
{
    dyns.users.insert(dyns.users.upper_bound(dptr->userid), dptr);

    BOOST_FOREACH(auto const & x, fans)
    {
        dynamic_rcpt_t rcpt;
        rcpt.dyn = dptr;
        rcpt.rcpt = x.first;
        dyns.rcpts.insert(rcpt);

        auto & st = dyns.stat.insert( std::make_pair(x.first, dynamic_userstat()) ).first->second;
        st.last_dynshare_id = dptr->id;
        st.last_dynshare_userid = dptr->userid;
    }

    // to-self
    dynamic_rcpt_t rcpt;
    rcpt.dyn = dptr;
    rcpt.rcpt = dptr->userid;
    dyns.rcpts.insert(rcpt);

    auto & stat = dyns.stat.insert( std::make_pair(dptr->userid, dynamic_userstat()) ).first->second;
    stat.mylast_moment_id = dptr->id;

    // if (0) // (!dptr->barsid.empty())
    // {
    //     auto bar = bars_mgr::inst().get_session_bar(dptr->barsid);
    //     if (bar)
    //     {
    //         dynamic_bar_t d;
    //         d.dyn = dptr;
    //         d.barsid = dptr->barsid;
    //         dyns.bars.insert(d);
    //     }
    // }

    return dptr;
}

dynamic_datas & dynamic_datas::inst(UID uid)
{
    static dynamic_datas dyns;
    if (uid)
    {
        auto p = dyns.stat.insert( std::make_pair(uid,dynamic_userstat()) );
        if (p.second)
            ; // p.first->second.load_trace_last_sync(uid);
        init_dynamic_user(dyns, p.first->second, uid, 1);
    }
    return dyns;
}

// dynamic_datas & dynamic_datas::instb(std::string const & barsid)
// {
//     static std::set<std::string> bar_gots;
// 
//     dynamic_datas & ds = inst(0);
// 
//     auto p = bar_gots.insert(barsid);
//     if (!p.second)
//         return ds;
// 
//     auto bp = bars_mgr::inst().get_session_bar(barsid);
//     if (bp)
//     {
//         BOOST_FOREACH(auto & uid, bp->fans)
//             init_dynamic_user(ds, uid, 0);
//     }
//     return ds;
// }

template <typename OutIter>
void Load_newrsp(UID uid, OutIter iter)
{
    boost::format fmt("SELECT CommentId FROM dynamic_userstat WHERE UserId=%1%");
    sql::datas datas(fmt % uid);
    while (sql::datas::row_type row = datas.next())
    {
        *iter++ = boost::lexical_cast<UInt>(row.at(0));
    }
}

void dynamic_datas::init_dynamic_user(dynamic_datas & dyns, dynamic_userstat& stat, UID uid, int recur)
{
    if (!stat.load_self)
    {
        stat.load_self = true;

        UID last_uid = 0;
        std::set<UInt> rspcids;
        Load_newrsp(uid, std::inserter(rspcids,rspcids.end()));

        std::vector<dynamic_message> vec;
        dynamic_message::Load(str(boost::format("UserId=%1% AND id=DynamicId") % uid)
                , std::back_inserter(vec));

        auto cli = xindex::get(uid);
        if (!cli)
            return;

        BOOST_FOREACH(auto & dmsg, vec)
        {
            dynamic_state_ptr dptr(new dynamic_state(dmsg));
            add_dynamic(dyns, dptr, cli->get_fans());

            stat.mylast_moment_id = dptr->id;

            std::vector<dynamic_message> cv;
            dynamic_message::Load(str(boost::format("DynamicId=%1% AND DynamicId!=id") % dmsg.id)
                    , std::back_inserter(cv));

            BOOST_FOREACH(auto & cm, cv)
            {
                if (dptr->add_comment(cm)) // new_dynamic_comment(dyns, dptr, cm);
                {
                    if (rspcids.find(cm.id) != rspcids.end())
                    {
                        dyns.newrsps.insert( dynamic_newrsp_t(dptr, cm) );
                        last_uid = cm.userid;
                        stat.unread.count++;
                    }
                }
            }
        }

        stat.unread.last_userid = last_uid;
        stat.load_trace_last_sync(uid);
    }

    if (!stat.load_friends && recur-- > 0) 
    {
        stat.load_friends = true;

        if (auto cli = xindex::get(uid))
        {
            auto & admires = cli->get_admires();
            BOOST_FOREACH(auto & x, admires)
            {
                auto p = dyns.stat.insert( std::make_pair(x.first, dynamic_userstat()) );
                auto & st = p.first->second;
                init_dynamic_user(dyns, st, x.first, 0);

                stat.last_dynshare_id = std::max(st.mylast_moment_id, stat.last_dynshare_id);
                if (stat.last_dynshare_id == st.mylast_moment_id)
                    stat.last_dynshare_userid = x.first;
            }
        }
    }
}

dynamic_state_ptr new_dynamic(dynamic_message const & m)
{
    auto & dyns = dynamic_datas::inst(m.userid);
    auto cli = xindex::get(m.userid);
    if (!cli)
        return dynamic_state_ptr();

    dynamic_state_ptr dptr(new dynamic_state(m));
    dptr->Save(dptr->id);
    add_dynamic(dyns, dptr, cli->get_fans());

    return dptr;
}

dynamic_state_ptr new_dynamic_comment(UInt id, dynamic_message const & m)
{
    auto & dyns = dynamic_datas::inst(m.userid);
    auto dptr = get_dynamic(id, 0);
    if (dptr)
    {
        // if (m.is_like())
        //     if (!dptr->supports.insert(m.userid).second)
        //         return dptr;
        if (dptr->add_comment(m))
        {
            dyns.newrsps.insert( dynamic_newrsp_t(dptr, m) );
            m.Save(dptr->id);

            if (m.userid != dptr->userid)
            {
                auto & st = get_dynamic_stat(dptr->userid);
                st.unread.count++;
                st.unread.last_userid = m.userid;
                // st.unread.icon = get_head_icon(m.userid);
                // st.last_dyncomment_id = dptr->id;

                boost::format fmt("INSERT INTO dynamic_userstat(UserId,CommentId) VALUES(%1%,%2%)");
                sql::exec(fmt % dptr->userid % m.id);
            }
        }
    }

    return dptr;
}

struct DysIdCmp {
    bool operator()(dynamic_message const & m, UInt id) const { return m.id < id; }
};

void delete_dynamic(UID userid, UInt did/*, UInt cid*/)
{
    auto & dyns = dynamic_datas::inst(userid);
    auto & ids = boost::get<TagId>(dyns.users);
    auto it = ids.find(did);
    if (it != ids.end()) // if (did == cid)
    {
        auto dptr = *it;
        auto & vec = *dptr;
        BOOST_FOREACH(auto const & d, vec)
        {
            dynamic_message::Delete(d.id);
        }
        dynamic_message::Delete(did);

        int n1 = boost::get<TagDynPtr>(dyns.bars).erase(dptr);
        int n2 = boost::get<TagDynPtr>(dyns.rcpts).erase(dptr);
        ids.erase(it); // int n3 = boost::get<TagUsrId>(dyns.users).erase(dptr->userid);
        LOG_I << *dptr <<" "<< n1 <<" "<< n2;
    }
}

void delete_dynamic_comment(UID userid, UInt did, UInt cid)
{
    dynamic_state_ptr dptr = get_dynamic(did, userid);
    if (dptr)
    {
        dptr->delete_comment(cid);
    }
}


