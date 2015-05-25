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
#include <boost/range/adaptor/reversed.hpp>

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/multi_index/sequenced_index.hpp>

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
#include "dynamic_state.h"
#include "asclient.h"

#include "sqls.h"

using namespace std;
using namespace boost;

filesystem::path Audio_Dir = "/var/www/vhosts/moon.kklink.com/public/attachment";
bool Service::alipay=true;
bool Service::upPayPlugin=true;
static std::string user_protocol_fn_ = "etc/moon.d/user-protocol.txt";

/////////////
//
namespace {
#undef  Error_Category
#define Error_Category service_error_category
struct service_error_category : client_error_category
{
    service_error_category();
};

enum enum_service_error
{
    EN_NotEnough_Money = 415,
    EN_Good_NotFound,
    EN_Bar_NotFound,
    EN_ExchageCondiction_Insufficient,
    EN_TargetUser_NotFound,
    ////EN_Operation_NotPermited = 420,

    EN_FileName_Invalid = 425,
    EN_WriteFile_Fail,
    EN_File_NotFound,
    EN_File_ShouldNotEmpty,

    EN_Black = 435,
    EN_Blacked,

    EN_Moment_NotExist = 455,
};

service_error_category::service_error_category()
{
    static code_string ecl[] = {
        { EN_NotEnough_Money, "没有足够的金币" },
        { EN_Good_NotFound, "不存在的商品" },
        { EN_Bar_NotFound, "不存在的商家" },
        { EN_ExchageCondiction_Insufficient, "交易条件不满足"},
        { EN_TargetUser_NotFound, "目标用户不存在" },
        //{ EN_Operation_NotPermited, "不被允许的操作" },

        { EN_FileName_Invalid, "文件名不正确" },
        { EN_WriteFile_Fail, "写文件出错" },
        { EN_File_NotFound, "文件不存在" },
        { EN_File_ShouldNotEmpty, "不能传空文件" },

        { EN_Black, "已屏蔽对方"},
        { EN_Blacked, "已被对方屏蔽"},

        { EN_Moment_NotExist, "已经被删除"},
    };

    Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])], "service");
}
} // namespace

int IndividualAlbum_message_id();

void record_sms(string user_phone, string lover_phone, int id, string msgtype, int len)
{
    // format fmt("insert into sms_records(user_phone,lover_phone,whisper_id,msg_type,"
    //         " msg_len) values(%s,%s,%d,'%s',%d)");
    // fmt %user_phone %lover_phone %id %msgtype %len;
    // sql::exec(fmt);
}

bool my_regex_match(const string&src, const string&dst)
{
    bool ret = false;
    int src_size = src.length();
    int dst_size = dst.length();

    if(src_size >= dst_size){
        if(0 == dst_size){ 
            ret = true;
        }
        else{
            string ex = "*" + dst + "*";
            if(0 == fnmatch(ex.c_str(),src.c_str(),FNM_CASEFOLD)){
                ret = true;
            }
        }
    }

    // LOG_I<<"src:"<<src<<" "<<"dst:"<<dst<<" "<<ret;
    return ret;
}

static UID uid_logdbg_ = 0;

#define CHAT_BOTTLE_ID_SET "drifting_bottle:id"
static std::set<uint32_t> chat_bottle_id;
static void load_chat_bottle_id()
{
    auto reply = redis::command("SMEMBERS", CHAT_BOTTLE_ID_SET);
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        return;
    }

    for (UInt i = 0; i < reply->elements; i++) {
        UInt id = lexical_cast<UInt>(reply->element[i]->str);
        chat_bottle_id.insert(id);
    }
}

static void record_chat_bottle_id(UInt bid) {
    redis::command("SADD", CHAT_BOTTLE_ID_SET, bid);
    chat_bottle_id.insert(bid);
}

static void erase_chat_bottle_id(UInt bid) {
    redis::command("SREM", CHAT_BOTTLE_ID_SET, bid);
    chat_bottle_id.erase(bid);
}

json::object Service::loguser(http::request & req, client_ptr& cli)
{
    std::string suid = req.param("uid", std::string());
    UID uid = uid_logdbg_;
    if (!suid.empty())
    {
        uid_logdbg_ = lexical_cast<UID>(suid);
    }
    LOG_I << uid <<" "<< uid_logdbg_;
    return json::object()("old", uid)("new", uid_logdbg_);
}

json::object Service::logsave(http::request & req, client_ptr& cli)
{
    json::object ret;

    UID uid = uid_logdbg_;
    uid_logdbg_ = 0;
    LOG_I << cli <<" "<< uid;

    if (cli && cli->is_authorized())
        uid = cli->user_id();

    if (uid)
    {
        string path = uniq_relfilepath(lexical_cast<string>(uid), "log");
        string url = write_file(path, req.content());
        ret.put("log", complete_url(url)) ;
    }
    return ret;
}

int GetFansStatistic(UID userid)
{
    format fmt(SELECT_CONTACTS_FANS_STATISTIC1);
    fmt %userid;
    return GetStatistic(fmt);
}

int GetAdmiresStatistic(UID userid)
{
    format fmt(SELECT_CONTACTS_ADMIRE_STATISTIC1);
    fmt %userid;
    return GetStatistic(fmt);
}

json::object GetGiftsStatistic(UID userid)
{
    format fmt(SELECT_USERGIFTS_STATISTIC1);
    sql::datas datas(fmt %userid);
    int flower=0,car=0,ship=0,plane=0;
    while(sql::datas::row_type row = datas.next()){
        int i = lexical_cast<int>(row.at(0));
        switch(i){
            case FLOWER:
                ++flower;
                break;
            case CAR:
                ++car;
                break;
            case PLAN:
                ++plane;
                break;
            case YACHT:
                ++ship;
                break;
            default:
                break;
        }
    }
    return json::object()("flower", flower)
                         ("car", car)
                         ("ship", ship)
                         ("plane", plane)
            ;         
}

json::object GetUserInfo(client_ptr& cli)
{
    json::object obj = cli->user_info();
    obj.rename("nick", "userName");
    obj.rename("icon", "headIcon");
    obj.rename("signature", "sign");
    obj.erase("user");
    return obj;
}

json::object GetUserInfo(UID userid)
{
    client_ptr cli = xindex::get(userid);
    if( !cli ) return json::object(); 

    return GetUserInfo(cli);
}

json::object GetIndividualDatas(UID userid)
{
    format fmt(SELECT_INDIVIDUALDATAS1);
    sql::datas datas(fmt %userid);
    json::object obj;
    if(sql::datas::row_type row = datas.next()){
        const char *pmoney = row.at(1,"0");
        const char *pRejectAll = row.at(2,"0");
        const char *pPushStatus = row.at(3,"1");
        const char *pbackground = row.at(4,"");
        const char *pattention_bars = row.at(6,"");
        obj("money",lexical_cast<int>(pmoney))
            ("fans",GetFansStatistic(userid)) 
            ("admires", GetAdmiresStatistic(userid))
            ("RejectAll",lexical_cast<int>(pRejectAll))
            ("PushStatus",lexical_cast<int>(pPushStatus))
            ("background", complete_url(pbackground))
            ("attention_bars", pattention_bars)
            ;
    }

    return obj;
}

json::array GetIndividualAlbum(UID userid)
{
    json::array imgs;
    format fmt(SELECT_INDIVIDUALALBUM1);
    fmt%userid;
    sql::datas datas(fmt);
    while(sql::datas::row_type row = datas.next()){
        const char *pid = row.at(1);
        const char *purl = row.at(2);
        const char *puploadtime = row.at(3,"");
        if(NULL==pid || NULL==purl) continue;
        imgs(
                json::object()("icon", complete_url(purl))
                ("iconid", lexical_cast<int>(pid))
                ("date", puploadtime)
            );
    }

    return imgs;
}

bool inline CheckWhisper(UID userid, UID otherid)
{
    bool issay = false;

    sql::datas datas(format(SELECT_WHISPERS2) %userid %otherid);
    if(/*sql::datas::row_type row =*/ datas.next()){
        issay = true; 
    }

    return issay;
}

static void fs_mkdir(const filesystem::path & di)
{
    if (filesystem::exists(di))
        return;
    // fs_mkdir(di.parent_path());
    filesystem::create_directories(di);
}

string uniq_relfilepath(filesystem::path fnp, string dir)
{
    static unsigned int six = 1;

    if (fnp.empty()){
        THROW_EX(EN_FileName_Invalid);
    }

    fs_mkdir(Client::Files_Dir / dir);
    string base = urlencode( basename(fnp) );
    std::string fn;
    for ( ;; )
    {
        fn = str(format("%s-%x%s") %base %six++ %fnp.extension().string());
        if (!filesystem::exists(Client::Files_Dir / dir / fn))
            break;
    }

    LOG_I<<"filename: " << dir+"/" +fn;

    return dir+"/" +fn;
}

static Magick::Image & sample(Magick::Image & img, Magick::Geometry const & new_size)
{
    Magick::Geometry old_size = img.size();
    if(old_size.height()>new_size.height() || old_size.width()>new_size.width())
    {
        img.sample(new_size);
    }
    return img;
}

// static string thumbnail(filesystem::path fnp, const string & source )
// {
//     if(source.empty()) return string();
// 
//     filesystem::path pwd = fnp.parent_path();
//     string exten = fnp.extension().string();
//     string base_name = basename(fnp) +"_thumb";
//     string thum_file = (Files_Dir / pwd / base_name).string() + exten;
//     cout<<"thumbnail:"<<thum_file<<endl;
// 
//     Magick::Blob block(source.data(), source.length());
//     Magick::Image obj(block);
//     sample(obj, Magick::Geometry("230x230"));
//     obj.write(obj.magick() + ":" + thum_file);
// 
//     return thum_file;
// }

static void thumbnail(filesystem::path const & src, filesystem::path const & thumb)
{
    //filesystem::path pwd = src.parent_path();
    //string base_name = basename(src) +"_thumb";
    //string thum_file = (pwd / base_name / src.extension()).string();
    LOG_I << "thumbnail:" << src <<" "<< thumb;

    //ifstream source(fnp.string().c_str());
    //string content(istreambuf_iterator<char>(source), (istreambuf_iterator<char>()));
    //source.close();

    Magick::Image obj(src.string());
    sample(obj, Magick::Geometry("230x230"));
    obj.write(obj.magick() + ":" + thumb.string());

    // return thumb;
}

string write_file(filesystem::path fnp, const string & cont)
{
    if (cont.empty())
    {
        LOG_E << fnp <<" is empty";
        THROW_EX(EN_File_ShouldNotEmpty);
    }

    boost::filesystem::ofstream f(Client::Files_Dir/fnp, ios_base::binary|ios_base::trunc);

    if (!f){
        LOG_I << format("file put %s fail") % (Client::Files_Dir/fnp).string();
        THROW_EX(EN_WriteFile_Fail);
    }

    f << cont;

    if (!f)
        THROW_EX(EN_WriteFile_Fail);
    // return Files_Url + "/" + fnp.string();

    return fnp.string();
}

string read_file(std::string const & fnp, bool no_throw)
{
    filesystem::path p = Client::Files_Dir / fnp; //filesystem::path(++fnp.begin(), fnp.end());
    LOG_I<<"read_file,path:"<<p.string();

    filesystem::ifstream ins(p); //(.string().c_str());
    if (!ins)
    {
        if (no_throw)
            return std::string();
        THROW_EX(EN_File_NotFound);
    // if (!filesystem::exists(p)) { THROW_EX(EN_File_NotFound); }
    }

    ostringstream outs;
    outs << ins.rdbuf();

    return outs.str();
}

string get_file(filesystem::path const & fnp, bool no_throw)
{
    filesystem::path p = Client::Files_Dir / fnp;
    LOG_I<<"get_file,path:"<<p.string();
    if (!filesystem::exists(p))
    {
        const string& name = basename(p);
        if(!ends_with(name, "_thumb")) {
            if (no_throw)
                return std::string();
            THROW_EX(EN_File_NotFound);
        }

        filesystem::path pwd = p.parent_path(); 
        filesystem::path source = pwd / (string(name.begin(),name.end()-6) + p.extension().string());
        if(!filesystem::exists(source)){
            if (no_throw)
                return std::string();
            THROW_EX(EN_File_NotFound);
        }

        thumbnail(source, p);
        if (!filesystem::exists(p))
        {
            if (no_throw)
                return std::string();
            THROW_EX(EN_File_NotFound);
        }
    }

    ostringstream outs;
    {
        boost::filesystem::ifstream ins(p);
        outs << ins.rdbuf();
    }

    return outs.str();
}

void convert_audio(filesystem::path src)
{
    time_t timep;
    time(&timep);
    struct tm* p = gmtime(&timep);
    format fmt("%1%_/%2%/%3%");
    fmt %(1900+p->tm_year) %(1+p->tm_mon) %(p->tm_mday);
    filesystem::path fnp = Audio_Dir/fmt.str();
    string dst = (fnp/basename(src)).string()+ ".mp3";

    if(filesystem::exists(dst)) return;

    fs_mkdir(fnp);
    string convet_str = "sox -q " + src.string() + " " + dst;
    LOG_I <<convet_str <<std::endl;
    ::system(convet_str.c_str());
}

json::object pay_setting(const string& version="")
{
    return json::object()("alipay", Service::alipay)("upPayPlugin", Service::upPayPlugin);
}

json::object json_bar_brief(s_bar_info const & b)
{
    return json::object()
        ("userid", b.trader_id)
        ("sessionid", b.sessionid)
        ("title", b.barname)
        ("icon", b.logo)
        ("city", b.city)
        ("zone", b.zone)
        ("address", b.address)
        ("phone", b.phone)
        ("introduce", b.introduction)
        ;
}

// static void InitSessions()
// {
// #define SELECT_SESSIONS ("select CreatorId,SessionId from sessions")
//     format fmt(SELECT_SESSIONS);
//     sql::datas datas1(fmt);
//     while(sql::datas::row_type row1 = datas1.next()){
// 
//         const char *pCreatorId = row1.at(0);
//         const char *pSessionId = row1.at(1);
//         if(NULL==pCreatorId || NULL==pSessionId) continue;
// 
//         std::vector<UID> uids;
// #define SELECT_SESSION_MEMBERS ("select UserId from SessionMembers where SessionId = '%s'")
//         fmt = format(SELECT_SESSION_MEMBERS) %pSessionId;
// 
//         sql::datas datas2(fmt);
//         while(sql::datas::row_type row2 = datas2.next()){
//             const char *pUserId = row2.at(0);
//             if(NULL == pUserId) continue;
// 
//             uids.push_back(lexical_cast<int>(pUserId));
//         }
//         if(!uids.empty()){
//             chat_group cg(lexical_cast<int>(pCreatorId), pSessionId, uids.begin(), uids.end());
//             chatmgr::inst().insert( make_pair(pSessionId, cg) );
//         }
//     }
// }

json::object Service::updateScenseIcon(http::request & req, client_ptr& cli)
{
    // UID userid = cli->user_id();
    string sessionid = req.param("sid");

    string path = uniq_relfilepath(req.param("fileName"), "bar_"+sessionid);
    string url = write_file(path, req.content());

    // thumbnail(path, req.content());
    format fmt(INSERT_BARLIVEALBUM2);
    sql::exec(fmt %sessionid %url);  
    return json::object()("img",complete_url(url));
}

// json::object Service::messageNotification(http::request & req, client_ptr& cli)
// {
//     UID userid = cli->user_id();
//     string sessionid = req.param("sessionid");
//     // string userid = req.param("userid");
//     string status = req.param("status");
// 
//     format UPDATE_SESSIONMEMBERS_REJECTMSGFLAG3("update SessionMembers"
//             " set RejectMsgFlag = %s where SessionId = '%s' and UserId = %ld");
//     sql::exec(UPDATE_SESSIONMEMBERS_REJECTMSGFLAG3 %status %sessionid %userid);
// 
//     return json::object();
// }

json::object Service::modifyGroupName(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sessionid");
    // string userid = req.param("userid");
    string groupName = req.param("groupName");

    chat_group & cg = chatmgr::inst().chat_group(sessionid);
    if(groupName != cg.name())cg.rename(groupName, cli);

    return json::object();
}

// GET /leaveJoinTalkBar sessionid=KK1008A type=2
json::object Service::leaveJoinTalkBar(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sessionid");
    int type = lexical_cast<int>(req.param("type"));

    if(1 == type){
        if (sessionid == cli->roomsid_)
            Client::check_join_chat(cli, string());
    }
    if(2 == type){
        if (sessionid != cli->roomsid_)
            Client::check_join_chat(cli, sessionid);
    }

    return json::object();
}

json::object Service::says(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID otherid = lexical_cast<int>(req.param("otherUserid"));
    int relation = 1;

    if(1000 == otherid) THROW_EX(EN_Operation_NotPermited);
    if(  !(cli->gwid().empty()) ) {
        bar_ptr pbar = bars_mgr::inst().get_session_bar(cli->gwid());
        if ( pbar->trader_id == otherid ) {
            THROW_EX(EN_Operation_NotPermited);
        }
    }
    if(CheckWhisper(userid, otherid))return json::object()("relationship",relation);

    sql::exec(format(INSERT_WHISPERS2) %userid %otherid);

    json::object rep;
    sql::datas datas1(format(SELECT_WHISPERS2) %otherid %userid); 
    if(sql::datas::row_type row1 = datas1.next()){
        json::array my_ot;
        json::object my;
        json::object ot;

        json::object o_user = GetUserInfo(otherid);
        if(o_user.empty()) THROW_EX(EN_TargetUser_NotFound);
        string o_whisper_time = row1.at(3,"");
        ot("userid", otherid)
            ("icon",o_user.get<string>("headIcon"))
            ("userName",o_user.get<string>("userName"))
            ("gender",o_user.get<string>("gender"))
            ("guest",false)
            ("date",o_whisper_time)
            ;

        string m_whisper_time = time_string();
        json::object m_user = cli->user_info();
        my("userid", userid)
            ("icon",m_user.get<string>("icon"))
            ("userName",m_user.get<string>("nick"))
            ("gender", m_user.get<string>("gender"))
            ("guest",false)
            ("date",m_whisper_time)
            ;
        relation = 3;
        my_ot(ot)(my);

        imessage::message msg("chat/pair", SYSADMIN_UID, make_p2pchat_id(otherid, SYSADMIN_UID));
        // msg.gid = make_p2pchat_id(otherid, SYSADMIN_UID);
        // msg.from = SYSADMIN_UID;
        // msg.type = "chat/pair";
        client_ptr system_ptr = sysadmin_client_ptr();
        msg.body
            ("content", my)
            // ("from", system_ptr->brief_user_info()) ("time", m_whisper_time)
            ;
        chatmgr::inst().send(msg, cli);

        rep("relationship",relation)("resultArray",my_ot);
    }
    else{
        rep("relationship",relation);
    }

    return rep;
}

json::object Service::personInfo(http::request & req, client_ptr& cli)
{
    string otherid = req.param("contactid");
    UID userid = cli->user_id();
    UID i_otherid = lexical_cast<int>(otherid);

    client_ptr other = xindex::get(i_otherid);
    if(!other){ THROW_EX(EN_TargetUser_NotFound); }
    json::object users = GetUserInfo(other);
    if(users.empty()){ THROW_EX(EN_TargetUser_NotFound); }

    int relation1 = 3;
    int relation2 = 3;
    int fans = 0;
    string remark;
    bool guest = other->is_guest();
    string bgIcon;
    bool issay = false;
    json::array imgs;

    if(!guest){
        json::object datas = GetIndividualDatas(i_otherid);
        fans = datas.get<int>("fans",0);
        bgIcon = datas.get<string>("background","");
        imgs = GetIndividualAlbum(i_otherid);
        issay = CheckWhisper(userid, i_otherid);

        if(1000 != i_otherid){
#define SELECT_CONTACT2 "select relation,OtherName from contacts where UserId=%1% and OtherId=%2%"
            sql::datas datas1(format(SELECT_CONTACT2) %userid %otherid);
            if(sql::datas::row_type row1 = datas1.next()){
                relation1 = lexical_cast<int>(row1.at(0));
                // if(3!=relation1) remark = row1.at(1, "");
                remark = row1.at(1, "");
            }

            sql::datas datas2(format(SELECT_CONTACT2) %otherid %userid );
            if(sql::datas::row_type row2 = datas2.next()){
                relation2 = lexical_cast<int>(row2.at(0));
            }
        }
        else{
            relation1 = 1;
            relation2 = 1;
        }
    }

    users.erase("phone");
    users.erase("mail");
    return users("remark",remark)
        ("fans", fans)
        ("guest", guest)
        ("bgIcon", bgIcon)
        ("relationShip", relation1)
        ("otherRelationship", relation2)
        ("isSay", issay)
        ("icons", imgs)
        ;
}

json::object Service::updateUserBgIcon(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();

    json::object datas = GetIndividualDatas(userid);
    string path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
    string url = write_file(path, req.content());

    format fmt("UPDATE users SET background='%1%' WHERE UserId=%2%");
    sql::exec(fmt % url % userid);
    cli->user_info().put("background", complete_url(url));

    //format fmt(UPDATE_INDIVIDUALDATAS_BACKGROUND2);
    //sql::exec(fmt %url %userid);
    return json::object()("icon", complete_url(url));
}

// GET /attentions attentionid=15250 type=1
json::object Service::attentions(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID touid = lexical_cast<UID>(req.param("attentionid"));
    int type = lexical_cast<int>(req.param("type"));

//    RECORD_TIMER(timer_log);
    if(cli->is_guest()) THROW_EX(EN_Operation_NotPermited);
    if(cli->check_black(touid)) THROW_EX(EN_Black);
    if(1000 == touid) THROW_EX(EN_Operation_NotPermited);

    client_ptr ot = xindex::get(touid);
    if(!ot){ THROW_EX(EN_TargetUser_NotFound); }
    if(ot->is_guest()) THROW_EX(EN_Operation_NotPermited);
    if(ot->check_black(userid)) THROW_EX(EN_Blacked);

    if(1 == type){
        // sql::exec(format(UPDATE_CONTACTS_INSERT_RELATION3)%userid %touid %1);
        string other_name = "";
        format fmt(SELECT_CONTACTS_OTHERNAME3);
        sql::datas datas(fmt %userid %touid);
        if(sql::datas::row_type row = datas.next()){
            other_name = row.at(0,"");
            sql::exec(format(UPDATE_CONTACTS_RELATION3)%1 %userid %touid);
            if(!other_name.empty()){
                sql::exec(format(UPDATE_CONTACTS_USERNAME3)%other_name %touid %userid);
            }
        }
        else{
            int charm = 5;
            sql::exec(format(INSERT_CONTACTS3) %userid %touid %1);
            // sql::exec(format(INSERT_STATISTICS_CHARISMA) %touid %userid %charm %cli->gwid());
            incr_charms(touid, charm, userid);
            //if ( client_ptr o = xindex::get( touid ) ) {
            //    // o->set_bar_charm( cli->gwid(), charm);
            //    o->set_charm( userid, touid, charm );
            //    // push_charm(cli, touid, charm);
            //}
        }

        cli->insert_admire(touid,time_string(), other_name);
        ot->insert_fan(userid, time_string());

        imessage::message msg("chat/fans", cli->user_id(), make_p2pchat_id(touid, cli->user_id()));
        json::object from = cli->brief_user_info();
        from("sign", cli->user_info().get<string>("signature",""));
        msg.body
            ("from", from)
            ;
        chatmgr::inst().send(msg, cli);

        //version 1.1
        // format fmt(SELECT_CONTACTS_OTHERNAME3);
        // sql::datas datas(fmt %userid %touid);
        // if(sql::datas::row_type row = datas.next()){
        //     const char* p_other_name = row.at(0,"");
        //     cli->insert_admire(touid,time_string(), p_other_name);
        //     ot->insert_fan(userid, time_string());
        // }
    }
    else if(2 == type){
        sql::exec(format(UPDATE_CONTACTS_RELATION3)%3 %userid %touid);
        // sql::exec(format(UPDATE_CONTACTS_USERNAME3)%"" %touid %userid);

        imessage::message msg("chat/fans-lose", cli->user_id(), make_p2pchat_id(touid, cli->user_id()));
        chatmgr::inst().send(msg, cli);
        
        //version 1.1
        cli->delete_admire(touid);
        ot->delete_fan(userid);
    }

    return json::object();
}

json::object Service::relationship(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    if ( 2000 == userid ) {
        userid = lexical_cast<UID>(req.param("userid"));
        cli = xindex::get( userid );
    }
    UID otherid = lexical_cast<UID>(req.param("memberid"));
    int type = lexical_cast<int>(req.param("type"));

    if(1000 == otherid) THROW_EX(EN_Operation_NotPermited);
    if(1 == type){
        sql::exec(format(UPDATE_CONTACTS_INSERT_RELATION3)%userid %otherid %2);
        sql::exec(format(UPDATE_CONTACTS_RELATION_NOREJECT3)%3 %otherid %userid);

        imessage::message msg("chat/black", cli->user_id(), make_p2pchat_id(otherid, cli->user_id()));
        chatmgr::inst().send(msg, cli);

        cli->insert_black(otherid);
        //version 1.1
        cli->delete_admire(otherid);
        cli->delete_fan(otherid);

        client_ptr other = xindex::get(otherid);
        cli->delete_admire(userid);
        cli->delete_fan(userid);

    } else if(2 == type){
        sql::exec(format(UPDATE_CONTACTS_RELATION3)%3 %userid %otherid);
        cli->erase_black(otherid);
    }

    return json::object();
}

json::object Service::remark(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID otherid = lexical_cast<UID>(req.param("userid"));
    string remark = sql::db_escape(req.param("remark",""));
    
    if(1000 == otherid) THROW_EX(EN_Operation_NotPermited);
    if(cli->check_black(otherid)) THROW_EX(EN_Black);
    client_ptr ot = xindex::get(otherid);
    if(!ot){ THROW_EX(EN_TargetUser_NotFound); }
    if(ot->check_black(userid)) THROW_EX(EN_Blacked);

    sql::exec(format(UPDATE_CONTACTS_OTHERNAME3) %remark %userid %otherid);
    sql::exec(format(UPDATE_CONTACTS_INSERT_USERNAME3)%otherid %userid %remark);

    //version 1.1
    cli->remark_admire(otherid, remark);
    return json::object();
}

json::object Service::contactList(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    int devtype = lexical_cast<int>(req.param("deviceType","3"));
    format fmt(SELECT_CONTACTS_ADMIRE1);
    json::array fans;
    json::array admires;
    json::array blacks;
    json::array barlis;
    set<UID> public_users;

    boost::system::error_code ec;
    UInt has_ver = lexical_cast<UInt>( req.param("ver","0") );
    UInt ver = has_ver;

    BOOST_FOREACH(auto & bsid, cli->get_attention_bars()) {
        boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(bsid,ec);
        if (!pbar)
            continue;
        UInt v = pbar->bar_user_info_ver();
        if (v <= has_ver)
            continue;
        ver = std::max(ver, v);

        json::object obj = json_bar_brief(*pbar);
        if( 1 == devtype ) {
            string background_url("");
            sql::datas datas(format("SELECT I_image1,absolute_url "
                        "FROM ci_background_pic WHERE SessionId='%1%'") %pbar->sessionid);
            if(sql::datas::row_type row = datas.next()){
                const char* p_img = row.at(0);
                const char* p_url = row.at(1);
                if (NULL!=p_url && NULL!=p_img) {
                    background_url = p_url;
                    background_url += p_img;
                }
            }
            obj("background_url", background_url);
        }
        barlis.put(obj);
        public_users.insert( pbar->trader_id );
    }

    //    RECORD_TIMER(timer_log);
    {
        json::object system = sysadmin_client_ptr()->user_info();
        UInt v = sysadmin_client_ptr()->user_info_ver();
        if (v > has_ver)
        {
            ver = std::max(ver, v);
            admires(json::object()("userid", 1000)
                    ("nickName", system.get<string>("nick"))
                    ("icon",system.get<string>("icon"))
                    ("sign", system.get<string>("signature"))
                    ("remarkName", "")
                    ("gender",system.get<string>("gender"))
                    ("date", cli->user_info().get<string>("regist_time"))
                   );
            fans(json::object()("userid", 1000)
                    ("nickName", system.get<string>("nick"))
                    ("icon",system.get<string>("icon"))
                    ("sign", system.get<string>("signature"))
                    ("remarkName", "")
                    ("gender",system.get<string>("gender"))
                    ("date", cli->user_info().get<string>("regist_time"))
                );
        }
    }

    sql::datas datas1(fmt %userid);
    while(sql::datas::row_type row1 = datas1.next()){
        const char *potherid = row1.at(2);
        const char *pothername = row1.at(3,"");
        int relation = lexical_cast<int>(row1.at(4, "0"));
        const char *ptime = row1.at(5);
        if(NULL == potherid) continue;
        UID otherid = lexical_cast<int>(potherid);
        if ( public_users.end() != public_users.find( otherid ) ) {
            continue;
        }
        client_ptr pother = xindex::get(otherid);
        if (!pother)
            continue;
        UInt v = pother->user_info_ver();
        if (v <= has_ver)
            continue;
        ver = std::max(ver, v);

        json::object user_info = GetUserInfo(pother);
        if(user_info.empty()) continue;

        if(1 == relation){ // 关注
        admires(json::object()("userid",otherid)
               ("nickName", user_info.get<string>("userName"))
               ("icon",user_info.get<string>("headIcon"))
               ("sign", user_info.get<string>("sign"))
               ("remarkName",pothername)
               ("gender",user_info.get<string>("gender"))
               ("date", ptime)
               );
        }
        else if(2 == relation){ // 拉黑
        blacks(json::object()("userid",otherid)
               ("nickName", user_info.get<string>("userName"))
               ("icon",user_info.get<string>("headIcon"))
               ("sign", user_info.get<string>("sign"))
               ("remarkName",pothername)
               ("gender",user_info.get<string>("gender"))
               ("date", ptime)
               );
        }
    }

    fmt = format(SELECT_CONTACTS_FANS1) %userid;
    sql::datas datas2(fmt);
    while(sql::datas::row_type row2 = datas2.next()){
        const char *ppeerid = row2.at(0);
        const char *ppeername = row2.at(1,"");
        int relation = lexical_cast<int>(row2.at(4, "0"));
        const char *ptime = row2.at(5);
        if(NULL == ppeerid) continue;
        UID userid = lexical_cast<int>(ppeerid);
        client_ptr puser = xindex::get(userid);
        if (!puser)
            continue;
        json::object user = GetUserInfo(puser);
        if(user.empty() || 1!=relation) continue;

        UInt v = puser->user_info_ver();
        if (v <= has_ver)
            continue;
        ver = std::max(ver, v);

        fans(json::object()("userid",userid)
               ("nickName", user.get<string>("userName"))
               ("icon",user.get<string>("headIcon"))
               ("sign", user.get<string>("sign"))
               ("remarkName",ppeername)
               ("gender",user.get<string>("gender"))
               ("date", ptime)
            );
    }

    return json::object()("attentionList", admires)
                         ("relationShipList", blacks)
                         ("fansList", fans)
                         ("bars", barlis)
                         ("ver", ver)
                         ;
}

json::object Service::relationShipList(http::request & req, client_ptr& cli)
{
    UID userid = lexical_cast<int>(req.param("userid"));
    int type = lexical_cast<int>(req.param("type"));

    if(1000 == userid) THROW_EX(EN_Operation_NotPermited);
    format fmt;
    if(1 == type){
        fmt = format(SELECT_CONTACTS_ADMIRE1) %userid;
    }
    else if (2 == type){
        fmt = format(SELECT_CONTACTS_FANS1) %userid;
    }
    else{
        THROW_EX(EN_Input_Data);
    }

    json::array list;
    sql::datas datas(fmt);
    string peername;
    string peerid;
    string admire_time;
    while(sql::datas::row_type row = datas.next()){
        int relation = 3;
        if(1 == type){
           peerid = row.at(2,"0");
           peername = row.at(3,"");
           relation = lexical_cast<int>(row.at(4, "0"));
           admire_time  = row.at(5);
        }
        else if(2 == type){
           peerid = row.at(0,"0");
           peername = row.at(1,"");
           relation = lexical_cast<int>(row.at(4, "0"));
           admire_time = row.at(5);
        }

        UID userid = lexical_cast<int>(peerid);
        if(0==userid || 1!=relation) continue;

        json::object user = GetUserInfo(userid);
        if(user.empty()) continue;

        user.erase("mail");
        user.erase("phone");
        user.erase("constellation");
        user.rename("headIcon", "icon");
        user.rename("userName", "nickName");
        user("remarkName", peername)
            ("date", admire_time)
            ;

        list.push_back(user);
    }

    return json::object()("list",list);
}

json::object Service::memberList(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sessionid");
    // UID userid = cli->user_id();

//    RECORD_TIMER(timer_log);
    chat_group & cg = chatmgr::inst().chat_group(sessionid);
    // if(!(cg.is_member(cli->user_id()))) THROW_EX(EN_Operation_NotPermited); 

    boost::system::error_code ec;
    UID public_user = INVALID_UID;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid, ec);
    if ( !ec ) {
        public_user = pbar->trader_id;
    }
    auto const& memberlist = cg.alive_members();
    json::array list;
    for(auto itr=memberlist.begin(); itr != memberlist.end(); ++itr)
    {
        UID otherid = *itr;
        // if (otherid == cli->user_id()) continue;
        if ( otherid == public_user ) continue;
        client_ptr other = xindex::get(otherid);
        if(other){
            json::object user = GetUserInfo(other);
            if(user.empty() || other->is_guest()) continue;

            user.erase("mail");
            user.erase("phone");
            user.erase("constellation");
            user.rename("headIcon","icon");
            user("guest", other->is_guest());

            list.push_back(user);
        }
    }

    return json::object()("groupName", cg.name())("memberList",list);
}

json::object Service::myInfo(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();

    json::object users = GetUserInfo( cli );

    json::object datas = GetIndividualDatas(userid);
    json::array imgs = GetIndividualAlbum(userid);

    users.erase("mail");
    users.erase("phone");
    users.rename("headIcon","icon");
    users("guest", cli->is_guest());

    users("attentions", datas.get<int>("admires",0))
        ("fans", datas.get<int>("fans",0))
        ("coins",datas.get<int>("coins",0))
        ("bgIcon", datas.get<string>("background",""))
        ("icons", imgs)
        ;
    return users += GetGiftsStatistic(userid);
}

json::object Service::uploadMyPhoto(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    int charm = 10;

    string path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
    string url = write_file(path, req.content());
    int id = IndividualAlbum_message_id();
    format fmt(INSERT_INDIVIDUALALBUM3);
    sql::exec(fmt %userid %id %url);

    if ( cli->pic_charm_count > 0 ) {
        incr_charms(cli, charm);
        // cli->set_charm( userid, userid, charm );
        --cli->pic_charm_count;
    }

    return json::object()
        ("charm", charm)
        ("iconid", id)
        ("icon",complete_url(url))
        ("date", time_string())
        ;
}

json::object Service::uploadFile(http::request & req, client_ptr& cli)
{
    string path = uniq_relfilepath(
            req.param("fileName"), lexical_cast<string>( cli->user_id() ));
    string url = write_file(path, req.content());
    return json::object() ("dynamicImg", url) ;
}

static std::string get_bar_address(std::string const & barsid)
{
    if (!barsid.empty())
    {
        auto b = bars_mgr::inst().get_session_bar(barsid);
        if (b)
        {
            return b->barname; // field changed
            return b->address; // old
        }
    }
    return std::string();
}

static json::object json_user_info(client_ptr c)
{
    // client_ptr c = xindex::get(uid);
    if (!c)
        return json::object();
    std::string emptystr; // = complete_url("default/dynamic-bg.png");
    json::object const & uinf = c->user_info();
    return json::object()
        ("headIcon", uinf.get<std::string>("icon"))
        ("userName", uinf.get<std::string>("nick"))
        ("userid", c->user_id())
        ("gender", uinf.get<std::string>("gender"))
        ("sign", uinf.get<std::string>("signature"))
        ("age", uinf.get<std::string>("age"))
        ("backgroudImg", uinf.get<std::string>("background", emptystr))
        ;
}
static json::object json_user_info_2(client_ptr c)
{
    if (!c)
        return json::object();
    json::object const & uinf = c->user_info();
    return json::object()
        ("headIcon", uinf.get<std::string>("icon"))
        ("userName", uinf.get<std::string>("nick"))
        ("userid", c->user_id())
        ;
}
static json::object json_dynamic_newrsp(dynamic_newrsp_t const & drsp)
{
    json::array imgs;
    BOOST_FOREACH(auto & x, drsp.rsp.imgs)
        imgs.put( complete_url(x) );
    json::object ret = json_user_info_2( xindex::get(drsp.rsp.userid) );
    ret
        ("message", drsp.dyn->words) 
        ("iconList", imgs) 
        ("support", drsp.dyn->n_support()) // 顶条数
        //
        ("commentText", drsp.rsp.words) 
        ("date", time_string(drsp.rsp.xtime))
        ;
    return ret;
}
static json::object json_dynamic(dynamic_state const & ds, client_ptr & c)
{
    json::array imgs;
    BOOST_FOREACH(auto & x, ds.imgs)
        imgs.put( complete_url(x) );
    json::object ret = json_user_info( xindex::get(ds.userid) );
    ret
        // ("index", ds.max_id())
        ("index", ds.id)
        ("dynamicid", ds.id)
        ("comment", ds.n_comment()) // 评论条数
        ("support", ds.n_support()) // 顶条数
        ("deviceType", ds.devtype) 
        ("publishAddress", "")//(get_bar_address(ds.barsid))
        ("message", ds.words) 
        ("iconList", imgs) 
        // ("type", type)
        ("date", time_string(ds.xtime))
        ("status", bool(ds.supports.find(c->user_id())!=ds.supports.end()))
        ;
    return ret;
}
static json::object json_dyncomment(dynamic_message & c, dynamic_state_ptr dyn)
{
    json::object ret = json_user_info( xindex::get(c.userid) );
    return ret
        ("index", c.id)
        ("commentid", c.id)
        // ("type", int)
        ("message", c.words)
        ("date", time_string(c.xtime))
        ("dynamicid", dyn->id)
        ;
}

// echo "{\"message\":\"@`date`\", \"imgList\":[]}" | POST /publishDynamicMessage deviceType=1 
json::object Service::publishDynamicMessage(http::request & req, client_ptr& cli)
{
    json::object jd = json::decode(req.content());

    std::string words = jd.get<std::string>("message");
    json::array imgs;
    imgs = jd.get<json::array>("imgList", imgs);
    if (imgs.empty() && words.empty())
        MYTHROW(EN_Input_Data,client_error_category);

    dynamic_message m(cli->user_id());
    m.barsid = cli->spot_;
    m.devtype = lexical_cast<int>(req.param("deviceType"));
    m.words.swap( words );
    BOOST_FOREACH(auto & x, imgs)
        m.imgs.push_back( boost::get<std::string>(x) );

    new_dynamic(m);

    return json::object()("id",m.id);
}

// POST /publishComment dynamicid=1 deviceType=1
json::object Service::publishComment(http::request & req, client_ptr& cli)
{
    UInt dynid = lexical_cast<int>(req.param("dynamicid"));

    std::string words = req.content();
    if (words.empty())
        MYTHROW(EN_Input_Data,client_error_category);

    dynamic_message m(cli->user_id());
    m.barsid = cli->spot_;
    m.devtype = lexical_cast<int>(req.param("deviceType"));
    m.words.swap( words );

    BOOST_ASSERT(!m.is_like());
    if (!new_dynamic_comment(dynid, m))
        MYTHROW(EN_Moment_NotExist,service_error_category);

    return json::object();
}

// GET /supportComment dynamicid=1
json::object Service::supportComment(http::request & req, client_ptr& cli)
{
    UInt dynid = lexical_cast<UInt>( req.param("dynamicid") );

    dynamic_message m(cli->user_id());
    m.barsid = cli->spot_;
    m.devtype = lexical_cast<int>(req.param("deviceType"));

    BOOST_ASSERT(m.is_like());
    if (!new_dynamic_comment(dynid, m))
        MYTHROW(EN_Moment_NotExist,service_error_category);

    // like_dynamic(dynid, cli->user_id());
    return json::object();
}

// GET /dynamicMessage type=3 index=0 pageSize=20
json::object Service::dynamicMessage(http::request & req, client_ptr& cli)
{
    int type = lexical_cast<int>( req.param("type") ); // 1:好友，2:现场，3:我的动态
    UInt idx = lexical_cast<UInt>( req.param("index") );
    int count = lexical_cast<int>( req.param("pageSize") );
    // UInt dynid = lexical_cast<UInt>( req.param("dynamicid") );

    auto & dynst = get_dynamic_stat(cli->user_id());
    client_ptr puser = cli;
    json::array dyns;

    enum { DT_friends=1, DT_spot, DT_userid, DT_spot2 };
    if (type == DT_friends)
    {
        auto rng = list_rcpt_dynamic( cli->user_id(), idx, count );
        if (!boost::empty(rng))
        {
            UInt last_id = 0;
            auto const & rev = boost::adaptors::reverse(rng);
            BOOST_FOREACH(auto & it, rev)
            {
                dyns.put( json_dynamic(*it.dyn, cli)("type",type) );
                last_id = std::max(last_id, it.dyn->id);
            }
            //LOG_I << last_id <<" "<< boost::begin(rev)->dyn->id;
            LOG_I << cli->user_id()
                <<" "<< last_id
                <<" "<< dynst.synced_dynshare_id
                <<" "<< dynst.last_dynshare_id
                <<" "<< dynst.last_dynshare_userid
                ;
            dynst.trace_last_sync(cli->user_id(), *boost::begin(rev));
        }
    }
    //else if (type == DT_spot || type == DT_spot2)
    //{
    //    std::string sid;
    //    if (type == DT_spot)
    //        sid = cli->gwid();
    //    else
    //        sid = req.param("sceneid");
    //    if (!sid.empty())
    //    {
    //        auto rng = list_bar_dynamic( sid, idx, count );
    //        BOOST_FOREACH(auto & ds,  boost::adaptors::reverse(rng))
    //            dyns.put( json_dynamic(*ds.dyn, cli)("type",type) );
    //    }
    //    // MYTHROW(EN_Bar_NotFound,service_error_category);
    //}
    else if (type == DT_userid)
    {
        UID uid = lexical_cast<UID>(req.param("userid","0"));
        if (uid && uid != cli->user_id())
        {
            puser = xindex::get(uid);
            if (!puser)
                MYTHROW(EN_User_NotFound,client_error_category);
        }
        auto rng = list_user_dynamic( puser->user_id(), idx, count );
        BOOST_FOREACH(auto & ds,  boost::adaptors::reverse(rng))
            dyns.put( json_dynamic(*ds, cli)("type",type) );
    }
    else
    {
        MYTHROW(EN_Input_Data,client_error_category);
    }

    json::object unread;
    unread.put("messages", dynst.unread.count);
    unread.put("userid", dynst.unread.last_userid);
    unread.put("headIcon", get_head_icon(dynst.unread.last_userid));

    json::object ret = json_user_info(puser);
    return ret("pageSize", count)("unRead", unread)("dynamicList", dyns);
}

// GET /unReadList type=1 index=0 pageSize=20
json::object Service::unReadList(http::request & req, client_ptr& cli)
{
    int idx = lexical_cast<UInt>( req.param("index","0") );
    int count = lexical_cast<int>( req.param("pageSize","100") );

    auto rng = fetch_dynrsps( cli->user_id(), idx, count );

    json::array dyns;
    BOOST_FOREACH(auto & ds, boost::adaptors::reverse(rng))
    {
        dyns.put( json_dynamic_newrsp(ds)
                    ("dynamic", json_dynamic(*ds.dyn, cli))
                );
    }

    return json::object()("unreadList", dyns);
}

// GET /dynamicCommentList index=0 pageSize=20 dynamicid=1
json::object Service::dynamicCommentList(http::request & req, client_ptr& cli)
{
    int idx = lexical_cast<int>( req.param("index") );
    int count = lexical_cast<int>( req.param("pageSize") );
    UInt dynid = lexical_cast<UInt>( req.param("dynamicid") );
    
    json::array comments;

    auto dyn = get_dynamic(dynid, cli->user_id());
    if (dyn)
    {
        auto rng = list_dyncomments( dyn, idx, count );
        BOOST_FOREACH(auto & c, boost::adaptors::reverse(rng))
            comments.put( json_dyncomment(c, dyn) );
    }

    return json::object()
        ("pageSize", (int)comments.size())
        ("commentList", comments)
        ;
}

// GET /deleteComment dynamicid=x commentid=x
json::object Service::deleteComment(http::request & req, client_ptr& cli)
{
    UInt dynid = lexical_cast<UInt>( req.param("dynamicid") );
    UInt comid = lexical_cast<UInt>( req.param("commentid") );
    delete_dynamic_comment(cli->user_id(), dynid, comid);
    return json::object();
}

// GET /deleteDynamic dynamicid=x
json::object Service::deleteDynamic(http::request & req, client_ptr& cli)
{
    UInt dynid = lexical_cast<UInt>( req.param("dynamicid") );
    delete_dynamic(cli->user_id(), dynid);
    return json::object();
}

json::object Service::modifyMyHead(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();

    if(cli->is_guest()) return json::object();
    string url;
    string path;
    path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
    url = write_file(path, req.content());
    // thumbnail(path, req.content());

    UInt ver = cli->user_info_ver(+1);
    format fmt("UPDATE users set icon='%1%',ver=%2% where UserId=%3%");
    sql::exec(fmt %url % ver %userid);
    url = complete_url(url);
    cli->user_info().put("icon", url);

    return json::object()("img",url);
}

json::object Service::modifyMyName(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) return json::object();
    UID userid = cli->user_id();
    string nick = req.param("userName");
    UInt ver = cli->user_info_ver(+1);
    format fmt("UPDATE users set nick='%1%',ver=%2% where UserId=%3%") ;
    sql::exec(fmt %sql::db_escape(nick) %ver %userid);

    cli->user_info().put("nick", nick);

    return json::object();
}

json::object Service::modifyMyAge(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) return json::object();
    UID userid = cli->user_id();
    string age = req.param("age");
    string constellation = req.param("constellation");
    UInt ver = cli->user_info_ver(+1);
    format fmt("update users set age='%1%',constellation='%2%',ver=%3% where UserId=%4%") ;
    sql::exec(fmt %age %constellation %ver %userid);

    cli->user_info().put("age", age);
    cli->user_info().put("constellation", constellation);

    return json::object();
}

json::object Service::modifySign(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    if(cli->is_guest()) return json::object();

    const string& sign = req.content();
    UInt ver = cli->user_info_ver(+1);
    format fmt("update users set signature='%1%',ver=%2% where UserId=%3%");
    sql::exec(fmt %sql::db_escape(sign) % ver %userid);

    int charm = 0;
    if ( cli->user_info().get<string>("sign", "").empty() ) {
        charm = 10;
        incr_charms(cli, charm);
        // cli->set_charm( userid, userid, charm );
    }

    cli->user_info().put("sign", sign);

    return json::object()("charm", cli->get_charm());
}

json::object Service::deleteMyPhoto(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) return json::object();
    UID userid = cli->user_id();
    int id = lexical_cast<int>(req.param("iconid"));
    sql::exec(format(DELETE_INDIVIDUALALBUM2)%id %userid);

    return json::object();
}

json::object Service::inviteJoinGroup(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) return json::object();
    // UID userid = cli->user_id();
    string sessionid = req.param("sessionid");

    chat_group & cg = chatmgr::inst().chat_group(sessionid);
    if(!(cg.is_alive_member(cli->user_id()))) THROW_EX(EN_Operation_NotPermited); 

    // if (!cg.ierror_categorys_member(userid))
    // {
    //     THROW_EX(EN_Operator_Not_Permited);
    // }

    // format fmt(SELECT_SESSIONMEMBERS_USERID2);
    // sql::datas datas1(fmt %sessionid %userid);
    // if(sql::datas::row_type row1 = datas1.next())
    {

        vector<UID> MemberVector;
        json::object jo = json::decode(req.content());
        json::array members = jo.get<json::array>("members");
        for (json::array::iterator it = members.begin(); it != members.end(); ++it){
            UID memberid = get<int>(*it);
            client_ptr pmem = xindex::get(memberid);
            if(pmem){
                if(pmem->is_guest()) THROW_EX(EN_Operation_NotPermited);
                MemberVector.push_back(memberid);
            }
        }

        vector<UID>::iterator it = unique(MemberVector.begin(), MemberVector.end());
        MemberVector.resize(distance(MemberVector.begin(),it));

        cg.add(MemberVector.begin(), MemberVector.end(), cli);
    }

    return json::object();
}

json::object Service::delGroupMember(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) return json::object();
    string sessionid = req.param("sessionid");
    if (is_p2pchat(sessionid))
        return json::object();

    UID uid = lexical_cast<UID>(req.param("memberid"));

    chat_group & cg = chatmgr::inst().chat_group(sessionid);
    if(!(cg.is_alive_member(cli->user_id()))) THROW_EX(EN_Operation_NotPermited); 
    cg.remove(uid, cli);

    return json::object();
}

json::object Service::quitGroup(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) return json::object();
    string sessionid = req.param("sessionid");
    if (is_p2pchat(sessionid))
        return json::object();

    UID userid = cli->user_id();
    chat_group & cg = chatmgr::inst().chat_group(sessionid);
    cg.remove(userid, cli);

    return json::object();
}

json::object Service::login(http::request & req, client_ptr& cli)
{
    //    RECORD_TIMER(timer_log);
    // string sid = req.param("sessionid","");
    // if (!sid.empty() && is_barchat(sid))
    // {
    //     Client::check_join_chat(cli, sid);
    // }

    UID userid = cli->user_id();

    json::object info = cli->user_info();
    info.rename("icon", "headIcon");
    info.rename("nick", "userName");
    info.rename("signature", "sign");
    info.put("token", cli->client_id());

    json::array imgs = GetIndividualAlbum(userid);
    info.put("icons", imgs);

    json::object datas = GetIndividualDatas(userid);
    info.put("attention",datas.get<int>("admires",0));
    info.put("fans", datas.get<int>("fans",0));
    info.put("coins",datas.get<int>("money",0));
    info.put("bgIcon",datas.get<string>("background",""));
    info.put("rich", cli->get_rich());
    info.put("charm", cli->get_charm());

    cli->load_attention_bars(datas.get<string>("attention_bars",""));

    info += GetGiftsStatistic(userid);
    info += pay_setting();

    if (userid == uid_logdbg_)
    {
        info.put("allowRecord",true);
    }
    // TODO

    return info;
}

json::object Service::logout(http::request & req, client_ptr& cli)
{
    // cli->cache().put("nopush", true);

    UID userid = cli->user_id();
   
    // Not necessary // sql::exec(format(DELETE_QUIT_SESSIONS1) %userid);
    sql::exec(format(UPDATE_INDIVIDUALDATAS_PUSHSTATUS1) %userid);

    return json::object();
}

json::object Service::receivedGifts(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    long index = lexical_cast<int>(req.param("index","0"));

    if(index < 1) index = LONG_MAX;
    json::array gifts;
    format fmt(SELECT_USERGIFTS2);
    sql::datas datas1(fmt %index %userid);
    while(sql::datas::row_type row1 = datas1.next()){
        const char *pid = row1.at(0);
        const char *pfromuserId = row1.at(2);
        const char *ptype = row1.at(3);
        const char *precvtime = row1.at(4,"");
        if(NULL==pid || NULL==pfromuserId || NULL==ptype) continue;

        UID peerid = lexical_cast<int>(pfromuserId);
        client_ptr other = xindex::get(peerid);
        if(!other) continue;
        json::object user = other->brief_user_info();
        if(user.empty()) continue;

        gifts(json::object()("userid",peerid)
                ("sendIcon",user.get<string>("icon"))
                ("index",lexical_cast<int>(pid))
                ("sendName",user.get<string>("userName"))
                ("type",lexical_cast<int>(ptype))
                ("date", precvtime)
             );
        // fmt = format(SELECT_GIFTS1) %pgiftid;
        // sql::datas datas2(fmt);
        // if(sql::datas::row_type row2 = datas2.next()){
        //     const char *pgiftname = row2.at(1,"");
        //     const char *pgifturl = row2.at(2);
        //     const char *ptype = row2.at(3,"1");
        //     const char *pprice = row2.at(4,"0");
        //     if(NULL == pgifturl) continue;

        //     gifts(json::object()("userid",user.get<int>("userid"))
        //             ("sendIcon",user.get<string>("icon"))
        //             ("index",lexical_cast<int>(pid))
        //             ("sendName",user.get<string>("userName"))
        //             ("giftName",pgiftname)
        //             ("type",lexical_cast<int>(ptype))
        //             ("giftIcon",pgifturl)
        //             ("date", precvtime)
        //             ("price",lexical_cast<int>(pprice))
        //          );
        // }
        // else {continue;}
    }

    return json::object()("pageSize", static_cast<int>(gifts.size()))
                         ("giftList",gifts);
}

json::object Service::shops(http::request & req, client_ptr& cli)
{
    // UID userid = cli->user_id();
    format fmt(SELECT_GOODS_ALL);
    json::array goods;
    sql::datas datas(fmt);
    while(sql::datas::row_type row = datas.next())
    {
        const char *pid = row.at(0);
        const char *pgoodsname = row.at(1,"");
        const char *picon = row.at(2,"");
        if(!pid) continue;

        goods(json::object()("shopName",pgoodsname)
                            ("icon", complete_url(picon))
                            ("shopid",lexical_cast<int>(pid))
             );
    }

    return json::object()("shopList",goods);
}

json::object Service::goodsInfo(http::request & req, client_ptr& cli)
{
    int goodID = lexical_cast<int>(req.param("shopid"));
    format fmt(SELECT_GOODS_ID1);
    sql::datas datas(fmt %goodID);
    json::array imgList;

    if(sql::datas::row_type row = datas.next()){
        const char *pgoodsname = row.at(1,"");
        const char *pbrief = row.at(3,"");
        const char *pflower = row.at(4,"0");
        const char *pcar = row.at(5,"0");
        const char *pship = row.at(6,"0");
        const char *pplane = row.at(7,"0");
        const char *pimgList = row.at(8,"[]");
        json::array tmp = json::decode<json::array>(pimgList);
        for(json::array::iterator itr=tmp.begin(); itr!=tmp.end(); ++itr)
        {
            imgList(json::object()("icon", complete_url(get<string>(*itr))));
        }

        // LOG_I<<imgList;
        return json::object()("brief", pbrief)
                             ("shopName", pgoodsname)
                             ("flower", lexical_cast<int>(pflower))
                             ("car", lexical_cast<int>(pcar))
                             ("ship", lexical_cast<int>(pship))
                             ("plane", lexical_cast<int>(pplane))
                             ("imgList", imgList)
            ;

    }

    return json::object();
}

json::object Service::myExchange(http::request & req, client_ptr& cli)
{
#define SELECT_EXCHANGERADDRESS1 ("select id,UserName,UserPhone,zone,DetailAddress,goodId,express,express_number,status,indent_number,exchange_time,deliever_time from ExchangerAddress where UserId=%1% order by id desc")

    if(cli->is_guest()) return json::object();
    const string exchange_status[] ={"未发货","已发货","完成"};
    json::array myexchange;
    bool delivered;
    sql::datas datas1(format(SELECT_EXCHANGERADDRESS1) %cli->user_id());
    while(sql::datas::row_type row1 = datas1.next()){
        int id = lexical_cast<int>(row1.at(0,"0"));
        const char* UserName = row1.at(1);
        const char* UserPhone = row1.at(2,"");
        const char* zone = row1.at(3,"");
        const char* DetailAddress = row1.at(4);
        int goodId = lexical_cast<int>(row1.at(5,"0"));
        const char* express = row1.at(6,"");
        const char* express_number = row1.at(7,"");
        int status = lexical_cast<int>(row1.at(8,"3"));
        const char* indent_number = row1.at(9,"");
        const char* exchange_time = row1.at(10,"");
        const char* deliever_time = row1.at(11,"");

        if(0==goodId || NULL==UserName || NULL==DetailAddress || 3==status) continue;

        string goodsName,icon;
        sql::datas datas2(format(SELECT_GOODS_ID1) %goodId);
        if(sql::datas::row_type row2 = datas2.next()){
            goodsName = row2.at(1,"");
            icon = row2.at(2,"");
        }
        if(goodsName.empty()) continue;

        delivered = status? true:false;
        myexchange(
                json::object()("id", id)
                              ("userName",UserName)
                              ("userPhone",UserPhone)
                              ("zone",zone)
                              ("detailAddress",DetailAddress)
                              ("goodsId", goodId)
                              ("express", express)
                              ("expressNumber", express_number)
                              ("status",exchange_status[status])
                              ("delivered", delivered)
                              ("indentNumber", indent_number)
                              ("exchangeTime", exchange_time)
                              ("delieverTime", deliever_time)
                              ("goodsName", goodsName)
                              ("icon", complete_url(icon))
                );
    }

    return json::object()("myexchange", myexchange);
}

json::object Service::exchangeUserInfo(http::request & req, client_ptr& cli)
{
    extern int alloc_ExchangerAddress_id();

    if(cli->is_guest()) return json::object();
    json::object jo = json::decode(req.content());
    int goodId = jo.get<int>("shopid");
    string userName = jo.get<string>("userName");
    string phone = jo.get<string>("phone");
    string zone = jo.get<string>("zone");
    string detailAddress = jo.get<string>("detailAddress");
    string password = jo.get<string>("password");
    UID userid = cli->user_id();

    // string username = sql::db_escape(req.param("userName"));
    // string phone = sql::db_escape(req.param("phone"));
    // string zone = sql::db_escape(req.param("zone"));
    // string detailAddress = sql::db_escape(req.param("detailAddress"));
    // string password = req.param("password");

    if(!cli->is_third_part()){
        if (passwd_verify(password, "UserId", userid) != userid)
        {
            THROW_EX(EN_Password_Incorrect);
        }
    }

    json::object mygifts = GetGiftsStatistic(userid);

    sql::datas datas2(format(SELECT_GOODS_ID1) %goodId);
    if(sql::datas::row_type row2 = datas2.next()){
        int flower = lexical_cast<int>(row2.at(4,"0"));
        int car = lexical_cast<int>(row2.at(5,"0"));
        int ship = lexical_cast<int>(row2.at(6,"0"));
        int plane = lexical_cast<int>(row2.at(7,"0"));

        if (flower>mygifts.get<int>("flower")
                || car>mygifts.get<int>("car")
                || ship>mygifts.get<int>("ship")
                || plane>mygifts.get<int>("plane"))
        {
            THROW_EX(EN_ExchageCondiction_Insufficient);
        }

        int id = alloc_ExchangerAddress_id();
        string indent_number = lexical_cast<string>(userid) + lexical_cast<string>(goodId) + lexical_cast<string>(id);
        sql::exec(format(INSERT_EXCHANGERADDRESS7) %sql::db_escape(userName) %sql::db_escape(phone) %sql::db_escape(zone) %sql::db_escape(detailAddress) %goodId %indent_number %cli->user_id());

        if(flower > 0) sql::exec(format(DELETE_EXCHANGEGOODS3) %userid %FLOWER %flower);
        if(car > 0) sql::exec(format(DELETE_EXCHANGEGOODS3) %userid %CAR %car);
        if(ship > 0) sql::exec(format(DELETE_EXCHANGEGOODS3) %userid %YACHT %ship);
        if(plane > 0) sql::exec(format(DELETE_EXCHANGEGOODS3) %userid %PLAN %plane);
    }
    else{
        THROW_EX(EN_Good_NotFound);
    }

    return json::object();
}

json::object Service::chargeList(http::request & req, client_ptr& cli)
{
    // UID userid = cli->user_id();
    format fmt(SELECT_MONEY);
    sql::datas datas(fmt);
    json::array list;
    while(sql::datas::row_type row = datas.next()){
        const char *pid = row.at(0);
        const char *psubject = row.at(1);
        const char *pcoins = row.at(2);
        const char *pmoney = row.at(3);
        const char *pbrief = row.at(4,"");
        const char *pintroduce = row.at(5,"");
        const char *pproductid = row.at(6,"");

        if(NULL==pcoins || NULL==pmoney || NULL==pid || NULL==psubject){
            continue;
        }
        
        list(json::object()("pid", lexical_cast<int>(pid))
                      ("coins", lexical_cast<int>(pcoins))
                      ("subject", psubject)
                      ("total_fee", (lexical_cast<int>(pmoney)/100.0))
                      ("introduction", pbrief)
                      ("body", pintroduce)
                      ("productid", pproductid)
            );
        
    }

    return json::object()("chargeList", list);
}

json::object Service::bars_people(http::request & req, client_ptr&)
{
    LOG_I<<req.content();
    json::object jo = json::decode(req.content());

    json::object rsp;
    json::array members = jo.get<json::array>("SessionIds");
    for (json::array::iterator i = members.begin(); i != members.end(); ++i)
    {
        string SessionId = get<string>(*i);
        boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(SessionId);
        if (!pbar){
            rsp(SessionId, 0);
        } else {
            rsp(SessionId, pbar->get_total());
        }
    }

    return rsp;
}

json::object Service::feedback(http::request& req, client_ptr& cli)
{
    UID userid = cli->user_id();
    string systemversion = req.param("systemVersion");
    string devicetype = req.param("deviceType");
    format fmt(INSERT_FEEDBACKS4);
    fmt %userid %systemversion %sql::db_escape(req.content()) %devicetype; 
    sql::exec(fmt);  

    return json::object();
}

// GET /createGroup 
json::object Service::create_chat(http::request & req, client_ptr& cli)
{
    LOG_I<<req.content();
    json::object jo = json::decode(req.content());

    json::array members = jo.get<json::array>("members");
    string gid = make_chatgroup_id();
    // string gid = jo.get<string>("gwid",defa);
    //if (gid != defa)
    //{
    //    if (!is_match(gid, "[^._].{4,32}"))
    //        THROW_EX(EN_Input_Form);
    //}

    // TODO rollback/commit

    std::vector<UID> uids;
    for (auto i = members.begin(); i != members.end(); ++i)
        uids.push_back(get<int>(*i)); // TODO: get<UID>

    chatmgr::inst().create_chat_group(cli->user_id(), gid, "", uids);
    // cg.add(uids.begin(), uids.end(), cli, 0);

    return json::object()("sessionid", gid);
}

/// sendMessage
// userid	int	用户ID
// sessionid	string	会话ID（群、单聊）
// type	int	1.图片，2.音频，3.文字表情
// deviceType	int	1.iphone,2.ipad,3.android
// option
// img	stream	图片（body）
// audio	stream	音频(body)
// text	string	聊天文本、表情(body)
// fileName	string	文件名
// 
// url	string	图片、音频路径
json::object Service::chat(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    string gid = req.param("sessionid");
    UID touid = lexical_cast<UID>(req.param("userid","0"));
    int msgtype = lexical_cast<int>(req.param("type"));

//    RECORD_TIMER(timer_log);
    string cont, url;
    int id=0;

    enum { MSG_PIC = 1, MSG_AUDIO, MSG_TEXT, MSG_CARTOON};
    const char* ms[] = { "", "chat/image", "chat/audio", "chat/text","chat/cartoon" };

    if( is_p2pchat(gid) ) {
        boost::regex re("[.!](\\d+)[.!](\\d+)([.!](\\d+))?");
        // boost::regex re("[.!](\\d+)[.!](\\d+).*");
        boost::smatch subs;
        if (boost::regex_match(gid, subs, re))
        {
            UID otherid = lexical_cast<int>(subs[1]);
            otherid = (userid!=otherid)? otherid:lexical_cast<int>(subs[2]);

            if ( is_bottlechat( gid ) ){
                unsigned int bid = lexical_cast<unsigned int>(subs[4]);
                auto i = drifting::inst(cli->user_id()).holding_find(bid);
                if ( i != drifting::inst(cli->user_id()).holding_end() ) {
                    if ( chat_bottle_id.find(bid) == chat_bottle_id.end() ) {

                        std::string cont = (*i)->content;
                        if ( (*i)->type == BottleMessage_Audio 
                                || (*i)->type == BottleMessage_Image)
                        {
                            cont = complete_url(cont);
                        }

                        const char* bms[] = { "", "chat/audio", "chat/image", "chat/text","chat/cartoon" };
                        imessage::message msg(bms[(*i)->type], otherid, gid);
                        msg.time_ = (*i)->xtime;
                        msg.body("content", cont) ;
                        Client::pushmsg(otherid, msg);

                        record_chat_bottle_id(bid);
                    }
                } else if (drifting::inst(otherid).holding_find(bid)
                        !=drifting::inst(otherid).holding_end())
                {
                    ;

                } else {
                    LOG_I << "Invalid bottle id:" << bid <<" "<<gid;
                    return json::object()("id", 0);
                }
            }

            // if(cli->check_black(otherid)) THROW_EX(EN_Black);
            client_ptr ot = xindex::get(otherid);
            if(!ot){ THROW_EX(EN_TargetUser_NotFound); }
            if(ot->check_black(userid)) THROW_EX(EN_Blacked);
        }
    }
    else if( is_barchat(gid) ) {
        bar_ptr pbar = bars_mgr::inst().get_session_bar(gid);
        if ( pbar->black_list.end() != pbar->black_list.find(userid) ) {
            THROW_EX(EN_Operation_NotPermited);
        }
        if (gid!=cli->gwid() && userid!=pbar->trader_id) THROW_EX(EN_Operation_NotPermited);
    }

    switch (msgtype)
    {
    case MSG_PIC:
    case MSG_AUDIO:
        {
            if(cli->is_guest()) THROW_EX(EN_Operation_NotPermited);
            string path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(userid));
            url = cont = complete_url(write_file(path, req.content()));

            if(MSG_PIC==msgtype){
                // thumbnail(path, req.content());
            }
            else if(MSG_AUDIO==msgtype && is_barchat(gid)){
                try { convert_audio(Client::Files_Dir/path); }
                catch (std::exception const & e) { LOG_I << e.what(); }
            }
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
        imessage::message msg(ms[msgtype], userid, gid);
        msg.a = touid;
        msg.body
            ("content", cont)
            ;
        if (touid != INVALID_UID)
            msg.body.put("to", touid);

        chatmgr::inst().send(msg, cli);
        id = msg.id();
    }

    return json::object()("url", url)("id", id);
}

json::object Service::sendGift(http::request& req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID touid = lexical_cast<UID>(req.param("userid"));
    string type = req.param("type");
    int gift_id = lexical_cast<int>(req.param("gift"));
    string devicetype = req.param("deviceType");
    string sessionid = req.param("sessionid");

    // RECORD_TIMER(timer_log);
    if(1000 == touid) THROW_EX(EN_Operation_NotPermited);
    imessage::message msg("chat/gift", userid, sessionid); // chat session id
    msg.a = touid;
    json::object datas = GetIndividualDatas(userid);
    int money = datas.get<int>("money", 0); 
    int post = 0, charm=0;
    switch(gift_id){
        case FLOWER:
            post = 99;
            charm = 50;
            break;
        case CAR:
            post = 999;
            charm = 100;
            break;
        //case PLAN:
        //    post = 1000;
        //    charm = 500;
        //    break;
        case YACHT:
            post = 9999;
            charm = 500;
            break;
        default:
            THROW_EX(EN_Input_Data);
    }
    if(money < post)
    {
        THROW_EX(EN_NotEnough_Money); 
    }

    if (msg.a == INVALID_UID)
        THROW_EX(EN_TargetUser_NotFound);

    client_ptr c = xindex::get(msg.a);
    if (!c) THROW_EX(EN_TargetUser_NotFound);
    sql::exec(format(UPDATE_INDIVIDUALDATAS_SPEND_MONEY2)%post %userid);  

    cli->set_rich( userid, touid, post );

    msg.body
        ("content", gift_id)
        ("to", c->brief_user_info())
        // ("from", cli->brief_user_info())
        // ("time", time_string())
        ;

    chatmgr::inst().send(msg, cli);
    //TODO
    
    sql::exec(format(INSERT_USERGIFTS3) % touid %userid %gift_id);
    incr_charms(touid, charm, userid);
    // if ( client_ptr o = xindex::get( touid ) ) {
    //     o->set_charm( userid, touid, charm );
    //     // push_charm(cli, touid, charm);
    // }

    return json::object()("rich", post);
}

json::object Service::expressLoveResult(http::request& req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID touid = lexical_cast<UID>(req.param("userid"));
    string devicetype = req.param("deviceType");

    json::array rtn;
    int relation=2;
    sql::datas datas1(format(SELECT_WHISPERS2) %userid %touid); 
    if(sql::datas::row_type row1 = datas1.next()){
        json::object my;
        json::object ot;
        string m_whisper_time = row1.at(3,"");
        json::object m_user = cli->brief_user_info();
        relation = 1;
        my("userid",userid)
            ("icon",m_user.get<string>("icon"))
            ("userName",m_user.get<string>("userName"))
            ("date",m_whisper_time)
            ;
        rtn(my);

        // sql::datas datas2(fmt %touid %userid); 
        sql::datas datas2(format(SELECT_WHISPERS2) %touid %userid); 
        if(sql::datas::row_type row2 = datas2.next()){
            string o_whisper_time = row2.at(3,"");
            client_ptr other = xindex::get(touid);
            if(other){
                json::object o_user = other->brief_user_info();
                ot("userid", touid)
                    ("icon",o_user.get<string>("icon"))
                    ("userName",o_user.get<string>("userName"))
                    ("date",o_whisper_time)
                    ;
                relation = 3;
                rtn(ot);
            }
        }
    }

    return json::object()("relationship",relation)
        ("resultArray",rtn);
}


// typedef json::object (Service::*serve_memfn_type)(http::request & req, client_ptr& cli);
// static std::map<std::string, serve_memfn_type> urlmap_;
static std::map<std::string, service_fn_type> urlmap_;

Service::initializer::initializer(const boost::property_tree::ptree& ini)
{
    Magick::InitializeMagick(NULL);

    Client::Files_Dir = ini.get<string>("Files_Dir");
    Client::Files_Url = ini.get<string>("Files_Url");

    Service::alipay = ini.get<bool>("alipay", true); 
    Service::upPayPlugin = ini.get<bool>("upPayPlugin", true);

    Client::serve("/", &Service::serve);
    // InitSessions();

    // personal
    urlmap_["personInfo"] = &Service::personInfo;
    urlmap_["updateScenseIcon"] = &Service::updateScenseIcon;
    // urlmap_["messageNotification"] = &Service::messageNotification;

    urlmap_["contactList"] = &Service::contactList;
    urlmap_["relationShipList"] = &Service::relationShipList;
    urlmap_["attentions"] = &Service::attentions;
    urlmap_["relationship"] = &Service::relationship;
    urlmap_["remark"] = &Service::remark;

    urlmap_["myInfo"] = &Service::myInfo;
    urlmap_["uploadMyPhoto"] = &Service::uploadMyPhoto;
    urlmap_["modifyMyHead"] = &Service::modifyMyHead;
    urlmap_["modifyMyName"] = &Service::modifyMyName;
    urlmap_["modifySign"] = &Service::modifySign;
    urlmap_["deleteMyPhoto"] = &Service::deleteMyPhoto;
    urlmap_["modifyMyAge"] = &Service::modifyMyAge;

    // urlmap_["updateUserBgIcon"] = &Service::updateUserBgIcon;
    urlmap_["updateDynamicBgIcon"] = &Service::updateUserBgIcon;

    urlmap_["uploadFile"] = &Service::uploadFile;

    urlmap_["publishDynamicImg"] = &Service::uploadFile;
    urlmap_["publishDynamicMessage"] = &Service::publishDynamicMessage;
    urlmap_["supportComment"] = &Service::supportComment;
    urlmap_["deleteComment"] = &Service::deleteComment;
    urlmap_["deleteDynamic"] = &Service::deleteDynamic;
    urlmap_["publishComment"] = &Service::publishComment;
    urlmap_["dynamicCommentList"] = &Service::dynamicCommentList;
    urlmap_["dynamicMessage"] = &Service::dynamicMessage;
    urlmap_["unReadList"] = &Service::unReadList;
    // urlmap_["clearUnread"] = &Service::clearUnread;

    // IM
    //
    urlmap_["createGroup"] = &Service::create_chat;
    urlmap_["memberList"] = &Service::memberList;
    urlmap_["inviteJoinGroup"] = &Service::inviteJoinGroup;
    urlmap_["joinGroup"] = &Service::joinGroup;
    urlmap_["delGroupMember"] = &Service::delGroupMember;
    urlmap_["quitGroup"] = &Service::quitGroup;
    urlmap_["modifyGroupName"] = &Service::modifyGroupName;
    urlmap_["groupInfo"] = &Service::groupInfo;

    urlmap_["sendMessage"] = &Service::chat;
    urlmap_["sendGift"] = &Service::sendGift;
    urlmap_["receivedGifts"] = &Service::receivedGifts;
    urlmap_["says"] = &Service::says;
    urlmap_["expressLoveResult"] = &Service::expressLoveResult;

    urlmap_["login"] = &Service::login;
    urlmap_["logout"] = &Service::logout;

    // shop
    urlmap_["shops"] = &Service::shops;
    urlmap_["goodsInfo"] = &Service::goodsInfo;
    urlmap_["chargeList"] = &Service::chargeList;
    urlmap_["exchangeUserInfo"] = &Service::exchangeUserInfo;
    urlmap_["myExchange"] = &Service::myExchange;

    urlmap_["setNumerator"] = &Service::setNumerator;
    // bars
    //

    urlmap_["leaveJoinTalkBar"] = &Service::leaveJoinTalkBar;
    urlmap_["bars_people"] = &Service::bars_people;
    //
    // None authorized required
    // urlmap_["barInfo"] = &Service::barInfo;
    // urlmap_["barList"] = &Service::barList;

    // urlmap_["scense"] = &Service::scense;
    // urlmap_["actives"] = &Service::actives; // BarParties
    urlmap_["openCity"] = &Service::openCity;
    urlmap_["openZone"] = &Service::openZone;
    urlmap_["searchBar"] = &Service::searchBar;

    // misc
    //
    urlmap_["checkUpdate"] = &Service::checkUpdate;
    urlmap_["feedback"] = &Service::feedback;
    urlmap_["userProtocol"] = &Service::userProtocol;
    urlmap_["hello"] = &Service::hello;
    urlmap_["log/user"] = &Service::loguser;
    urlmap_["uploadLog"] = &Service::logsave;
    
    // version 1.1
    // urlmap_["programList"] = &Service::programList;
    urlmap_["barFriendOnline"] = &Service::barFriendOnline;
    urlmap_["personResource"] = &Service::personResource; // 
    urlmap_["nearByBar"] = &Service::nearByBar; // 活动：发现/附近的酒吧
    urlmap_["activeTonight"] = &Service::activeTonight; // 活动：现场/首页活动
    urlmap_["activeToscene"] = &Service::activeToscene; // 活动：现场/所有活动
    urlmap_["activeInfo"] = &Service::activeInfo; // 活动：选美：投票人列表
    urlmap_["joinParty"] = &Service::joinParty; // 活动：选美：参赛人员列表
    urlmap_["partyResult"] = &Service::partyResult;
    urlmap_["gameCenter"] = &Service::gameCenter;
    urlmap_["barInfos"] = &Service::barInfos;
    urlmap_["attentionsBar"] = &Service::attentionsBar;
    urlmap_["ballot"] = &Service::ballot;
    urlmap_["shakePartysByNear"] = &Service::shakePartysByNear;
    urlmap_["myPrize"] = &Service::myPrize;
    urlmap_["myCoupon"] = &Service::myCoupon;
    urlmap_["checkShake"] = &Service::checkShake;
    urlmap_["prizeByShake"] = &Service::prizeByShake;
    urlmap_["shakeChance"] = &Service::shakeChance;
    urlmap_["shakePartyInfo"] = &Service::shakePartyInfo;
    urlmap_["bar_reject"] = &Service::bar_reject;
    urlmap_["whisperLove"] = &Service::whisperLove;
    urlmap_["whisperLoveStatus"] = &Service::whisperLoveStatus;
    urlmap_["whisperList"] = &Service::whisperList;
    urlmap_["barOnlineLike"] = &Service::barOnlineLike;
    urlmap_["chargeSuccess"] = &Service::chargeSuccess;

    // thifting bottle
    urlmap_["sendBottle"] = &Service::sendBottle;
    urlmap_["myBottle"] = &Service::myBottle;
    urlmap_["throwBottle"] = &Service::throwBottle;
    urlmap_["deleteBottle"] = &Service::deleteBottle;
    urlmap_["receiveBottle"] = &Service::receiveBottle;
    urlmap_["bottleChance"] = &Service::bottleChance;

    // bgadmin_urlsetup(urlmap_);
    // void bgadmin_urlsetup(std::map<std::string, service_fn_type>& urlmap_)
    {
        urlmap_["initBarSession"] = &Service::initBarSession;
        urlmap_["DelBarSession"] = &Service::DelBarSession;

        //manager setting
        urlmap_["initBarActivity"] = &Service::initBarActivity;
        urlmap_["updateBarActivity"] = &Service::updateBarActivity;
        urlmap_["deleteBarActivity"] = &Service::deleteBarActivity;
        urlmap_["changeBarActivityIndex"] = &Service::changeBarActivityIndex;

        urlmap_["initBarAdvertising"] = &Service::initBarAdvertising;
        urlmap_["updateBarAdvertising"] = &Service::updateBarAdvertising;
        urlmap_["deleteBarAdvertising"] = &Service::deleteBarAdvertising;

        urlmap_["initBarCoupons"] = &Service::initBarCoupons;
        urlmap_["updateBarCoupons"] = &Service::updateBarCoupons;
        urlmap_["deleteBarCoupons"] = &Service::deleteBarCoupons;

        urlmap_["getBarSession"] = &Service::getBarSession;
        urlmap_["noticeScreen"] = &Service::noticeScreen;

        urlmap_["sendBarFans"] = &Service::sendBarFans;
        urlmap_["merchantChat"] = &Service::merchantChat;
        urlmap_["publicNotify"] = &Service::publicNotify;
        urlmap_["exchangeCoins"] = &Service::exchangeCoins;
    }

    load_chat_bottle_id();
    service_error_category();
}

Service::initializer::~initializer()
{
    LOG_I << __FILE__ << __LINE__;
}
// bool is_authorized_request(http::request& req)
// {
// 
//     std::map<std::string, service_fn_type>::iterator it = urlmap_.find(req.path());
//     if (it == urlmap_.end())
//     {
//         return true;
//     }
// 
//     if(it->second == &Service::barList
//             || it->second == &Service::searchBar
//             || it->second == &Service::scense
//             || it->second == &Service::actives
//             || it->second == &Service::openCity
//             || it->second == &Service::openZone
//             || it->second == &Service::barInfo
//             || it->second == &Service::bars_people
//             || it->second == &Service::userProtocol
//             || it->second == &Service::initBarSession
//             || it->second == &Service::DelBarSession
//             || it->second == &Service::checkUpdate
//             || it->second == &Service::logsave || it->second == &Service::loguser
//             || it->second == &Service::hello)
//     {
//         return false;
//     }
//     else{
//         return true;
//     }
// }

json::object Service::serve(http::request & req, client_ptr& cli) 
{
    std::map<std::string, service_fn_type>::iterator it = urlmap_.find(req.path());
    if (it == urlmap_.end())
    {
        THROW_EX(EN_HTTPRequestPath_NotFound);
    }

    if( //it->second == &Service::barList
            // || it->second == &Service::scense
            // || it->second == &Service::actives
            it->second == &Service::barInfos
            || it->second == &Service::openCity
            || it->second == &Service::openZone
            || it->second == &Service::searchBar
            // || it->second == &Service::barInfo // TODO: clear
            || it->second == &Service::bars_people
            || it->second == &Service::userProtocol
            // || it->second == &Service::initBarSession
            // || it->second == &Service::DelBarSession
            || it->second == &Service::checkUpdate
            || it->second == &Service::hello
            || it->second == &Service::setNumerator
            )
    {
        //Service s0; return (s0.*it->second)(req, cli);
        return (it->second)(req, cli);
    }

    if (!cli || !cli->is_authorized())
    {
        THROW_EX(EN_Unauthorized);
    }

    //Service s; return (s.*it->second)(req, cli);
    return (it->second)(req, cli);
}

// json::object Service::barInfo(http::request & req, client_ptr& cli)
// {
//     string sessionid = req.param("sid");
//     boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
//     if(!pbar)
//         THROW_EX(EN_Bar_NotFound);
//     string gwid;
//     if (cli)
//         gwid = cli->gwid();
// 
//     bool isCurrentBar = (!gwid.empty() && gwid==(pbar)->sessionid);
//     return json::object()("sid",sessionid)
//                         ("icon", pbar->logo)
//                         ("title", pbar->barname)
//                         ("hot", pbar->get_hot())
//                         ("total", (int)(pbar->inspot_users2_.size()))
//                         ("femals", pbar->get_female())
//                         ("males", pbar->get_male())
//                         ("brief", pbar->introduction)
//                         ("address", pbar->address)
//                         ("contact", pbar->phone)
//                         ("sessionid", pbar->sessionid)
//                         ("longtitude", pbar->longtitude)
//                         ("latitude", pbar->latitude)
//                         ("isCurrentBar", isCurrentBar)
//                         ("city", pbar->city)
//                         ("zone", pbar->zone)
//                         ;
// }

json::object Service::groupInfo(http::request & req, client_ptr& cli)
{
    string gid = req.param("sessionid");
    if (is_p2pchat(gid))
        return json::object();

    chat_group & cg = chatmgr::inst().chat_group(gid);
    json::array headIcons;
    auto const & members = cg.alive_members();
    BOOST_FOREACH(auto uid, members) {
        json::object users = GetUserInfo(uid);
        if(users.empty()) continue;

        headIcons(users.get<string>("headIcon"));
    }

    return json::object()("groupName", cg.name())("headIcons", headIcons);
}

json::object Service::joinGroup(http::request & req, client_ptr& cli)
{
    if(cli->is_guest()) THROW_EX(EN_Operation_NotPermited);

    string gid = req.param("sessionid");
    //if (is_p2pchat(gid)) return json::object();

    chat_group & cg = chatmgr::inst().chat_group(gid);
    cg.add_from_qr_code(cli);

    json::array members_info;
    auto const& members = cg.alive_members();
    for (auto itr=members.begin(); itr!=members.end(); ++itr)
    {
        client_ptr pmem = xindex::get(*itr);
        if(!pmem || pmem->is_guest()) continue;
        members_info(pmem->brief_user_info());
    }

    return json::object()("members", members_info);
}

// json::object Service::barList(http::request & req, client_ptr& cli)
// {
//     enum {HOT=1, TOTAL, DIST};
//     int pagesize = lexical_cast<int>(req.param("pageSize","0"));
//     unsigned index = lexical_cast<int>(req.param("index","0"));
//     double longtitude = lexical_cast<double>(req.param("longtitude","0"));
//     double latitude = lexical_cast<double>(req.param("latitude","0"));
//     string city = req.param("city");
//     string province = req.param("province", "");
//     string zone = req.param("zone","");
//     int type = lexical_cast<int>(req.param("type","0"));
// 
//     // TODO: clear
//     string gwid = req.header("gwid", "");
// 
//     if (gwid.empty() && cli) gwid = cli->gwid();
//     //LOG_I << cli <<" gwid "<< gwid;
//     //cli->cache().put("longtitude", longtitude);
//     //cli->cache().put("latitude", latitude);
// 
//     vector< boost::shared_ptr<s_bar_info> > list;
//     json::array bars;
//     bars_mgr::inst().get_bars_bycity(city, list);
//     if (!list.empty()){
//         vector< boost::shared_ptr<s_bar_info> >tmp;
//         for(vector< boost::shared_ptr<s_bar_info> >::iterator itr=list.begin(); 
//                 itr!=list.end(); ++itr)
//         {
//             if(province.empty() || province ==(*itr)->province){
//                 if(zone.empty() || (*itr)->zone==zone) tmp.push_back(*itr);
//             }
//         }
//         list.swap(tmp);
//     }
// 
//     if(index < list.size()){
//         Position dist(latitude, longtitude);
//         switch(type){
//             case HOT:
//                 sort(list.begin(), list.end(), hot_compare);
//                 break;
//             case TOTAL:
//                 sort(list.begin(), list.end(), total_compare);
//                 break;
//             case DIST:
//             default:
//                 sort(list.begin(), list.end(), dist);
//                 break;
//         }
// 
//         int hasCur = 0;
//         for(vector< boost::shared_ptr<s_bar_info> >::iterator itr=list.begin()+index; 
//                 itr!=list.end()&&0<pagesize--; ++itr)
//         {
//             bool isCurrentBar = (!gwid.empty() && gwid==(*itr)->sessionid);
//             double distance = isCurrentBar? 0:dist.get_distance(**itr);
//             json::object tmp;
//             tmp("sid", (*itr)->sessionid)
//                 ("index", ++index)
//                 ("icon", (*itr)->logo)
//                 ("title", (*itr)->barname)
//                 ("hot", (*itr)->get_hot())
//                 ("total", (*itr)->get_total())
//                 ("femals", (*itr)->get_female())
//                 ("males", (*itr)->get_male())
//                 // ("range", dist.get_distance(**itr))
//                 ("range", distance)
//                 ("brief", (*itr)->introduction)
//                 ("address", (*itr)->address)
//                 ("contact", (*itr)->phone)
//                 ("sessionid", (*itr)->sessionid)
//                 ("longtitude", (*itr)->longtitude)
//                 ("latitude", (*itr)->latitude)
//                 ("isCurrentBar", isCurrentBar)
//                 ("city", (*itr)->city)
//                 ("zone", (*itr)->zone)
//                 ("province", (*itr)->province)
//                 ;
//             if(isCurrentBar){
//                 bars.push_front(tmp);
//             }
//             else{
//                 bars(tmp);
//             }
//             hasCur += isCurrentBar;
//         }
//         LOG_I << hasCur <<" "<< gwid; // LOG_I << cli <<" "<< hasCur <<" "<< gwid;
//     }
//     return json::object()("pageSize", static_cast<int>(bars.size()))
//                          ("customerList",bars)
//                          ;
// }

json::object Service::searchBar(http::request & req, client_ptr& cli)
{
    double longtitude = lexical_cast<double>(req.param("longtitude","0"));
    double latitude = lexical_cast<double>(req.param("latitude","0"));
    string city = req.param("city");
    string title = req.param("title");
    string province = req.param("province", "");
    trim(title);

    // TODO: clear
    string const & gwid = cli->gwid();
    // if(gwid.empty() && cli) gwid = cli->gwid();
    //cli->cache().put("longtitude", longtitude);
    //cli->cache().put("latitude", latitude);

    vector< boost::shared_ptr<s_bar_info> > list;
    json::array bars;
    bars_mgr::inst().get_bars_bycity(city, list);
    if (!list.empty()){
        vector< boost::shared_ptr<s_bar_info> >tmp;
        for(vector< boost::shared_ptr<s_bar_info> >::iterator itr=list.begin(); 
                itr!=list.end(); ++itr)
        {
            if(province.empty() || province==(*itr)->province){
                if (title.empty() || my_regex_match((*itr)->barname,title)) {
                    tmp.push_back(*itr);
                }
            }
        }
        list.swap(tmp);

        Position dist(latitude, longtitude);
        sort(list.begin(), list.end(), dist);
        int hasCur = 0, index=0;
        for(vector< boost::shared_ptr<s_bar_info> >::iterator itr=list.begin(); 
                itr!=list.end(); ++itr)
        {
            bool isCurrentBar = (!gwid.empty() && gwid==(*itr)->sessionid);
            json::object tmp;
            tmp("sid", (*itr)->sessionid)
                ("index", ++index)
                ("icon", (*itr)->logo)
                ("title", (*itr)->barname)
                ("hot", (*itr)->get_hot())
                ("total", (*itr)->get_total())
                ("femals", (*itr)->get_female())
                ("males", (*itr)->get_male())
                ("range", dist.get_distance(**itr))
                ("brief", (*itr)->introduction)
                ("address", (*itr)->zone+(*itr)->address)
                ("contact", (*itr)->phone)
                ("sessionid", (*itr)->sessionid)
                ("longtitude", (*itr)->longtitude)
                ("latitude", (*itr)->latitude)
                ("isCurrentBar", isCurrentBar)
                ("city", (*itr)->city)
                ("zone", (*itr)->zone)
                ("province", (*itr)->province)
                ;
            if(isCurrentBar){
                bars.push_front(tmp);
            }
            else{
                bars(tmp);
            }
            hasCur += isCurrentBar;
        }
        LOG_I << hasCur <<" "<< gwid; //LOG_I << cli <<" "<< hasCur <<" "<< gwid;
    }

    return json::object()("pageSize", static_cast<int>(bars.size()))
        ("customerList",bars)
        ;
}

json::object Service::openCity(http::request & req, client_ptr&)
{
    format fmt(SELECT_BARS_CITY);
    json::array cities;
    sql::datas datas(fmt);
    while(sql::datas::row_type row = datas.next())
    {
        const char *pcity = row.at(0);
        if(NULL == pcity) continue;
        cities(json::object()("city", pcity));
    }

    return json::object()("citys",cities);
}

json::object Service::openZone(http::request & req, client_ptr&)
{
    string city = req.param("city");
    format fmt(SELECT_BARS_ZONE1);
    json::array zones;
    sql::datas datas(fmt %city);
    while(sql::datas::row_type row = datas.next()){
        const char *pzone = row.at(0);
        if(NULL == pzone) continue;
        zones(json::object()("zone", pzone));
    }

    return json::object()("zones",zones);
}

// json::object Service::scense(http::request & req, client_ptr&)
// {
//     string barid = req.param("sid");
//     string count = req.param("pageSize");
//     long id = lexical_cast<int>(req.param("index","0"));
// 
//     if(id < 1) id = LONG_MAX;
//     format fmt(SELECT_BARLIVEALBUM3);
//     sql::datas datas(fmt %id %barid %count);
//     json::array imgs;
//     while(sql::datas::row_type row = datas.next()){
//         // const char *pBarId = row.at(0);
//         const char *pid = row.at(1);
//         const char *purl = row.at(2);
//         const char *pUploadTime = row.at(3);
//         // if(!pBarId || !pid || !purl || !pUploadTime) continue;
//         if(!pid || !purl || !pUploadTime) continue;
// 
//         imgs(json::object()("index", atoi(pid))
//                 ("icon", complete_url(purl))
//                 ("time", pUploadTime)
//             );
//     }
// 
//     return json::object()("pageSize", static_cast<int>(imgs.size()))
//         ("sid", barid)
//         ("scenseList", imgs)
//         ;
// }

// json::object Service::actives(http::request & req, client_ptr&)
// {
//     string sessionid = req.param("sid");
// 
//     format fmt(SELECT_BARPARTIES1);
//     sql::datas datas(fmt %sessionid);
//     json::object activity;
//     if(sql::datas::row_type row = datas.next()){
//         const char *pid = row.at(0);
//         const char *ptitle = row.at(2, "酒吧活动"); 
//         const char *purl = row.at(3, "");
//         const char *pcontent = row.at(4, "");
//         const char *pBeginTime = row.at(5);
//         const char *pEndTime = row.at(6, "");
//         const char *paddress = row.at(8);
//         if(!pid || !pBeginTime || !paddress) return json::object();
// 
//         string now = time_string(time(0));
//         int type;
//         if(now.compare(pEndTime) > 0){type=3;}
//         else if(now.compare(pBeginTime) < 0)type = 1; 
//         else type = 2; 
// 
//         activity("sid", sessionid)
//             ("title", ptitle)
//             ("icon",complete_url(purl))
//             ("content",pcontent)
//             ("beginTime",pBeginTime)
//             ("endTime",pEndTime)
//             ("type",type)
//             ("address",paddress)
//             ;
//     }
// 
//     return activity;
// }

json::object Service::hello(http::request& req, client_ptr& cli)
{
    typedef map<UID, pair<unsigned int, string> > ClientInfo;
    typedef map<UID, pair<unsigned int, string> >::iterator ClientInfoIterator;
    json::object hosts;
    if (cli)
    {
        // std::string ipa = cli->ipaddr_.to_v4().to_string();
        std::string ipa = cli->wan_ipaddress().to_v4().to_string();
        // if (boost::asio::ip::is_private(cli->ipaddr_) && boost::starts_with(ipa, "10."))
        //     ipa = "58.67.160.249"; // ping test.kklink.com
        hosts
            ("server.kklink.com", ipa)
            ("time", time_string())
            // ("payment.kklink.com", ipa)
            // ("yx-api.kklink.com", ipa)
            ;
        LOG_I << ipa;

        static ClientInfo client_infos;
        UID uid = cli->user_id();
        unsigned int device_type = lexical_cast<unsigned int>( req.param("deviceType","0"));
        string version = req.param("version","");

        if ( INVALID_UID!=uid && 0!=device_type && !version.empty()) { 

            bool is_record = false;
            ClientInfoIterator itr = client_infos.find( uid ); 
            if ( itr != client_infos.end() ) { 
                if ( itr->second.first!=device_type && itr->second.second!=version ) { 
                    itr->second.first = device_type ; 
                    itr->second.second = version;
                    is_record = true;
                } 
            } else {
                client_infos.insert(make_pair(uid, make_pair(device_type, version)));
                is_record = true;
            }

            if ( is_record ) {
                sql::exec(format("REPLACE client_record(userid,deviceType,version) VALUES(%1%, %2%, '%3%')") 
                        %uid %device_type %version);
            }
        }
    }
    return hosts;
}

json::object Service::checkUpdate(http::request& req, client_ptr&)
{
    string devicetype = req.param("deviceType");

    format fmt("SELECT AppVersion,version,url,content"
            " from versions where DeviceType=%s order by UploadTime desc limit 1");
    sql::datas datas(fmt % devicetype);
    if(sql::datas::row_type row = datas.next()){
        const char *pappversion = row.at(0,"1");
        const char *pversion = row.at(1,"1.0");
        const char *purl = row.at(2,"");
        const char *pcontent = row.at(3,"");
        return json::object()
            ("versionCode",lexical_cast<int>(pappversion))
            ("versionstring",pversion)
            ("downURL", complete_url(purl))
            ("content", pcontent)
            ;
    }

    return json::object()("versionCode", 1)("versionstring","1.0")("downURL","")("content","");
}

json::object Service::userProtocol(http::request& req, client_ptr&)
{
    std::string p = readfile(user_protocol_fn_);
    return json::object()("content", p); 
    //
    // format fmt("SELECT * from protocols");
    // sql::datas datas(fmt);
    // if(sql::datas::row_type row = datas.next()){
    //     const char *pcontent = row.at(1, "");
    //     return json::object()("content", pcontent); 
    // }
    // return json::object()("content","");
}

json::object Service::programList(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid");
    if(sessionid.empty()) return json::object();

    std::vector<program> programs = program_mgr::instance().get_porgrams(sessionid);
    json::array pros;
    for(std::vector<program>::const_iterator itr=programs.begin(); itr!=programs.end(); ++itr)
    {
        json::array actors;
        for(std::vector<UID>::const_iterator i=itr->actors_.begin();
                i!=itr->actors_.end(); ++i)
        {
            client_ptr pmem = xindex::get(*i);
            Client::client_admires_iterator j;
            bool ret = cli->get_admire(*i, j);
            string remarkName = ret ? j->second.second: "";
            json::object usr_info = pmem->user_info();
            json::object at;
            at("userid", *i)
                ("userName", usr_info.get<string>("nick",""))
                ("headIcon",usr_info.get<string>("icon",""))
                ("sign",usr_info.get<string>("signature",""))
                ("gender",usr_info.get<string>("gender",""))
                ("age",usr_info.get<string>("age",""))
                ("constellation",usr_info.get<string>("constellation",""))
                ("remarkName",remarkName);
            actors(at);
        }

        json::object jo;
        jo("id", itr->id_)
          ("type", itr->type_)
          ("programName", itr->show_name_)
          ("detail", itr->detail_)
          ("beginDate", time_string(itr->begin_))
          ("endDate", time_string(itr->end_))
          ("system_time", time_string())
          ("joinArray", actors)
          ;

        pros(jo);
    }

    return json::object()("programs", pros);
}

json::object Service::barFriendOnline(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid");
    if(sessionid.empty()) return json::object();
    // chat_group & cg = chatmgr::inst().chat_group(sessionid);
    bar_ptr pbar = bars_mgr::inst().get_session_bar(sessionid);
    // if(!(cg.is_member(cli->user_id()))) THROW_EX(EN_Operation_NotPermited); 
    //    RECORD_TIMER(timer_log);
    // const std::vector<UID>& memberlist = cg.alive_members();
    json::array list;
    for (auto itr=pbar->inspot_users2_.rbegin();
            itr != pbar->inspot_users2_.rend(); ++itr)
    {
        UID otherid = itr->uid;
        // if (otherid == cli->user_id()) continue;
        client_ptr other = xindex::get(otherid);
        if(other){
            Client::client_admires_iterator i;
            bool ret = cli->get_admire(otherid, i);
            string remarkName = ret ? i->second.second: "";
            json::object user = GetUserInfo(other);
            if(user.empty() || other->is_guest()) continue;

            user.erase("mail");
            user.erase("phone");
            user.erase("constellation");
            user("guest", other->is_guest());
            user("remarkName", remarkName);
            user("charm", other->get_charm());

            list.push_back(user);
        }
    }

    return json::object()("onlineList",list);
}

// GET /personResource userid=
json::object Service::personResource(http::request & req, client_ptr& cli)
{
    UID i_otherid = lexical_cast<UID>(req.param("userid"));
    UID userid = cli->user_id();

//    RECORD_TIMER(timer_log);
    client_ptr other = xindex::get(i_otherid);
    if(!other){ THROW_EX(EN_TargetUser_NotFound); }
    json::object users = GetUserInfo(other);
    if(users.empty()){ THROW_EX(EN_TargetUser_NotFound); }

    int relation1 = 3;
    int relation2 = 3;
    string remark;
    string bgIcon;
    bool issay = false;
    json::array imgs;
    json::array fans;

    json::object datas = GetIndividualDatas(i_otherid);
    bgIcon = datas.get<string>("background","");
    imgs = GetIndividualAlbum(i_otherid);
    issay = CheckWhisper(userid, i_otherid);

    if(1000!=i_otherid){
        Client::client_fans_type fans_list = other->get_fans();
        for(Client::client_fans_iterator itr=fans_list.begin();
                itr != fans_list.end(); ++itr)
        {
            client_ptr c = xindex::get(itr->first);
            if(c){
                Client::client_admires_iterator i;
                bool ret = cli->get_admire(itr->first, i);
                string remarkName = ret ? i->second.second: "";
                json::object const & usr_info = c->user_info();
                json::object jo;
                jo("userid", itr->first)
                    ("userName", usr_info.get<string>("nick",""))
                    ("headIcon",usr_info.get<string>("icon",""))
                    ("sign",usr_info.get<string>("signature",""))
                    ("gender",usr_info.get<string>("gender",""))
                    ("age",usr_info.get<string>("age",""))
                    ("constellation",usr_info.get<string>("constellation",""))
                    ("remarkName",remarkName);
                fans(jo);
            }
        }

        Client::client_fans_iterator i;
        if(cli->get_fan(i_otherid, i)){
            relation2 = 1;
        }

        Client::client_admires_iterator j;
        if(cli->get_admire(i_otherid, j)){
            relation1 = 1;
            remark = j->second.second;
        }
        if ( cli->check_black( other) ) {
            relation1 = 2;
        }
        if ( other->check_black( cli ) ) {
            relation2 = 2;
        }
    }

    auto & dyns = dynamic_datas::inst(i_otherid);
    auto & idxs = boost::get<TagUsrId>(dyns.users);
    auto rng = idxs.equal_range(i_otherid);

    json::array dynamic_imgs;
    std::string words;
    if (!boost::empty(rng)) {
        auto last_dynamic = boost::rbegin(rng);
        BOOST_FOREACH(const auto & x, (*last_dynamic)->imgs) {
            dynamic_imgs.put( complete_url(x) );
        }
        words = (*last_dynamic)->words;
    }

    users.erase("phone");
    users.erase("mail");
    return users("remarkName",remark)
        ("bgIcon", bgIcon)
        ("relationShip", relation1)
        ("otherRelationship", relation2)
        ("isSay", issay)
        ("iconList", imgs)
        ("fansList", fans)
        ("gameList",json::array())
        ("dynamicContent", json::object()("iconlist",dynamic_imgs)("message", words))
        ("rich",  other->get_rich())
        ("charm", other->get_charm())
        ;
}

json::object json_bar_detail(s_bar_info const & b)
{
    return json::object()
        ("userid", b.trader_id)
        ("sessionid", b.sessionid)
        ("title", b.barname)
        ("icon", b.logo)
        ("hot", b.get_hot())
        ("total", (int)(b.inspot_users2_.size()))
        ("femals", b.get_female())
        ("males", b.get_male())
        ("brief", b.introduction)
        ("address", b.address)
        ("phone", b.phone)
        ("longtitude", b.longtitude)
        ("latitude", b.latitude)
        ("city", b.city)
        ("zone", b.zone)
        ("province", b.province)
        ;
}

json::object Service::nearByBar(http::request & req, client_ptr& cli)
{
    const char* GET_CI_BACKGROUND_PIC = "select A_image1,A_size1,A_image2,A_size2,I_image1,"
        " I_size1,absolute_url FROM ci_background_pic WHERE SessionId='%1%' limit 1";

    enum {DIST=1,HOT};
    int pagesize = lexical_cast<int>(req.param("pageSize","0"));
    UInt index = lexical_cast<int>(req.param("index","0"));
    double longtitude = lexical_cast<double>(req.param("longtitude","0"));
    double latitude = lexical_cast<double>(req.param("latitude","0"));
    string city = req.param("city");
    string province = req.param("province", "");
    string zone = req.param("zone","");
    int type = lexical_cast<int>(req.param("sortType","1"));
    int deviceType = lexical_cast<int>(req.param("deviceType","1"));

//    RECORD_TIMER(timer_log);
    vector< boost::shared_ptr<s_bar_info> > list;
    json::array bars;
    bars_mgr::inst().get_bars_bycity(city, list);
    if (!list.empty()) {
        vector< boost::shared_ptr<s_bar_info> >tmp;
        for(vector< boost::shared_ptr<s_bar_info> >::iterator itr=list.begin(); 
                itr!=list.end(); ++itr)
        {
            auto & pbar = *itr;
            if(province.empty() || province ==pbar->province){
                if(zone.empty() || pbar->zone==zone) tmp.push_back(pbar);
            }
        }
        list.swap(tmp);
    }

    if(index < list.size()){
        Position dist(latitude, longtitude);
        switch(type){
            case HOT:
                sort(list.begin(), list.end(), hot_compare());
                break;
            case DIST:
            default:
                sort(list.begin(), list.end(), dist);
                break;
        }

        UInt pos = 0;
        bool en_put_alive_bar = !cli->gwid().empty();

        for(vector<boost::shared_ptr<s_bar_info> >::iterator itr=list.begin(); itr!=list.end(); ) {
            bar_ptr pbar;
            LOG_I << " gwid "<<cli->gwid() << " sessionId "<<(*itr)->sessionid;
            if ( en_put_alive_bar ) {
                if ( 0 == pos ) {
                    pbar = bars_mgr::inst().get_session_bar( cli->gwid() );
                } else {
                    pbar = *itr++;
                    if ( pbar->sessionid == cli->gwid() ) { continue; }
                }
            } else {
                pbar = *itr++;
            }

            if ( ++pos <= index ) { continue; }
            if ( pagesize-- <= 0 ) { break; }

            LOG_I<< "pos:"<< pos <<" pagesize " << pagesize;
            json::object bar_active;
            bar_active("background_url", "");
            auto & idxs = boost::get<TagActivityOpenTime>(pbar->activities2);
            int activeid=0;
            string banner,activeName;
            // pbar->activities2.project<0>( idxs.rbegin() );

            if (!pbar->activities2.empty()) {
                BOOST_FOREACH(auto const & a, idxs) {
                    if (!a.is_closed()) {
                        activeid = a.id;
                        banner = a.post_img;
                        activeName = a.activity_name;
                        break;
                    }
                }
                if (activeid == 0) {
                    auto rb = idxs.rbegin();
                    activeid=rb->id;
                    banner=rb->post_img;
                    activeName=rb->activity_name;
                }
            } else {
                {
                    auto & icity = boost::get<TagGactivityCity>(global_mgr::inst().g_activities_);
                    auto p = icity.equal_range(city);
                    for ( auto q=p.first; q!=p.second; ++q ) {
                        // if ( q->SessionIds.empty() ){
                        //     if (std::find(q->SessionIds.begin(),q->SessionIds.end(),pbar->sessionid)
                        //             !=q->SessionIds.end())
                        //     {
                        activeid = q->id;
                        banner = q->post_img;
                        activeName = q->activity_name;
                        //     }
                        // }
                    }
                }
                sql::datas datas(format(GET_CI_BACKGROUND_PIC) %pbar->sessionid);
                if ( sql::datas::row_type row = datas.next() ) {
                    const char* pA_image1 = row.at(0);
                    // const char* pA_size1 = row.at(1);
                    const char* pA_image2 = row.at(2);
                    // const char* pA_size2 = row.at(3);
                    const char* pI_image1 = row.at(4);
                    // const char* pI_size1 = row.at(5);
                    const char* pabs_url = row.at(6);

                    if ( NULL != pabs_url ) {
                        string background_url(pabs_url);
                        if ( 1 == deviceType ) {
                            if ( NULL!=pI_image1 ) {
                                bar_active("background_url", (background_url+pI_image1));
                            }
                        } else {
                            int pic_type = lexical_cast<int>(req.param("pic_type","1"));
                            if( 1 == pic_type ){
                                if ( NULL!=pA_image2 ) {
                                    bar_active("background_url", (background_url+pA_image2));
                                } 
                            }else {
                                if (NULL!=pA_image1 ){
                                    bar_active("background_url", (background_url+pA_image1));
                                }
                            }
                        }
                    }
                }
            }

            double distance = dist.get_distance(*pbar);
            bar_active += json_bar_detail(*pbar);
            bar_active("index", ++index)
                ("activeid", activeid)
                ("banner", banner)
                ("activeName", activeName)
                ("range", distance)
                ;
            bar_active.rename("phone", "contact");
            bars(bar_active);
        }
    }
    return json::object()("pageSize", static_cast<int>(bars.size()))
        ("barList",bars)
        ;
}

json::object Service::chargeSuccess(http::request & req, client_ptr& cli)
{
// #define UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2 ("UPDATE IndividualDatas set money=money+%1% WHERE UserId=%2%")
    UID userid = lexical_cast<UID>(req.param("userid"));
    string out_trade_no = req.param("out_trade_no");
    int quantity = lexical_cast<int>(req.param("quantity"));
    int coins = lexical_cast<int>(req.param("coins"));
    int channel = lexical_cast<int>(req.param("channel"));

    client_ptr c = xindex::get(userid);
    if ( !c ){
        THROW_EX(EN_Input_Data);
    }

    // sql::exec(format(UPDATE_INDIVIDUALDATAS_CHARGE_MONEY2)%(quantity*coins)%userid);  
    imessage::message msg("status/bill", SYSADMIN_UID, make_p2pchat_id(userid, SYSADMIN_UID));
    msg.body
        ("out_trade_no",out_trade_no)
        ("status", true)
        ("coins", quantity*coins)
        ("channel",channel)
        ;
    sendto(msg, c);

    return json::object();
}

json::object Service::barInfos(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    if (sessionid.empty())
        return json::object();

    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar)
        THROW_EX(EN_Bar_NotFound);

    int isFan = 0;
    if (cli)
    {
        isFan = pbar->check_fan(cli->user_id());

        string gwid = cli->gwid();
        if (sessionid != gwid) {
            boost::format fmt("INSERT INTO bar_spot_access(SessionId,FromSessionId,UserId,Enter)"
                    " VALUES('%1%','%2%',%3%,1)");
            sql::exec(fmt % sessionid %gwid %cli->user_id());
        }
    }

    json::object ret = json_bar_detail(*pbar);
    ret.rename("brief", "introduce");
    ret ("status", isFan)
        ("band", "") // TODO band
        ;
    return ret;
    // return json::object()("icon", pbar->logo)
    //                     ("title", pbar->barname)
    //                     ("hot", pbar->get_hot())
    //                     ("total", pbar->get_total())
    //                     ("femals", pbar->get_female())
    //                     ("males", pbar->get_male())
    //                     ("introduce", pbar->introduction)
    //                     ("address", pbar->address)
    //                     ("phone", pbar->phone)
    //                     ("sessionid", pbar->sessionid)
    //                     ("longtitude", pbar->longtitude)
    //                     ("latitude", pbar->latitude)
    //                     ("city", pbar->city)
    //                     ("zone", pbar->zone)
    //                     ("band", "")
    //                     ("status", isFan)
    //                     ;
}

json::object Service::attentionsBar(http::request & req, client_ptr& cli)
{
    const char* INSERT_BAR_FAN_OPERATION = "INSERT INTO "
        " bar_fan_operation(SessionId,FromSessionId,UserId,Fan) "
        " VALUES('%1%', '%2%', %3%, '%4%')";
    string sessionid = req.param("sceneid","");
    int type = lexical_cast<int>(req.param("type","0"));
    if(sessionid.empty() || 0==type) return json::object();

    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar)
        THROW_EX(EN_Bar_NotFound);

    if (1 == type){
        pbar->add_fans(cli->user_id());
        cli->add_attention_bar(sessionid);
    }
    else if ( 2 == type){
        pbar->delete_fans(cli->user_id());
        cli->delete_attention_bar(sessionid);
    }

    // UInt ver = cli->user_info_ver(+1);
    // format fmt("UPDATE users SET ver=%1% WHERE UserId=%2%");
    // sql::exec(fmt % ver % cli->user_id());

    int val = 1==type? 1:0;
    sql::exec( format(INSERT_BAR_FAN_OPERATION) %sessionid %cli->gwid() %cli->user_id() %val );

    return json::object();
}

// json::object Service::attentionsBarList(http::request & req, client_ptr& cli)
// {
//     json::array lis;
//     BOOST_FOREACH(auto & bsid, cli->get_attention_bars())
//     {
//         boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(bsid);
//         lis.put( json_bar_brief(*pbar) );
//     }
//     return json::object()("bars", lis);
// }

static json::object json_activity(s_activity_info const & a)
{
    return json::object()
        ("activeid", a.id)
        ("banner", a.post_img)
        ("type", a.type)
        ("activeName", a.activity_name)
        ("beginDate", time_string(a.time_begin))
        ("endDate", time_string(a.time_end))
        ("system_time", time_string())
        ("is_global", false )
        ;
};

json::object Service::activeTonight(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    if(sessionid.empty())
        return json::object();

//  RECORD_TIMER(timer_log);
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar)
        MYTHROW(EN_Bar_NotFound2,bar_error_category);

    json::array show_list;
    time_t now = time(NULL);

    for (auto itr = pbar->activities2.begin(); itr != pbar->activities2.end(); ++itr)
    {
        if (itr->foremost && itr->time_end>now) {
            show_list( json_activity(*itr) );
        }
    }

    auto & icity = boost::get<TagGactivityCity>(global_mgr::inst().g_activities_);
    auto p = icity.equal_range(pbar->city);
    for ( auto q=p.first; q!=p.second; ++q ) {
        if (q->foremost && q->time_end>now) {
            if ( q->SessionIds.empty() ){
                if (std::find(q->SessionIds.begin(),q->SessionIds.end(),sessionid)==q->SessionIds.end())
                {
                    continue;
                }
            }
            show_list( json_activity(*q)("is_global", true ) );
        }
    }

    return json::object()("activeList", show_list);
}

json::object Service::activeToscene(http::request & req, client_ptr& cli)
{
#define COUNT 10
    string sessionid = req.param("sceneid","");
    int activeid = lexical_cast<int>(req.param("activeid","0"));
    if(sessionid.empty()) return json::object();

//    RECORD_TIMER(timer_log);
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    json::array show_list;
    if (!pbar){
        MYTHROW(EN_Bar_NotFound2,bar_error_category);
    }

    int count = 0;
    auto & idxs = boost::get<TagActivityId>(pbar->activities2);
    for (auto i=idxs.rbegin(); i!=idxs.rend(); ++i) {
        if ( 0 != activeid ) {
            if ( i->id >= activeid ) {
                continue;
            }
        }
        if ( ++count > COUNT ) {
            break;
        }

        show_list( json_activity(*i) );
    }

    return json::object()("sceneList", show_list)
        ("size", (int)(pbar->activities2.size()))
        ("pageSize", COUNT);
}

json::object Service::activeInfo(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    int activeid = lexical_cast<int>(req.param("activeid","0"));
    if(sessionid.empty() || 0 == activeid) { return json::object(); }
    bar_ptr pbar = bars_mgr::inst().get_session_bar(sessionid);

//    RECORD_TIMER(timer_log);
    json::object rep;
    auto it = pbar->find_activity(activeid);
    if (it != pbar->activities2.end()) {
        json::array joinArray, prize;
        // BOOST_FOREACH(auto const & q , it->shows2){
        //     BOOST_FOREACH(auto const & v , q.voters){
        BOOST_FOREACH( const auto&p, it->joins) {
            client_ptr c = xindex::get(p);
            if(c){
                Client::client_admires_iterator i;
                bool ret = cli->get_admire(p, i);
                string remarkName = ret ? i->second.second: "";
                json::object usr_info = c->user_info();
                json::object jo;
                jo("userid", p)
                    ("userName", usr_info.get<string>("nick",""))
                    ("headIcon",usr_info.get<string>("icon",""))
                    ("sign",usr_info.get<string>("signature",""))
                    ("gender",usr_info.get<string>("gender",""))
                    ("age",usr_info.get<string>("age",""))
                    ("constellation",usr_info.get<string>("constellation",""))
                    ("remarkName",remarkName);
                joinArray(jo);
            }
        }
            //     }
            // }
        BOOST_FOREACH(const auto&a, it->awards) {
            prize(json::object()("id", a.id)
                          ("count", a.count)
                          ("name", a.name)
                          ("icon", a.img)
                          ("brief", a.brief)
                          );
        }

        rep("activeid", activeid)
        ("banner", it->shorten_banner)
        ("srcBanner", it->post_img)
        ("type", it->type)
        ("activeInfo", it->brief)
        ("activeName", it->activity_name)
        ("beginDate", time_string(it->time_begin))
        ("endDate", time_string(it->time_end))
        ("system_time", time_string())
        ("share", it->shorten_share)
        ("joinArray", joinArray)
        ("prize", prize);

    } else {
        boost::system::error_code err;
        auto it = global_mgr::inst().find_g_activity(activeid, err);
        if ( !err ) {
            rep("activeid", activeid)
                ("banner", it->post_img)
                ("type", it->type)
                ("activeInfo", it->brief)
                ("activeName", it->activity_name)
                ("beginDate", time_string(it->time_begin))
                ("system_time", time_string())
                ("endDate", time_string(it->time_end))
                ("is_global", true )
                ("joinArray", json::array());
        } else {
            MYTHROW(EN_Activity_NotFound, bar_error_category);
        }
    }

    return rep;
}

json::object Service::joinParty(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    int activeid = lexical_cast<int>(req.param("activeid","0"));
    if(sessionid.empty() || 0==activeid)
        return json::object();

    //    RECORD_TIMER(timer_log);
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar)
        THROW_EX(EN_Bar_NotFound);

    // std::list<s_activity_info>& activities = pbar->activities;
    auto it = pbar->find_activity(activeid);
    if (it == pbar->activities2.end())
        MYTHROW(EN_Bar_NotFound,bar_error_category);

    json::array partyList;

    if ( it->time_begin<time(NULL) && it->time_end>time(NULL)) {
        if ( !it->shows2.empty() ) {
            BOOST_FOREACH(auto const & p , it->shows2){
                client_ptr c = xindex::get(p.actor);
                if (c) {
                    int status=(p.voters.end()!=p.voters.find(cli->user_id()))?1:2;
                    Client::client_admires_iterator i;
                    bool ret = cli->get_admire(p.actor, i);
                    string remarkName = ret ? i->second.second: "";
                    json::object usr_info = c->user_info();
                    json::object jo;
                    json::array imgs = GetIndividualAlbum(p.actor);

                    // json::object last_image = boost::get<json::object>(*(imgs.rbegin()));
                    string headicon = usr_info.get<string>("icon","");
                    if ( headicon.empty() ) {
                        continue;
                    }

                    string file = url_to_local(headicon);
                    if ( file.empty() ) {
                        continue;
                    }
                    float height=0.0;
                    float width=0.0;
                    Magick::Image obj(file);
                    Magick::Geometry geo_size = obj.size();
                    height = geo_size.height();
                    width = geo_size.width();

                    jo("userid", p.actor)
                        ("supports", p.countof_voter)
                        ("status", status)
                        ("remarkName",remarkName)
                        ("icons",imgs)
                        ("height", height)
                        ("width", width)
                        // user info
                        ("userName", usr_info.get<string>("nick",""))
                        ("headIcon", headicon)
                        ("sign",usr_info.get<string>("signature",""))
                        ("gender",usr_info.get<string>("gender",""))
                        ("age",usr_info.get<string>("age",""))
                        ("constellation",usr_info.get<string>("constellation",""))
                        ;
                    partyList(jo);
                }
            }
        }
    }
    return json::object()("partyList",partyList);
}
json::object Service::partyResult(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    int activeid = lexical_cast<int>(req.param("activeid","0"));
    if(sessionid.empty() || 0==activeid) return json::object();

    json::array partyList;
    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar)
        THROW_EX(EN_Bar_NotFound);

    auto it = pbar->find_activity(activeid);
    if (it == pbar->activities2.end())
        MYTHROW(EN_Bar_NotFound,bar_error_category);

    if ( s_activity_info::EN_VoteBeauty == it->type ) {
        if (!it->shows2.empty() && it->time_end<time(NULL)) {
            if ( it->result_rank_list.empty() ) {
                // insert order prefered
                auto & ac = const_cast<s_activity_info&>(*it);
                unsigned num = ac.winners_setting;
                num = num > ac.shows2.size()? ac.shows2.size() : num;
                ac.result_rank_list.resize(num, bar_activity_show(0));
                std::partial_sort_copy(it->shows2.begin(), it->shows2.end(),
                        ac.result_rank_list.begin(), ac.result_rank_list.end(),
                        bar_activity_show::OrderByVotes());
            }

            BOOST_FOREACH(auto const & p , it->result_rank_list){
                client_ptr c = xindex::get(p.actor);
                if (c) {
                    Client::client_admires_iterator i;
                    bool ret = cli->get_admire(p.actor, i);
                    string remarkName = ret ? i->second.second: "";
                    json::object usr_info = c->user_info();
                    json::object jo;
                    jo("userid", p.actor)
                        ("supports", p.countof_voter)
                        ("userName", usr_info.get<string>("nick",""))
                        ("headIcon",usr_info.get<string>("icon",""))
                        ("sign",usr_info.get<string>("signature",""))
                        ("gender",usr_info.get<string>("gender",""))
                        ("age",usr_info.get<string>("age",""))
                        ("constellation",usr_info.get<string>("constellation",""))
                        ("remarkName",remarkName)
                        ("icons",GetIndividualAlbum(p.actor))
                        ;
                    partyList(jo);
                }
            }
            LOG_I << "result_rank_list size: "<< it->result_rank_list.size() 
                << "  partyList size: "<< partyList.size();
        }
    } else if ( s_activity_info::EN_LuckyShake == it->type) {
        const char* get_activity_result = "SELECT t1.name,t1.img,t1.brief,t2.UserId,t2.PrizeType "
            " FROM bar_activity_prize t1 JOIN  personal_activity_result t2 " 
            " ON t1.id=t2.PrizeId WHERE t1.activity_id=%1%"; //" order by t1.level";

        sql::datas datas(format( get_activity_result ) %activeid);
        while(sql::datas::row_type row = datas.next()){
            string name = row.at(0,"");
            string img = row.at(1,"");
            string brief = row.at(2,"");
            UID uid = lexical_cast<UID>(row.at(3,"0"));
            int prizeType = lexical_cast<int>(row.at(4));
            if ( 0 == uid ) { continue;}

            client_ptr c = xindex::get(uid);
            if ( c ) {
                json::object obj (c->brief_user_info());
                obj.rename("icon", "headIcon");
                obj("prizeType", prizeType)
                    ("name", name)
                    ("img", complete_url(img))
                    ("brief", brief);
                partyList.push_front(obj);
            }
        }
    }

    return json::object()("partyList",partyList);
}

json::object Service::ballot(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    int activeid = lexical_cast<int>(req.param("activeid","0"));
    UID actorid = lexical_cast<int>(req.param("userid","0"));
    if(sessionid.empty() || 0==activeid) return json::object();

    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar)
        THROW_EX(EN_Bar_NotFound);

    auto it = pbar->find_activity(activeid);
    if (it == pbar->activities2.end())
        MYTHROW(EN_Bar_NotFound,bar_error_category);

    int countof_voter = 0;
    //BOOST_FOREACH(auto &  , activities){
    if (!it->shows2.empty() && it->time_begin<time(NULL) && it->time_end>time(NULL)){
       auto i = it->find_show(actorid);
       if (i != it->shows2.end())
           if ( const_cast<bar_activity_show&>(*i).vote(cli->user_id()) ) {
               imessage::message msg("activity/beauty/vote", 
                       SYSADMIN_UID, 
                       make_p2pchat_id(pbar->trader_id, pbar->assist_id));

               msg.body("activeid", activeid)
                   ("sessionid", pbar->sessionid)
                   ("userid", actorid);

               chatmgr::inst().send(msg, sysadmin_client_ptr());

               redis::command("HSET", make_activity_msgs_key(it->id), msg.id(), 
                       impack_activity_msg(msg));
           }
           const_cast<s_activity_info&>(*it).joins.insert( cli->user_id());
           countof_voter = i->countof_voter;
    }

    return json::object()("count", countof_voter);
}

json::object Service::myPrize(http::request & req, client_ptr& cli)
{
    const char* SELECT_PERSONAL_ACTIVITY_RESULT = "select SessionId,ActiveId,score,result,PrizeId,"
        " is_global from personal_activity_result where UserId=%1% AND (PrizeType=1 or PrizeType=2)";

    const char* SELECT_ACTIVITY_PRIZE = "select name,img from bar_activity_prize where id=%1%";

    const char* SELECT_G_ACTIVITY_PRIZE = "select name,img from g_activity_prize where id=%1%";

    const int index = lexical_cast<int>(req.param("index","0"));
    const int pageSize = lexical_cast<int>(req.param("pageSize","0"));
    if(0 == pageSize) return json::object();

    int count=0;
    json::array prizeList;
    sql::datas datas1(format(SELECT_PERSONAL_ACTIVITY_RESULT) % cli->user_id());
    while(sql::datas::row_type row1 = datas1.next()){
        if (count < index) {
            ++count;
            continue;
        }
        if (count+1 >= index+pageSize) break;

        int PrizeId = lexical_cast<int>(row1.at(4,"0"));
        int is_global = lexical_cast<int>(row1.at(5,"0"));

        if ( is_global ) {

            if (0 == PrizeId) { continue; }
            sql::datas datas2(format(SELECT_G_ACTIVITY_PRIZE) % PrizeId);
            if(sql::datas::row_type row2 = datas2.next()){
                string name = row2.at(0,"");
                const char* pimg = row2.at(1, "");
                if ( name.empty() ) continue;
                name += "金币";


                prizeList(json::object()("index", ++count)
                        ("sceneid", "")
                        ("prizeName", name)
                        ("prizeIcon", complete_url(pimg))
                        ("icon", "")
                        ("title", "")
                        );
            }
        } else {
            const char* pSessionId = row1.at(0);
            if (NULL==pSessionId || 0==PrizeId) continue;

            sql::datas datas3(format(SELECT_ACTIVITY_PRIZE) % PrizeId);
            if(sql::datas::row_type row3 = datas3.next()){
                const char* pname = row3.at(0);
                const char* pimg = row3.at(1, "");
                if (NULL==pname) continue;

                boost::system::error_code ec;
                boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(pSessionId, ec);
                if (!pbar) { 
                    LOG_I<< "failed to find bar "<< pSessionId;
                    continue; 
                }

                prizeList(json::object()("index", ++count)
                        ("sceneid", pSessionId)
                        ("prizeName", pname)
                        ("prizeIcon", complete_url(pimg))
                        ("icon", pbar->logo)
                        ("title", pbar->barname)
                        );
            }
        }
    }

    return json::object()("pageSize", count-index)("prizeList", prizeList);
}

json::object Service::myCoupon(http::request & req, client_ptr& cli)
{
    const char* SELECT_MYCOUPONS = "select SessionId,ActiveId,score,result,PrizeId from \
                                    personal_activity_result where UserId=%1% and PrizeType=3 \
                                    order by id desc";

    const char* SELECT_COUPON = "select name,sessionid,price,brief,beginDate,endDate \
                                 from coupons where id=%1%";

    const int index = lexical_cast<int>(req.param("index","0"));
    const int pageSize = lexical_cast<int>(req.param("pageSize","0"));
    if(0 == pageSize) return json::object();

    int count=0;
    json::array couponList;
    sql::datas datas1(format(SELECT_MYCOUPONS) % cli->user_id());
    while(sql::datas::row_type row1 = datas1.next()){
        if (count < index) {
            ++count;
            continue;
        }
        if (count+1 >= index+pageSize) break;

        const char* pSessionId = row1.at(0);
        int PrizeId = lexical_cast<int>(row1.at(4,"0"));
        if (NULL==pSessionId || 0==PrizeId) continue;

        sql::datas datas2(format(SELECT_COUPON) % PrizeId);
        if(sql::datas::row_type row2 = datas2.next()){
            const char* pname = row2.at(0);
            // const char* pimg = row2.at(1, "");
            int price = lexical_cast<int>(row2.at(2, ""));
            const char* pbrief = row2.at(3, "");
            const char* pbegin = row2.at(4, "");
            const char* pend = row2.at(5, "");
            int type = pend>time_string()?2:1;
            if (NULL==pname) continue;
            boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(pSessionId);
            if(!pbar) continue;

            couponList(json::object()("index", ++count)
                          ("sceneid", pSessionId)
                          ("prizeName", pname)
                          ("icon", pbar->logo)
                          ("type", type)
                          ("title", pbar->barname)
                          ("amount", price)
                          ("beginDate", pbegin)
                          ("endDate", pend)
                          ("condition", pbrief)
                    );
        }
    }

    return json::object()("pageSize", count-index)("couponList", couponList);
}

json::object Service::shakePartysByNear(http::request & req, client_ptr& cli)
{
    int pagesize = lexical_cast<int>(req.param("pageSize","0"));
    unsigned index = lexical_cast<int>(req.param("index","0"));
    double longtitude = lexical_cast<double>(req.param("longtitude","0"));
    double latitude = lexical_cast<double>(req.param("latitude","0"));
    string city = req.param("city");
    string province = req.param("province", "");
    string zone = req.param("zone","");

//    RECORD_TIMER(timer_log);
    vector< boost::shared_ptr<s_bar_info> > list;
    json::array partyList;
    unsigned count=0;
    bars_mgr::inst().get_bars_bycity(city, list);
    if (!list.empty() && index<list.size()){
        Position dist(latitude, longtitude);
        sort(list.begin(), list.end(), dist);
        BOOST_FOREACH(auto const & p , list){
            if (!p->activities2.empty()) {
                time_t now=time(NULL);
                BOOST_FOREACH(auto const&q , p->activities2){
                    if(1==q.type && now<q.time_end) {
                        if (count < index) {
                            ++count;
                            continue;
                        }
                        if (count+1 > index+pagesize) break;

                        json::object obj;
                        obj("sessionid", p->sessionid)
                           ("index", ++count)
                           ("activeid", q.id)
                           ("banner", q.post_img)
                           ("activeName", q.activity_name)
                           ("icon", p->logo)
                           ("address", p->address)
                           ("title", p->barname)
                           ("joinPartys", int(q.joins.size()))
                           ("beginDate", time_string(q.time_end))
                           ("endDate", time_string(q.time_begin))
                           ("system_time", time_string())
                           ("partyInfo", q.brief);
                        if (p->sessionid == cli->gwid()){
                            partyList.push_front(obj);
                        }
                        else{
                            partyList(obj);
                        }

                        break;
                    }
                }
            }
        }
    }

    return json::object()("pageSize",count-index)("partyList",partyList);
}

json::object Service::shakePartyInfo(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    int activeid = lexical_cast<int>(req.param("activeid","0"));
    if(sessionid.empty() || 0==activeid) return json::object();

    boost::shared_ptr<s_bar_info> pbar = bars_mgr::inst().get_session_bar(sessionid);
    if(!pbar) THROW_EX(EN_Bar_NotFound);

    auto it = pbar->find_activity(activeid);
    if (it == pbar->activities2.end()) MYTHROW(EN_Bar_NotFound,bar_error_category);

    return json::object()("banner", it->post_img)
        ("activeName", it->activity_name)
        ("partyInfo", it->brief)
        ("title", pbar->barname)
        ("phone", pbar->phone)
        ("beginDate", time_string(it->time_end))
        ("endDate", time_string(it->time_begin))
        ("system_time", time_string())
        ("icon", pbar->logo)
        ("address", pbar->address);
}

// POST /sendBottle type=3
json::object Service::sendBottle(http::request & req, client_ptr& cli)
{
    UID uid = cli->user_id();
    int type = lexical_cast<int>(req.param("type"));
    string cont;

    if (type == BottleMessage_Audio || type == BottleMessage_Image) {
        std::string path = uniq_relfilepath(req.param("fileName"), lexical_cast<string>(uid));
        cont = write_file(path, req.content());
    } else {
        cont = req.content();
    }

    bottle_ptr b( new bottle(cli->user_id(), cli->gwid()) );
    b->type = BottleMessage_Type(type);
    b->content = cont;
    LOG_I << *b;

    auto ec = drifting::inst(cli->user_id()).Throw(b);
    if (ec)
        mythrow(__LINE__,__FILE__,ec);

    return json::object()("id", b->id)
        ("throw_count_left",drifting::inst(cli->user_id()).throw_count_left());
}

static json::object json_bottle(bottle_ptr const & b)
{
    client_ptr c = xindex::get(b->initiator);
    if (!c)
        MYTHROW(EN_User_NotFound,client_error_category);

    std::string cont = b->content;
    if (b->type == BottleMessage_Audio || b->type == BottleMessage_Image)
        cont = complete_url(cont);

    json::object const & uinf = c->user_info();
    return json::object()
        ("bottleid", b->id)
        ("type", int(b->type))
        ("message", cont)
        ("date", time_string(b->xtime))
        // user-info
        ("headIcon", uinf.get<std::string>("icon"))
        ("userName", uinf.get<std::string>("nick"))
        ("userid", uinf.get<UID>("userid"))
        ("gender", uinf.get<std::string>("gender"))
        ("sign", uinf.get<std::string>("signature"))
        ("age", uinf.get<std::string>("age"))
        ;
}

json::object Service::receiveBottle(http::request & req, client_ptr& cli)
{
    auto p = drifting::inst(cli->user_id()).Catch(cli->gwid());
    if (p.second)
        mythrow(__LINE__,__FILE__,p.second);

    if (!p.first)
        return json::object()("type",4)
            ("catch_count_left",drifting::inst(cli->user_id()).catch_count_left());

    LOG_I << *p.first;
    return json_bottle(p.first)
            ("catch_count_left",drifting::inst(cli->user_id()).catch_count_left());
}

json::object Service::throwBottle(http::request & req, client_ptr& cli)
{
    unsigned int bid = lexical_cast<unsigned int>(req.param("bottleid"));
    drifting::inst(cli->user_id()).Rethrow(bid);
    return json::object();
}

json::object Service::deleteBottle(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("bottleSid","");
    if ( sessionid.empty() ) {
        drifting::inst(cli->user_id()).deleteAll();
    } else {

        if ( is_bottlechat( sessionid ) ) {
            boost::regex re("[.!](\\d+)[.!](\\d+)([.!](\\d+))?");
            boost::smatch subs;
            if (boost::regex_match(sessionid, subs, re)) {
                unsigned int bid = lexical_cast<unsigned int>(subs[4]);
                drifting::inst(cli->user_id()).Delete(bid);
                erase_chat_bottle_id(bid);
            }
        } else {
            THROW_EX(EN_Input_Data);
        }
    }

    return json::object();
}

json::object Service::myBottle(http::request & req, client_ptr& cli)
{
    json::array ret;
    auto p = drifting::inst(cli->user_id()).Bottles();
    BOOST_FOREACH(auto const& b, p) {
        LOG_I << *b;
        ret.put( json_bottle(b) );
    }
    return json::object()("myBottleList", ret);
}

json::object Service::bottleChance(http::request & req, client_ptr& cli)
{
    auto& p = drifting::inst(cli->user_id());
    return json::object()("catch_count_left", p.catch_count_left())
        ("throw_count_left", p.throw_count_left());
}

json::object Service::checkShake(http::request & req, client_ptr& cli)
{
    string sessionid = cli->gwid();
    string city = req.param("city", "深圳市");

    boost::shared_ptr<s_bar_info> pbar;
    if ( !sessionid.empty() ) {
        pbar= bars_mgr::inst().get_session_bar(sessionid);
    }

    json::object jo;
    if ( pbar ) {
        time_t now=time(NULL);
        BOOST_FOREACH(auto &q , pbar->activities2) {
            if (q.time_begin<now && q.time_end>now && 1==q.type) {
                jo("status", true)
                    ("sceneid", sessionid)
                    ("beginDate", time_string(q.time_begin))
                    ("endDate", time_string(q.time_end))
                    ("system_time", time_string())
                    ("is_global", false)
                    ("chance", cli->lucky_shake_count)
                    ("activeid", q.id);
                break;
            }
        }
    }

    if ( jo.empty() ) {
        string city = req.param("city");
        auto & icity = boost::get<TagGactivityCity>(global_mgr::inst().g_activities_);
        auto p = icity.equal_range(city);
        for ( auto q=p.first; q!=p.second; ++q ) {
            // if ( q->SessionIds.empty() ){
            //     if (std::find(q->SessionIds.begin(),q->SessionIds.end(),pbar->sessionid)
            //             !=q->SessionIds.end())
            //     {
            jo("status", true)
                ("sceneid", "")
                ("beginDate", time_string(q->time_begin))
                ("endDate", time_string(q->time_end))
                ("system_time", time_string())
                ("is_global", true)
                ("chance", cli->lucky_shake_count)
                ("activeid", q->id);
            //     }
            // }
        }
    }

    return jo;
}

json::object Service::prizeByShake(http::request & req, client_ptr& cli)
{
#define DENOMINATOR 1000
    const char* INSERT_PERSONAL_ACTIVITY_RESULT = "insert into personal_activity_result("
        "UserId,SessionId, ActiveId,score,result,PrizeId,PrizeType, is_global"
        ") values(%1%,'%2%',%3%,%4%,'%5%',%6%, %7%, %8%)";
    const char* UPDATE_BAR_ACTIVITY_PRIZE = "update bar_activity_prize set count=count-1 where id=%1%";
    // const char* UPDATE_COUPONS = "update coupons set count=count-1 where id=%1%";

    int activeid = lexical_cast<int>(req.param("activeid"));
    unsigned luck_number = randuint(1,DENOMINATOR);
    extern unsigned int numerator;
    extern unsigned int g_numerator;
    json::object obj;

    boost::shared_ptr<s_bar_info> pbar;
    s_bar_info::activities_type::iterator it;

    {
        string sessionid = cli->gwid();
        if (!sessionid.empty()) {
            pbar = bars_mgr::inst().get_session_bar(sessionid);
            if (pbar) {
                it = pbar->find_activity(activeid);
            }
        }
    }

    if (pbar && it!=pbar->activities2.end()) {
        if ( !s_bar_info::check_employee( pbar->trader_id, cli->user_id())) {
            if (luck_number < numerator) {
                time_t now=time(NULL);
                if (it->time_begin < now && now < it->time_end) {
                    int count = activity_count_incr(it->id, cli->user_id(), it->prize_setting);
                    int n = it->awards.size();
                    if(n>0 && count <= it->prize_setting) {
                        vector<int> awd_indexs;
                        int index = 0;
                        s_activity_info& activity = const_cast<s_activity_info&>(*it);
                        BOOST_FOREACH( auto& p, activity.awards ) {
                            awd_indexs.insert(awd_indexs.end(), p.count, index++);
                        }
                        std::random_shuffle(awd_indexs.begin(), awd_indexs.end());
                        index = awd_indexs[randuint(0, awd_indexs.size()-1)];

                        auto it2 = activity.awards.begin()+index;
                        LOG_I << "size():"<<n <<" luck_number:"<<luck_number <<" index:"<< index;

                        --it2->count;
                        LOG_I<<"GET a prize "<<it2->name<<" "<<it2->img<< " left: "<<it2->count;
                        obj("type", 1)
                            ("sceneid", pbar->sessionid)
                            ("logo",pbar->logo)
                            ("title", pbar->barname)
                            ("prize_img", it2->img)
                            ("prize", it2->name);

                        sql::exec(format(INSERT_PERSONAL_ACTIVITY_RESULT)%cli->user_id() 
                                %pbar->sessionid %activeid %0 %"" %it2->id %1 %0);  

                        sql::exec(format(UPDATE_BAR_ACTIVITY_PRIZE)% it2->id);

                        //{ //
                        //    imessage::message msg0("activity/shake/prize", 
                        //            pbar->trader_id, make_p2pchat_id(cli->user_id(), pbar->trader_id));
                        //    string mark = str(format("恭喜你在活动%1%中获得了奖品%2%，请前往现场领取奖品")
                        //            %activity.activity_name %it2->name);
                        //    msg0.body("mark", mark)("prize_img", it2->img);
                        //    // chatmgr::inst().send(msg0, sysadmin_client_ptr());
                        //    sendto(msg0, cli);
                        //}
                        if (0 >= it2->count) { activity.awards.erase(it2); }

                        {
                            imessage::message msg("activity/shake/lucky", pbar->trader_id, pbar->sessionid);
                            msg.body
                                ("activity_id", activity.id)
                                ("activity_name", activity.activity_name)
                                ("activity_type", activity.type)
                                ("activity_begin", time_string(activity.time_begin))
                                ("activity_end", time_string(activity.time_end))
                                ("content", cli->brief_user_info())
                                ("sequence", msg.id())
                                ("prize", obj)
                                ;
                            std::set<UID> tmpset; //( pbar->inspot_users2_.begin(), pbar->inspot_users2_.end() );
                            BOOST_FOREACH(auto & x, pbar->inspot_users2_)
                                tmpset.insert(x.uid);
                            tmpset.insert(pbar->assist_id);
                            sendto(msg, tmpset);
                            //LOG_I << tmpset;
                            //sendto(msg, pbar->inspot_users2_);
                            //if (client_ptr c = xindex::get(pbar->assist_id))
                            //    Client::pushmsg(c, msg);

                            redis::command("HSET", make_activity_msgs_key(activity.id), 
                                    msg.id(), impack_activity_msg(msg));
                        }
                    }
                }
            }
        }
    } else {
        if ( cli->lucky_shake_count <= 0 ) {
            THROW_EX(EN_Luckshake_Count_Limit);
        }

        extern const char* HKEY_USER_LUCK_SHAKE_COUNT_LEFT;
        --cli->lucky_shake_count;
        redis::command("HSET", HKEY_USER_LUCK_SHAKE_COUNT_LEFT, cli->user_id(), cli->lucky_shake_count);

        if (luck_number < g_numerator) {
            boost::system::error_code err;
            auto i = global_mgr::inst().find_g_activity(activeid, err);
            if (err) { 
                mythrow(__LINE__,__FILE__,err);
            }

            if ( !i->g_awards.empty() ) {
                double probability = randuint(1,DENOMINATOR);

                for ( auto j=i->g_awards.begin(); j!=i->g_awards.end(); ++j) {
                    probability -= j->probability*DENOMINATOR;
                    if ( probability <= 0 ) {
                        LOG_I<<"GET a prize "<<j->name;
                        obj("type", 2)
                            ("prize_img", j->img)
                            ("prize", j->name);

                        sql::exec(format(INSERT_PERSONAL_ACTIVITY_RESULT)%cli->user_id() 
                                %"" %activeid %0 %"" %j->id %2 %1 );  

                        return obj;
                    }
                }
            }
        }
    }

    // if (!pbar || 0==pbar->bar_coupons_.size()) {
    //     std::vector<bar_ptr>& list=bars_mgr::inst().get_coupon_bar_list();
    //     if ( !list.empty() ) {
    //         pbar = *std::min_element(list.begin(), list.end(), Position(latitude, longtitude));
    //     }
    // }

    // if ( pbar ) {
    //     int n = pbar->bar_coupons_.size();
    //     if ( n ) {
    //         int coup_num = randuint(0,n-1);
    //         vector<s_bar_info::bar_coupon>::iterator itr=pbar->bar_coupons_.begin()+coup_num;
    //         --itr->count;
    //         LOG_I<<"GET a Coupons "<<itr->name<< " left: "<<itr->count;
    //         obj("type", 3)
    //             ("sceneid", pbar->sessionid)
    //             ("logo",pbar->logo)
    //             ("title", pbar->barname)
    //             ("coupon", itr->name);

    //         sql::exec(format(INSERT_PERSONAL_ACTIVITY_RESULT)%cli->user_id() 
    //                 %pbar->sessionid %0 %0 %"" %itr->id %3);  
    //         sql::exec(format(UPDATE_COUPONS)% itr->id);
    //         if ( 0 == itr->count ) { pbar->bar_coupons_.erase(itr); }

    //         if ( 0 == pbar->bar_coupons_.size()){
    //             std::vector<bar_ptr>& list=bars_mgr::inst().get_coupon_bar_list();
    //             list.erase(remove(list.begin(), list.end(), pbar), list.end());
    //         }
    //     }
    // }
    LOG_I << luck_number <<" lucky random "<< numerator <<" "<< bool(!obj.empty());
    if ( obj.empty() ) obj("type", 4);
    return obj;
}

json::object Service::bar_reject(http::request & req, client_ptr& cli)
{
    // UID userid = cli->user_id();
    // client_ptr c = xindex::get(userid);
    UID userid = lexical_cast<UID>(req.param("userid"));
    UID touid = lexical_cast<UID>(req.param("rejected"));
    int type = lexical_cast<int>(req.param("type"));

    bar_ptr pbar = bars_mgr::inst().get_bar_byuid(userid);

    if(1 == type){
        sql::exec(format(UPDATE_CONTACTS_INSERT_RELATION3)%userid %touid %0);
        // imessage::message msg;
        // msg.gid = make_p2pchat_id(touid, cli->user_id());
        // msg.from = cli->user_id();
        // msg.type = "chat/fans";
        // json::object from = cli->brief_user_info();
        // from("sign",cli->user_info().get<string>("signature",""));
        // msg.body
        //     ("from", from)
        //     //("time", time_string())
        //     ;
        // chatmgr::inst().send(msg, cli);

        // //version 1.1
        pbar->black_list.insert(touid);
    }
    else if(2 == type){
        sql::exec(format(DELETE_CONTACTS2)%userid %touid);
        // sql::exec(format(UPDATE_CONTACTS_USERNAME3)%"" %touid %userid);

        // imessage::message msg;
        // msg.gid = make_p2pchat_id(touid, cli->user_id());
        // msg.from = cli->user_id();
        // msg.type = "chat/fans-lose";
        // //msg.body
        //     //("from", cli->brief_user_info())
        //     //("time", time_string())
        //     //;
        // chatmgr::inst().send(msg, cli);
        // 
        // //version 1.1
        pbar->black_list.erase(touid);
    }

    return json::object();
}

json::object Service::gameCenter(http::request & req, client_ptr& cli)
{
    string sessionid = req.param("sceneid","");
    if(sessionid.empty()) return json::object();

    return json::object();
}

// bool inline CheckWhisperLove(UID userid, UID otherid)
// {
//     const char* SELECT_WHISPERLOVES = "select UserId,OtherId,content,SessionId,whisperTime"
//                                        " from whisperLoves where UserId=%1% and OtherId=%2%";
//     bool issay = false;
//     format fmt(SELECT_WHISPERLOVES);
//     fmt %userid %otherid;
//     sql::datas datas(fmt);
//     if( datas.next() ) { issay = true; }
// 
//     return issay;
// }
 
// GET /whisperLove otherUserid=1000
json::object Service::whisperLove(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID otherid = lexical_cast<int>(req.param("otherUserid"));
    string content = req.content();
    const char* INSERT_WHISPERLOVES = "insert into whisperLoves(UserId,OtherId,content,SessionId) values(%1%, %2%, '%3%', '%4%')";
    const char* SELECT_WHISPERLOVES = "select UserId,OtherId,content,SessionId,whisperTime \
                                       from whisperLoves where UserId=%1% and OtherId=%2%";
    sql::exec(format(INSERT_WHISPERLOVES) %userid %otherid %content %cli->gwid());
    json::object rep;
    bool otherMyWhisper = false;
    rep("myWhisper", true);
    format fmt ( SELECT_WHISPERLOVES );
    sql::datas datas(fmt %otherid %userid);
    if(sql::datas::row_type row = datas.next()){
       otherMyWhisper = true; 
    }

    json::object my, m_user = cli->user_info();
    my("userid", userid)
        ("icon",m_user.get<string>("icon"))
        ("userName",m_user.get<string>("nick"))
        ("gender", m_user.get<string>("gender"))
        ("guest",false)
        ("date",time_string())
        ;

    imessage::message msg("chat/pair", SYSADMIN_UID, make_p2pchat_id(otherid, SYSADMIN_UID));
    client_ptr system_ptr = sysadmin_client_ptr();
    msg.body
        ("content", my)
        ("success", otherMyWhisper)
        ;
    chatmgr::inst().send(msg, cli);

    return rep("otherMyWhisper", otherMyWhisper);
}

json::object Service::shakeChance(http::request & req, client_ptr& cli)
{
    return json::object()("chance", cli->lucky_shake_count);
}

json::object Service::whisperLoveStatus(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    UID otherid = lexical_cast<int>(req.param("otherUserid"));
    const char* SELECT_WHISPERLOVES = "select UserId,OtherId,content,SessionId,whisperTime from \
                                       whisperLoves where (UserId=%1% and OtherId=%2%) \
                                       or (UserId=%2% and OtherId=%1%)";
    // json::array status;
    bool myWhisper=false,otherMyWhisper=false;
    format fmt(SELECT_WHISPERLOVES);
    sql::datas datas(fmt %userid %otherid);
    while ( sql::datas::row_type row = datas.next() ) {
        UID uid = lexical_cast<UID>(row.at(0));
        // UID oid = lexical_cast<UID>(row.at(1));
        // const char* pcontent = row.at(2, "");
        // const char* pSessionId = row.at(3, "");
        // const char* ptime = row.at(4, "");
        if ( uid == userid ) {
            myWhisper = true;
            // status(
            //         myWhisper
            //         otherWhisper
            //         json::object()("userid",uid)
            //         ("otherid",oid)
            //         ("content",pcontent)
            //         ("sceneid",pSessionId)
            //         ("sentTime", ptime)
            //       );
        } else {
            otherMyWhisper = true;
            // status (
            //         json::object()("userid",uid)
            //         ("otherid",oid)
            //         ("content",pcontent)
            //         ("sceneid",pSessionId)
            //         ("recvTime", ptime)
            //        );
        }
    }

    return json::object()("used", cli->whisper_love_count)
                      ("myWhisper", myWhisper)
                      ("otherMyWhisper",otherMyWhisper);
}

json::object Service::whisperList(http::request & req, client_ptr& cli)
{
    UID userid = cli->user_id();
    const char* SELECT_RECVWHISPERLOVES = "select UserId,OtherId,content,SessionId,whisperTime \
                                           from whisperLoves where UserId=%1% or OtherId=%1% order by whisperTime desc";
    json::array mySaidList,otherSaidList;
    format fmt(SELECT_RECVWHISPERLOVES);
    sql::datas datas(fmt %userid);
    while ( sql::datas::row_type row = datas.next() ) {
        UID uid = lexical_cast<UID>(row.at(0));
        UID oid = lexical_cast<UID>(row.at(1));
        const char* pcontent = row.at(2, "");
        string SessionId = row.at(3, "");
        string barname;
        if ( !SessionId.empty() ) {
            bar_ptr pbar = bars_mgr::inst().get_session_bar(SessionId);
            if ( pbar ){
                barname = pbar->barname;
            }
        }
        const char* ptime = row.at(4, "");
        if ( uid == userid ){
            client_ptr c = xindex::get(oid);
            if ( c ) {
            json::object usr_info = c->user_info();
            mySaidList(json::object()("userid", oid)
                ("userName", usr_info.get<string>("nick",""))
                ("headIcon",usr_info.get<string>("icon",""))
                ("gender",usr_info.get<string>("gender",""))
                ("age",usr_info.get<string>("age",""))
                ("content",pcontent)
                ("sceneid",SessionId)
                ("title", barname)
                ("date", ptime)
                );
            }
        } else {
            client_ptr c = xindex::get(uid);
            if ( c ) {
            json::object usr_info = c->user_info();
            otherSaidList(json::object()("userid", uid)
                ("userName", usr_info.get<string>("nick",""))
                ("headIcon",usr_info.get<string>("icon",""))
                ("gender",usr_info.get<string>("gender",""))
                ("age",usr_info.get<string>("age",""))
                ("content",pcontent)
                ("sceneid",SessionId)
                ("title", barname)
                ("date", ptime)
                );
            }
        }
    }

    return json::object()("mySaidList", mySaidList)("otherSaidList",otherSaidList);
}

json::object Service::barOnlineLike(http::request & req, client_ptr& cli)
{
//    RECORD_TIMER(timer_log);
    UID userid = cli->user_id();
    UID otherid = lexical_cast<int>(req.param("otherUserid"));
    int type = lexical_cast<int>(req.param("type"));
    string sessionid = req.param("sceneid");
    const char* INSERT_BAR_ONLINE_LIKE = "insert into bar_online_like(UserId,OtherId,SessionId,type) values(%1%, %2%, '%3%', %4%)";
    const char* SELECT_BAR_ONLINE_LIKE = "select type from bar_online_like where UserId=%1% and OtherId=%2% and SessionId='%3%'";

    if ( otherid == userid ) { THROW_EX(EN_Operation_NotPermited);}
    sql::datas datas(format(SELECT_BAR_ONLINE_LIKE) %userid %otherid %sessionid);
    if(sql::datas::row_type row = datas.next()){
       return json::object()("result", 2+lexical_cast<int>(row.at(0,"0")));
    } else {
        sql::exec(format(INSERT_BAR_ONLINE_LIKE) %userid %otherid %sessionid %type);
        if ( 1 == type ) { 
            int charm = 10;
            incr_charms(otherid, charm, userid);
            // sql::exec(format(INSERT_STATISTICS_CHARISMA) %otherid %userid %charm %cli->gwid());
            //if ( client_ptr o = xindex::get( otherid ) ) {
            //    // o->set_charm( userid, otherid, charm );
            //    // push_charm(cli, otherid, charm);
            //}
        }
    }

    return json::object()("result", type);
}

