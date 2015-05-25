#ifndef _BARS_H_
#define _BARS_H_

#include <string>
#include <vector>
#include <list>
#include <cmath>
#include <iterator>
#include <array>
#include <boost/smart_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>

#include "chat.h"
#include "log.h"
#include "jss.h"
#include "service.h"

enum enum_bar_error
{
    EN_Bar_NotFound2 = 515,
    EN_Activity_NotFound,
    EN_Prize_NotFound,
    EN_Luckshake_Count_Limit,
    EN_Advertising_NotFound,
};

struct bar_error_category : myerror_category
{
    bar_error_category();
};

json::array GetIndividualAlbum(UID userid);

inline std::string impack_activity_msg(const imessage::message & m)
{
    extern json::object impack_body_object(const imessage::message & m);

    json::object obj = impack_body_object(m);
    return json::encode( obj("method", m.type) ("sequence", m.id()));
}

inline std::string make_activity_msgs_key( int activity_id )
{
    std::string key("activity_");
    return key + boost::lexical_cast<std::string>(activity_id) + "_msgs";
}

inline double rad(double d)
{
    const double pi = 3.14159265;
    return d * pi / 180.0;
}

inline double GetDistance(double lat1, double lng1, double lat2, double lng2)
{
    const double EARTH_RADIUS = 6378.137;
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double s = 2 * asin(sqrt(pow(sin(a/2),2)+cos(radLat1)*cos(radLat2)*pow(sin(b/2),2)));
    s = s * EARTH_RADIUS;
    s = round(s * 10000) / 10000;
    return s;
}

struct activity_prize
{
    unsigned int id;
    int count, initial_count;
    std::string name;
    std::string img;
    std::string brief;
    activity_prize() { id = 0; }

    template <typename OutIter> static void load(int activity_id, OutIter it);
};

template <typename OutIter>
void activity_prize::load(int activity_id, OutIter it)
{
    boost::format fmt("SELECT id,count,initial_count,name,img,brief FROM bar_activity_prize WHERE activity_id=%1%");
    sql::datas datas(fmt % activity_id);
    while (sql::datas::row_type row = datas.next())
    {
        activity_prize a;
        a.id = boost::lexical_cast<unsigned int>(row.at(0));
        a.count = boost::lexical_cast<int>(row.at(1));
        if ( 0 >= a.count ) { continue; }

        a.initial_count = boost::lexical_cast<unsigned>(row.at(2));
        a.name = row.at(3,"");
        a.img = complete_url(row.at(4,""));
        a.brief = row.at(5,"");
        *it++ = a;
    }
}

struct advertising
{
    static const char *db_table_name() { return "advertising"; }
    static int alloc_id() { return max_index<advertising>(+1); }
    int id;
    std::string name;
    std::string A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2;
    time_t beginDate;
    time_t endDate;

};

struct bar_activity_show
{
    static const char *db_table_name() { return "bar_activity_show"; }
    static int alloc_id() { return max_index<bar_activity_show>(+1); }
    struct OrderByVotes {
        bool operator()(bar_activity_show const &a, bar_activity_show const & b) const
            { return a.countof_voter>b.countof_voter; }
    };

    int id;
    UID actor;
    std::set<UID> voters;
    int countof_voter;

    explicit bar_activity_show(UID a)
    {
        actor = a;
        countof_voter = 0;
        id = alloc_id();
    }

    bool vote(UID vter)
    {
        auto p = voters.insert(vter);
        if (!p.second)
            return false;
        ++countof_voter;
        const char* VOTE_ACTIVITY_SHOW = "UPDATE bar_activity_show SET voters='%2%' WHERE id=%1%";
        // boost::algorithm::join(this->voters, " ");
        std::ostringstream out;
        BOOST_FOREACH(auto const & p , this->voters)
            out << p << " ";
        sql::exec(boost::format(VOTE_ACTIVITY_SHOW) % this->id % out.str());
        return true;
    }

public:
    template <typename OutIter> static void load_shows(int activity_id, OutIter iter);

private:
    bar_activity_show() {}
};

template <typename OutIter>
void bar_activity_show::load_shows(int activity_id, OutIter iter)
{
    const char* LOAD_ACTIVITY_SHOW = "SELECT id,actor,voters FROM bar_activity_show WHERE activity_id=%1%";
    sql::datas datas( boost::format(LOAD_ACTIVITY_SHOW) % activity_id );
    while (sql::datas::row_type row = datas.next()) {
        bar_activity_show s;
        s.id = boost::lexical_cast<int>(row.at(0));
        s.actor = boost::lexical_cast<int>(row.at(1));
        if (row.at(2,NULL) != NULL) {
            std::istringstream ins(row.at(2));
            std::istream_iterator<UID> i(ins), end;
            s.voters.insert(i, end);
        }
        s.countof_voter = s.voters.size();
        *iter++ = s;
    }
}

struct Actor {};
struct ShowId {};

void activity_count_load(UInt activity_id);
int activity_count_incr(UInt activity_id, UID uid, int limit_max);
int activity_count_erase(UInt activity_id);

struct s_activity_info
{
    static const char *db_table_name() { return "bar_activity2"; }
    static int alloc_id() { return max_index<s_activity_info>(+1); }
    // typedef std::list<bar_activity_show>::iterator iterator;
    // std::list<bar_activity_show> shows;
    typedef boost::multi_index_container<
        bar_activity_show,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ShowId>, BOOST_MULTI_INDEX_MEMBER(bar_activity_show,int,id)
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<Actor>, BOOST_MULTI_INDEX_MEMBER(bar_activity_show,UID,actor)
            >
        >
    > shows_type;
    shows_type shows2;
    std::vector<bar_activity_show> result_rank_list;

    int id, type, show_index;
    enum enum_type {
        EN_LuckyShake = 1,
        EN_VoteBeauty = 2,
        EN_NormalActivity = 3,
    };
    // std::string sessionid;

    std::string activity_name;
    time_t time_begin;
    time_t time_end;
    // flags
    bool foremost; // 是否 首页推广
    bool on_off; // 是否通知大屏幕展示 

    int candidate_setting;
    int winners_setting;
    int attendees_setting;
    int prize_setting;
    //struct activity_award : activity_prize
    //{
    //    unsigned short count;
    //    activity_award() { count = 0; }
    //};
    std::vector<activity_prize> awards;

    std::string brief;
    std::string post_img;
    std::string share_url;
    std::string shorten_share;
    std::string shorten_banner;
    std::string screen_img;
    // std::vector<std::string> imgs;
    // std::string detail;
    // bool is_exhibition;
    bool already_end, already_begin;
    std::string result;
    std::set<UID> joins;

    bool is_closed() const { return already_end; }
    shows_type::iterator find_show(UID actor) const
    {
        s_activity_info *thiz = const_cast<s_activity_info*>(this);
        boost::multi_index::index<shows_type,Actor>::type& idx = boost::get<Actor>(thiz->shows2);
        return thiz->shows2.project<0>( idx.find(actor) );
    }

    shows_type::iterator add_show(UID actor)
    {
#define ONLINE_MEN 1
#define ONLINE_WOMEN 2
        client_ptr c = xindex::get(actor);
        if ( !c ) { return shows2.end(); }
        std::string headicon = c->user_info().get<std::string>("icon","");
        if ( headicon.empty() ) {
            return shows2.end();
        }
        std::string file = url_to_local(headicon);
        if ( file.empty() ) {
            return shows2.end();
        }

        if ( ONLINE_MEN == candidate_setting ) {
            if ( "F" == c->user_gender() ) { return shows2.end(); }
        } else if ( ONLINE_WOMEN == candidate_setting ) {
            if ( "M" == c->user_gender() ) { return shows2.end(); }
        }

        bar_activity_show bs(actor);
        auto p = shows2.insert(shows2.end(), bs);
        if (p.second) {
            const char* ADD_ACTIVITY_SHOW = "INSERT INTO bar_activity_show(id,activity_id,actor) \
                                             VALUES(%1%,%2%,%3%)";
            sql::exec(boost::format(ADD_ACTIVITY_SHOW) % bs.id %this->id % bs.actor);
        }
        return p.first;
    }

    void del_show(shows_type::iterator i)
    {
        if (i != shows2.end()) {
            const char* DEL_ACTIVITY_SHOW = "DELETE FROM bar_activity_show WHERE id=%1%";
            sql::exec(boost::format(DEL_ACTIVITY_SHOW) % i->id);
            shows2.erase(i);
        }
    }

    // -1:closed, 0:open, 1:will-open
    int time_status(time_t t=0) const
    {
        if (!t)
            t = time(0);
        if (time_end <= t)
            return -1;
        if (time_begin <= t)
            return 0;
        return 1;
    }

public:
    template <typename Iter> static void load(std::string const & cond, Iter iter);

private:
    s_activity_info() {}
    friend struct global_activity;
};

template <typename Iter>
void s_activity_info::load(std::string const & cond, Iter iter)
{
    const char* get_lucky_shake_settting = "select attendees_setting,prize_setting from lucky_shake_settting where activity_id=%1%";
    const char* get_beauty_vote_settting = "select candidate_setting,winners_setting,attendees_setting from beauty_vote_settting where activity_id=%1%";
    boost::format fmt("SELECT"
        " id,type,unix_timestamp(`tm_start`) as `t0`,unix_timestamp(`tm_end`) as `t1`,foremost,"
        " activity_name,brief,post_img, show_index, screen_img, on_off, share_url, shorten_share,"
        " shorten_banner"
        " FROM bar_activity2 "
        " WHERE (DATE_SUB(CURDATE(), INTERVAL 7 DAY) <= date(tm_end)) AND %1%");
    sql::datas datas( fmt % cond );
    while (sql::datas::row_type row = datas.next()) {
        s_activity_info activity;

        activity.id = boost::lexical_cast<int>(row.at(0));
        activity.type = boost::lexical_cast<int>(row.at(1));
        activity.time_begin = boost::lexical_cast<time_t>(row.at(2,"0"));
        activity.time_end = boost::lexical_cast<time_t>(row.at(3,"0"));
        activity.foremost = boost::lexical_cast<int>(row.at(4));

        activity.activity_name = row.at(5);
        activity.brief = row.at(6,"");
        activity.post_img = complete_url(row.at(7,""));
        activity.show_index = boost::lexical_cast<int>(row.at(8,"0"));
        activity.already_end = (activity.time_end < time(0));
        activity.already_begin = (activity.time_begin < time(0));

        activity.screen_img = "";
        activity.on_off = false;

        activity_prize::load(activity.id, std::back_inserter(activity.awards) );
        if (activity.type == s_activity_info::EN_VoteBeauty) {
            bar_activity_show::load_shows(activity.id, std::back_inserter(activity.shows2));
            sql::datas datas1((boost::format(get_beauty_vote_settting) % activity.id));
            if (sql::datas::row_type row1 = datas1.next()) {
                activity.candidate_setting = boost::lexical_cast<int>(row1.at(0));
                activity.winners_setting = boost::lexical_cast<int>(row1.at(1));
                activity.attendees_setting = boost::lexical_cast<int>(row1.at(2));
                BOOST_FOREACH(const auto& p, activity.shows2) {
                    activity.joins.insert(p.voters.begin(), p.voters.end());
                }
            }
        }
        else if ( activity.type == s_activity_info::EN_LuckyShake ) {
            sql::datas datas2((boost::format(get_lucky_shake_settting)% activity.id));
            if (sql::datas::row_type row2 = datas2.next()) {
                activity.attendees_setting = boost::lexical_cast<int>(row2.at(0));
                activity.prize_setting = boost::lexical_cast<int>(row2.at(1));
                if (activity.prize_setting == 0)
                    activity.prize_setting = std::numeric_limits<int>::max();
                LOG_I << activity.id <<" "<< activity.prize_setting;
            }
        }
        else if ( activity.type == s_activity_info::EN_NormalActivity ) {
            activity.screen_img = complete_url(row.at(9,""));
            activity.on_off = boost::lexical_cast<int>(row.at(10,"0"));
        }

        activity.share_url = complete_url(row.at(11,""));
        activity.shorten_share = complete_url(row.at(12,""));
        if (activity.shorten_share.empty()) { activity.shorten_share = activity.share_url;}
        activity.shorten_banner = complete_url(row.at(13,""));
        if (activity.shorten_banner.empty()) { activity.shorten_banner = activity.post_img;}

        activity_count_load(activity.id);

        *iter++ = activity;
        LOG_I << activity;
    }
}

template <typename Outs>
Outs & operator<<(Outs & out, s_activity_info const & a)
{
    return out << a.id <<"/"<< a.activity_name <<"/"<< time_string(a.time_begin) <<"~"<< time_string(a.time_end);
}

struct TagActivityId {};
struct TagActivityCloseTime {};
struct TagActivityOpenTime {};
struct TagSessionId {};
struct TagCity {};
struct TagUserId {};

struct coupon
{
    int id;
    std::string name;
    std::string brief;
    int price;
    time_t beginDate;
    time_t endDate;
    coupon(){id=0;}
};

struct s_bar_info
{
    // indexs
    std::string sessionid;
    std::string city;
    UID trader_id,assist_id;

    client_ptr trader() const { return xindex::get(trader_id); };

    std::string zone;
    std::string barname;
    std::string address;
    double  longtitude;
    double  latitude;
    std::string introduction;
    std::string phone;
    std::string logo;
    std::string province;
    // time_t set_pos_time;
    // friend struct Position;

    // typedef std::list<s_activity_info>::iterator iterator;
    // std::list<s_activity_info> activities;
    typedef boost::multi_index_container<
        s_activity_info,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagActivityId>, BOOST_MULTI_INDEX_MEMBER(s_activity_info,int,id)
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagActivityCloseTime>, BOOST_MULTI_INDEX_MEMBER(s_activity_info,time_t,time_end)
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagActivityOpenTime>, BOOST_MULTI_INDEX_MEMBER(s_activity_info,time_t,time_begin)
            >
        >
    > activities_type;

    activities_type activities2;

    // struct ActivityCmpF
    // {
    //     explicit ActivityCmpF(time_t tp) : tp_(tp) {}
    //     time_t tp_;
    //     bool operator<(activities_type::iterator const & lhs, activities_type::iterator const & rhs) const
    //     {
    //         int ts = lhs->time_status(tp_);
    //         int ts_rhs = rhs->time_status(tp_);
    //         if (ts == ts_rhs) {
    //             return (lhs->foremost < rhs->foremost || lhs->id < rhs->id);
    //         }
    //         return ts < ts_rhs;
    //     }
    // };
    // std::vector<activities_type::iterator> activities_sorted;
    // time_t tp_sort_;

    std::set<UID> fans2;
    std::set<UID> black_list;

    struct user_inspot
    {
        UID uid;
        boost::posix_time::ptime t;
        explicit user_inspot(UID x) : uid(x), t(boost::posix_time::second_clock::local_time()) {}
    };

    boost::multi_index_container<
        user_inspot,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            // boost::multi_index::ordered_unique< boost::multi_index::identity<UID> >
            boost::multi_index::ordered_unique< BOOST_MULTI_INDEX_MEMBER(user_inspot,UID,uid) >
        >
    > inspot_users2_;

    struct bar_coupon:coupon
    {
        static const char *db_table_name() { return "coupons"; }
        static int alloc_id() { return max_index<bar_coupon>(+1); }
        int count;
        int initial_count;
        bar_coupon(){initial_count=count=0;}

        template <typename OIter> static void load(std::string const & cond, OIter it);

    };
    struct bar_advertising:advertising
    {
        template <typename OutIter> static void load(std::string const & cond, OutIter it);
    };
    std::vector<bar_coupon> bar_coupons_;
    std::vector<bar_advertising> bar_advers_;

public:
    explicit s_bar_info(const std::string& sessionId);

    static time_t timed_loop(bar_ptr thiz, time_t t);
    time_t check_open_time(time_t t);
    time_t check_close_time(time_t t);

    static bool check_employee(UID trader_id, UID uid);
    static void update_expires_time(bar_ptr);

    void delete_activity(int activity_id)
    {
        auto & idxs = boost::get<TagActivityId>(this->activities2);
        idxs.erase(activity_id);
    }
    activities_type::iterator find_activity(int activity_id)
    {
        auto & idxs = boost::get<TagActivityId>(this->activities2);
        return this->activities2.project<0>( idxs.find(activity_id) );
    }

    UInt bar_user_info_ver() const { return trader()->user_info_ver(); }
    UInt bar_user_info_ver(int) { return trader()->user_info_ver(+1); }

    int get_male() const
    {
        return count_[1];
    }

    int get_female() const
    {
        return count_[0];
    }

    int get_total() const
    {
        return count_[0] + count_[1];
    }

    double get_hot() const
    {
        int n_msg = 0;
        boost::system::error_code ec;
        if (chat_group* pcg = chatmgr::inst().chat_group(&ec, sessionid))
            n_msg = pcg->message_count();
        return 0.60* n_msg
            + 0.15 * (count_[0] + count_[1])
            + 0.15 * count_[0]
            + 0.10 * count_[1]
            ;
    }

    // void set_chat_gourp()
    // {
    //     if(NULL==pcg && !sessionid.empty()){
    //         pcg = chatmgr::inst().chat_group(NULL, sessionid); 
    //         LOG_I<<"set pcg";
    //     }
    // } 

    // std::pair<int,int> count_chats() const
    // {
    //     const_cast<s_bar_info*>(this)->set_chat_gourp();
    //     if (!pcg)
    //     {
    //         LOG_I<<"pcg hasn't been set yet";
    //         return std::pair<int,int>(0,0);
    //     }
    //     return pcg->people_count();
    // }

    void add_fans(UID uid);
    int check_fan(UID uid) const
    {
        std::set<UID>::iterator itr=fans2.find(uid);
        if (itr == fans2.end()){
            return 2;
        }
        return 1;
    }
    void delete_fans(UID uid);

    void leave_spot(client_ptr & cli);
    void enter_spot(client_ptr & cli);
    void refresh_spot_time(client_ptr & cli) { return enter_spot(cli); }

    void leave_chat(client_ptr & cli, bool leave_spot=0);
    chat_group* join_chat(client_ptr & cli);
    chat_group* join_chat(UID uid);

    struct gender_count : std::array<int,2>
    {
        void incr(client_ptr & cli)
        {
            if (cli->user_gender() == "M")
                (*this)[1]++;
            else
                (*this)[0]++;
        }
        void decr(client_ptr & cli)
        {
            if (cli->user_gender() == "M")
                (*this)[1]--;
            else
                (*this)[0]--;
        }
        gender_count() { std::fill_n(begin(), size(), 0); }
    } count_;

private:
    friend class bars_mgr;

    void update_bar_fans();

    template <typename OutIter> static void load(std::string const & cond, OutIter iter);
    template <typename Set> static void load_fans(UID trader_id, Set & fans, Set & blacks);

public:
    // statistics message

    void message_finish(imessage::message const & m, UID uid);
    static void recover_in_spot_users(bar_ptr pbar);
};
typedef boost::shared_ptr<s_bar_info> bar_ptr;

template <typename Outs> Outs & operator<<(Outs & out, s_bar_info const & a)
{
    return out << a.sessionid;
}

template <typename Set>
void s_bar_info::load_fans(UID trader_id, Set & fans, Set & blacks)
{
    boost::format fmt("SELECT UserId,OtherId,relation"
            " FROM contacts WHERE UserId='%1%' OR OtherId='%1%'");
    sql::datas datas( fmt % trader_id );
    while (sql::datas::row_type row = datas.next()) {
        UID uid = boost::lexical_cast<UID>(row.at(0));
        UID oid = boost::lexical_cast<UID>(row.at(1));
        int rel = boost::lexical_cast<int>(row.at(2));
        if (uid == trader_id) {
            if (rel == 0 || rel == 2)
                blacks.insert(oid);
        } else {
            if (rel == 1)
                fans.insert(uid);
        }
    }
}

template <typename OutIter>
void s_bar_info::bar_coupon::load(std::string const & cond, OutIter it)
{
    boost::format fmt("SELECT id,name,price,count,initial_count,brief,unix_timestamp(beginDate),unix_timestamp(endDate) FROM coupons WHERE %1%");
    sql::datas datas(fmt % cond);
    while (sql::datas::row_type row = datas.next())
    {
        bar_coupon coupon;
        coupon.id = boost::lexical_cast<unsigned int>(row.at(0));
        coupon.name = row.at(1,"");
        coupon.price = boost::lexical_cast<int>(row.at(2));
        coupon.count = boost::lexical_cast<int>(row.at(3));
        if ( 0 == coupon.count ) { continue; }

        coupon.initial_count = boost::lexical_cast<int>(row.at(4));
        coupon.brief= row.at(5,"");
        coupon.beginDate = boost::lexical_cast<time_t>(row.at(6));
        coupon.endDate = boost::lexical_cast<time_t>(row.at(7));
        if ( coupon.count > 0 ) {
            *it++ = coupon;
        }
    }
}

template <typename OutIter>
void s_bar_info::load(std::string const & cond, OutIter iter)
{
    // if (!cond.empty())
    // {
    //     LOG_I << cond;
    //     return;
    // }
    boost::format fmt("SELECT"
        " city,zone,BarName,address,longtitude,latitude,introduction,phone,logo,SessionId,province,trader_id,assist_id"
        " FROM bars %1%");
    sql::datas datas(fmt % cond);
    while (sql::datas::row_type row = datas.next()) {
        std::string sessionid=row.at(9,"");
        if (!sessionid.empty()) {
            boost::shared_ptr<s_bar_info> tmp(new s_bar_info(sessionid));
            tmp->city = row.at(0,"");
            tmp->zone = row.at(1,"");
            tmp->barname = row.at(2,"");
            tmp->address = row.at(3,"");
            tmp->longtitude = boost::lexical_cast<double>(row.at(4,"0"));
            tmp->latitude = boost::lexical_cast<double>(row.at(5,"0"));
            tmp->introduction = row.at(6,"");
            tmp->phone = row.at(7,"");
            tmp->logo = complete_url(row.at(8,""));
            // 9 - sessionid
            tmp->province = row.at(10,"");
            tmp->trader_id = boost::lexical_cast<UID>(row.at(11,"0"));
            tmp->assist_id = boost::lexical_cast<UID>(row.at(12,"0"));
            {
                UID uid = tmp->trader_id;
                if (uid == 0)
                {
                    LOG_I << uid;
                    continue;
                }
            }

            s_bar_info::load_fans(tmp->trader_id, tmp->fans2, tmp->black_list);
            s_activity_info::load(str(boost::format("UserId='%1%' order by show_index desc") %tmp->trader_id)
                    , std::back_inserter(tmp->activities2));
            s_bar_info::bar_coupon::load(str(boost::format("SessionId='%1%' and enable=1") % tmp->sessionid)
                    , std::back_inserter(tmp->bar_coupons_));
            bar_advertising::load(str(boost::format("UserId='%1%'") % tmp->trader_id)
                    , std::back_inserter(tmp->bar_advers_));

            *iter++ = tmp;
        }
    }
}

template <typename OutIter>
void s_bar_info::bar_advertising::load(std::string const & cond, OutIter it)
{
    boost::format fmt("SELECT id,name,A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2,unix_timestamp(beginDate),unix_timestamp(endDate) FROM advertising WHERE %1%");
    sql::datas datas(fmt % cond);
    while (sql::datas::row_type row = datas.next())
    {
        bar_advertising a;
        a.id = boost::lexical_cast<unsigned int>(row.at(0));
        a.name = row.at(1);
        a.A_image1 = complete_url(row.at(2));
        a.A_size1 = row.at(3);
        a.A_image2 = complete_url(row.at(4));
        a.A_size2 = row.at(5);
        a.I_image1 = complete_url(row.at(6));
        a.I_size1 = row.at(7);
        a.I_image2 = complete_url(row.at(8));
        a.I_size2 = row.at(9);
        a.beginDate = boost::lexical_cast<time_t>(row.at(10));
        a.endDate = boost::lexical_cast<time_t>(row.at(11));
        *it++ = a;
    }
}

struct Position
{
    Position(double a, double b):lat1(a),lng1(b)
    {}

    double get_distance(s_bar_info &place) const
    {
        return GetDistance(lat1, lng1, place.latitude, place.longtitude);
    }
    bool operator ()(boost::shared_ptr<s_bar_info> a, boost::shared_ptr<s_bar_info> b) const
    {
        return get_distance(*a) < get_distance(*b);
    }
    double lat1,lng1;
};

class bars_mgr : boost::noncopyable
{
public:
    static bars_mgr& inst(); //(bars_mgr* initial = 0); // { return *g_bars_mgr_; }

    void get_bars_bycity(std::string const &city, std::vector<bar_ptr>& barlist) const;

    void delete_bar(std::string const & sessionid);
    void add_bar(bar_ptr pbar);

    bar_ptr get_session_bar(std::string const & sessionId, boost::system::error_code* pec=0) const;
    bar_ptr get_session_bar(std::string const & sessionId, boost::system::error_code& ec) const
        { return get_session_bar(sessionId, &ec); }

    bar_ptr get_bar_byuid(UID userid, boost::system::error_code * pec=0) const;
    bar_ptr get_bar_byuid(UID userid, boost::system::error_code& ec) const
        { return get_bar_byuid(userid, &ec); }

    std::vector<bar_ptr>& get_coupon_bar_list(){ return coupon_bar_list;}

    inline bool has(UID uid) const
    {
        auto & idxs = boost::get<TagUserId>(bars_);
        return idxs.find(uid) != idxs.end();
    }

    bar_ptr find_nearest_bar(double latitude, double longtitude, int devtype, UID uid);

    // singleton
    explicit bars_mgr(boost::asio::io_service & io_s);
private:
    std::vector<bar_ptr> coupon_bar_list;

    typedef boost::multi_index_container<
        bar_ptr,
        boost::multi_index::indexed_by<
            // boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagSessionId>, BOOST_MULTI_INDEX_MEMBER(s_bar_info,std::string,sessionid)
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagCity>, BOOST_MULTI_INDEX_MEMBER(s_bar_info,std::string,city)
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagUserId>, BOOST_MULTI_INDEX_MEMBER(s_bar_info,UID,trader_id)
            >
        >
    > bars_type;
    bars_type bars_;
    std::map<std::string, bool> city_load_flag;

    static void post_init_bars();
public:
    struct activity_manager
    {
        explicit activity_manager(boost::asio::io_service & io_s)
            : timer_(io_s)
        {}

        template <typename Fn>
        void wait(UInt tag, time_t tpx, Fn const & fn)
        {
            if (tpx == std::numeric_limits<time_t>::max())
                return;
            erase_bytag(tag);

            time_t tp0 = timed_list_.empty()
                ? std::numeric_limits<time_t>::max()
                : timed_list_.begin()->first;

            timed_list_.insert(std::make_pair(tpx, Func(tag,fn)));

            time_t tpcur = time(0);
            LOG_I << tpx <<" "<< tp0 <<" in-seconds "<< tpx - tpcur;
            if (tpx < tp0) {
                if (tpx < tpcur)
                    tpx = tpcur;
                timer_.expires_from_now(boost::posix_time::seconds(tpx - tpcur));
                timer_.async_wait(boost::bind(&activity_manager::timer_fn, this, boost::asio::placeholders::error));
            }
        }

        void erase_bytag(UInt tag)
        {
            for (auto it = timed_list_.begin(); it != timed_list_.end(); ++it)
            {
                if (it->second.tag == tag)
                {
                    timed_list_.erase(it);
                    return;
                }
            }
        }

        boost::asio::io_service & get_io_service() { return timer_.get_io_service(); }

    private:
        void timer_fn(const boost::system::error_code & ec);

        typedef boost::function<time_t (time_t)> fn_type;
        struct Func : fn_type
        {
            Func(UInt x, fn_type fn) : fn_type(fn) { tag=x; }
            UInt tag;
        };
        std::multimap<time_t, Func> timed_list_;
        boost::asio::deadline_timer timer_;
    };

    activity_manager activity_mgr;

    boost::asio::io_service & get_io_service() { return activity_mgr.get_io_service(); }

public:
    void load_bars(std::string const & cond);
};

struct global_advertising:advertising
{
    std::string city,zone;
    template <typename OutIter> static void load(std::string const & cond, OutIter it);
    std::vector<std::string> SessionIds;
};

template <typename OutIter>
void global_advertising::load(std::string const & cond, OutIter it)
{
    boost::format fmt("SELECT id,name,A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2,unix_timestamp(beginDate),unix_timestamp(endDate),city,zone,SessionIds FROM advertising WHERE %1%");
    sql::datas datas(fmt % cond);
    while (sql::datas::row_type row = datas.next())
    {
        global_advertising a;
        a.id = boost::lexical_cast<unsigned int>(row.at(0));
        a.name = row.at(1,"");
        a.A_image1 = complete_url(row.at(2));
        a.A_size1 = row.at(3);
        a.A_image2 = complete_url(row.at(4));
        a.A_size2 = row.at(5);
        a.I_image1 = complete_url(row.at(6));
        a.I_size1 = row.at(7);
        a.I_image2 = complete_url(row.at(8));
        a.I_size2 = row.at(9);
        a.beginDate = boost::lexical_cast<time_t>(row.at(10));
        a.endDate = boost::lexical_cast<time_t>(row.at(11));
        a.city = row.at(12);
        a.zone = row.at(13);
        if ( row.at(14) ) {
            std::istringstream ins( row.at(14) );
            std::istream_iterator<std::string> i(ins), end;
            std::copy(i, end, std::back_inserter(a.SessionIds));
        }
        *it++ = a;
    }
}

struct g_activity_prize
{
    unsigned int id;
    double probability;
    std::string name;
    std::string img;
    std::string brief;
    g_activity_prize() { id = 0; }

    template <typename OutIter> static void load(int activity_id, OutIter it);
};

struct global_activity:s_activity_info
{
    std::string city,zone;
    template <typename OutIter> static void load(std::string const & cond, OutIter it);
    std::vector<std::string> SessionIds;
    std::vector<g_activity_prize> g_awards;
};

template <typename OutIter>
void g_activity_prize::load(int activity_id, OutIter it)
{
    boost::format fmt("SELECT id, probability, name, img,brief "
            " FROM g_activity_prize WHERE activity_id=%1% order by probability");

    sql::datas datas(fmt % activity_id);
    while (sql::datas::row_type row = datas.next()) {
        g_activity_prize a;
        a.id = boost::lexical_cast<unsigned int>(row.at(0));
        a.probability = boost::lexical_cast<double>(row.at(1));
        a.name = row.at(2,"");
        a.img = complete_url(row.at(3,""));
        a.brief = row.at(4,"");
        *it++ = a;
    }
}

template <typename Iter>
void global_activity::load(std::string const & cond, Iter iter)
{
    // const char* get_lucky_shake_settting = "select attendees_setting,prize_setting from lucky_shake_settting where activity_id=%1%";
    const char* get_beauty_vote_settting = "select candidate_setting,winners_setting,attendees_setting from beauty_vote_settting where activity_id=%1%";
    boost::format fmt("SELECT"
        " id,type,unix_timestamp(`tm_start`) as `t0`,unix_timestamp(`tm_end`) as `t1`,foremost"
        ",activity_name,brief,post_img,show_index, city, zone, SessionIds, screen_img, on_off"
        " FROM bar_activity2 WHERE (DATE_SUB(CURDATE(), INTERVAL 7 DAY) <= date(tm_end)) AND %1%");
    sql::datas datas( fmt % cond );
    while (sql::datas::row_type row = datas.next()) {
        global_activity activity;

        activity.id = boost::lexical_cast<int>(row.at(0));
        activity.type = boost::lexical_cast<int>(row.at(1));
        activity.time_begin = boost::lexical_cast<time_t>(row.at(2,"0"));
        activity.time_end = boost::lexical_cast<time_t>(row.at(3,"0"));
        activity.foremost = boost::lexical_cast<int>(row.at(4));

        activity.activity_name = row.at(5);
        activity.brief = row.at(6,"");
        activity.post_img = complete_url(row.at(7,""));
        activity.show_index = boost::lexical_cast<int>(row.at(8,"0"));
        activity.already_end = (activity.time_end < time(0));
        activity.already_begin = (activity.time_begin < time(0));

        activity.screen_img = "";
        activity.on_off = false;

        activity_prize::load(activity.id, std::back_inserter(activity.awards) );
        if (activity.type == s_activity_info::EN_VoteBeauty) {
            bar_activity_show::load_shows(activity.id, std::back_inserter(activity.shows2));
            sql::datas datas1((boost::format(get_beauty_vote_settting) % activity.id));
            if (sql::datas::row_type row1 = datas1.next()) {
                activity.candidate_setting = boost::lexical_cast<int>(row1.at(0));
                activity.winners_setting = boost::lexical_cast<int>(row1.at(1));
                activity.attendees_setting = boost::lexical_cast<int>(row1.at(2));
            }
        }
        else if ( activity.type == s_activity_info::EN_LuckyShake ) {
            // sql::datas datas2((boost::format(get_lucky_shake_settting)% activity.id));
            // if (sql::datas::row_type row2 = datas2.next()) {
            //     activity.attendees_setting = boost::lexical_cast<int>(row2.at(0));
            //     activity.prize_setting = boost::lexical_cast<int>(row2.at(1));
            // }
            g_activity_prize::load(activity.id, std::back_inserter(activity.g_awards) );
        }
        else if ( activity.type == s_activity_info::EN_NormalActivity ) {
            activity.screen_img = complete_url(row.at(12,""));
            activity.on_off = boost::lexical_cast<int>(row.at(13,"0"));
        }

        activity.city = row.at(9);
        activity.zone = row.at(10);
        if ( row.at(11,NULL) ) {
            std::istringstream ins( row.at(11) );
            std::istream_iterator<std::string> i(ins), end;
            std::copy(i, end, std::back_inserter(activity.SessionIds));
        }

        *iter++ = activity;
        LOG_I << activity;
    }
}

struct TagGadverId {};
struct TagGadverCity {};
struct TagGactivityId {};
struct TagGactivityCity {};

struct global_mgr
{
    static global_mgr& inst();

    typedef boost::multi_index_container<
        global_advertising,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<TagGadverId>, BOOST_MULTI_INDEX_MEMBER(advertising,int,id)
            >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagGadverCity>, BOOST_MULTI_INDEX_MEMBER(global_advertising, std::string, city)
            >
            >
            > global_adver_type;

    typedef boost::multi_index_container<
        global_activity,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<TagGactivityId>, BOOST_MULTI_INDEX_MEMBER(s_activity_info,int,id)
            >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<TagGactivityCity>, BOOST_MULTI_INDEX_MEMBER(global_activity, std::string, city)
            >
            >
            > global_activity_type;
    global_adver_type g_advers_;
    global_activity_type g_activities_;

    global_adver_type::iterator find_g_advert(int advertisingid, boost::system::error_code& ec)
    {
        auto & idxs = boost::get<TagGadverId>(g_advers_);
        global_adver_type::iterator it = g_advers_.project<0>( idxs.find(advertisingid) );
        if ( it == g_advers_.end() ) {
            ec = boost::system::error_code(EN_Advertising_NotFound, bar_error_category::inst<bar_error_category>());
        }
        return it;
    }

    void erase_g_advert(int advertisingid)
    {
        auto & idxs = boost::get<TagGadverId>(g_advers_);
        idxs.erase(advertisingid);
    }
    void load_adver(const std::string& cond)
    {
        global_advertising::load(cond, std::back_inserter(g_advers_));
    }

    global_activity_type::iterator find_g_activity(int activity_id, boost::system::error_code& ec)
    {
        auto & idxs = boost::get<TagGactivityId>(g_activities_);
        global_activity_type::iterator it = g_activities_.project<0>( idxs.find(activity_id) );
        if ( it == g_activities_.end() ) {
            ec = boost::system::error_code(EN_Activity_NotFound, bar_error_category::inst<bar_error_category>());
        }
        return it;
    }

    void erase_g_activity(int activity_id)
    {
        auto & idxs = boost::get<TagGactivityId>(g_activities_);
        idxs.erase(activity_id);
    }
    void load_activity(const std::string& cond)
    {
        global_activity::load(cond, std::back_inserter(g_activities_));
    }

private:
    void load_all()
    {
        global_advertising::load("city is not NULL", std::back_inserter(g_advers_));
        global_activity::load("city is not NULL", std::back_inserter(g_activities_));
    }

    global_mgr(){ load_all();};
};

//inline bool total_compare(boost::shared_ptr<s_bar_info> a, boost::shared_ptr<s_bar_info> b)
//{
//    return a->get_total() > b->get_total();
//}

struct hot_compare {
    bool operator()(bar_ptr a, bar_ptr b) const
    {
        return a->get_hot() > b->get_hot();
    }
};

struct program
{
    void json_to_vector(const json::array actors)
    {
        for (json::array::const_iterator i=actors.begin(); i!=actors.end(); ++i)
        {
            actors_.push_back(boost::get<int>(*i));
        }
    }
    json::array vector_to_json()
    {
        json::array arr;
        for(std::vector<UID>::iterator itr=actors_.begin();
                itr != actors_.end(); ++itr)
        {
            arr(*itr);
        }
        return arr;
    }

    int id_;
    int type_; 
    std::string show_name_;
    std::string SessionId_; 
    std::string detail_; 
    time_t begin_;
    time_t end_;
    std::vector<UID> actors_;
};
class program_mgr
{
    public:
        typedef std::map<std::string,std::vector<program>>::iterator program_mgr_iterator;
        static program_mgr& instance(){static program_mgr inst; return inst;}
        std::vector<program> get_porgrams(const std::string& SessionId);
    private:
        program_mgr_iterator load(const std::string& SessionId);
        std::map<std::string, std::vector<program> > session_program_list;
};

#endif

