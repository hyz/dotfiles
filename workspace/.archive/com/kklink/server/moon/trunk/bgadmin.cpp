#include "myconfig.h"
#include <stdint.h>
#include <sstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <list>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iterator.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/random.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/timer/timer.hpp>
#include <boost/regex.hpp>

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include <ctime>
#include <cstdlib>
#include <fnmatch.h>

//#include <GraphicsMagick/Magick++.h>
#include <Magick++.h>

#include "log.h"
#include "dbc.h"
#include "jss.h"
#include "util.h"
#include "client.h"
#include "myerror.h"
#include "chat.h"
#include "service.h"
// #include "operate_db.h"
#include "bars.h"
#include "bottle_drifting.h"
#include "asclient.h"

#include "sqls.h"

using namespace std;
using namespace boost;

unsigned int numerator = 500;
unsigned int g_numerator = 500;

static void change_user(UID uid, std::string const & user)
{
    sql::exec(format("UPDATE users SET UserName='%1%' WHERE UserId=%2% LIMIT 1")
            % sql::db_escape(user) % uid);
}

static void change_pass(UID uid, std::string const & pwd)
{
    sql::exec(format("UPDATE users SET password='%1%' WHERE UserId=%2% LIMIT 1")
            % sql::db_escape(pwd) % uid);
}

json::object Service::setNumerator(http::request& req, client_ptr&)
{
    int type=lexical_cast<unsigned int>(req.param("type","1"));
    if ( 1 == type ) {
        unsigned int num = lexical_cast<unsigned int>(req.param("numerator","0"));
        if ( 0 < num && 1000 > num ) {
            numerator = num;
        }
    } else if ( 2 == type ) {
        unsigned int num = lexical_cast<unsigned int>(req.param("numerator","0"));
        if ( 0 < num && 1000 > num ) {
            g_numerator = num;
        }
    }
    
    return json::object();
}

json::object Service::initBarSession(http::request& req, client_ptr&)
{
    // UID userid = req.param("userid");
    string city = req.param("city");
    string zone = req.param("zone");
    string BarName = req.param("BarName");
    string address = req.param("address");
    string longtitude = req.param("longtitude","0");
    string latitude = req.param("latitude","0");
    string introduction = req.param("introduction");
    string phone = req.param("phone");
    string logo = req.param("logo","");
    string SessionId = req.param("SessionId","");
    string trader = req.param("trader","");
    string trader_password = req.param("trader_password","");
    string salt = req.param("salt","");
    string province = req.param("province", "");
    string screensize = req.param("screensize", "");
    string mark = req.param("mark", "");
    int isSigned = lexical_cast<int>(req.param("isSigned", "0"));
    UID trader_id = 0;

    try
    {
        if ( !SessionId.empty() ) {
            mark = SessionId;
            format fmt("SELECT trader,trader_password,trader_id,assist_id, screensize"
                    " FROM bars WHERE SessionId='%1%'");
            sql::datas datas(fmt %sql::db_escape(SessionId));
            if (sql::datas::row_type row = datas.next()) {
                chat_group & cg = chatmgr::inst().chat_group(SessionId);
                if (cg.name() != BarName)
                    cg.rename(BarName, sysadmin_client_ptr());

                trader_id = lexical_cast<UID>(row.at(2,"0"));
                if ( 0 != trader_id ) {
                    if (trader != row.at(0))
                        change_user(trader_id, trader);
                    if (trader_password != row.at(1)) {
                        change_pass(lexical_cast<int>(row.at(2)), trader_password);
                        change_pass(lexical_cast<int>(row.at(3)), trader_password);
                    }

                    if (screensize.empty()) {
                        screensize = row.at(4,"");
                    }

                    const char* UPDATE_BARS14 = "UPDATE bars SET city='%1%',zone='%2%',"
                        " BarName='%3%',address='%4%',longtitude=%5%,latitude=%6%,"
                        " introduction='%7%',phone='%8%',logo='%9%',trader='%11%',"
                        " trader_password='%12%',salt='%13%',province='%14%',screensize='%15%',"
                        " isSigned=%16%, mark='%17%' "
                        " where SessionId='%10%'";
                    format fmt2(UPDATE_BARS14);
                    fmt2 %sql::db_escape(city) %sql::db_escape(zone) 
                        %sql::db_escape(BarName) %sql::db_escape(address) 
                        %longtitude %latitude %sql::db_escape(introduction) 
                        %sql::db_escape(phone) %sql::db_escape(logo) 
                        %sql::db_escape(SessionId) %sql::db_escape(trader) 
                        %sql::db_escape(trader_password) %sql::db_escape(salt) 
                        %sql::db_escape(province) %sql::db_escape(screensize)
                        %isSigned %sql::db_escape(mark);
                    sql::exec(fmt2);  

                    bar_ptr pbar = bars_mgr::inst().get_session_bar(SessionId);
                    if ( pbar->barname != BarName 
                            || url_to_relative(pbar->logo) != logo ) {
                        UInt ver = pbar->bar_user_info_ver(+1);
                        sql::exec( format("update users set icon='%1%',nick='%2%',ver=%3% where UserId=%4%") 
                                % sql::db_escape(logo) % sql::db_escape(BarName) % ver % trader_id);

                        if (client_ptr c = pbar->trader())
                        {
                            c->user_info().put("icon", complete_url(logo));
                            c->user_info().put("nick", BarName);
                        }
                    }
                    bars_mgr::inst().delete_bar(SessionId); //(tmp);
                    bars_mgr::inst().load_bars(str(format(" where SessionId='%1%'") %SessionId));
                    bar_ptr p = bars_mgr::inst().get_session_bar(SessionId); //(tmp);
                    // restore inspot-users
                    std::swap(p->inspot_users2_, pbar->inspot_users2_);
                    std::swap(p->count_, pbar->count_);
                }

                LOG_I << "Update " << SessionId;
            }
        } else {
            trader_id = _user_regist(trader, trader_password, logo);
            sql::exec( format("update users set nick='%1%' where UserId=%2%")
                    % sql::db_escape(BarName) % trader_id);
            // int assist_id = _user_regist(trader+"_assist", trader_password, logo);
            int assist_id = INVALID_UID;

            SessionId = lexical_cast<string>(trader_id);
            mark = SessionId;
            chatmgr::inst().create_chat_group(trader_id, SessionId, BarName);

            format fmt2("INSERT INTO"
                    " bars(city,zone,BarName,address,longtitude,latitude,introduction,phone," 
                    " logo,SessionId,trader,trader_password,salt,province,trader_id, assist_id,screensize,"
                    " isSigned, mark)"
                    " VALUES('%1%','%2%','%3%','%4%',%5%,%6%,'%7%','%8%','%9%'," 
                    " '%10%', '%11%', '%12%', '%13%', '%14%',%15%, %16%, '%17%', %18%, '%19%')");
            sql::exec(fmt2 %sql::db_escape(city) %sql::db_escape(zone) 
                    %sql::db_escape(BarName) %sql::db_escape(address) 
                    %longtitude %latitude %sql::db_escape(introduction) 
                    %sql::db_escape(phone) %sql::db_escape(logo) 
                    %sql::db_escape(SessionId) %sql::db_escape(trader) 
                    %sql::db_escape(trader_password) %sql::db_escape(salt) 
                    %sql::db_escape(province) % trader_id %assist_id
                    %sql::db_escape(screensize) %isSigned %sql::db_escape(mark));

            bars_mgr::inst().load_bars(str(format(" where SessionId='%1%'") %SessionId));
            LOG_I << "Insert " << SessionId;
        }
    } catch (...) {}


    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(SessionId);
    // boost::shared_ptr<s_bar_info> tmp(new s_bar_info(SessionId));
    // tmp->sessionid = SessionId;
    // tmp->trader_id = trader_id;

    // tmp->city = city;
    // tmp->zone = zone;
    // tmp->barname = BarName;
    // tmp->address = address;
    // tmp->longtitude = lexical_cast<double>(longtitude);
    // tmp->latitude = lexical_cast<double>(latitude);
    // tmp->introduction = introduction;
    // tmp->phone = phone;
    // tmp->logo = logo;
    // tmp->province = province;

    // bars_mgr::inst().add_bar(tmp);

    return json::object()("result", "successed");
}

json::object Service::DelBarSession(http::request& req, client_ptr&)
{
    string SessionId = req.param("SessionId");

    try
    {
        sql::exec(format("DELETE FROM bars WHERE SessionId='%1%'") % sql::db_escape(SessionId));
        sql::exec(format("DELETE FROM sessions WHERE SessionId='%1%'") % sql::db_escape(SessionId));

        // boost::shared_ptr<s_bar_info> tmp(new s_bar_info());
        // tmp->sid = SessionId;
        // tmp->sessionid = SessionId;
        bars_mgr::inst().delete_bar(SessionId); //(tmp);
        // bars_mgr::inst().delete_session_bar(SessionId); //(tmp);
        // tmp.reset();

        LOG_I << "Delete " << SessionId;
    } catch (...) {}

    return json::object()("result", string("successed"));
}

imessage::message activityNotifyMsg(const bar_ptr pbar, int activeid, boost::system::error_code& e)
{
    json::array prize;
    json::object active;

    if ( !pbar ) {
        THROW_EX(EN_Bar_NotFound2);
    }
    auto it = pbar->find_activity( activeid );

    imessage::message msg("activity/notify", SYSADMIN_UID, 
            make_p2pchat_id(SYSADMIN_UID, pbar->assist_id)); 

    if (it == pbar->activities2.end()) {
        boost::system::error_code ec(EN_Activity_NotFound, bar_error_category::inst<bar_error_category>());
        e = ec;
    }

    BOOST_FOREACH(const auto&a, it->awards) {
        prize(json::object()("id", a.id)
                ("count", a.count)
                ("name", a.name)
                ("icon", a.img)
                // ("brief", a.brief)
             );
    }

    active("activeid", it->id)
        ("sessionid", pbar->sessionid)
        ("banner", it->post_img)
        ("type", it->type)
        ("activeInfo", it->brief)
        ("activeName", it->activity_name)
        ("prize", prize)
        ("beginDate", time_string(it->time_begin))
        ("endDate", time_string(it->time_end))
        ("screen_img", complete_url(it->screen_img))
        ("system_time", time_string());

    msg.body
        ("on_off", it->on_off)
        ("content", active);
    return msg;
}
//manager setting
void del_g_activity_prize(http::request& req, int activeid, client_ptr& cli)
{
    const char* DELETE_G_ACTIVITY_PRIZE = "delete from g_activity_prize where activity_id=%1%";
    sql::exec(format(DELETE_G_ACTIVITY_PRIZE) % activeid);
}
void add_g_activity_prize(http::request& req, int activeid, client_ptr& cli)
{
    const char* values_temp = "(%1%, %2%, '%3%', '%4%', '%5%',%6%)";
    string INSERT_G_ACTIVITY_PRIZE = "insert into g_activity_prize(activity_id, "
        " probability,name,img,brief, level) values";

    if ( req.content().empty() ) return;
    LOG_I<<"prizes:"<< req.content();
    json::object jo = json::decode(req.content());
    json::array prizes = jo.get<json::array>("prizes", json::array());
    if ( prizes.empty() ) return;

    del_g_activity_prize(req, activeid, cli);

    for ( auto itr=prizes.begin(); itr!=prizes.end(); ++itr) {
        json::object obj =  boost::get<json::object>(*itr);
        string img = obj.get<string>("image","");
        string name = obj.get<string>("name","");
        string brief = obj.get<string>("brief","");
        double probability = obj.get<double>("probability", 0.0);
        int level = obj.get<int>("level", 0);
        if ( probability - 0.0 < 0.0001 ) continue;
        INSERT_G_ACTIVITY_PRIZE += (format(values_temp) %activeid %probability 
                %sql::db_escape(name) %sql::db_escape(img) %sql::db_escape(brief) %level).str()+",";
    }
    *(INSERT_G_ACTIVITY_PRIZE.rbegin()) = ' ';

    sql::exec(INSERT_G_ACTIVITY_PRIZE);
}

void del_activity_prize(http::request& req, int activeid, client_ptr& cli)
{
    const char* DELETE_BAR_ACTIVITY_PRIZE = "delete from bar_activity_prize where activity_id=%1%";
    sql::exec(format(DELETE_BAR_ACTIVITY_PRIZE) % activeid);
}

void add_activity_prize(http::request& req, int activeid, client_ptr& cli)
{
    const char* values_temp = "(%1%, %2%, %2%, '%3%', '%4%', '%5%',%6%)";
    string INSERT_BAR_ACTIVITY_PRIZE = "insert into bar_activity_prize(activity_id,count,initial_count,name,img,brief, level) values";

    if ( req.content().empty() ) return;
    LOG_I<<"prizes:"<< req.content();
    json::object jo = json::decode(req.content());
    json::array prizes = jo.get<json::array>("prizes", json::array());
    if ( prizes.empty() ) return;

    del_activity_prize(req, activeid, cli);

    for ( auto itr=prizes.begin(); itr!=prizes.end(); ++itr) {
        json::object obj =  boost::get<json::object>(*itr);
        string img = obj.get<string>("image","");
        string name = obj.get<string>("name","");
        string brief = obj.get<string>("brief","");
        int count = obj.get<int>("count", 0);
        int level = obj.get<int>("level", 0);
        INSERT_BAR_ACTIVITY_PRIZE += (format(values_temp) %activeid %count %sql::db_escape(name) %sql::db_escape(img) %sql::db_escape(brief) %level).str()+",";
    }
    *(INSERT_BAR_ACTIVITY_PRIZE.rbegin()) = ' ';

    sql::exec(INSERT_BAR_ACTIVITY_PRIZE);
}

void set_lucky_shake(http::request& req, int activeid, client_ptr& cli)
{
    const char* INSERT_LUCKY_SHAKE_SETTTING = "insert into lucky_shake_settting(activity_id,attendees_setting,prize_setting) values(%1%,%2%,%3%)";
    const char* UPDATE_LUCKY_SHAKE_SETTTING = "update lucky_shake_settting set attendees_setting=%1%,prize_setting=%2% where activity_id=%3%";

    int attendees_setting = lexical_cast<int>(req.param("attendees_setting","-1"));
    int prize_setting = lexical_cast<int>(req.param("prize_setting","-1"));

    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(cli->user_id(), ec);
    if ( !ec ) { 
        auto it = pbar->find_activity(activeid);
        if (it != pbar->activities2.end()) {
            attendees_setting = -1==attendees_setting?it->attendees_setting: attendees_setting;
            prize_setting = -1==prize_setting?it->prize_setting: prize_setting;
            sql::exec(format(UPDATE_LUCKY_SHAKE_SETTTING)%attendees_setting %prize_setting %activeid);
        } else {
            try {
                sql::exec(format(INSERT_LUCKY_SHAKE_SETTTING) %activeid %attendees_setting %prize_setting);
            } catch(...) {
                sql::exec(format("delete from lucky_shake_settting where activity_id=%1%") %activeid);
            }
        }
    } else {
        boost::system::error_code err;
        auto it = global_mgr::inst().find_g_activity(activeid, err);
        if ( !err ){
            attendees_setting = -1==attendees_setting?it->attendees_setting: attendees_setting;
            prize_setting = -1==prize_setting?it->prize_setting: prize_setting;
            sql::exec(format(UPDATE_LUCKY_SHAKE_SETTTING)%attendees_setting %prize_setting %activeid);
        } else {
            try {
            sql::exec(format(INSERT_LUCKY_SHAKE_SETTTING) %activeid %attendees_setting %prize_setting);
            } catch(...) {
                sql::exec(format("delete from lucky_shake_settting where activity_id=%1%") %activeid);
            }
        }
    }
}

void set_beauty_vote(http::request& req, int activeid, client_ptr& cli)
{
    const char* INSERT_BEAUTY_VOTE_SETTTING = "insert into beauty_vote_settting(activity_id,candidate_setting,winners_setting,attendees_setting) values(%1%,%2%,%3%,%4%)";
    const char* UPDATE_BEAUTY_VOTE_SETTTING = "update beauty_vote_settting set candidate_setting=%1%,winners_setting=%2%,attendees_setting=%3% where activity_id=%4%";

    int candidate_setting = lexical_cast<int>(req.param("candidate_setting","-1"));
    int winners_setting = lexical_cast<int>(req.param("winners_setting","-1"));
    int attendees_setting = lexical_cast<int>(req.param("attendees_setting","-1"));

    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(cli->user_id(), ec);
    if ( !ec ) { 
        auto it = pbar->find_activity(activeid);
        if (it != pbar->activities2.end()) {
            candidate_setting = -1==candidate_setting?it->candidate_setting:candidate_setting;
            winners_setting = -1==winners_setting?it->winners_setting:winners_setting;
            attendees_setting = -1==attendees_setting?it->attendees_setting: attendees_setting;
            sql::exec(format(UPDATE_BEAUTY_VOTE_SETTTING)%candidate_setting 
                    %winners_setting %attendees_setting %activeid);
        } else {
            try {
                sql::exec(format(INSERT_BEAUTY_VOTE_SETTTING) %activeid 
                        %candidate_setting %winners_setting %attendees_setting);
            } catch(...) {
                sql::exec(format("delete from beauty_vote_settting where activity_id=%1%") %activeid);
            }
        }

    } else {
        boost::system::error_code err;
        auto it = global_mgr::inst().find_g_activity(activeid, err);
        if ( !err ) {
            candidate_setting = -1==candidate_setting?it->candidate_setting:candidate_setting;
            winners_setting = -1==winners_setting?it->winners_setting:winners_setting;
            attendees_setting = -1==attendees_setting?it->attendees_setting: attendees_setting;
            sql::exec(format(UPDATE_BEAUTY_VOTE_SETTTING)%candidate_setting %winners_setting %attendees_setting %activeid);
        } else {
            sql::exec(format(INSERT_BEAUTY_VOTE_SETTTING) %activeid %candidate_setting %winners_setting %attendees_setting);
        }
    }
}

json::object Service::initBarActivity(http::request& req, client_ptr& cli)
{
    const char* INSERT_BAR_ACTIVE2 = "INSERT INTO bar_activity2(id,UserId,SessionIds," 
        " type,tm_start,tm_end,activity_name,post_img,brief,show_index,city,"
        " zone,screen_img,on_off, share_url, shorten_share, shorten_banner) VALUES(%1%,%2%,'%3%',%4%,"
        " '%5%','%6%','%7%','%8%','%9%',%10%,'%11%','%12%','%13%',%14%,'%15%','%16%','%17%')";

    bool result = false;
    int type = lexical_cast<int>(req.param("type"));
    string activeName = req.param("activeName");
    time_t beginDate = lexical_cast<time_t>(req.param("begin"));
    time_t endDate = lexical_cast<time_t>(req.param("end"));
    string en_top = req.param("en_top","0");
    string banner = req.param("banner","");
    string city = req.param("city","");
    string zone = req.param("zone","");
    string share_url = req.param("share_url","");
    string shorten_share = req.param("shorten_share","");
    string shorten_banner = req.param("shorten_banner","");
    int on_off = lexical_cast<int>(req.param("on_off","0"));
    string sessions = "",brief = "";
    string screen_img = req.param("screen_img", "");
    int is_glob_shake = lexical_cast<int>(req.param("is_glob_shake","0"));
    if ( endDate<time(NULL) ) {
        return json::object()("result", result);
    }
    if ( !req.content().empty() ) {
        LOG_I<<"SessionIds:"<< req.content();
        json::object jo = json::decode(req.content());
        json::array sids = jo.get<json::array>("SessionIds", json::array());
        for ( auto itr=sids.begin(); itr!=sids.end(); ++itr) {
            string sid =  boost::get<string>(*itr);
            if ( !sid.empty() ) { sessions += sid + " ";}
        }
        brief = jo.get<string>("brief","");
    }

    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    client_ptr c = xindex::get(userid);
    int show_index = 0;
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) {
        sessions = pbar->sessionid;
        city = zone = "";
        show_index = pbar->activities2.begin()->show_index + 1;
    }

    int id = s_activity_info::alloc_id();

    try {
        sql::exec(format(INSERT_BAR_ACTIVE2) %id % userid %sql::db_escape(sessions) 
                %type %time_string(beginDate) %time_string(endDate) %sql::db_escape(activeName) 
                %sql::db_escape(banner) %sql::db_escape(brief) %show_index %sql::db_escape(city) 
                %sql::db_escape(zone) %sql::db_escape(screen_img) %on_off %sql::db_escape(share_url) 
                %sql::db_escape(shorten_share) %sql::db_escape(shorten_banner));

        switch ( type ) {
            case s_activity_info::EN_LuckyShake:
                set_lucky_shake(req, id, c);
                result = true;
                break;
            case s_activity_info::EN_VoteBeauty:
                set_beauty_vote(req, id, c);
                result = true;
                break;
            case s_activity_info::EN_NormalActivity:
                result = true;
                break;
            default:
                break;
        }

        if ( is_glob_shake ) {
            add_g_activity_prize(req, id, c);
        } else {
            add_activity_prize(req, id, c);
        }
        if ( !ec ) {
            pbar->activities2.clear();
            s_activity_info::load(str(boost::format("UserId='%1%' order by show_index desc") %userid)
                    , std::back_inserter(pbar->activities2));

            if ( s_activity_info::EN_LuckyShake==type || s_activity_info::EN_VoteBeauty==type) {
                std::string key =  make_activity_msgs_key(id);
                imessage::message m = activityNotifyMsg( pbar, id , ec);
                if ( !ec ) {
                    redis::command("HSET", key, m.id(), impack_activity_msg(m));
                    redis::command("EXPIREAT", key, endDate+ 36*60*60);
                }
            }

            s_bar_info::update_expires_time(pbar);
        } else {
            global_mgr::inst().load_activity(str(boost::format("id=%1% and UserId=%2%")%id %userid));
        }
    } catch ( ... ) {
        sql::exec(format("delete from bar_activity2 where id=%1%")%id);
    }

    return json::object()("result", result);
}

void del_activity_setting(http::request& req, int activeid, client_ptr& cli)
{
    const char* set_table[] = {"","lucky_shake_settting", "beauty_vote_settting"};
    const char* DELETE_BAR_ACTIVITY_SETTING = "delete from %1% where activity_id=%2%";
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(cli->user_id(), ec);
    if ( !ec ) { 
        auto it = pbar->find_activity(activeid);
        if ( it!=pbar->activities2.end() && it->type ) {
            sql::exec(format(DELETE_BAR_ACTIVITY_SETTING)%set_table[it->type] % activeid);
        }
    } else {
        boost::system::error_code err;
        auto it = global_mgr::inst().find_g_activity(activeid, err);
        if ( ! err  && it->type ) {
            sql::exec(format(DELETE_BAR_ACTIVITY_SETTING)%set_table[it->type] % activeid);
        }
    }
}

// GET /deleteBarActivity userid=10206 type= activeid=
json::object Service::deleteBarActivity(http::request& req, client_ptr& cli)
{
    const char* DELETE_BAR_ACTIVE2 = "delete from bar_activity2 where id=%1% and UserId=%2%";
    bool result = false;
    int type = lexical_cast<int>(req.param("type"));
    int activeid = lexical_cast<int>(req.param("activeid"));
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    client_ptr c = xindex::get(userid);
    int is_glob_shake = lexical_cast<int>(req.param("is_glob_shake","0"));
    time_t now = time(NULL);

    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        // auto it = pbar->find_activity(activeid);
        // if ( now > it->time_begin ) return json::object()("result", result);
    } else {
        // boost::system::error_code err;
        // auto it = global_mgr::inst().find_g_activity(activeid, err);
        // if ( now > it->time_begin ) return json::object()("result", result);
    }

    sql::exec(format(DELETE_BAR_ACTIVE2) %activeid %userid);
    switch ( type ) {
        case s_activity_info::EN_LuckyShake:
            {
                if ( is_glob_shake ) {
                    del_g_activity_prize(req, activeid, cli);
                } else {
                    del_activity_prize(req, activeid, c);
                }
                del_activity_setting(req, activeid, c);
                result = true;
            }
            break;
        case s_activity_info::EN_VoteBeauty:
            del_activity_setting(req, activeid, c);
            result = true;
            break;
        case s_activity_info::EN_NormalActivity:
            result = true;
            break;
        default:
            break;
    }

    if (pbar) { 
        pbar->delete_activity(activeid);

        //std::string key;
        //redis::command("DEL", key);
    } else {
        global_mgr::inst().erase_g_activity(activeid);
    }
    
    return json::object()("result", result);
}

json::object Service::changeBarActivityIndex(http::request& req, client_ptr& cli)
{
    const char* UPDATEBARACTIVITYINDEXS = "update bar_activity2 set show_index=show_index+(%1%) where UserId=%2% and show_index>%3% and show_index<%4% and show_index>0";
    const char* UPDATEBARACTIVITYINDEX = "update bar_activity2 set show_index=%1% where id=%2% and UserId=%3%";
    bool result = false;
    int newIndex = lexical_cast<int>(req.param("newindex"));
    int activeid = lexical_cast<int>(req.param("activeid"));
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);

    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( activeid>0 && !ec ) { 
        s_bar_info::activities_type::iterator oldItr = pbar->activities2.end();
        s_bar_info::activities_type::iterator newItr = pbar->activities2.end();

        for (s_bar_info::activities_type::iterator itr=pbar->activities2.begin(); 
                itr != pbar->activities2.end(); ++itr)
        {
            if ( itr->show_index == newIndex ) { newItr = itr; }

            if ( activeid == itr->id ) { oldItr=itr; }

            if ( oldItr!=pbar->activities2.end() 
                    && newItr!=pbar->activities2.end()
                    && newItr!=oldItr )
            {
                int begin=0,end=0,dist=0;
                if ( newIndex >oldItr->show_index ) {
                    begin = oldItr->show_index;
                    end = newIndex+1;
                    dist = -1;

                    for ( s_bar_info::activities_type::iterator itr=oldItr; ; ++itr ) {
                        --(const_cast<s_activity_info&>(*itr).show_index);
                        if ( itr == newItr ) break;
                    }
                } else {
                    begin = newIndex-1;
                    end = oldItr->show_index;
                    dist = 1;

                    for ( s_bar_info::activities_type::iterator itr=newItr; ; ++itr ) {
                        ++(const_cast<s_activity_info&>(*itr).show_index);
                        if ( itr == oldItr ) break;
                    }
                }
                const_cast<s_activity_info&>(*oldItr).show_index = newIndex;
                sql::exec(format(UPDATEBARACTIVITYINDEXS)%dist %userid %begin %end); 
                sql::exec(format(UPDATEBARACTIVITYINDEX)%newIndex %activeid %userid); 
                pbar->activities2.splice( newItr, pbar->activities2, oldItr);
                result = true;
                break;
            }
        }
    }
    
    return json::object()("result", result);
}

json::object Service::updateBarActivity(http::request& req, client_ptr& cli)
{
    const char* UPDATE_BAR_ACTIVE2 = "update bar_activity2 set tm_start='%1%',tm_end='%2%'," 
        " activity_name='%3%',post_img='%4%',brief='%5%',city='%6%',zone='%7%',SessionIds='%8%',"
        " screen_img='%9%', on_off=%10%, share_url='%11%', shorten_share='%12%', shorten_banner='%13%' "
        " where id=%14% and UserId='%15%'";

    bool result = false;
    int type = lexical_cast<int>(req.param("type"));
    int activeid = lexical_cast<int>(req.param("activeid"));
    string activeName = req.param("activeName","");
    time_t beginDate = lexical_cast<time_t>(req.param("begin","0"));
    time_t endDate = lexical_cast<time_t>(req.param("end","0"));
    string top = req.param("en_top","");
    string banner = req.param("banner","");
    string city = req.param("city","");
    string zone = req.param("zone","");
    string share_url = req.param("share_url","");
    string  shorten_share = req.param("shorten_share","");
    string  shorten_banner = req.param("shorten_banner","");
    int on_off = lexical_cast<int>(req.param("on_off","0"));
    string screen_img=req.param("screen_img","");
    int is_glob_shake = lexical_cast<int>(req.param("is_glob_shake","0"));
    string sessions = "", brief ="";
    if ( !req.content().empty() ) {
        LOG_I<<"SessionIds:"<< req.content();
        json::object jo = json::decode(req.content());
        json::array sids = jo.get<json::array>("SessionIds", json::array());
        for ( auto itr=sids.begin(); itr!=sids.end(); ++itr) {
            string sid =  boost::get<string>(*itr);
            if ( !sid.empty() ) { sessions += sid + " ";}
        }
        brief = jo.get<string>("brief","");
    }

    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    time_t now = time(NULL);
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        auto it = pbar->find_activity(activeid);
        if (it != pbar->activities2.end()) {
            if ( now > it->time_begin ) return json::object()("result", result);

            sessions = pbar->sessionid;
            beginDate  = beginDate==0? it->time_begin : beginDate;
            endDate  = endDate==0? it->time_end : endDate;
            share_url = share_url.empty()?it->share_url : share_url;
            shorten_share = shorten_share.empty()?it->shorten_share : shorten_share;
            shorten_share = shorten_share.empty()?share_url:shorten_share;

            activeName = activeName.empty()? it->activity_name : activeName;
            brief = brief.empty()? it->brief : brief;
            // int en_top = top.empty()? 0 : lexical_cast<int>(top);
            string ban = url_to_relative( it->post_img );
            ban = ban.empty()? it->post_img : ban;
            banner = banner.empty()? ban : banner;
            shorten_banner = shorten_banner.empty()?it->shorten_banner : shorten_banner;
            shorten_banner = shorten_banner.empty()?banner:shorten_banner;
            city = zone = "";
        }
    } else {
        boost::system::error_code err;
        auto it = global_mgr::inst().find_g_activity(activeid, err);
        if ( !err ) {
            // if ( now > it->time_begin ) return json::object()("result", result);

            beginDate  = beginDate==0? it->time_begin : beginDate;
            endDate  = endDate==0? it->time_end : endDate;

            activeName = activeName.empty()? it->activity_name : activeName;
            brief = brief.empty()? it->brief : brief;
            // int en_top = top.empty()? 0 : lexical_cast<int>(top);
            string ban = url_to_relative( it->post_img );
            ban = ban.empty()? it->post_img : ban;
            banner = banner.empty()? ban : banner;

            string screen = url_to_relative( it->screen_img );
            screen = screen.empty()? it->screen_img : screen;
            screen_img = screen_img.empty()? screen : screen_img;

            city = city.empty()? it->city : city;
            zone = zone.empty()? it->zone : zone;
            if ( sessions.empty() ) {
                for ( auto i=it->SessionIds.begin(); i!=it->SessionIds.end(); ++i) {
                    sessions += *i + " ";
                }
            }
        }
    }
    sql::exec(format(UPDATE_BAR_ACTIVE2)%time_string(beginDate) %time_string(endDate) 
            %sql::db_escape(activeName) %sql::db_escape(banner) %sql::db_escape(brief) 
            %sql::db_escape(city) %sql::db_escape(zone) %sql::db_escape(sessions) 
            %sql::db_escape(screen_img) %on_off %sql::db_escape(share_url) 
            %sql::db_escape(shorten_share) %sql::db_escape(shorten_banner) %activeid %userid );

    switch ( type ) {
        case s_activity_info::EN_LuckyShake:
            set_lucky_shake(req, activeid, c);
            result = true;
            break;
        case s_activity_info::EN_VoteBeauty:
            set_beauty_vote(req, activeid, c);
            result = true;
            break;
        case s_activity_info::EN_NormalActivity:
            result = true;
            break;
        default:
            break;
    }

    if ( result ) {
        if ( is_glob_shake ) {
            add_g_activity_prize(req, activeid, c);
        } else {
            add_activity_prize(req, activeid, c);
        }
        if ( !ec ) {
            pbar->activities2.clear();
            s_activity_info::load(str(boost::format("UserId='%1%' order by show_index desc") %userid)
                    , std::back_inserter(pbar->activities2));

            if ( s_activity_info::EN_LuckyShake==type || s_activity_info::EN_VoteBeauty==type) {
                std::string key = make_activity_msgs_key(activeid);
                redis::command("DEL", key);
                imessage::message m = activityNotifyMsg( pbar, activeid, ec);
                if ( !ec ) {
                    redis::command("HSET", key, m.id(), impack_activity_msg(m));
                    redis::command("EXPIREAT", key, endDate+ 36*60*60);
                }
            }

            s_bar_info::update_expires_time(pbar);
        } else {
            global_mgr::inst().erase_g_activity(activeid);
            global_mgr::inst().load_activity(str(boost::format("id=%1% and UserId=%2%")%activeid %userid));
        }
    }

    return json::object()("result", result);
}

json::object Service::initBarAdvertising(http::request& req, client_ptr& cli)
{
    const char* INSERT_ADVERTISING = "INSERT INTO advertising(id,UserId,name,A_image1,A_size1,"
          " A_image2,A_size2,I_image1,I_size1,I_image2,I_size2,beginDate,endDate,city,zone,"
          " SessionIds, is_default) VALUES(%1%,%2%,'%3%','%4%','%5%','%6%','%7%','%8%','%9%',"
          " '%10%','%11%','%12%','%13%','%14%','%15%','%16%', %17%)";

    bool result = false;
    string name = req.param("advertisingName");
    time_t beginDate = lexical_cast<time_t>(req.param("begin"));
    time_t endDate = lexical_cast<time_t>(req.param("end"));
    string city = req.param("city","");
    string zone = req.param("zone","");
    int is_default = lexical_cast<int>(req.param("is_default","0"));
    string A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2;
    string sessions = "";
    if ( endDate<time(NULL) ) {
        return json::object()("result", result);
    }
    if ( !req.content().empty() ) {
        LOG_I<<"SessionIds:"<< req.content();
        json::object jo = json::decode(req.content());
        A_image1 = jo.get<string>("A_image1","");
        A_size1 = jo.get<string>("A_size1","");
        A_image2 = jo.get<string>("A_image2","");
        A_size2 = jo.get<string>("A_size2","");
        I_image1 = jo.get<string>("I_image1","");
        I_size1 = jo.get<string>("I_size1","");
        I_image2 = jo.get<string>("I_image2","");
        I_size2 = jo.get<string>("I_size2","");

        json::array sids = jo.get<json::array>("SessionIds", json::array());
        for ( auto itr=sids.begin(); itr!=sids.end(); ++itr) {
            string sid =  boost::get<string>(*itr);
            if ( !sid.empty() ) { sessions += sid + " ";}
        }
    }

    if ( A_image1.empty() || A_size1.empty() || A_image2.empty() || A_size2.empty()
            || I_image1.empty() || I_size1.empty() || I_image2.empty() || I_size2.empty() )
    {
        return json::object()("result", result);
    }

    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) {
        sessions = pbar->sessionid;
        city = zone = "";
    }

    int id = advertising::alloc_id();
    sql::exec(format(INSERT_ADVERTISING) %id %userid %sql::db_escape(name) 
            %sql::db_escape(A_image1) %sql::db_escape(A_size1) %sql::db_escape(A_image2) 
            %sql::db_escape(A_size2) %sql::db_escape(I_image1) %sql::db_escape(I_size1) 
            %sql::db_escape(I_image2) %sql::db_escape(I_size2) %time_string(beginDate) 
            %time_string(endDate) %sql::db_escape(city) %sql::db_escape(zone) 
            %sql::db_escape(sessions) %is_default); 

    if ( is_default ) {
        sql::exec(format(UPDATE_ADVERTISING_DEFAULT) %userid %id);
    }
    if ( !ec ) {
        s_bar_info::bar_advertising::load(str(boost::format("id=%1% and UserId='%2%'")%id %userid)
                , std::back_inserter(pbar->bar_advers_));
    }else {
        global_mgr::inst().load_adver(str(boost::format("id=%1% and UserId=%2%")%id %userid));
    }
    result = true;

    return json::object()("result", result);
}

json::object Service::deleteBarAdvertising(http::request& req, client_ptr& cli)
{
    const char* DELETE_ADVERTISING = "delete from advertising where id=%1% and UserId='%2%'";

    bool result = false;
    int advertisingid = lexical_cast<int>(req.param("advertisingid"));

    // UID userid = cli->user_id();
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        for ( auto it=pbar->bar_advers_.begin(); it!=pbar->bar_advers_.end(); ++it)
        {
            if ( it->id == advertisingid ) {
                pbar->bar_advers_.erase(it);
                result = true;
                break;
            }
        }
    } else {
        global_mgr::inst().erase_g_advert( advertisingid );
        result = true;
    }
    
    sql::exec(format(DELETE_ADVERTISING) %advertisingid %userid);

    return json::object()("result", result);
}

json::object Service::updateBarAdvertising(http::request& req, client_ptr& cli)
{
    const char* UPDATE_ADVERTISING = "UPDATE advertising SET name='%1%',A_image1='%2%',"
        " A_size1='%3%',A_image2='%4%',A_size2='%5%',I_image1='%6%',I_size1='%7%'," 
        " I_image2='%8%',I_size2='%9%',beginDate='%10%', endDate='%11%',city='%12%'," 
        " zone='%13%',SessionIds='%14%',is_default=%15% "
        " WHERE id=%16% and UserId=%17%";
   
    bool result = false;
    int advertisingid = lexical_cast<int>(req.param("advertisingid"));
    string name = req.param("advertisingName","");
    time_t beginDate = lexical_cast<time_t>(req.param("begin","0"));
    time_t endDate = lexical_cast<time_t>(req.param("end","0"));
    string city = req.param("city","");
    string zone = req.param("zone","");
    int is_default = lexical_cast<int>(req.param("is_default","0"));
    string A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2;
    string sessions = "";
    if ( !req.content().empty() ) {
        LOG_I<<"Advertising:"<< req.content();
        json::object jo = json::decode(req.content());
        A_image1 = jo.get<string>("A_image1","");
        A_size1 = jo.get<string>("A_size1","");
        A_image2 = jo.get<string>("A_image2","");
        A_size2 = jo.get<string>("A_size2","");
        I_image1 = jo.get<string>("I_image1","");
        I_size1 = jo.get<string>("I_size1","");
        I_image2 = jo.get<string>("I_image2","");
        I_size2 = jo.get<string>("I_size2","");

        json::array sids = jo.get<json::array>("SessionIds", json::array());
        for ( auto itr=sids.begin(); itr!=sids.end(); ++itr) {
            string sid =  boost::get<string>(*itr);
            if ( !sid.empty() ) { sessions += sid + " ";}
        }
    }

    // UID userid = cli->user_id();
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        for ( std::vector<s_bar_info::bar_advertising>::iterator it=pbar->bar_advers_.begin(); 
                it!=pbar->bar_advers_.end(); ++it) 
        {
            if ( it->id == advertisingid ) {
                name = name.empty() ? it->name: name;

                std::string _image_1 = url_to_relative(it->A_image1);
                _image_1 = _image_1.empty()? it->A_image1:_image_1;
                A_image1 = A_image1.empty()? _image_1:A_image1; 
                A_size1 = A_size1.empty()?it->A_size1: A_size1;

                std::string _image_2 = url_to_relative(it->A_image2);
                _image_2 = _image_2.empty()? it->A_image2:_image_2;
                A_image2 = A_image2.empty()?_image_2:A_image2;
                A_size2 = A_size2.empty()?it->A_size2:A_size2;

                std::string _image_3 = url_to_relative(it->I_image1);
                _image_3 = _image_3.empty()? it->I_image1:_image_3;
                I_image1 = I_image1.empty()?_image_3:I_image1;
                I_size1 = I_size1.empty()?it->I_size1:I_size1;

                std::string _image_4 = url_to_relative(it->I_image2);
                _image_4 = _image_4.empty()? it->I_image2:_image_4;
                I_image2 = I_image2.empty()?_image_4:I_image2;
                I_size2 = I_size2.empty()?it->I_size2:I_size2;

                beginDate = beginDate==0 ? it->beginDate:beginDate;
                endDate = endDate==0 ? it->endDate:endDate;
                sessions = pbar->sessionid;
                pbar->bar_advers_.erase(it);
                city = zone = "";
                break;
            }
        }
    } else { 
        boost::system::error_code err;
        global_mgr::global_adver_type::iterator it = global_mgr::inst().find_g_advert(advertisingid, err);
        if ( !err ) {
            name = name.empty() ? it->name: name;

            std::string _image_1 = url_to_relative(it->A_image1);
            _image_1 = _image_1.empty()? it->A_image1:_image_1;
            A_image1 = A_image1.empty()? _image_1:A_image1; 
            A_size1 = A_size1.empty()?it->A_size1: A_size1;

            std::string _image_2 = url_to_relative(it->A_image2);
            _image_2 = _image_2.empty()? it->A_image2:_image_2;
            A_image2 = A_image2.empty()?_image_2:A_image2;
            A_size2 = A_size2.empty()?it->A_size2:A_size2;

            std::string _image_3 = url_to_relative(it->I_image1);
            _image_3 = _image_3.empty()? it->I_image1:_image_3;
            I_image1 = I_image1.empty()?_image_3:I_image1;
            I_size1 = I_size1.empty()?it->I_size1:I_size1;

            std::string _image_4 = url_to_relative(it->I_image2);
            _image_4 = _image_4.empty()? it->I_image2:_image_4;
            I_image2 = I_image2.empty()?_image_4:I_image2;
            I_size2 = I_size2.empty()?it->I_size2:I_size2;

            beginDate = beginDate==0 ? it->beginDate:beginDate;
            endDate = endDate==0 ? it->endDate:endDate;

            if ( sessions.empty() ) {
                for ( auto i=it->SessionIds.begin(); i!=it->SessionIds.end(); ++i) {
                    sessions += *i + " ";
                }
            }
            city = city.empty() ? it->city: city;
            zone = zone.empty() ? it->zone: zone;
            global_mgr::inst().erase_g_advert(advertisingid);
        }
    }

    if ( A_image1.empty() || A_size1.empty() || A_image2.empty() || A_size2.empty()
            || I_image1.empty() || I_size1.empty() || I_image2.empty() || I_size2.empty() )
    {
        return json::object()("result", result);
    }

    sql::exec(format(UPDATE_ADVERTISING) %sql::db_escape(name) %sql::db_escape(A_image1) 
            %sql::db_escape(A_size1) %sql::db_escape(A_image2) %sql::db_escape(A_size2) 
            %sql::db_escape(I_image1) %sql::db_escape(I_size1) %sql::db_escape(I_image2) 
            %sql::db_escape(I_size2) %time_string(beginDate) %time_string(endDate) 
            %sql::db_escape(city) %sql::db_escape(zone) %sql::db_escape(sessions) 
            %is_default %advertisingid %userid); 

    if ( is_default ) {
        sql::exec(format(UPDATE_ADVERTISING_DEFAULT) %userid %advertisingid);
    }

    if ( !ec ) {
        s_bar_info::bar_advertising::load(str(boost::format("id=%1% and UserId='%2%'")%advertisingid %userid)
                , std::back_inserter(pbar->bar_advers_));
    } else {
        global_mgr::inst().load_adver(str(boost::format("id=%1% and UserId=%2%")%advertisingid %userid));
    }
    result = true;

    return json::object()("result", result);
}

json::object Service::initBarCoupons(http::request& req, client_ptr& cli)
{
    const char* INSERT_COUPONS = "INSERT INTO coupons(id,SessionId,name,price,count," 
        " initial_count,brief,beginDate,endDate,enable) " 
        " VALUES(%1%,'%2%','%3%','%4%','%5%','%5%','%6%','%7%','%8%',%9%)";
    bool result = false;
    string name = req.param("couponName");
    int price = lexical_cast<int>(req.param("price"));
    int count = lexical_cast<int>(req.param("count"));
    time_t beginDate = lexical_cast<time_t>(req.param("begin"));
    time_t endDate = lexical_cast<time_t>(req.param("end"));
    int enable = lexical_cast<int>(req.param("enable","1"));
    if ( endDate<time(NULL) ) {
        return json::object()("result", result);
    }
    string brief;
    if ( !req.content().empty() ) {
        LOG_I<<"Coupons:"<< req.content();
        json::object jo = json::decode(req.content());
        brief = jo.get<string>("brief","");
    }

    // UID userid = cli->user_id();
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        int id = s_bar_info::bar_coupon::alloc_id();
        sql::exec(format(INSERT_COUPONS) %id %pbar->sessionid %sql::db_escape(name) %price %count %sql::db_escape(brief) %time_string(beginDate) %time_string(endDate) %enable); 
        s_bar_info::bar_coupon::load(str(boost::format("id=%1% and SessionId='%2%'")%id %pbar->sessionid)
                , std::back_inserter(pbar->bar_coupons_));
        if ( 1 == pbar->bar_coupons_.size() ){
            std::vector<bar_ptr>& list=bars_mgr::inst().get_coupon_bar_list();
            list.push_back(pbar);
        }
        result = true;
    }

    return json::object()("result", result);
}

json::object Service::deleteBarCoupons(http::request& req, client_ptr& cli)
{
    bool result = false;
    const char* DELETE_COUPONS = "DELETE FROM coupons WHERE id=%1% AND SessionId='%2%'";
    int couponid = lexical_cast<int>(req.param("couponid"));

    // UID userid = cli->user_id();
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        for ( std::vector<s_bar_info::bar_coupon>::iterator it=pbar->bar_coupons_.begin(); 
                it!=pbar->bar_coupons_.end(); ++it) 
        {
            if ( it->id == couponid ) { pbar->bar_coupons_.erase(it); }
            if ( 0 == pbar->bar_coupons_.size() ) {
                std::vector<bar_ptr>& list=bars_mgr::inst().get_coupon_bar_list();
                list.erase(remove(list.begin(), list.end(), pbar), list.end());
            }
            sql::exec(format(DELETE_COUPONS) %couponid %pbar->sessionid);
            result = true;
            break;
        }
    }

    return json::object()("result", result);
}

json::object Service::updateBarCoupons(http::request& req, client_ptr& cli)
{
    const char* UPDATE_COUPONS = "UPDATE coupons SET name='%1%',price='%2%',count='%3%',"
        " initial_count='%3%',brief='%4%', beginDate='%5%', endDate='%6%',enable=%7% " 
        " WHERE id=%8% AND SessionId='%9%'";

    bool result = false;
    int couponid = lexical_cast<int>(req.param("couponid"));
    string name = req.param("couponName","");
    int price = lexical_cast<int>(req.param("price","0"));
    int count = lexical_cast<int>(req.param("count","-1"));
    time_t beginDate = lexical_cast<time_t>(req.param("begin","0"));
    time_t endDate = lexical_cast<time_t>(req.param("end","0"));
    int enable = lexical_cast<int>(req.param("enable","1"));
    string brief;
    if ( !req.content().empty() ) {
        LOG_I<<"Coupons:"<< req.content();
        json::object jo = json::decode(req.content());
        brief = jo.get<string>("brief","");
    }

    // UID userid = cli->user_id();
    UID userid = lexical_cast<UID>(req.param("userid","2000"));
    // client_ptr c = xindex::get(userid);
    boost::system::error_code ec;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_bar_byuid(userid, ec);
    if ( !ec ) { 
        for ( std::vector<s_bar_info::bar_coupon>::iterator it=pbar->bar_coupons_.begin(); 
                it!=pbar->bar_coupons_.end(); ++it) 
        {
            if ( it->id == couponid ) {
                if ( time(NULL) > it->endDate ) return json::object()("result", result);

                it->name = name.empty() ? it->name: name;
                it->price = price==0? it->price: price;
                it->count = count<0? it->count: count;
                it->brief = brief.empty() ? it->brief: brief;
                it->beginDate = beginDate==0? it->beginDate: beginDate;
                it->endDate = endDate==0? it->endDate: endDate;
                sql::exec(format(UPDATE_COUPONS)%sql::db_escape(it->name) %it->price 
                        %it->count %sql::db_escape(it->brief) %time_string(it->beginDate) 
                        %time_string(it->endDate) %enable %couponid %pbar->sessionid ); 

                if ( 0 == enable ) {
                    pbar->bar_coupons_.erase(it);
                } 
                result = true;
                break;
            }
        }
    }

    return json::object()("result", result);
}

struct gender_pred
{
    char const* gender;
    gender_pred(char const* a) { gender=a; }
    bool operator()(client_ptr const & c) const { return (!gender || gender==c->user_gender()); }
};

// POST /sendBarFans userid=15252 type=3
json::object Service::sendBarFans(http::request & req, client_ptr& cli)
{
    // UID uid = cli->user_id();
    UID userid = lexical_cast<UID>(req.param("userid"));
    client_ptr bcli = xindex::get(userid);
    bar_ptr bptr = bars_mgr::inst().get_bar_byuid( userid );

    if (!bptr)
        return json::object()("error","No bar for you");

    int msgtype = lexical_cast<int>(req.param("type"));
    int category = lexical_cast<int>(req.param("category","0")); //0,all; 1,male; 2 female
    enum { MSG_PIC = 1, MSG_AUDIO, MSG_TEXT, MSG_CARTOON, MSG_PIC_TXT, MSG_ACTIVITY};
    const char* stype[] = { "", "chat/image", "chat/audio", "chat/text","chat/cartoon","chat/pic_txt","chat/activity" };

    json::object body;
    switch (msgtype)
    {
    case MSG_PIC:
    case MSG_AUDIO:
        {
            std::string file = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
            file = complete_url(write_file(file, req.content()));
            if ( !file.empty() ) { body("content", file); }
        }
        break;
    case MSG_PIC_TXT:
        {
            LOG_I<<req.content();
            json::object jo = json::decode(req.content());
            string image = jo.get<string>("image");
            jo("image", complete_url(image));
            if ( !jo.empty() ) { body("content", jo); }
        } 
        break;
    case MSG_ACTIVITY:
        {
            LOG_I<<req.content();
            json::object jo = json::decode(req.content());
            string image = jo.get<string>("image");
            jo("image", complete_url(image));
            if ( !jo.empty() ) { body("content", jo);}
        }
        break;
    case MSG_TEXT:
    case MSG_CARTOON:
        if ( ! ( req.content().empty()) ){ 
            body("content", req.content()); 
        }
        break;
    default:
        THROW_EX(EN_Input_Data);
    }
    
    {
        bptr->trader()->user_info().put("public_flag", 1); //fix public_flag lose where login
    }

    if ( !body.empty() ) {
        imessage::message msg(stype[msgtype], userid, "."); 

        const char* gender = NULL;
        if (category == 1)
            gender = "M";
        else if (category==2)
            gender = "F";
        msg.body = body;
        sendto(msg, bptr->fans2, gender_pred(gender));

        return json::object()("id", msg.id());
    }

    return json::object();
}

json::object Service::merchantChat(http::request & req, client_ptr& cli)
{
    UID userid = lexical_cast<UID>(req.param("userid"));
    UID otherid = lexical_cast<UID>(req.param("otherid"));
    int msgtype = lexical_cast<int>(req.param("type"));

    string cont, url;
    int id=0;

    enum { MSG_PIC = 1, MSG_AUDIO, MSG_TEXT, MSG_CARTOON};
    const char* ms[] = { "", "chat/image", "chat/audio", "chat/text","chat/cartoon" };

    switch (msgtype)
    {
    case MSG_PIC:
    case MSG_AUDIO:
        {
            if(cli->is_guest()) THROW_EX(EN_Operation_NotPermited);
            string path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
            url = cont = complete_url(write_file(path, req.content()));
        }
        break;
    case MSG_TEXT:
    case MSG_CARTOON:
        cont = req.content();
        break;
    default:
        THROW_EX(EN_Input_Data);
    }

    if(!cont.empty()){
        imessage::message msg(ms[msgtype], userid, make_p2pchat_id(userid, otherid)); 
        msg.a = otherid;
        msg.body
            ("content", cont)
            // ("from", cli->brief_user_info())
            // ("time", time_string())
            ;
        if (otherid != INVALID_UID)
            msg.body.put("to", otherid);

        chatmgr::inst().send(msg, cli);
        id = msg.id();
    }

    return json::object()("url", url)("id", id);
}

// POST /publicNotify type=3
json::object Service::publicNotify(http::request & req, client_ptr& cli)
{
    int msgtype = lexical_cast<int>(req.param("type"));
    UID userid = cli->user_id();
    string cont, url;
    int id=0;

    enum { MSG_PIC = 1, MSG_AUDIO, MSG_TEXT, MSG_CARTOON};
    const char* ms[] = { "", "chat/image", "chat/audio", "chat/text","chat/cartoon" };
    if( SYSADMIN_UID != userid ) THROW_EX(EN_Operation_NotPermited);

    switch (msgtype)
    {
    case MSG_PIC:
    case MSG_AUDIO:
        {
            string path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
            url = cont = complete_url(write_file(path, req.content()));
        }
        break;
    case MSG_TEXT:
    case MSG_CARTOON:
        cont = req.content();
        break;
    default:
        THROW_EX(EN_Input_Data);
    }

    if(!cont.empty()){
        imessage::message msg(ms[msgtype], userid, ".");
        msg.body
            ("content", cont)
            ;
        id = msg.id();

        store_public_messages(msg);
    }

    return json::object()("url", url)("id", id);
}

json::object Service::exchangeCoins(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID exchanger = lexical_cast<UID>(req.param("exchanger"));
    int count = lexical_cast<int>(req.param("count"));
    int pid = lexical_cast<int>(req.param("pid"));

    if( SYSADMIN_UID != userid ) THROW_EX(EN_Operation_NotPermited);

    bool result = false;
    if ( count > 0 ) {
        if ( client_ptr c = xindex::get(exchanger) ) {
            sql::exec(format("UPDATE IndividualDatas SET money=money+%1% "
                        " WHERE UserId=%2%")%count %exchanger);
            sql::exec(format("UPDATE personal_activity_result SET accepted=1 "
                        " WHERE id=%1%")% pid);
            result = true;
        }
    }

    return json::object()("result", result);
}

json::object Service::getBarSession(http::request& req, client_ptr& cli)
{
    std::string session;
    const char* get_session = "select SessionId from bars where assist_id=%1%";
    sql::datas datas(format( get_session ) % cli->user_id());
    if( sql::datas::row_type row = datas.next() ) {
        session = row.at(0,"");
    }

    return json::object()("session", session);
}

json::object Service::noticeScreen(http::request& req, client_ptr& cli)
{
    // const char* get_activity_result = "SELECT t1.name,t1.img,t1.brief,t1.level,t2.UserId,t2.PrizeType "
    //     " FROM bar_activity_prize t1 JOIN  personal_activity_result t2 " 
    //     " ON t1.id=t2.PrizeId WHERE t1.activity_id=%1% order by t1.level";

    const char* UPDATE_BAR_ACTIVITY2 = "update bar_activity2 set on_off=%1% %2%";

    UID userid = lexical_cast<UID>(req.param("userid"));
    int activeid = lexical_cast<UID>(req.param("activeid"));
    int on_off = lexical_cast<UID>(req.param("on_off"));

    bar_ptr pbar = bars_mgr::inst().get_bar_byuid(userid);
    auto it = pbar->find_activity(activeid);
    if (it != pbar->activities2.end()) {
        json::array activity_data, prize;
        json::object active;

        BOOST_FOREACH(const auto&a, it->awards) {
            prize(json::object()("id", a.id)
                          ("count", a.count)
                          ("name", a.name)
                          ("icon", a.img)
                          // ("brief", a.brief)
                          );
        }

        active("activeid", activeid)
        ("sessionid", pbar->sessionid)
        ("banner", it->post_img)
        ("type", it->type)
        ("activeInfo", it->brief)
        ("activeName", it->activity_name)
        ("prize", prize)
        ("beginDate", time_string(it->time_begin))
        ("endDate", time_string(it->time_end))
        ("screen_img", complete_url(it->screen_img))
        ("system_time", time_string());

        if ( 1 == on_off ) {
            // if ( s_activity_info::EN_LuckyShake == it->type ) {
            //     sql::datas datas(format( get_activity_result ) %activeid);
            //     while(sql::datas::row_type row = datas.next()){
            //         string name = row.at(0,"");
            //         string img = row.at(1,"");
            //         string brief = row.at(2,"");
            //         int level = lexical_cast<int>(row.at(3,"0"));
            //         UID uid = lexical_cast<UID>(row.at(4,"0"));
            //         int PrizeType = lexical_cast<int>(row.at(5));
            //         if ( 0 == uid ) { continue;}

            //         client_ptr c = xindex::get(uid);
            //         if ( c ) {
            //             json::object obj (c->brief_user_info());
            //             obj("PrizeType", PrizeType)
            //                 ("name", name)
            //                 ("img", complete_url(img))
            //                 ("level", level)
            //                 ("brief", brief);
            //             activity_data(obj);
            //         }
            //     }
            //     active("activity_data", activity_data);
            // } else if ( s_activity_info::EN_VoteBeauty == it->type ) {
            //     BOOST_FOREACH(auto const & p , it->shows2){
            //         client_ptr c = xindex::get(p.actor);
            //         if (c) {
            //             activity_data(json::object(c->brief_user_info())
            //                     ("userid", p.actor)
            //                     ("supports", p.countof_voter)
            //                     ("icons",GetIndividualAlbum(p.actor)));
            //         }
            //     }
            //     active("activity_data", activity_data);
            // } else {
            //     active("activity_data", complete_url(it->screen_img));

            sql::exec( format(UPDATE_BAR_ACTIVITY2) %0 % str( format("where id!=%1%") %activeid));
            sql::exec( format(UPDATE_BAR_ACTIVITY2) %on_off % str( format("where id=%1%") %activeid));
        } else {
            sql::exec( format(UPDATE_BAR_ACTIVITY2) %on_off % str( format("where id=%1%") %activeid));
        }

        imessage::message msg("activity/notify", SYSADMIN_UID, 
                make_p2pchat_id(SYSADMIN_UID, pbar->assist_id)); 
        msg.body
            ("on_off", on_off)
            ("content", active);
        chatmgr::inst().send(msg, sysadmin_client_ptr());
    }

    return json::object();
}

