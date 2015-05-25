#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>
#include <boost/foreach.hpp>
#include "util.h"
#include "service.h"
#include "client.h"
#include "message.h"
#include "log.h"
#include "json.h"
#include "user.h"
#include "push.h"
#include "chatroom.h"

using namespace std;
using namespace boost;

// imc cmd
enum Cmdlist_IMC {
    // HELLO=91,
    // DELIEVER_REPORT=95, 

    AUTH_VERIFY=99, 

    //= 193

    DELIEVER_MSG=205,
    SET_PUSH=211,

    ENTER_CHATROOM=215, LEAVE_CHATROOM,
};
enum { Cmd_login_other=199 };
enum {
    Result_ok=0,
    Result_fail=1,
    Result_login_other=199
};

static void auth_fail(io_context& ctx, int result)
{
    json::object out_body;
    out_body.emplace("result", result);

    json::object out;
    json::insert(out)
        ("cmd", 99) ("error", 0) ("body", out_body)
        ;

    ctx.send(out);
    ctx.close(__LINE__,__FILE__);
}

static int auth_verify(io_context& ctx, json::object const& in, json::object& out)
{
    UInt uid = json::value<UInt>(in,"uid");
    std::string token = json::value<std::string>(in,"token");
    std::string appid = json::value<std::string>(in,"appid");

    auto & inst = clients::inst();
    auto it = inst.find_client(uid, true);
    if ( it == inst.end() ) {
        LOG << uid << "fail";
        auth_fail(ctx, Result_login_other);
        return Result_login_other;
    }

    std::string tok = it->second.get_token();
    if (token != tok)
    {
        LOG << uid << token << tok << "fail";
        auth_fail(ctx, Result_login_other);
        return Result_login_other;
    }

    if (io_context c2 = ctx.tag(uid)) // if (io_context c2 = io_context::find(uid))
    {
        json::object rsp, body;
        body.emplace("time", time(0));
        json::insert(rsp)
            ("cmd", Cmd_login_other) ("error",0) ("body", body)
            ;
        c2.send(rsp);
        c2.close(__LINE__,__FILE__);
        // connection replaced
    }
    else
    {
        // new connection
    }

    return Result_ok;
}

typedef std::vector<std::string>::const_iterator vector_const_iterator;
vector_const_iterator search_any_of(std::string const& txt, std::vector<std::string> const& subs);
vector_const_iterator search_any_of(json::object const& jv, std::vector<std::string> const& subs);

static std::vector<std::string> keywords_; // = { "__KKLINK__", "__KK无线__" };

static UInt _chatroom_message(io_context& ctx, std::string const& sid, json::object const& in_body)
{
    auto fr_uid = ctx.tag();
    auto& mgr = chatroom_mgr::instance();
    room_ptr room = find_room(mgr, sid);
    if (!room) {
        return 0;
    }

    UInt msgid = mgr.new_message(fr_uid, room, in_body); // message_mgr::alloc_msgid();
    if (msgid == 0) {
        LOG << "new_message fail" << sid;
        return 0;;
    }

    json::object _body = in_body;
    _body.erase("to");
    _body.erase("notification");
    _body.erase("tag");
    _body.emplace("msgid", msgid);
    _body.emplace("fr_uid", fr_uid);

    json::object msg;
    json::insert(msg)
        ("cmd", 200) ("error", 0) ("body", _body)
        ;

    std::vector<UInt> del_uids;
    for (auto& m : mgr.members(room)) {
        if (m.uid == fr_uid)
            continue;
        if (io_context c2 = io_context::find(m.uid)) {
            c2.send(msg);
        } else {
            del_uids.push_back(m.uid);
        }
    }
    if (io_context c2 = io_context::find(UID_PRESERVED_CHATROOM)) {
        c2.send(_body);
    } else {
        LOG << UID_PRESERVED_CHATROOM << "not found";
    }
    mgr.erase_users(del_uids);
    return msgid;
}

static int deliever_msg(io_context& ctx, json::object const& in_body, json::object& out)
{
    // auto mic = Microseconds();//boost::posix_time::microsec_clock::local_time();

    std::string const &sid = json::value<string>(in_body,"sid");
    json::object const &data = json::value<json::object>(in_body,"data");

    UInt fr_uid = ctx.tag();
    UInt msgid = 0;
    int stype;

    vector_const_iterator it_kw;
    if ( (it_kw = search_any_of(data, keywords_)) != keywords_.end()) {
        LOG << "black keywords" << *it_kw;
        goto Pos_ret_;
    }

    // LOG << sid;
    stype = atoi(sid.c_str());

    if ( stype == SType_official ) {
        msgid = public_message_save( fr_uid, sid, data );
        // msgid = public_message_save2( fr_uid, sid, data );
        service_mgr::instance().set_max_public_msg_id( msgid );

    } else if ( stype == SType_chatroom ) {
        msgid = _chatroom_message(ctx, sid, in_body);
    } else {
        string empty_str;
        string noti = json::as<string>(in_body,"notification").value_or(empty_str);
        if ( (it_kw = search_any_of(noti, keywords_)) != keywords_.end()) {
            LOG << "black keywords noti" << *it_kw;
            goto Pos_ret_;
        }

        json::array const &tos = json::value<json::array>(in_body,"to");
        std::vector<destination> dsts, dsts_puppets;
        set<UInt> dst_uids;
        json::array virtual_users;

        BOOST_FOREACH(auto const &u, tos) {
            auto const& obj = json::value<json::object>(u);
            UInt uid = json::value<UInt>(obj, "uid");
            if (uid < 10000) {
                LOG << uid << "lt 10000";
            }

            if (stype==21 && !PuppetAgent::instance().is_empty() && virtual_user_mgr::instance().is_exist( uid ) ) {
                virtual_users.insert( virtual_users.end(), u );
                std::string fr_name = "马甲";
                dsts_puppets.push_back( destination( PuppetAgent::instance().get_agent( uid ), sid, fr_name ) );
                continue;
            }

            if (!dst_uids.insert(uid).second) {
                continue;
            }

            std::string fr_name = json::as<string>(obj, "name").value_or(empty_str);
            dsts.push_back(destination(uid, sid, fr_name));
        }

        if ( !dsts.empty() ) {
            msgid = message_mgr::instance().post(dsts, fr_uid, stype, data, noti);
        }

        if ( !virtual_users.empty() ) {
            if ( !dsts_puppets.empty() ) {
                json::object a = data;
                auto const& user = json::value<json::object>( *virtual_users.begin() );
                auto const& uid = json::value<UID>( user, "uid" );
                a.emplace( "puppet", uid );
                msgid = message_mgr::instance().post( dsts_puppets, fr_uid, stype, a, noti);
            } else {
                msgid = message_mgr::alloc_msgid();
            }

            json::object in2 = in_body;
            in2.emplace( "to", virtual_users );
            in2.erase( "notification" );
            in2.emplace( "msgid", msgid );

            // 100: uid of proxy
            if ( io_context c2 = io_context::find( UID_PRESERVED_DUMMY_USER ) ) {
                c2.send( in2 );
            }
        }
        LOG << dsts;
        //{ auto mic2 = Microseconds(); LOG << msgid << mic << mic2-mic << "205 TRACE"; }
    }
    BOOST_ASSERT(msgid > 0);

Pos_ret_:
    if (msgid == 0) {
        return Result_fail;
    }

    out.emplace("sid", sid);
    out.emplace("msgid", msgid);

    return Result_ok;
}

// static int deliever_report(io_context& ctx, json::object const& in, json::object& out);
static int set_push(io_context& ctx, json::object const& in, json::object& out)
{
    string empty_str;
    UInt uid = json::value<UInt>(in,"uid");
    int devtype = json::value<int>(in,"devtype");
    string devtoken = json::value<string>(in,"devtok");
    string bundleid = json::as<string>(in,"bundleid").value_or(empty_str);

    if ( !clients::inst().set_device( uid, devtype, devtoken, bundleid ) ) {
        return Result_fail;
    }

    push_mgr::instance().remove_reject( uid );

    return Result_ok;
}

static int x_hourly(io_context& ctx, json::object const& in, json::object& out)
{
    extern void tf_hourly();
    tf_hourly();
    return Result_ok;
}

static int enter_chatroom(io_context& ctx, json::object const& in, json::object& out_body)
{
    auto& mgr = chatroom_mgr::instance();
    std::string const& sid = json::value<string>(in,"sid");
    room_ptr room = find_room(mgr, sid);
    if (!room) {
        return Result_fail;
    }

    if (!mgr.join_chat(room, ctx.tag())) {
        LOG << "join-chat fail" << sid << ctx.tag();
        return Result_fail;
    }
    out_body.emplace("msgid", room->last_msgid());
    return Result_ok;
}

static int leave_chatroom(io_context& ctx, json::object const& in, json::object& out_body)
{
    auto& mgr = chatroom_mgr::instance();
    std::string const& sid = json::value<string>(in,"sid");
    room_ptr room = find_room(mgr, sid);
    if (!room) {
        return Result_fail;
    }
    mgr.leave_chat(room, ctx.tag());
    return Result_ok;
}

void ims_initialize(service_mgr& inst)
{

    inst.regist_service( AUTH_VERIFY, auth_verify );
    inst.regist_service( DELIEVER_MSG, deliever_msg );
    // inst.regist_service( DELIEVER_REPORT, deliever_report );
    inst.regist_service( SET_PUSH, set_push );
    inst.regist_service( 193, x_hourly );

    inst.regist_service( ENTER_CHATROOM, enter_chatroom );
    inst.regist_service( LEAVE_CHATROOM, leave_chatroom );
    // inst.regist_service( HELLO, hello );
}

