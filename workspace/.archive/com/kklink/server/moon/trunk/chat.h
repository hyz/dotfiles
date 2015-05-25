#ifndef CHAT_H__
#define CHAT_H__

#include <vector>
#include <set>
#include <string>

#include "proto_im.h"
#include "client.h"
// #include "bars.h"

struct chat_group //: boost::noncopyable
{
    enum join_attr
    {
        rdonly = 0x00,
        rdwr = 0x01,
        qrcode = 0x10,
    };

    typedef std::set<UID>::iterator iterator;
    typedef std::set<UID>::const_iterator const_iterator;

    bool remove(UID uid, client_ptr & cli);
    bool remove(UID uid) { return remove(uid, sysadmin_client_ptr()); }

    void add_quiet(client_ptr & cli)
    {
        members_.insert(cli->user_id());
    }

    void add_guest(client_ptr & cli)
    {
        members_.insert(cli->user_id());
    }

    template <typename I> void new_members(I beg, I end, bool msg_excl, bool qrcode, client_ptr & cli);

    void add_from_qr_code(client_ptr & cli)
    {
        UID uid = cli->user_id();
        return new_members(&uid, &uid+1, true, true, sysadmin_client_ptr());
    }

    void add_msg_excl(client_ptr & cli)
    {
        UID uid = cli->user_id();
        return new_members(&uid, &uid+1, true, false, sysadmin_client_ptr());
    }

    template <typename I> void add(I beg, I end, client_ptr & cli)
    {
        return new_members(beg, end, false, false, cli);
    }

    template <typename I, typename OIter> void filter_clients(I ubeg, I uend, OIter oit);

    // simplified version of add:insert
    template <typename I> void join(I beg, I end)
        { return new_members(beg, end, false, false, sysadmin_client_ptr()); }
    void join(UID uid) { return join(&uid, &uid+1); }

    void rename(const std::string & name, client_ptr & cli);

    bool is_alive_member(UID uid) const
    {
        return uid==SYSADMIN_UID
            || alive_members_.find(uid) != alive_members_.end();
    }

    void send(const imessage::message & msg, client_ptr & cli, std::vector<UID>filter=std::vector<UID>());

    iterator begin() const { return const_cast<chat_group*>(this)->members_.begin(); }
    iterator end() const { return const_cast<chat_group*>(this)->members_.end(); }

public:
    chat_group(const std::string & id, const std::string & name/*, UID ctor*/);

public:
    const groupid_type& id() const { return gid_; }
    const std::string& name() const { return name_; }

    const std::set<UID>& alive_members() const { return alive_members_; }

    std::list<imessage::message> const & messages() const { return messages_; }

    std::string logo() const;

    //int count_male() const {
    //    return male_count_;
    //}

    //int count_female() const {
    //    return female_count_;
    //}

    // std::pair<int,int> people_count() const {
    //     LOG_I << female_count_ <<" "<< male_count_ <<" "<< visitor_count_ <<" "<< gid_;
    //     return std::make_pair(female_count_, male_count_);
    // }

    int message_count() const { return message_count_; };

private:
    groupid_type gid_;
    std::string name_;
    // UID creator_;
    std::set<UID> alive_members_;
    std::set<UID> members_;

    std::list<imessage::message> messages_;

    friend struct chatmgr;

    int message_count_;
    // int visitor_count_, male_count_, female_count_;

    chat_group(const std::string & id, const std::string & name, const std::vector<UID>& membs);

    void member_required(client_ptr & cli);
    void save_new_member(groupid_type const & gid, UID uid);

private:
    chat_group & operator=(chat_group const &);
};

template <typename Group, typename Pred>
void sendto(const imessage::message & msg, Group const & group, Pred const & pred)
{
    if (boost::empty(group))
        return;

    boost::format fmt("INSERT INTO messages(id,SessionId,UserId,PeerId,MsgType,content)"
            " VALUES(%1%,'%2%',%3%,%4%,'%5%','%6%')");
    sql::exec(fmt % msg.id() % msg.gid % msg.from % msg.a % msg.type % sql::db_escape(json::encode(msg.body)));

    std::vector<UID> dv;
    BOOST_FOREACH(auto const & a , group)
    {
        client_ptr c = xindex::get(a);
        if (!c)
            continue;
        if (!pred(c))
            continue;
        if (msg.gid == ".")
        {
            imessage::message tmp( msg );
            tmp.gid = make_p2pchat_id(msg.from, a);
            Client::pushmsg(c, tmp);
        }
        else
        {
            Client::pushmsg(c, msg);
        }
        dv.push_back(a);
    }

    LOG_I << msg.id() <<"/"<< msg.type << "/" << msg.from <<"/"<< msg.gid <<": " << dv;
}

template <typename T> struct refset_helper {
    template <typename X> bool operator()(X const& x) const { return set.find(x) != set.end(); }
    T const & set;
    refset_helper(T const& s) : set(s) {}
};
template <typename T> refset_helper<T> is_member_of(T const& s) { return refset_helper<T>(s); }

struct true_fn { template <typename ...A> bool operator()(A...) const { return true; } };

template <typename Group>
void sendto(const imessage::message & msg, Group const & group)
{
    return sendto(msg, group, true_fn());
}

void sendto(const imessage::message & msg, client_ptr & cli);

template <typename Iter, typename OI>
void map_uid_client(Iter it, Iter end, OI oit)
{
    for ( ; it != end; ++it)
    {
        if (client_ptr c = xindex::get(*it))
        {
            *oit++ = c;
        }
    }
}

template <typename I, typename OIter>
void chat_group::filter_clients(I ubeg, I uend, OIter oit)
{
    std::set<UID> uids;
    for (auto i = ubeg; i != uend; ++i)
    {
        if (!is_normal_uid(*i)
                || alive_members_.find(*i) != alive_members_.end())
            continue;

        client_ptr c = xindex::get(*i);
        if (!c)
            continue;
        if (c->is_guest())
        {
            members_.insert(*i);
            continue;
        }

        if (uids.insert(*i).second)
            *oit++ = c;
    }
}

template <typename I>
void chat_group::new_members(I ubeg, I uend, bool msg_excl_adds, bool qrcode, client_ptr & cli)
{
    member_required(cli);
    LOG_I << cli <<" "<< gid_; // << " add " << uids;

    std::vector<client_ptr> cv;
    filter_clients(ubeg, uend, std::back_inserter(cv));

    if (cv.empty())
        return;

    json::array membs;
    std::set<UID> uids;

    BOOST_FOREACH(auto & c, cv)
    {
        BOOST_ASSERT(!c->is_guest());
        if (is_groupchat(gid_))
            save_new_member(gid_, c->user_id());

        json::object user = c->brief_user_info();
        //if ("M" == user.get<std::string>("gender","")) {
        //    ++male_count_;
        //} else {
        //    ++female_count_;
        //}
        //++visitor_count_;

        membs.push_back(user);
        uids.insert(c->user_id());
    }

    if (uids.empty())
        return;

    imessage::message msg("chat/join", cli->user_id(), gid_);
    msg.body
        ("gname", name_)
        ("content", membs)
        ;
    if (qrcode)
        msg.body.put("qrcode", qrcode);

    if (msg_excl_adds)
    {
        std::vector<UID> res;
        std::set_difference(members_.begin(), members_.end()
                , uids.begin(), uids.end()
                , std::back_inserter(res));
        sendto(msg, res);

        members_.insert(uids.begin(), uids.end());
        alive_members_.insert(uids.begin(), uids.end());
    }
    else
    {
        members_.insert(uids.begin(), uids.end());
        alive_members_.insert(uids.begin(), uids.end());

        sendto(msg, members_);
    }
    LOG_I << cli <<" "<< gid_ <<" alive_members_:"<< alive_members_<<" members_:"<< members_ ;
}

struct chatmgr : private std::map<groupid_type,chat_group>
{
    static chatmgr & inst();

    template <typename Rng>
    ::chat_group & create_chat_group(UID ctor, const groupid_type& gid, const std::string& name, Rng const & rng);
    ::chat_group & create_chat_group(UID ctor, const groupid_type& gid, const std::string& name)
            { return create_chat_group(ctor, gid, name, std::vector<UID>()); }

    ::chat_group * chat_group(boost::system::error_code * ec, const groupid_type & gid);
    ::chat_group & chat_group(const groupid_type & gid)
            { return *chat_group(0, gid); }

    void send(const imessage::message & msg, client_ptr & cli, std::vector<UID>filter=std::vector<UID>());
    void remove_group(const groupid_type& gid){
        iterator i = find(gid);
        if(i != end()) erase(i);
    }

    void testmsg(UID uid);

private:
    iterator load(const groupid_type& gid);

public:
    struct initializer : boost::noncopyable
    {
        initializer();
        ~initializer();
    };
    friend struct initializer;
};

enum {
    EN_Group_NotFound = 315,
    EN_Group_Exist,
    EN_Group_Invalid,
    EN_NotMember,
};

struct Chat_Error_Category : ::myerror_category
{
    Chat_Error_Category();
};

template <typename R>
json::array json_array_brief_user_info(R const & rng)
{
    json::array a;
    BOOST_FOREACH(auto & c, rng)
        a.put(c->brief_user_info());
    return a;
}

template <typename Rng>
::chat_group & chatmgr::create_chat_group(UID ctor, const groupid_type & gid, const std::string& name, Rng const & rng)
{
    std::pair<iterator,bool> res = insert( make_pair(gid, ::chat_group(gid, name)) );
    if (!res.second)
        MYTHROW(EN_Group_Exist, Chat_Error_Category);

    boost::format fmt("INSERT INTO sessions(CreatorId,SessionId,SessionName) VALUES(%1%,'%2%','%3%')");
    sql::exec(fmt % ctor % gid % sql::db_escape(name));

    ::chat_group & cg = res.first->second;
    cg.save_new_member(gid, ctor); // save db

    if (!boost::empty(rng))
    {
        std::vector<client_ptr> cl;

        BOOST_FOREACH(auto uid, rng)
        {
            if (client_ptr c = xindex::get(uid))
            {
                cg.save_new_member(gid, uid); // save db
                cl.push_back(c);
            }
        }
        
        imessage::message msg("chat/join", ctor, gid);
        msg.body
            ("gname", cg.name_)
            ("content", json_array_brief_user_info(cl))
            ;

        cg.alive_members_.insert(boost::begin(rng), boost::end(rng));
        cg.members_.insert(boost::begin(rng), boost::end(rng));

        sendto(msg, cg.members_);
    }

    cg.alive_members_.insert(ctor);
    cg.members_.insert(ctor);

    return cg;
}

inline void store_msg( const imessage::message& msg )
{
    boost::format fmt("INSERT INTO messages(id,SessionId,UserId,PeerId,MsgType,content)"
            " VALUES(%1%,'%2%',%3%,%4%,'%5%','%6%')");
    sql::exec(fmt % msg.id() % msg.gid % msg.from % msg.a % msg.type % sql::db_escape(json::encode(msg.body)));
}

template <typename Iter>
inline void load_msg(std::string const & cond, Iter it )
{
    boost::format SELECT_MESSAGES("SELECT id,UserId,MsgType,content,UNIX_TIMESTAMP(MsgTime),"
            " SessionId FROM messages %1%");
    sql::datas datas(SELECT_MESSAGES % cond);
    while (sql::datas::row_type row = datas.next()) {
        time_t utime = boost::lexical_cast<time_t>(row.at(4));
        unsigned int id = boost::lexical_cast<unsigned int>(row.at(0));
        UID uid = boost::lexical_cast<UID>(row.at(1));
        char const * msgtype = row.at(2);
        char const * cont = row.at(3);
        char const * gid = row.at(5);

        imessage::message m(id, msgtype, uid, gid, utime);
        m.body = json::decode(cont);

        *it=m;
        ++it;
    }
}

#endif

