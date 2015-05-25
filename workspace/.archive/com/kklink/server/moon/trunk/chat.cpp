#include "myconfig.h"
#include <algorithm>
#include <string>
#include <vector>
// #include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include "dbc.h"
#include "chat.h"
#include "bars.h"

using namespace std;

namespace {
} // namespace
Chat_Error_Category::Chat_Error_Category()
{
    static code_string ecl[] = {
        { EN_Group_NotFound, "聊天群组不存在" },
        { EN_Group_Exist, "聊天群组已经存在" },
        { EN_Group_Invalid, "无效的聊天群组名" },
        { EN_NotMember, "不是聊天群组成员" },
    };

    Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])], __FILE__);
}

chatmgr & chatmgr::inst()
{
    static chatmgr ins;
    return ins;
}

void chatmgr::send(const imessage::message & msg, client_ptr & cli, std::vector<UID>filter)
{
    if (is_p2pchat(msg.gid))
    {
        boost::regex re("[.!](\\d+)[.!](\\d+)([.!](\\d+))?");
        // boost::regex re("[.!](\\d+)[.!](\\d+)");
        boost::smatch subs;
        if (!boost::regex_match(msg.gid, subs, re))
        {
            MYTHROW(EN_Group_Invalid, Chat_Error_Category);
        }

        try
        {
            vector<UID> memb;
            memb.push_back( boost::lexical_cast<UID>(subs[1]) );
            memb.push_back( boost::lexical_cast<UID>(subs[2]) );
            ::chat_group cg(msg.gid,"", memb);
            cg.send(msg, cli);
        }
        catch (std::exception const & e)
        {
            LOG_I << e.what() << "\n"
                << msg.gid << " " << subs[1] << " " << subs[2];
        }

        return;
    }

    this->chat_group(msg.gid).send(msg, cli);
}

chatmgr::iterator chatmgr::load(const groupid_type& gid)
{
    UID ctor;
    string name;
    std::vector<UID> membs;
    {
        boost::format SELECT_CG("SELECT CreatorId,SessionName FROM sessions WHERE SessionId='%1%'");
        sql::datas datas(SELECT_CG % gid);
        sql::datas::row_type row = datas.next();
        if (!row)
        {
            return end();
        }

        ctor = boost::lexical_cast<UID>(row.at(0));
        name = row.at(1,"");
    }
    if (is_groupchat(gid))
    {
        boost::format SELECT_MEMBERS ("SELECT UserId FROM SessionMembers WHERE SessionId='%1%'");
        sql::datas datas(SELECT_MEMBERS % gid);
        while (sql::datas::row_type row = datas.next())
        {
            membs.push_back( boost::lexical_cast<UID>(row.at(0)) );
        }
    }

    std::pair<iterator,bool> res = insert( make_pair(gid, ::chat_group(gid, name, membs)) );
    ::chat_group & cg = res.first->second;
    if (is_barchat(gid))
    {
        boost::format SELECT_MESSAGES("SELECT id,UserId,MsgType,content,UNIX_TIMESTAMP(MsgTime)"
                " FROM messages WHERE SessionId='%1%' ORDER BY id DESC LIMIT 20");
        sql::datas datas(SELECT_MESSAGES % gid);
        while (sql::datas::row_type row = datas.next())
        {
            time_t utime = boost::lexical_cast<time_t>(row.at(4));
            if (utime + (60*60*1) > time(0))
            {
                unsigned int id = boost::lexical_cast<unsigned int>(row.at(0));
                UID uid = boost::lexical_cast<UID>(row.at(1));
                char const * msgtype = row.at(2);
                char const * cont = row.at(3);

                imessage::message m(id, msgtype, uid, gid, utime);
                m.body = json::decode(cont);

                cg.messages_.push_front(m);
            }
        }

        bar_ptr pbar = bars_mgr::inst().get_session_bar(gid);
        cg.members_.insert( pbar->assist_id ); // cg.add_quiet( pbar->assist_id );
    }

    return res.first;
}

::chat_group * chatmgr::chat_group(boost::system::error_code * ec, const groupid_type & gid)
{
    iterator i = find(gid);
    if (i == end())
    {
        i = load(gid);
        if (i == end())
        {
            if (ec)
            {
                *ec = boost::system::error_code(
                        EN_Group_NotFound, Error_category::inst<Chat_Error_Category>());
                return 0;
            }
            MYTHROW(EN_Group_NotFound, Chat_Error_Category);
        }
    }
    return &i->second;
}

void chat_group::save_new_member(groupid_type const & gid, UID uid)
{
    const char* INSERT_SESSIONMEMBERS2 = "INSERT SessionMembers(UserId,SessionId) VALUES(%1%,'%2%')";
    sql::exec(boost::format(INSERT_SESSIONMEMBERS2) % uid % gid);
}

chat_group::chat_group(const std::string & id, const std::string & name, const std::vector<UID>& membs)
    : gid_(id)
    , name_(name)
    , alive_members_(membs.begin(), membs.end())
    , members_(membs.begin(), membs.end())
{ 
    // creator_ = ctor; 
    // if (is_normal_uid(ctor)) members_.push_back(ctor); 

    message_count_ = 0;
    // visitor_count_ = male_count_ = female_count_ = 0;
}

chat_group::chat_group(const std::string & id, const std::string & name)
    : gid_(id)
    , name_(name)
{ 
    // creator_ = ctor; 
    // if (is_normal_uid(ctor)) members_.push_back(ctor); 

    message_count_ = 0;
    // visitor_count_ = male_count_ = female_count_ = 0;
}

void chat_group::member_required(client_ptr & cli)
{
    if (is_normal_uid(cli->user_id())
            && !is_alive_member(cli->user_id()))
    {
        LOG_I << cli <<" "<< gid_ << " add alive_members_:"<< alive_members_;
        MYTHROW(EN_NotMember, Chat_Error_Category);
    }
}

void chat_group::send(const imessage::message & msg, client_ptr & cli, std::vector<UID>filter)
{
    if (!is_alive_member(msg.from))
    {
        LOG_I << cli << " " << gid_ << ":" << members_;
        MYTHROW(EN_NotMember, Chat_Error_Category);
    }
    // bool spot = boost::ends_with(msg.type, "spot");

    boost::format fmt("INSERT INTO messages(id,SessionId,UserId,PeerId,MsgType,content)"
            " VALUES(%1%,'%2%',%3%,%4%,'%5%','%6%')");
    sql::exec(fmt % msg.id() % msg.gid % msg.from % msg.a % msg.type % sql::db_escape(json::encode(msg.body)));
    ++message_count_;

    bool ing = false;

    if ( filter.empty() ) {
        for (auto i = members_.begin(); i != members_.end(); ++i)
        {
            if (*i != msg.from)
                Client::pushmsg(*i, msg);
            if (*i == msg.a)
                ing = true;
        }
    } else {
        std::vector<UID> tmp(members_.begin(), members_.end()), mem(members_.size());
        std::sort( filter.begin(), filter.end() );
        std::sort( tmp.begin(), tmp.end() );
        std::vector<UID>::iterator it = std::set_difference(tmp.begin(), tmp.end(), filter.begin(), filter.end(), mem.begin());
        mem.resize( it-mem.begin() );
        for (std::vector<UID>::iterator i=mem.begin(); i!=mem.end(); ++i) {
            if (*i != msg.from) {
                Client::pushmsg(*i, msg);
            }

            if (*i == msg.a) {
                ing = true;
            }
        }
    }

    if (is_barchat(gid_))
    {
        if (imessage::is_usermsg(msg.type) || msg.type == "chat/gift")
        {
            if (messages_.size() >= 20)
                messages_.pop_front();
            messages_.push_back(msg);
        }
    }

    if (msg.a && !ing)
    {
        // imessage::message cpy = msg;
        // cpy.gid = make_p2pchat_id(msg.a, msg.from);
        Client::pushmsg(msg.a, msg);
    }

    LOG_I << cli <<" "<< gid_ << " send " << msg.id() <<" "<< msg.from <<" "<< msg.type << ":" << members_;
}

// void chatmgr::remove(const groupid_type& gid, UID uid)
// {
//     iterator i = find(gid);
//     if (i != end())
//     {
//         i->second.remove(uid);
//     }
// }

// void chatmgr::add(const groupid_type& gid, const std::vector<UID> & uids)
// {
//     chat_group(gid).add(uids);
// }

// void chat_group::add(UID uid, client_ptr & cli, bool quiet)
// {
//     // if (quiet)
//     // {
//     //     if (std::find(members_.begin(), members_.end(), uid) == members_.end())
//     //         members_.push_back(uid);
//     //     return;
//     // }
// 
//     std::vector<UID> v(&uid, &uid+1);
//     return add(v, cli, quiet);
// }

// void chat_group::add(client_ptr c , client_ptr & cli, bool quiet, bool qrcode)
// {
//     UID uid = c? c->user_id():SYSADMIN_UID;
//     return add(uid, cli, quiet, qrcode);
// }
// void chat_group::add(UID uid, client_ptr & cli, bool quiet, bool qrcode)
// {
//     if(uid != SYSADMIN_UID){
//         return add(&uid, &uid+1, cli, quiet, qrcode);
//     }
// }

// void chat_group::add_ghost(UID uid, client_ptr & cli)
// {
//     if (std::find(ghos_.begin(), ghos_.end(), uid) == ghos_.end())
//     {
//         ghos_.push_back(uid);
//     }
// }

// void chat_group::add(const std::vector<UID> & uids, client_ptr & cli, bool quiet)
// {
//     if (!is_alive_member(cli->user_id()))
//     {
//         LOG_I << cli << " add " << gid_ <<":"<< members_;
//         THROW_EX(EN_NotMember);
//     }
// 
//     LOG_I << cli << " add0 " << uids << " " << gid_ <<" to "<< members_;
//     json::array membs;
// 
//     bool is_group = is_groupchat(gid_);
// 
//     for (vector<UID>::const_iterator i = uids.begin(); i != uids.end(); ++i)
//     {
//         if (std::find(members_.begin(), members_.end(), *i) == members_.end())
//         {
//             client_ptr c = xindex::get(*i);
//             if (c && SYSADMIN_UID != c->user_id())
//             {
//                 if(!c->is_guest()){
//                     json::object user = c->brief_user_info();
//                     if("M" == user.get<string>("gender","")){
//                         ++male_count_;
//                     }
//                     else{
//                         ++female_count_;
//                     }
//                     membs.push_back(user);
//                 }
//                 ++visitor_count_;
// 
//                 if (is_group) sql::exec(boost::format(INSERT_SESSIONMEMBERS2) % (*i) % gid_);
//                 members_.push_back(*i);
//             }
//         }
//     }
// 
//     if (!membs.empty() && !quiet && is_group)
//     {
//         imessage::message msg;
//         msg.gid = gid_;
//         msg.from = cli->user_id(); //cli->brief_user_info();
//         msg.type = "chat/join";
//         msg.body
//             ("from", cli->brief_user_info())
//             ("gname", name_)
//             ("content", membs)
//             ("time", time_string());
// 
//         this->send(msg, cli);
//     }
//     LOG_I << cli << " add " << uids <<" "<< gid_ <<":"<< members_;
// }

// void chat_group::add_bar_alive(const UID& uid)
// {
//     LOG_I << " bar add "<< uid <<" "<< gid_ <<":"<< alive_members_;
//     this->add(uid, sysadmin_client_ptr(), 0, true);
// }

bool chat_group::remove(UID uid, client_ptr & cli)
{
    LOG_I << cli <<" "<< gid_ <<" remove "<< uid <<":"<< members_;
    const char* DELETE_SESSIONMEMBERS2 = "DELETE FROM SessionMembers WHERE SessionId='%1%' AND UserId=%2%";
    if (!is_alive_member(cli->user_id()))
    {
        LOG_I << cli <<" "<< gid_ << ":" << members_;
        MYTHROW(EN_NotMember, Chat_Error_Category);
    }

    auto i = alive_members_.find(uid);
    if (i != alive_members_.end())
    {
        client_ptr c = xindex::get(uid);
        if (c)
        {
            if (is_groupchat(gid_)) {
                sql::exec(boost::format(DELETE_SESSIONMEMBERS2) % gid_ % uid);
            }

            if(!c->is_guest()) {
                json::object user = c->brief_user_info();
                imessage::message msg( (uid == cli->user_id() ? "chat/quit" : "chat/remove")
                        , cli->user_id(), gid_);
                msg.body
                    ("content", user)
                    ("gname", name_)
                    ;

                send(msg, cli);

                // if("M" == user.get<string>("gender","")){
                //     --male_count_;
                // }
                // else{
                //     --female_count_;
                // }
                // --visitor_count_
            }
        }

        alive_members_.erase(i);
    }

    return (members_.erase(uid) > 0);
}

//void chat_group::remove_bar_alive(const UID& uid)
//{
//    LOG_I << " bar remove "<< uid <<" "<< gid_ <<":"<< alive_members_;
//    std::vector<UID>::iterator i = find(alive_members_.begin(), alive_members_.end(), uid);
//    if(i != members_.end()){
//        client_ptr c = xindex::get(uid);
//        if (c && !(c->is_guest())) {
//            json::object user = c->brief_user_info();
//            client_ptr psystem = sysadmin_client_ptr();
//
//            imessage::message msg;
//            msg.gid = gid_;
//            msg.from = psystem->user_id();
//            msg.type = "chat/remove";
//            msg.body("gname", name_)("content", user)("time", time_string())
//                ("from", psystem->brief_user_info())
//                ;
//
//            send(msg, psystem);
//
//            if("M" == user.get<string>("gender","")){
//                --male_count_;
//            }
//            else{
//                --female_count_;
//            }
//            members_.erase(i);
//        }
//    }
//}

void chat_group::rename(const std::string & name, client_ptr & cli)
{
    if (!is_alive_member(cli->user_id()))
    {
        LOG_I << cli <<" "<< gid_ << ":" << members_;
        MYTHROW(EN_NotMember, Chat_Error_Category);
    }

    if (name_ == name)
        return;
    name_ = name;
    boost::format UPDATE_SESSIONS_SESSIONNAME2("UPDATE sessions SET SessionName='%1%' WHERE SessionId='%2%'");
    sql::exec(UPDATE_SESSIONS_SESSIONNAME2 % sql::db_escape(name) % gid_);

    imessage::message msg("chat/groupname", cli->user_id(), gid_);
    msg.body
        ("content", name)
        ;

    send(msg, cli);
}

std::string chat_group::logo() const
{
    boost::shared_ptr<s_bar_info> bar_ptr = bars_mgr::inst().get_session_bar(id());
    return bar_ptr->logo;
}

chatmgr::initializer::initializer()
{
    chatmgr::inst();
    Chat_Error_Category();
}

chatmgr::initializer::~initializer()
{
    LOG_I << __FILE__ << __LINE__;
}

void chatmgr::testmsg(UID uid)
{
    imessage::message msg("chat/text", 1000, make_p2pchat_id(uid,1000)); //".678.678"; //lexical_cast<string>(2);
    // msg.a; // @ destination id TODO
    msg.body("content", string("hello, test."));

    send(msg, sysadmin_client_ptr());

    //msg.id = message_id(); 
    //cli->messages_.push_back(m);
    //cli->writemsg();
    // Client::pushmsg(uid, msg); // send(msg);
}

void sendto(const imessage::message & msg, client_ptr & cli)
{
    boost::format fmt("INSERT INTO messages(id,SessionId,UserId,PeerId,MsgType,content)"
            " VALUES(%1%,'%2%',%3%,%4%,'%5%','%6%')");
    sql::exec(fmt % msg.id() % msg.gid % msg.from % msg.a % msg.type % sql::db_escape(json::encode(msg.body)));
    Client::pushmsg(cli, msg);

    LOG_I << msg.id() <<"/"<< msg.type << "/" << msg.from <<"/"<< msg.gid <<": " << cli->user_id();
}


//
//    std::vector<UID> tmps;
//    json::array membs;
//    for (auto i = ubeg; i != uend; ++i)
//    {
//        UID uid = *i;
//        if (SYSADMIN_UID == uid)
//            continue;
//        if (members_.find(uid) == members_.end()) //(std::find(members_.begin(),members_.end(),*i) == members_.end())
//            tmps.push_back(uid);
//        if (quiet)
//            continue;
//
//        // if(std::find(alive_members_.begin(),alive_members_.end(), uid) == alive_members_.end())
//        if (alive_members_.insert(uid).second) {
//            client_ptr c = xindex::get(uid);
//            if (c) {
//                if(!c->is_guest()) {
//                    json::object user = c->brief_user_info();
//                    if("M" == user.get<std::string>("gender","")){
//                        ++male_count_;
//                    } else {
//                        ++female_count_;
//                    }
//                    membs.push_back(user);
//                }
//                ++visitor_count_;
//
//                if (is_groupchat(gid_))
//                    save_new_member(gid_, uid);
//            }
//        }
//    }
//
//    bool insed = 0;
//    if(!qrcode && !tmps.empty()){
//        members_.insert(tmps.begin(), tmps.end());
//        insed = 1;
//    }
//
//    if (!membs.empty())
//    {
//        imessage::message msg("chat/join", cli->user_id(), gid_);
//        msg.body
//            ("gname", name_)
//            ("content", membs)
//            // ("from", cli->brief_user_info())
//            ;
//        if (qrcode)
//            msg.body.put("qrcode", qrcode);
//
//        this->send(msg, cli);
//    }
//
//    if (!insed) // (qrcode && !tmps.empty())
//        members_.insert(tmps.begin(), tmps.end());
//
