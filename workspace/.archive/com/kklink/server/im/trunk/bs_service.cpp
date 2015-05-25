#include <boost/timer/timer.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scope_exit.hpp>

#include "message.h"
#include "service.h"
#include "log.h"
#include "client.h"

#include "ioctx.h"
#include "json.h"
#include "push.h"
#include "chatroom.h"

using namespace std;
using namespace boost;

enum { Cmd_login_other=199 };
enum {
    Result_ok=0,
    Result_fail=1,
    Result_login_other=Cmd_login_other
};

static int _chatroom_message(std::string const& sid, json::object const& in_body)
{
    UInt fr_uid = json::value<UInt>(in_body,"fr_uid"); //ctx.tag();
    if (fr_uid < 1000) {
        LOG << fr_uid << "lt 1000";
    }
    auto& mgr = chatroom_mgr::instance();
    room_ptr room = find_room(mgr, sid);
    if (!room) {
        return Result_fail;
    }

    UInt msgid = mgr.new_message(0/*fr_uid*/, room, in_body); // message_mgr::alloc_msgid();
    if (msgid == 0) {
        LOG << "new_message fail" << sid;
        return Result_fail; //goto Pos_ret_;
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
    return 0;
} // End ( stype == SType_chatroom )

static int send_message(io_context& ctx, json::object const& in, json::object& out)
{
    string empty_str;
    auto const& sid = json::value<string>(in,"sid");
    json::object const& data = json::value<json::object>(in,"data");
    string noti = json::as<string>(in,"notification").value_or(empty_str);

    int stype = atoi(sid.c_str());
    switch (stype) {
    case SType_chatroom:
        return _chatroom_message(sid, in);
    }

    std::vector<destination> dsts;
    try {
        std::set<UInt> dst_uids;
        auto const& tos = json::value<json::array>(in,"to");
        BOOST_FOREACH(auto const &u, tos)
        {
            auto const& jo = json::value<json::object>(u);

            UInt uid = json::value<UInt>(jo, "uid");
            if (!dst_uids.insert(uid).second) {
                continue;
            }
            std::string fr_name = json::as<string>(jo, "name").value_or(empty_str);
            dsts.push_back( destination(uid, sid, fr_name) );
        }
    } catch (myerror & e) {
        e << json::errinfo_key("to");
        throw;
    }

    if ( dsts.empty() ) {
        return Result_fail;
    }

    UInt msgid = message_mgr::instance().post(dsts, 1, stype, data, noti);
    if (msgid == 0) {
        return Result_fail;
    }

    json::insert(out) ("msgid", msgid) ;

    return Result_ok;
}

static int send_message_p2p(io_context& ctx, json::object const& in, json::object& out)
{
    string empty_str;
    auto const& tos = json::value<json::array>(in,"to");
    auto const& sids = json::value<json::array>(in,"sids");
    json::object const& data = json::value<json::object>(in,"data");
    string noti = json::as<string>(in,"notification").value_or(empty_str);

    if ( tos.size() != sids.size() ) {
        LOG << "to size doesn't match sids size:"<< tos.size()<< " / "<<sids.size();
        return Result_fail;
    }

    int stype = 0;
    vector<destination> dsts;
    set<UInt> dst_uids;
    std::string fr_name = json::as<std::string>(in,"name").value_or(empty_str);

    for ( auto u=tos.begin(), s=sids.begin(); u!=tos.end(); ++u, ++s )
    {
        UInt uid = json::value<UInt>(*u);
        if (!dst_uids.insert(uid).second) {
            continue;
        }

        std::string sid = json::value<std::string>(*s);

        dsts.push_back( destination(uid, sid, fr_name) );
        if (stype == 0) {
            stype = atoi(sid.c_str());
        } else if (stype != atoi(sid.c_str())) {
            LOG << stype << sid;
        }
    }

    if ( dsts.empty() ) { 
        return Result_fail;
    }

    UInt msgid = message_mgr::instance().post(dsts, 1, stype, data, noti);
    if (msgid == 0) {
        return Result_fail;
    }

    json::array msgids;
    for (size_t i = 0, n = dsts.size(); i < n; ++i) {
        msgids.push_back(msgid);
    }

    json::insert(out) ("msgids", msgids) ;

    return Result_ok;
}

static int fetch_public_messages(io_context& ctx, json::object const& in, json::object& out)
{
    json::array msgs;
    UInt bid = service_mgr::instance().old_public_msg_id();
    if ( bid < service_mgr::instance().max_public_msg_id() ) {
        UInt retid = public_message_get( bid, msgs );
        service_mgr::instance().set_old_public_msg_id( retid );
    }

    json::insert(out)("msgs", msgs);
    return Result_ok;
}

static int get_servers_ips(io_context& ctx, json::object const& in, json::object& out)
{
    auto& ser = service_mgr::instance();
    json::insert(out)
        ("ip", ser.imc_ip())
        ("port", ser.imc_port())
        ;

    return Result_ok;
}

static int regist_client(io_context& ctx, json::object const& in, json::object& out)
{
    string empty_str;
    string token = json::value<string>(in,"token");
    UID uid  = json::value<UID>(in,"uid");
    string bundleid = json::as<string>(in,"bundleid").value_or(empty_str);

    LOG << uid << token;
    if (uid < 100) {
        LOG << uid << "<100 invalid";
        return Result_fail;
    }

    if (io_context c2 = io_context::find(uid))
    {
        json::object rsp, body;
        body.emplace("time", time(0));
        json::insert(rsp)
            ("cmd", Cmd_login_other) ("error",0) ("body", body)
            ;
        c2.send(rsp);
        c2.close(__LINE__,__FILE__);
    }

    clients::inst().add_client(uid, token, bundleid);
    push_mgr::instance().add_reject( uid );
    auto& ser = service_mgr::instance();

    json::insert(out)
        ("ip", ser.imc_ip())
        ("port", ser.imc_port())
        ;

    emit(user_mgr::instance().ev_regist, uid, 1);

    return Result_ok;
}

static int unregist_client(io_context& ctx, json::object const& in, json::object& out)
{
    const UID uid = json::value<UID>(in,"uid");
    LOG << uid;

    if (io_context c2 = io_context::find(uid))
    {
        LOG << c2;
        json::object rsp, body;
        body.emplace("time", time(0));
        json::insert(rsp)
            ("cmd", Cmd_login_other) ("error",0) ("body", body)
            ;
        c2.send(rsp);
        c2.close(__LINE__,__FILE__);
    }

    emit(user_mgr::instance().ev_regist, uid, 0);
    // TODO, im_server clear uid

    clients::inst().delete_client( uid );

    push_mgr::instance().add_reject( uid );

    return Result_ok;
}

static int regist_virtual_users(io_context& ctx, json::object const& in, json::object& out)
{
    auto const& users = json::value<json::array>(in,"users");
    std::set<UInt> virs;
    BOOST_FOREACH( auto& u, users ) {
        virs.insert( json::value<UInt>( u ) );
    }

    BOOST_FOREACH( auto& v, virs ) {
        virtual_user_mgr::instance().add( v );
    }

    return Result_ok;
}

static int unregist_virtual_users(io_context& ctx, json::object const& in, json::object& out)
{
    auto const& users = json::value<json::array>(in,"users");
    std::set<UInt> virs;
    BOOST_FOREACH( auto& u, users ) {
        virs.insert( json::value<UInt>( u ) );
    }

    BOOST_FOREACH( auto& v, virs ) {
        virtual_user_mgr::instance().remove(v);
    }

    return Result_ok;
}

static int regist_chatroom(io_context& ctx, json::object const& in, json::object& out)
{
    std::string const& sid = json::value<std::string>(in,"sid");
    char const* subsid = chatroom_subsid(sid);
    if (!subsid) {
        LOG << "invalid chatroom sid" << sid;
        return Result_fail;
    }
    LOG << sid << subsid;

    auto& mgr = chatroom_mgr::instance();
    auto p = mgr.create_room( subsid );
    if (!p.second) {
        LOG << "create room fail" << sid;
        return Result_fail;
    }
    return Result_ok;
}

static int unregist_chatroom(io_context& ctx, json::object const& in, json::object& out)
{
    auto& mgr = chatroom_mgr::instance();

    std::string const& sid = json::value<std::string>(in,"sid");
    char const* subsid = chatroom_subsid(sid);
    if (!subsid) {
        LOG << "invalid chatroom sid" << sid;
        return Result_fail;
    }
    mgr.destroy_room( subsid );
    return Result_ok;
}

void bs_initialize(service_mgr& inst)
{
    enum CMD {
        // bs cmd
        SEND_MSG=101,
        SEND_GROUP_MSG,
        NOTIFICATION,

        RECV_MSGS=106, CHECK_MSG_STATE,

        REGIST_CLIENT=111, UNREGIST_CLIENT,

        GET_SERVER_IPS=115,
        REGIST_VIRTUAL_USERS=121, UNREGIST_VIRTUAL_USERS,
        REGIST_CHATROOM=125, UNREGIST_CHATROOM,
        //REGIST_IMGROUP=131, UNREGIST_IMGROUP, IMGROUP_ADD_MEMBER=135, IMGROUP_REMOVE_MEMBER,
    };

    inst.regist_service( SEND_MSG, send_message );
    inst.regist_service( SEND_GROUP_MSG, send_message_p2p );
    inst.regist_service( RECV_MSGS, fetch_public_messages );
    // inst.regist_service( CHECK_MSG_STATE, check_msgs_state );
    inst.regist_service( REGIST_CLIENT, regist_client );
    inst.regist_service( UNREGIST_CLIENT, unregist_client );
    inst.regist_service( GET_SERVER_IPS, get_servers_ips );
    inst.regist_service( REGIST_VIRTUAL_USERS, regist_virtual_users );
    inst.regist_service( UNREGIST_VIRTUAL_USERS, unregist_virtual_users );

    inst.regist_service( REGIST_CHATROOM, regist_chatroom );
    inst.regist_service( UNREGIST_CHATROOM, unregist_chatroom );

    //inst.regist_service( REGIST_IMGROUP, regist_imgroup );
    //inst.regist_service( UNREGIST_IMGROUP, unregist_imgroup );
    //inst.regist_service( IMGROUP_ADD_MEMBER, imgroup_add_member );
    //inst.regist_service( IMGROUP_REMOVE_MEMBER, imgroup_remove_member );
}

