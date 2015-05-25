#ifndef SERV_H__
#define SERV_H__

#include <string>
#include <ostream>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include "client.h"
#include "proto_http.h"
#include "proto_im.h"

struct Service // : boost::noncopyable
{
    static json::object serve(http::request & req, client_ptr& client);

// private:

    static json::object get_profile(http::request& req, client_ptr& client);
    static json::object set_profile(http::request& req, client_ptr& client) ;
    static json::object feedback(http::request& req, client_ptr& client) ;
    static json::object sendGift(http::request& req, client_ptr& client) ;
    static json::object expressLoveResult(http::request& req, client_ptr& client) ;
    static json::object initBarSession(http::request& req, client_ptr& client) ;
    static json::object DelBarSession(http::request& req, client_ptr& client) ;
    static json::object set_photo(http::request& req, client_ptr& client) ;
    static json::object update_client(http::request& req, client_ptr& client) ;

    static json::object setNumerator(http::request& req, client_ptr& client) ;
    static json::object echo(http::request & req, client_ptr& client);
    static json::object loguser(http::request & req, client_ptr& cli);
    static json::object logsave(http::request & req, client_ptr& cli);

    static json::object updateScenseIcon(http::request & req, client_ptr& client);
    // static json::object messageNotification(http::request & req, client_ptr& client);
    static json::object modifyGroupName(http::request & req, client_ptr& client);
    static json::object leaveJoinTalkBar(http::request & req, client_ptr& client);
    static json::object says(http::request & req, client_ptr& client);
    static json::object personInfo(http::request & req, client_ptr& client);
    static json::object updateUserBgIcon(http::request & req, client_ptr& client);
    static json::object attentions(http::request & req, client_ptr& client);
    static json::object relationship(http::request & req, client_ptr& client);
    static json::object remark(http::request & req, client_ptr& client);
    static json::object contactList(http::request & req, client_ptr& client);
    static json::object relationShipList(http::request & req, client_ptr& client);
    static json::object memberList(http::request & req, client_ptr& client);
    static json::object myInfo(http::request & req, client_ptr& client);
    static json::object uploadMyPhoto(http::request & req, client_ptr& client);
    static json::object modifyMyHead(http::request & req, client_ptr& client);
    static json::object modifyMyName(http::request & req, client_ptr& client);
    static json::object modifySign(http::request & req, client_ptr& client);
    static json::object modifyMyAge(http::request & req, client_ptr& client);
    static json::object deleteMyPhoto(http::request & req, client_ptr& client);
    static json::object inviteJoinGroup(http::request & req, client_ptr& client);
    static json::object delGroupMember(http::request & req, client_ptr& client);
    static json::object quitGroup(http::request & req, client_ptr& client);

    static json::object uploadFile(http::request & req, client_ptr& cli);
    static json::object publishDynamicMessage(http::request & req, client_ptr& cli);
    static json::object supportComment(http::request & req, client_ptr& cli);
    static json::object deleteComment(http::request & req, client_ptr& cli);
    static json::object deleteDynamic(http::request & req, client_ptr& cli);
    static json::object publishComment(http::request & req, client_ptr& cli);
    static json::object dynamicCommentList(http::request & req, client_ptr& cli);
    static json::object dynamicMessage(http::request & req, client_ptr& cli);
    //static json::object clearUnread(http::request & req, client_ptr& cli);
    static json::object unReadList(http::request & req, client_ptr& cli);

    static json::object login(http::request & req, client_ptr& client);
    static json::object logout(http::request & req, client_ptr& client);
    static json::object receivedGifts(http::request & req, client_ptr& client);
    static json::object shops(http::request & req, client_ptr& client);
    static json::object goodsInfo(http::request & req, client_ptr& client);
    static json::object chargeList(http::request & req, client_ptr& client);
    //static json::object bills(http::request & req, client_ptr& client);
    static json::object bars_people(http::request & req, client_ptr& client);
    static json::object exchangeUserInfo(http::request & req, client_ptr& client);
    static json::object myExchange(http::request & req, client_ptr& client);

    static json::object chat(http::request & req, client_ptr& client);
    static json::object create_chat(http::request & req, client_ptr& client);

    static json::object barInfo(http::request & req, client_ptr& client);
    static json::object groupInfo(http::request & req, client_ptr& client);
    static json::object joinGroup(http::request & req, client_ptr& client);
    static json::object barList(http::request & req, client_ptr& client);
    static json::object userProtocol(http::request & req, client_ptr& client);
    static json::object searchBar(http::request & req, client_ptr& client);
    static json::object scense(http::request & req, client_ptr& client);
    static json::object actives(http::request & req, client_ptr& client);
    static json::object openCity(http::request & req, client_ptr& client);
    static json::object openZone(http::request & req, client_ptr& client);
    static json::object checkUpdate(http::request& req, client_ptr& client);
    static json::object hello(http::request& req, client_ptr& client);

    // version 1.1
    static json::object programList(http::request & req, client_ptr& client);
    static json::object barFriendOnline(http::request & req, client_ptr& client);
    static json::object personResource(http::request & req, client_ptr& client);
    static json::object nearByBar(http::request & req, client_ptr& client);
    static json::object activeTonight(http::request & req, client_ptr& client);
    static json::object activeToscene(http::request & req, client_ptr& client);
    static json::object activeInfo(http::request & req, client_ptr& client);
    static json::object gameCenter(http::request & req, client_ptr& client);
    static json::object barInfos(http::request & req, client_ptr& client);
    static json::object attentionsBar(http::request & req, client_ptr& client);
    static json::object attentionsBarList(http::request & req, client_ptr& cli);
    static json::object joinParty(http::request & req, client_ptr& client);
    static json::object partyResult(http::request & req, client_ptr& client);
    static json::object ballot(http::request & req, client_ptr& client);
    static json::object shakePartysByNear(http::request & req, client_ptr& client);
    static json::object myPrize(http::request & req, client_ptr& client);
    static json::object myCoupon(http::request & req, client_ptr& client);
    static json::object checkShake(http::request & req, client_ptr& client);
    static json::object prizeByShake(http::request & req, client_ptr& client);
    static json::object shakeChance(http::request & req, client_ptr& client);
    static json::object shakePartyInfo(http::request & req, client_ptr& client);
    static json::object bar_reject(http::request & req, client_ptr& client);
    static json::object whisperLove(http::request & req, client_ptr& client);
    static json::object whisperLoveStatus(http::request & req, client_ptr& client);
    static json::object whisperList(http::request & req, client_ptr& client);
    static json::object chargeSuccess(http::request & req, client_ptr& client);

    static json::object barOnlineLike(http::request & req, client_ptr& client);

    static json::object myBottle(http::request & req, client_ptr& cli) ;
    static json::object sendBottle(http::request & req, client_ptr& cli) ;
    static json::object throwBottle(http::request & req, client_ptr& cli) ;
    static json::object deleteBottle(http::request & req, client_ptr& cli) ;
    static json::object receiveBottle(http::request & req, client_ptr& cli) ;
    static json::object bottleChance(http::request & req, client_ptr& cli) ;

    //manager setting
    static json::object initBarActivity(http::request & req, client_ptr& client);
    static json::object updateBarActivity(http::request & req, client_ptr& client);
    static json::object deleteBarActivity(http::request & req, client_ptr& client);
    static json::object changeBarActivityIndex(http::request & req, client_ptr& client);

    static json::object initBarAdvertising(http::request & req, client_ptr& client);
    static json::object updateBarAdvertising(http::request & req, client_ptr& client);
    static json::object deleteBarAdvertising(http::request & req, client_ptr& client);

    static json::object initBarCoupons(http::request & req, client_ptr& client);
    static json::object updateBarCoupons(http::request & req, client_ptr& client);
    static json::object deleteBarCoupons(http::request & req, client_ptr& client);

    static json::object getBarSession(http::request & req, client_ptr& client);
    static json::object noticeScreen(http::request & req, client_ptr& client);
    static json::object sendBarFans(http::request & req, client_ptr& cli);
    static json::object merchantChat(http::request & req, client_ptr& client);
    static json::object publicNotify(http::request & req, client_ptr& client);
    static json::object exchangeCoins(http::request & req, client_ptr& client);

private:
    explicit Service(cache_type& cache) // : cache_(&cache)
    {}
    Service() // : cache_(0)
    {}

public:

    static bool alipay;
    static bool upPayPlugin;

public:
    struct initializer : boost::noncopyable
    {
        initializer(const boost::property_tree::ptree& ini);
        ~initializer();
    };
    friend struct initializer;
};

inline std::string url_to_relative(const std::string& url)
{
    std::string relative_path;
    if(!url.empty() && boost::starts_with(url, Client::Files_Url)){
       relative_path.append(url, Client::Files_Url.size()+1, std::string::npos);
    }

    return relative_path;
}

inline std::string url_to_local(const std::string& url)
{
    std::string relative_path, local_path;
    relative_path = url_to_relative( url );
    if ( !relative_path.empty() ) {
        local_path = (Client::Files_Dir / relative_path).string();
    }
    return local_path;
}

enum GIFT_TYPE {FLOWER=1, CAR, PLAN, YACHT};

json::array GetIndividualAlbum(UID userid);
#endif

