#include "myconfig.h"
#include <algorithm>
#include <boost/format.hpp>
#include "bars.h"
#include "chat.h"
#include "log.h"
#include "dbc.h"

using namespace std;
using namespace boost;

// std::istream& operator>>(std::istream& istrm, s_activity_info::activity_award & a)
// {
//     unsigned int id;
//     unsigned short nleft, nmax;
//     istrm >> id >> nleft >> nmax;
// 
//     boost::format fmt("SELECT name,img,brief FROM activity_prize WHERE id=%1%");
//     sql::datas datas(fmt % id);
//     sql::datas::row_type row = datas.next();
//     if (!row)
//         MYTHROW(EN_Prize_NotFound,bar_error_category);
// 
//     a.n_left = nleft;
//     a.n_max = nmax;
//     a.id = id;
//     a.name = row.at(0,"");
//     a.img = row.at(1,"");
//     a.brief = row.at(2,"");
// 
//     return istrm;
// }

//s_activity_info s_activity_info::load(int activity_id)
//{
//    s_activity_info activity;
//    const char* LOAD_PREPARING_ACTIVITIES = "SELECT"
//        " id,type,unix_timestamp(`tm_start`) as `t0`,unix_timestamp(`tm_end`) as `t1`,foremost"
//        ",activity_name,brief,post_img"
//        " FROM bar_activity WHERE id=%1%";
//
//    sql::datas datas( boost::format(LOAD_PREPARING_ACTIVITIES) % activity_id );
//    sql::datas::row_type row = datas.next();
//    if (!row)
//        MYTHROW(EN_Activity_NotFound,bar_error_category);
//
//    activity.id = activity_id;
//    activity.type = boost::lexical_cast<int>(row.at(1));
//    //if (row[2]) {
//    //    std::istringstream ins(row[2]);
//    //    std::istream_iterator<activity_award> i(ins), end;
//    //    std::copy(i, end, std::back_inserter(activity.awards));
//    //}
//    activity.time_begin = boost::lexical_cast<time_t>(row.at(2,"0"));
//    activity.time_end = boost::lexical_cast<time_t>(row.at(3,"0"));
//    activity.foremost = boost::lexical_cast<int>(row.at(4));
//
//    activity.activity_name = row.at(5);
//    activity.brief = row.at(6,"");
//    activity.post_img = complete_url(row.at(7,""));
//    activity.already_end = (activity.time_end < time(0));
//    activity.already_begin = (activity.time_begin < time(0));
//
//    activity_prize::load(activity.id, std::back_inserter(activity.awards) );
//    if (activity.type == s_activity_info::EN_VoteBeauty)
//        bar_activity_show::load_shows(activity_id, std::back_inserter(activity.shows2));
//
//    return activity;
//}

bar_ptr bars_mgr::get_bar_byuid(UID userid, boost::system::error_code * pec) const
{
    auto & ises = boost::get<TagUserId>(bars_);
    auto i = ises.find(userid);
    if (i == ises.end()) {
        //const_cast<bars_mgr*>(this)->load_bars(str(boost::format("WHERE trader_id='%1%'") % userid));
        //i = ises.find(userid);
        //if (i == ises.end())
        {
            boost::system::error_code ec(EN_Bar_NotFound2, bar_error_category::inst<bar_error_category>());
            if (!pec)
                mythrow(__LINE__,__FILE__, ec);
            *pec = ec;
            return bar_ptr();
        }
    }
    return const_cast<bar_ptr&>(*i);
}

bar_ptr bars_mgr::get_session_bar(const string & sessionId, boost::system::error_code* ex) const
{
    auto & ises = boost::get<TagSessionId>(bars_);
    auto it = ises.find(sessionId);
    if (it == ises.end()) {
        // const_cast<bars_mgr*>(this)->load_bars(str(boost::format("WHERE SessionId='%1%'") % sessionId));
        // it = ises.find(sessionId);
        // if (it == ises.end())
        {
            boost::system::error_code ec(EN_Bar_NotFound2, bar_error_category::inst<bar_error_category>());
            if (ex) {
                *ex = ec;
                return bar_ptr();
            }
            mythrow(__LINE__,__FILE__, ec); // MYTHROW(EN_Bar_NotFound2,bar_error_category);
        }
    }
    return const_cast<bar_ptr&>(*it);
}

// TODO: clearup
// void bars_mgr::add_bar(boost::shared_ptr<s_bar_info> bptr)
// {
//     bptr->logo = complete_url(bptr->logo);
// 
//     auto & ises = boost::get<TagSessionId>(bars_);
//     auto p = ises.insert(bptr);
//     if (!p.second) {
//         bool b = ises.replace(p.first, bptr);
//         BOOST_ASSERT(b);
//     }
// }

void bars_mgr::delete_bar(std::string const & sessionid)
{
    auto & ises = boost::get<TagSessionId>(bars_);
    auto it = ises.find(sessionid);
    if (it != ises.end())
    {
        bar_ptr b = *it;
        activity_mgr.erase_bytag(b->trader_id);

        ises.erase(it);
    }
}

void bars_mgr::get_bars_bycity(std::string const &city, vector< boost::shared_ptr<s_bar_info> >&barlist) const
{
    //map<std::string, bool>::const_iterator itr=city_load_flag.find(city);
    //if ( itr==city_load_flag.end() || false==itr->second) {
    //    const_cast<bars_mgr*>(this)->load_bars(str(boost::format("WHERE city='%1%'") % sql::db_escape(city)));
    //    const_cast<bars_mgr*>(this)->city_load_flag[city] = true;
    //}
    auto & icity = boost::get<TagCity>(bars_);
    auto p = icity.equal_range(city);
    // if (p.first == p.second) {
    //     p = icity.equal_range(city);
    // }
    std::copy(p.first,p.second, std::back_inserter(barlist));
}

bar_ptr bars_mgr::find_nearest_bar(double latitude, double longtitude, int devtype, UID uid)
{
    Position pos(latitude, longtitude);
    double min_dist = std::numeric_limits<double>::max();
    bar_ptr pbar;

    BOOST_FOREACH(const auto& b, bars_)
    {
        double dist = pos.get_distance(*b);
        if (dist < min_dist)
        {
            min_dist = dist;
            pbar = b;

            LOG_I << b->sessionid <<" "<< dist ; //<<" "<< b->latitude <<" "<< b->longtitude
        }
    }

    if (pbar)
        LOG_I << pbar->sessionid
            <<" "<< pbar->latitude <<" "<< pbar->longtitude
            <<" "<< latitude <<" "<< longtitude
            <<" "<< min_dist
            <<" "<< devtype <<" "<< uid
            <<" minimal dist"
            ;

    return pbar;
}

// TODO: cleanup
std::vector<program> program_mgr::get_porgrams(const std::string& SessionId)
{
    std::vector<program> programs;
    program_mgr_iterator itr = session_program_list.find(SessionId);
    if(itr == session_program_list.end()){
        itr = load(SessionId);
    }

    if(!itr->second.empty()){
        time_t now = time(NULL);
        for(std::vector<program>::iterator i=itr->second.begin(); i!=itr->second.end(); )
        {
            if(i->end_ < now){
                i=itr->second.erase(i);
            }
            else{
                // if(i->end > now + 24*60*60){
                //     break;
                // }
                programs.push_back(*i);
                ++i;
            }
        }
    }

    return programs;
}
program_mgr::program_mgr_iterator program_mgr::load(const std::string& SessionId)
{
#define LOAD_PROGRAMS ("select `id`, `type`, `show_name`,`SessionId`,`detail`,unix_timestamp(`begin`) as begin,unix_timestamp(`end`) as end,`actors` from shows where SessionId='%1%' and end>CURRENT_TIMESTAMP")
    std::vector<program> programs;

    boost::format fmt(LOAD_PROGRAMS);
    sql::datas datas(fmt %sql::db_escape(SessionId));
    while(sql::datas::row_type row = datas.next()){
        program p;
        p.id_ = lexical_cast<int>(row.at(0,"0"));
        if(p.id_ <= 0) continue;

        p.type_ = lexical_cast<int>(row.at(1,"0"));
        p.show_name_ = row.at(2,"");
        p.SessionId_ = SessionId; 
        p.detail_ = row.at(4,"");
        p.begin_ = lexical_cast<int>(row.at(5,"0"));
        p.end_ = lexical_cast<int>(row.at(6,"0"));
        std::string actors = row.at(7,"[]");
        p.json_to_vector(json::decode<json::array>(actors));

        programs.push_back(p);
    }

    std::pair<program_mgr_iterator,bool> itr = session_program_list.insert(make_pair(SessionId, programs));

    return itr.first;
}

s_bar_info::s_bar_info(const std::string& sessionId)
    : sessionid(sessionId)
{
    // tp_activity_close_ = std::numeric_limits<time_t>::max();
    BOOST_ASSERT(is_barchat(sessionId));
    // pcg = NULL;
}

// void s_bar_info::reinit(bar_ptr const & b)
// {
//     if (b.get() == this)
//         return;
//     BOOST_ASSERT(b->sessionid == this->sessionid);
// 
//     auto bfans = boost::move(this->fans);
//     auto bactivities = boost::move(this->activities2);
// 
//     *this = b;
//     std::swap(this->fans, bfans);
//     std::swap(this->black_list, );
//     std::swap(this->inspot_users_, );
//     std::swap(this->activities2, bactivities);
// }

void s_bar_info::add_fans(UID uid)
{
    pair< set<UID>::iterator, bool> ret;
    ret = this->fans2.insert(uid);
    if(ret.second){
        boost::format fmt("INSERT INTO contacts(UserId,OtherId,Relation) VALUES(%1%, %2%, 1)");
        sql::exec(fmt % uid % trader_id);
    }
}

void s_bar_info::delete_fans(UID uid)
{
    if (this->fans2.erase(uid)){
        boost::format fmt("DELETE FROM contacts WHERE UserId=%1% AND OtherId=%2%");
        sql::exec(fmt % uid % trader_id);
    }
}

// void s_bar_info::update_bar_fans()
// {
//     const char* UPDATE_BAR_FANS = "update bars set fans_list='%1%' WHERE SessionId='%2%'";
//     std::ostringstream out;
//     BOOST_FOREACH(auto const & p , this->fans)
//         out << p << " ";
//     sql::exec(boost::format(UPDATE_BAR_FANS) % out.str() % this->sessionid);
// }

time_t s_bar_info::timed_loop(bar_ptr thiz, time_t t)
{
    if (!boost::empty(thiz->inspot_users2_))
    {
        //using namespace boost::posix_time;
        //ptime tp = second_clock::local_time();

        //std::vector<UID> outs;
        //{
        //    BOOST_FOREACH(auto const& x, thiz->inspot_users2_)
        //    {
        //        if (x.t + hours(1) > tp)
        //            break;
        //        outs.push_back(x.uid);
        //        LOG_I << thiz->sessionid <<" "<< x.uid;
        //    }
        //}
        //BOOST_FOREACH(auto uid, outs)
        //{
        //    if (client_ptr c = xindex::get(uid))
        //    {
        //        Client::spot_main_entry(c, bar_ptr());
        //    }
        //}
    }

    time_t nxt = time(0) + 60*5;

    if (!thiz->activities2.empty())
    {
        time_t t0 = thiz->check_close_time(t);
        time_t t1 = thiz->check_open_time(t);
        nxt = std::min(std::min(nxt, t0), t1);
    }

    return nxt; // std::min(std::min(t0, t1), (time(0) + 60*15));
}

time_t s_bar_info::check_close_time(time_t t)
{
    time_t tpcur = time(0);
    LOG_I << "check_close_time " << sessionid <<" "<< t;

    auto & idxs = boost::get<TagActivityCloseTime>(activities2);
    auto iter = idxs.begin(); // idxs.lower_bound(tp_activity_close_);
    while (iter != idxs.end() && iter->is_closed()) {
        if (iter->time_end + (3600*24*7) < tpcur) {
            LOG_I << *iter << " erased";
            iter = idxs.erase(iter);
        } else
            ++iter;
    }
    if (iter != idxs.end()) {
        auto & activity = const_cast<s_activity_info&>(*iter);
        LOG_I << activity;
        if (activity.time_end <= tpcur) {
            activity.already_end = 1;
            ++iter;
            LOG_I << activity << " end";

            if (activity.type == s_activity_info::EN_VoteBeauty) {
                if ( activity.result_rank_list.empty() && !activity.shows2.empty() ) {
                    unsigned num = activity.winners_setting;
                    num = num > activity.shows2.size()? activity.shows2.size() : num;
                    activity.result_rank_list.resize(num, bar_activity_show(0));

                    std::partial_sort_copy(activity.shows2.begin(), activity.shows2.end(),
                            activity.result_rank_list.begin(), activity.result_rank_list.end(),
                            bar_activity_show::OrderByVotes());
                }

                json::array rank_list;
                for ( unsigned i=0; i < activity.result_rank_list.size(); ++i) {
                    client_ptr c = xindex::get(activity.result_rank_list[i].actor);
                    if (!c)
                        continue;
                    rank_list (
                            json::object()
                            ("user", c->brief_user_info())
                            ("score", activity.result_rank_list[i].countof_voter)
                            ("rank", i)  //TODO i>0 or i>=0
                            );
                }
                LOG_I <<"result_rank_list size: "<<activity.result_rank_list.size()
                    <<" rank_list size: "<< rank_list.size();

                imessage::message msg("activity/end/beauty", trader_id, sessionid);
                msg.body
                    ("activity_id", activity.id)
                    ("activity_name", activity.activity_name)
                    ("activity_type", activity.type)
                    ("content", rank_list)
                    ;

                chatmgr::inst().send(msg, sysadmin_client_ptr());

                redis::command("HSET", make_activity_msgs_key(activity.id), msg.id(), 
                        impack_activity_msg(msg));

                //format fmt("恭喜你在活动%1%中获得第%2%名，请在酒吧现场领取奖品。");
                //for ( unsigned i=0; i < activity.result_rank_list.size(); ++i) {
                //    imessage::message msg("chat/text", trader_id, 
                //            make_p2pchat_id(trader_id, activity.result_rank_list[i].actor));
                //    msg.body
                //        ("content", (fmt %activity.activity_name %(i+1)).str())
                //        ;
                //    chatmgr::inst().send(msg, sysadmin_client_ptr());
                //}

            } else if (activity.type == s_activity_info::EN_LuckyShake) {
                activity_count_erase(activity.id);
            }
        }
    }
    return (iter == idxs.end()
            ? std::numeric_limits<time_t>::max()
            : iter->time_end);
}

enum { TIMEGAP_FORCAST = 60*5 };

static json::object json_activity(s_activity_info const & activity)
{
    return json::object()
        ("id", activity.id)
        ("name", activity.activity_name)
        ("brief", activity.brief)
        ("image", activity.post_img)
        ("time_open", time_string(activity.time_begin))
        ("time_close", time_string(activity.time_end))
        ;
}

time_t s_bar_info::check_open_time(time_t t)
{
    // boost::multi_index::index<activities_type,TagActivityOpenTime>::type& idxs
    LOG_I << "check_open_time " << sessionid <<" "<< t;
    auto & idxs = boost::get<TagActivityOpenTime>(activities2);
    auto iter = idxs.begin(); // idxs.lower_bound(tp_activity_close_);
    while (iter != idxs.end() && iter->already_begin)
        ++iter;
    if (iter != idxs.end()) {
        auto & activity = const_cast<s_activity_info&>(*iter);
        LOG_I << activity;
        time_t tpcur = time(0);
        if (iter->time_begin <= tpcur + (TIMEGAP_FORCAST)) {
            activity.already_begin = 1;
            ++iter;
            LOG_I << activity << " begin";

            switch (activity.type) {
                case s_activity_info::EN_VoteBeauty:
                    {
                        imessage::message msg("activity/beauty/add", 
                                SYSADMIN_UID, 
                                make_p2pchat_id(SYSADMIN_UID, assist_id));
                        msg.body("activeid", activity.id)
                            ("sessionid", sessionid);

                        json::array users;
                        BOOST_FOREACH(auto const & v , inspot_users2_) {
                            if (v.uid != trader_id && v.uid != assist_id) {
                                if ( check_employee(trader_id, v.uid) ) { continue; }

                                if ( activity.find_show(v.uid) == activity.shows2.end()) {
                                    auto p = activity.add_show(v.uid);
                                    if ( p != activity.shows2.end()) { 
                                        client_ptr c = xindex::get(v.uid);
                                        if ( c ) {
                                            json::object user = c->brief_user_info();
                                            user.erase("public_flag");
                                            user.erase("gender");
                                            user.erase("attribute");
                                            user.erase("guest");
                                            users(user);
                                        }
                                    }
                                }
                            }
                        }

                        if ( !users.empty() ) {
                            msg.body("users", users);
                            chatmgr::inst().send(msg, sysadmin_client_ptr());

                            redis::command("HSET", make_activity_msgs_key(activity.id), 
                                    msg.id(), impack_activity_msg(msg));
                        }
                    }
                    break;
                case s_activity_info::EN_LuckyShake:
                    break;
            }
        }
    }

    return (iter == idxs.end()
            ? std::numeric_limits<time_t>::max()
            : iter->time_begin - TIMEGAP_FORCAST);
}

void s_bar_info::update_expires_time(bar_ptr thiz)
{
    time_t t = timed_loop(thiz, time(0));
    bars_mgr::inst().activity_mgr.wait(thiz->trader_id
            , t, boost::bind(&s_bar_info::timed_loop, thiz, _1));
}

void s_bar_info::leave_spot(client_ptr & cli)
{
    UID uid = cli->user_id();
    auto & idxs = boost::get<1>(inspot_users2_);
    auto it = idxs.find(uid);
    if (it != idxs.end())
    {
        count_.decr(cli);
        leave_chat(cli, 1);
        // boost::system::error_code ec;
        // chat_group *cg = chatmgr::inst().chat_group(&ec, sessionid);
        // if (cg)
        // {
        //     if (cg->remove(uid))
        //         ;
        // }
        idxs.erase(it);

        boost::format fmt("INSERT INTO bar_spot_access(SessionId,FromSessionId,UserId,Enter)"
                " VALUES('%1%','%1%',%2%,0)");
        sql::exec(fmt % sessionid % cli->user_id());

        redis::command("HDEL", str(format("spot/%1%/users") % trader_id), uid);
        redis::command("DEL", str(format("rec_%1%_%2%") % sessionid % uid));
        LOG_I << uid;
    }
}

void s_bar_info::enter_spot(client_ptr & cli)
{
    UID uid = cli->user_id();
    if (uid==assist_id || uid==trader_id
            || !is_normal_uid(cli->user_id()))
        return;

    user_inspot usrinf(uid);
    auto p = inspot_users2_.push_back(usrinf);
    LOG_I << *this <<" "<< uid <<" "<< p.second;

    if (!p.second)
    {
        inspot_users2_.erase(p.first);
        inspot_users2_.push_back(usrinf);
        return;
    }

    count_.incr(cli);

    // if (inspot_users2_.push_back(user_inspot(uid)).second)
    {
        time_t now = time(NULL);
        auto & activities = boost::get<TagActivityOpenTime>(activities2);
        for (auto itr = activities.begin(); itr != activities.end(); ++itr)
        {
            if (itr->time_end < now) continue;
            if (itr->time_begin > now+TIMEGAP_FORCAST) { break; }
            if (itr->type!=s_activity_info::EN_VoteBeauty) continue;

            LOG_I << sessionid <<" add shows "<<uid;
            if ( itr->find_show(uid) == itr->shows2.end()) {
                if ( !check_employee(trader_id, uid) ) { 
                    auto p = const_cast<s_activity_info&>(*itr).add_show(uid);
                    if (p != itr->shows2.end()) { 
                        client_ptr c = xindex::get(uid);
                        if ( c ) {
                            json::object user = c->brief_user_info();
                            user.erase("public_flag");
                            user.erase("gender");
                            user.erase("attribute");
                            user.erase("guest");

                            imessage::message msg("activity/beauty/add", 
                                    SYSADMIN_UID, 
                                    make_p2pchat_id(SYSADMIN_UID, assist_id));
                            msg.body("activeid", itr->id)
                                ("sessionid", sessionid)
                                ("users", json::array()(user));

                            chatmgr::inst().send(msg, sysadmin_client_ptr());

                            redis::command("HSET", make_activity_msgs_key(itr->id), msg.id(), 
                                    impack_activity_msg(msg));
                        }
                    }
                }
            }
        }

        boost::format fmt("INSERT INTO bar_spot_access(SessionId,FromSessionId,UserId,Enter)"
                " VALUES('%1%','%1%',%2%,1)");
        sql::exec(fmt % sessionid % cli->user_id());

        redis::command("HSET", str(format("spot/%1%/users") % trader_id), uid, 1);
        redis::command("SET", str(format("rec_%1%_%2%") % sessionid % uid), time(0));
    }
}

void s_bar_info::leave_chat(client_ptr & cli, bool leaving_spot)
{
    UID uid = cli->user_id();

    if (uid == trader_id || uid == assist_id)
        return;

    boost::system::error_code ec;
    chat_group *cg = chatmgr::inst().chat_group(&ec, sessionid);
    if (cg && cg->remove(uid))
    {
        if (!leaving_spot)
            redis::command("HINCRBY", str(format("spot/%1%/users") % trader_id), uid, -1);
    }
}

chat_group* s_bar_info::join_chat(client_ptr & cli)
{
    UID uid = cli->user_id();

    if (!is_normal_uid(uid))
        return 0;

    boost::system::error_code ec;
    chat_group *cg = chatmgr::inst().chat_group(&ec, sessionid);
    if (cg)
    {
        if (!cg->is_alive_member(uid))
        {
            cg->add_msg_excl(cli);
            redis::command("HSET", str(format("spot/%1%/users") % trader_id), uid, 2);
        }
    }
    return cg;
}

chat_group* s_bar_info::join_chat(UID uid)
{
    if (client_ptr c = xindex::get(uid))
        return join_chat(c);
    return 0;
}

void s_bar_info::recover_in_spot_users(bar_ptr pbar)
{
    LOG_I << pbar->trader_id;

    auto reply = redis::command("HGETALL", str(format("spot/%1%/users") % pbar->trader_id));
    if (reply && reply->type == REDIS_REPLY_ARRAY)
    {
        for (UInt i = 0; i < reply->elements/2; i++)
        {
            auto k = reply->element[i*2]->str;
            auto v = reply->element[i*2+1]->str;
            LOG_I << k <<" "<< v;

            UInt uid = lexical_cast<UInt>(k);
            int flag = lexical_cast<UInt>(v);
            if (client_ptr c = xindex::get(uid))
            {
                if (flag > 0)
                    Client::spot_main_entry(c, pbar);
                if (flag > 1)
                    pbar->join_chat(c);
            }
        }
    }
}

void bars_mgr::load_bars(std::string const & cond)
{
    LOG_I << cond;

    std::vector<bar_ptr> vec;
    s_bar_info::load(cond, std::back_inserter(vec));

    BOOST_FOREACH(bar_ptr & bptr, vec)
    {
        client_ptr c = bptr->trader();
        if (!c)
        {
            LOG_E << bptr->sessionid << " error trader_id " << bptr->trader_id;
            continue;
        }
        BOOST_ASSERT(c);

        c->user_info().put("public_flag", 1);

        if (!bptr->bar_coupons_.empty())
            coupon_bar_list.push_back(bptr);

        this->bars_.insert(bptr);
    }
        //load_bar_fans();
}

void bars_mgr::activity_manager::timer_fn(const boost::system::error_code & ec)
{
    if (ec)
        return;
    if (timed_list_.empty())
        return;
    time_t tpcur = time(0);

    while (!timed_list_.empty()) {
        std::pair<time_t, Func> p = *timed_list_.begin();
        if (p.first > tpcur)
            break;
        timed_list_.erase(timed_list_.begin());

        p.first = (p.second)(p.first);
        if (p.first != std::numeric_limits<time_t>::max())
            timed_list_.insert(p);
    }

    if (!timed_list_.empty()) {
        time_t tp = timed_list_.begin()->first;
        LOG_I << tp <<" in-seconds "<< tp - tpcur;// <<" thread:"<< pthread_self();
        if (tp < tpcur)
            tp = tpcur;
        timer_.expires_from_now(boost::posix_time::seconds(tp - tpcur));
        timer_.async_wait(boost::bind(&activity_manager::timer_fn, this, boost::asio::placeholders::error));
    }

    {
        using namespace boost::posix_time;
        static boost::gregorian::date s_date_(2000,1,1);

        ptime tp = second_clock::local_time();
        boost::gregorian::date date = tp.date();
        if (date != s_date_)
        {
            long xhour = tp.time_of_day().hours();
            if (xhour == 8)
            {
                int n = 0;
                auto & bars = bars_mgr::inst().bars_;

                std::vector<UID> outs;
                BOOST_FOREACH(auto & b, bars)
                {
                    BOOST_FOREACH(auto & x, b->inspot_users2_)
                    {
                        outs.push_back(x.uid);
                        // const_cast<ptime&>(x.t) -= hours(3);
                        ++n;
                        // LOG_I << b->sessionid <<" "<< x.uid <<" "<< x.t;
                    }
                }
                BOOST_FOREACH(auto uid, outs)
                {
                    if (client_ptr c = xindex::get(uid))
                    {
                        Client::spot_main_entry(c, bar_ptr());
                        c->clear_oldspot();
                    }
                }

                LOG_I << xhour <<" "<< s_date_ <<" "<< date <<" "<< n;
                s_date_ = date;
            }
            // else LOG_I << xhour <<" "<< s_date_ <<" "<< date;
        }
    }
}

static bars_mgr* g_bars_mgr_ = 0;

bars_mgr::bars_mgr(boost::asio::io_service & io_s)
    : activity_mgr(io_s)
{
    BOOST_ASSERT(!g_bars_mgr_);
    if (g_bars_mgr_)
        abort();
    g_bars_mgr_ = this;

    load_bars(std::string());

    LOG_I << "bar-time";
    io_s.post( &bars_mgr::post_init_bars );
}

void bars_mgr::post_init_bars()
{
    auto & self = inst();
    LOG_I << "bar-time";

    BOOST_FOREACH(bar_ptr pbar, self.bars_)
    {
        LOG_I << pbar->trader_id <<" "<< pbar->sessionid;

        chat_group* pcg = pbar->join_chat(pbar->trader_id);
        BOOST_ASSERT(pcg);
        if (!pcg)
        {
            LOG_E << pbar->sessionid << " error chat/room " << pbar->trader_id;
            continue;
        }

        LOG_I << pbar->trader_id << " post recover_in_spot_users";

        s_bar_info::recover_in_spot_users(pbar);

        self.activity_mgr.wait(pbar->trader_id, 1
                , boost::bind(&s_bar_info::timed_loop, pbar, _1));
    }
}

bars_mgr& bars_mgr::inst()
{
    BOOST_ASSERT(g_bars_mgr_);
    if (!g_bars_mgr_)
        abort();
    return *g_bars_mgr_;
}

global_mgr& global_mgr::inst()
{
    static global_mgr instance;
    return instance;
}

bar_error_category::bar_error_category()
{
    static code_string ecl[] = {
        { EN_Bar_NotFound2, "不存在的商家" },
        { EN_Activity_NotFound, "不存在的活动" },
        { EN_Prize_NotFound, "不存在的奖项" },
        { EN_Luckshake_Count_Limit, "摇一摇超过设置的次数" },
    };
    Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])], "bar");
}

bool s_bar_info::check_employee(UID trader_id, UID uid) 
{
    const char* prefix = "bar_staff_";
    std::string key = prefix + boost::lexical_cast<std::string>(trader_id);
    auto reply = redis::command("HEXISTS", key, uid);
    if ( reply->type == REDIS_REPLY_INTEGER ) {
        return reply->integer;
    }

    return false;
}

void s_bar_info::message_finish(imessage::message const & m, UID uid)
{
    char k[64];
    snprintf(k,sizeof(k), "message/%d/reachs", m.id());

    redis::reply y = redis::command("LPUSHX", k, uid);

    // hiredis::reply y = hiredis::inst().command("LPUSHX", k, uid);
    if (y && y->type == REDIS_REPLY_INTEGER && y->integer == 0)
    {
        redis::context ctx( redis::context::make() );
        // hiredis::context ctx( hiredis::inst().make_context() );
        ctx.append("LPUSH", k, uid)
            .append("EXPIRE", k, (60*60*24*30));
    }
    LOG_I << k << " redis";
}

static std::map<UInt,std::map<UID,int> > activity_count_;

void activity_count_load(UInt activity_id)
{
    auto it = activity_count_.insert(std::make_pair(activity_id, std::map<UID,int>())).first;

    auto reply = redis::command("HGETALL", str(format("activity/%1%/count") % activity_id));

    if (reply && reply->type == REDIS_REPLY_ARRAY)
    {
        for (UInt i = 1; i < reply->elements; i+=2)
        {
            auto k = reply->element[i-1]->str;
            auto v = reply->element[i]->str;
            LOG_I << k <<" "<< v;
            if (!k || !v)
                continue;
            it->second.insert(std::make_pair(lexical_cast<UID>(k), lexical_cast<int>(v)));
        }
    }
}

int activity_count_incr(UInt activity_id, UID uid, int limit_max)
{
    auto it = activity_count_.insert(std::make_pair(activity_id, std::map<UID,int>())).first;
    auto itu = it->second.insert(std::make_pair(uid, 0)).first;
    if (itu->second <= limit_max)
    {
        ++itu->second;
        redis::command("HSET", str(format("activity/%1%/count") % activity_id), uid, itu->second);
    }
    LOG_I << activity_id <<" "<< uid <<" "<< limit_max <<" "<< itu->second;

    return (itu->second);
}

int activity_count_erase(UInt activity_id)
{
    // redis::command("EXPIRE", str(format("activity/%1%/count") % activity_id), 60*60*24);
    return activity_count_.erase(activity_id);
}
