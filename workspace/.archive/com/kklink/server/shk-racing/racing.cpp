#include "log.h"
#include "server.h"
#include "racing.h"
#include "dbc.h"

////////////////////////// //////////////////////////
//
//

inline std::size_t hash_value(game_list::iterator const& it)
{
    return static_cast<std::size_t>(it.operator->() - static_cast<gamebs*>(0));
}
inline bool operator<(game_list::iterator const& lhs, game_list::iterator const& rhs)
{
    return (lhs.operator->()) < (rhs.operator->());
}

Int64 milliseconds()
{
    static time_t tpb_ = time(0);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return Int64(ts.tv_sec - tpb_) * 1000 + ts.tv_nsec / 1000000;
}

// namespace game {}
    struct verify_fail : myerror { using myerror::myerror; };
    struct bad_message : myerror { using myerror::myerror; };

    struct item_notfound : myerror { using myerror::myerror; };

    struct player_notfound : myerror { using myerror::myerror; };
    struct player_alreadyexist : myerror { using myerror::myerror; };

    struct game_notfound : myerror { using myerror::myerror; };
    struct game_alreadyexist : myerror { using myerror::myerror; };

    // typedef boost::error_info<size_t,size_t> error_id;
    //typedef boost::error_info<struct gamebs,size_t> errinfo_gid;
    //typedef boost::error_info<struct player,size_t> errinfo_pid;
    typedef boost::error_info<struct info,std::string> errinfo_str;
// {}

template <typename T>
boost::error_info<T,T> error_id(T const& x) { return boost::error_info<T,T>(x); }

template <typename Tag, typename Midx, typename K>
typename Midx::iterator myfind(Midx& midx, K const& xk)
{
    auto& idx = boost::get<Tag>(midx);
    auto it = idx.find(xk);
    if (it == idx.end()) {
        return midx.end();
    }
    return multi_index::project<0>(midx,it);
}

template <typename Tag, typename Midx, typename K>
typename Midx::iterator itref(Midx& midx, K const& xk)
{
    auto it = myfind<Tag>(midx,xk);
    if (it == midx.end()) {
        LOG << xk;
        BOOST_THROW_EXCEPTION( item_notfound() << error_id(xk) );
    }
    return it;
}

json::object http_user_verify(std::string const& url, UInt uid, UInt gid, std::string const& tok);
json::object http_game_verify(std::string const& url, UInt gid, std::string const& tok);
void http_send_message(std::string const& url, UInt gid, boost::array<UInt,3> const& av, UInt);

player::player(game_list::iterator it, UInt uid
        , std::string const& tok
        , char gender, std::string const& nick, std::string const& photo)
    : token_(tok), spxs_{0,0,0}, nick_(nick), photo_(photo)
{
    git_ = it;
    uid_ = uid;
    gender_ = gender;
    // pos_ = 0;

    tp_spx_ = 0;
    speeds_ix_ = 0;
    // std::fill(spxs_.begin(), spxs_.end(), 0);
}

UInt player::speed() const
{
    Int64 tpc = milliseconds();
    if (tpc - tp_spx_ < 1500) {
        return last_speed(); //spxs_[Ix(-1)];
    }
    if (tpc - tp_spx_ < 2500) {
        return last_speed() /2; //spxs_[Ix(-1)] / 2;
    }
    if (tpc - tp_spx_ < 3500) {
        return last_speed() /4;
    }
    return 0;
}

void player::speed_up(uint8_t spx)
{
    if (spx == 0) {
        return;
    }
    spxs_[speeds_ix_++ % spxs_.size()] = spx;
    tp_spx_ = milliseconds();
}

static std::string json_player(player const& p)
{
    char tmps[2] = {0,0};
    tmps[0] = p.gender_;

    json::object obj;
    json::insert(obj)
        ("uid", p.uid_)
        ("photo", p.photo_)
        ("nick", p.nick_)
        ("gender", &tmps[0])
        ("speed", p.speed())
        ;
    return json::encode(obj);
}


#include "json.h"

struct Mplayer : boost::noncopyable
{
    enum { Msg_sign=61, Msg_sign2, Msg_speedx=63 };
    Main& m_;

    Mplayer(Main& m, UInt uid) : m_(m) {}

    player_list::iterator sign_in(io_context& sk, json::object const& jin, int opx);
    void speedx(io_context& sk, player_list::iterator, json::object const& jin);

    void disconnect(io_context& sk, player_list::iterator it)
    {
        LOG << it->uid_;
        game_list::iterator git = it->git();
        // const_cast<gamebs&>(*git).exit(it->uid_);
    }

    player_list::iterator ref(UInt uid)
    {
        if (!uid) {
            BOOST_THROW_EXCEPTION( player_notfound() << error_id(0) );
        }

        player_list::iterator it = myfind<TagX>(m_.players_,uid);
        if (it == m_.players_.end()) {
            BOOST_THROW_EXCEPTION( player_notfound() << error_id(uid) );
        }
        return it;
    }
};

struct Mgamebs : boost::noncopyable
{
    enum { Msg_connect=51, Msg_connect2, Msg_end=55 };
    enum { Msg_newuser=56, Msg_speeds=57 };
    // typedef std::pair<gamebs, std::vector<player> > result_t;

    Main& m_;

    Mgamebs(Main& m) : m_(m) {}

    void connect(io_context& sk, json::object const& jin, int opx);
    void End(io_context& sk, game_list::iterator git, json::object const& jin);
    void disconnect(io_context& sk, game_list::iterator git);

public:
    UInt next_loop_tpx(Int64 tp_cur);
    void sync_res(game_list::iterator git, io_context& sk);
    void* loop(Int64 tp_cur);
};

void gamebs::reset(boost::array<UInt,3>* ends)
{
    if (gid_ > 0) {
        char rk[96];
        snprintf(rk,sizeof(rk), "shk/%u/%lu", gid_, tps_start_);
        if (ends) {
            char rk_end[96];
            snprintf(rk_end,sizeof(rk_end), "shk/%u", gid_);
            auto& v = *ends;
            char result[256];
            snprintf(result,sizeof(result), "%lu\t%d\t%u\t%u\t%u", tps_start_, n_player_, v[0], v[1], v[2]);
            redis::context redx;
            redx.append("LPUSH", rk_end, result)
              .append("LTRIM", rk_end, 0, 99)
              .append("EXPIRE", rk_end, (60*60*24*30))
              .append("EXPIRE", rk, (60*60*24*15))
              ;
        } else {
            redis::command("EXPIRE", rk, (60*60*24*15)) ;
        }
    }

    tp_sync_ = 0;
    tps_start_ = time(0); //milliseconds();
    ingames_.clear();
    resp_.clear();

    // uspx_.clear();
    n_player_ = 0;
}

void gamebs::join(UInt uid, int opx, std::string const& data)
{
    if (ingames_.insert(uid).second) {
        resp_.push_back( std::make_pair(opx, data) );
        LOG << uid;
    }
}

void gamebs::count_player(int n)
{
    n_player_ = n;
    //if (n > n_player_)
    {
        //struct tm tm; localtime_r(&tps_start_, &tm);
        //char tmp[64]; strftime(tmp,sizeof(tmp), "%Y%m%d/%H%M%S", &tm);
        // std::string hk = "shk/" + std::to_string(gid_);
        // redis::context redx;
        // redx.append("HSET", hk, tps_start_, n_player_)
        //     .append("EXPIRE", hk, (60*60*24*7))
        //     ;
    }
}

UInt Mgamebs::next_loop_tpx(Int64 tp_cur)
{
    if (!tp_cur) {
        tp_cur = milliseconds();
    }
    auto & games = boost::get<TagS>(m_.games_online_);
    if (games.empty()) {
        return 1000;
    }
    auto g = games.front();

    UInt ms = UInt(tp_cur - g->tp_sync_);
    if (ms >= 1000)
    {
        if (ms > 3000/* && g->tp_sync_*/) {
            LOG << ms << tp_cur << g->tp_sync_;
        }
        return 0;
    }
    return (1000 - ms);
}

void Mgamebs::sync_res(game_list::iterator git, io_context& sk)
{
    if (git->resp_.empty()) {
        return;
    }
    BOOST_FOREACH(auto &v, git->resp_) {
        sk.send(v.first, v.second);
    }
    const_cast<gamebs&>(*git).resp_.clear();
}

void pack_speed(std::string& vbuf, UInt uid, uint16_t spx)
{
    typedef union { uint32_t n; uint16_t h[2]; char s[4]; } UType;
    UType tmp[2];
    tmp[0].n = htonl(uid);
    tmp[1].h[0] = htons(spx);
    vbuf += std::string(&tmp->s[0], &tmp->s[6]); // vbuf.insert(vbuf.end(), tmp->s[0], tmp->s[0] + 6);
    LOG << uid << spx;
}

struct speed0 : std::vector<UInt> {
    UInt total;
    uint16_t operator()(player const& p) {
        this->total++;
        uint16_t x = p.speed();
        if (x == 0)
            push_back(p.uid_);
        return x;
    }
    speed0() { total=0; }
};

void* Mgamebs::loop(Int64 tp_cur)
{
    auto & games = boost::get<TagS>(m_.games_online_);
    if (games.empty()) {
        LOG << "no games";
        return 0;
    }
    auto git = games.front();
    UInt np = 0;

    if (io_context sk = io_context::find(git->game_id(), static_cast<bs_server*>(0)))
    {
        sync_res(git, sk);

        speed0 get_spx;
        std::string vbuf;
        auto pr = boost::get<TagG>(m_.players_).equal_range(git);
        BOOST_FOREACH(auto &p, pr) {
            pack_speed(vbuf, p.uid_, get_spx(p));
        }
        //BOOST_FOREACH(auto &uid, git->exgames_) {
        //    pack_speed(vbuf, uid, 0);
        //}

        if (vbuf.empty()) {
            LOG << "no users" << git->game_id();
        } else {
            sk.send(Mgamebs::Msg_speeds, vbuf);
            if (!get_spx.empty()) {
                LOG << get_spx.total <<"zeros"<< get_spx;
            }
        }
        np = get_spx.total;

        auto& gb = const_cast<gamebs&>(*git);
        gb.tp_sync_ = tp_cur;
        gb.count_player( get_spx.total );
        games.relocate(games.end(), games.begin());

    } else {
        LOG << "bug";
        games.pop_front();
    }

    LOG << *git << np << "loop end";
    return 0;
}

void Mgamebs::connect(io_context& sk, json::object const& jin, int opx)
{
    UInt gid = json::as<UInt>(jin,"bid").value();
    std::string tok = json::as<std::string>(jin,"token").value();
    LOG << gid << tok;

    game_list::iterator git = myfind<TagX>(m_.games_,gid);
    if (git == m_.games_.end()) {
        json::object obj = http_game_verify(m_.http_host_+m_.url_bar_info_, gid, tok);
        if (/*int ec = */json::as<int>(obj,"error").value_or(0)) {
            BOOST_THROW_EXCEPTION( verify_fail() << error_id(gid) << errinfo_str(tok) );
        }

        auto p = m_.games_.insert( gamebs(gid, tok) );
        git = p.first;
        LOG << gid << tok;

    } else if (tok != git->token_) {
        LOG << tok << git->token_ << "token differ";
        json::object obj = http_game_verify(m_.http_host_+m_.url_bar_info_, gid, tok);
        if (/*int ec = */json::as<int>(obj,"error").value_or(0)) {
            BOOST_THROW_EXCEPTION( verify_fail() << error_id(gid) << errinfo_str(tok) );
        }
        const_cast<gamebs&>(*git).token_ = tok;
    }

    Int64 tp_cur = milliseconds();
    bool is_reconnect = json::as<bool>(jin,"reconnect").value_or(false);
    if (is_reconnect) {
        LOG << gid << "re-connected" << git->is_avail();
    }

    if (!git->is_avail() || !is_reconnect)
    {
        auto& gb = const_cast<gamebs&>(*git);
        gb.reset(0);

        auto & player_idx = boost::get<TagG>(m_.players_);
        typedef decltype(player_idx.begin()) iterator;
        std::vector<iterator> dels;

        auto pr = player_idx.equal_range(git);
        for (auto it = boost::begin(pr); it != boost::end(pr); ++it) {
            auto& p = *it;
            const_cast<player&>(p).reset();
            if (io_context sk = io_context::find(p.uid_, static_cast<cs_server*>(0))) {
                gb.join( p.uid_, int(Mgamebs::Msg_newuser), json_player(p) );
            } else {
                dels.push_back(it);
            }
        }
        BOOST_FOREACH(auto it, dels) {
            LOG << it->uid_ << "offline";
            player_idx.erase(it);
        }
        LOG << gid << "new game" << dels.size();
    }

    boost::get<TagS>(m_.games_online_).push_front(git);

    if (auto c = sk.tag(gid, static_cast<bs_server*>(0))) {
        LOG << gid << "Multi";
        c.close(__LINE__,__FILE__);
    }
    sk.send(opx, "{\"error\":0}");
    sync_res(git, sk);
}

void Mgamebs::End(io_context& sk, game_list::iterator git, json::object const& jin)
{
    // auto git = myfind<TagX>(m_.games_,gid);
    // if (git == m_.games_.end()) { BOOST_THROW_EXCEPTION( game_notfound() << error_id(gid) ); }

    boost::array<UInt,3> av {0,0,0};
    auto it = av.begin();
    json::array ja = json::as<json::array>(jin,"ranking").value();
    BOOST_FOREACH(auto& a, ja) {
        *it = json::as<UInt>(a).value();
        if (++it == av.end()) {
            break;
        }
    }
    LOG << *git << av;

    http_send_message(m_.http_host_+m_.url_end_, git->gid_, av, git->n_player());

    const_cast<gamebs&>(*git).reset(&av);

    m_.games_online_.erase(git);
    sk.send(Mgamebs::Msg_end, "{\"error\":0}");
    sk.close(__LINE__,__FILE__);
}

//void Main::cleanup(game_list::iterator git)
//{
//    m_.games_online_.erase(git);
//    {
//        auto & idx = boost::get<TagG>(m_.players_);
//        auto pr = idx.equal_range(git);
//        BOOST_FOREACH(auto& p, pr) {
//            close;
//        }
//        idx.erase(pr.begin(), pr.end());
//    }
//    m_.games_.erase(git);
//    close;
//}

void Mgamebs::disconnect(io_context& sk, game_list::iterator git)
{
    LOG << *git;
    m_.games_online_.erase(git);
}

////// ////// ////// ////// ////// //////
//

void Mplayer::speedx(io_context& sk, player_list::iterator it, json::object const& jin)
{
    game_list::iterator git = it->git();

    UInt spx = json::as<UInt>(jin,"speed").value();

    //const_cast<gamebs&>(*git).uspeed(it->user, spx);
    const_cast<player&>(*it).speed_up( spx );
}

player_list::iterator Mplayer::sign_in(io_context& sk, json::object const& jin, int opx)
{
    player_list::iterator pit;

    UInt uid = json::as<UInt>(jin,"uid").value();
    //UInt gid = json::as<UInt>(jin,"bid").value_or(0);
    std::string tok = json::as<std::string>(jin,"token").value();
    LOG << opx << uid << tok; // << gid;
    if (opx == Mplayer::Msg_sign2)
    {
        pit = myfind<TagX>(m_.players_,uid);
        if (pit == m_.players_.end()) {
            BOOST_THROW_EXCEPTION( player_notfound() << error_id(uid) );
        }

        if (tok != pit->token_) {
            BOOST_THROW_EXCEPTION( verify_fail() << error_id(uid) << errinfo_str(tok) );
        }

    } else {
        json::object obj = http_user_verify(m_.http_host_+m_.url_user_info_, uid, 0/*gid*/, tok);
        if (/*int ec = */json::as<int>(obj,"error").value_or(0)) {
            BOOST_THROW_EXCEPTION( verify_fail() << error_id(uid) << errinfo_str(tok) );
        }
        UInt gid = json::as<UInt>(obj,"barid").value_or(0); //if (gid == 0) {//}

        std::string tmp = json::as<std::string>(obj, "sex").value();
        if (tmp.empty()) {
            BOOST_THROW_EXCEPTION( bad_message() << error_id(uid) );
        }
        std::string nick = json::as<std::string>(obj,"nick").value();
        std::string photo = json::as<std::string>(obj,"icon").value();
        char gender = *tmp.begin();

        game_list::iterator git = myfind<TagX>(m_.games_,gid);
        if (git == m_.games_.end()) {
            BOOST_THROW_EXCEPTION( game_notfound() << error_id(gid) );
        }

        player py(git, uid, tok, gender, nick, photo);
        auto px = m_.players_.insert( py );
        if (!px.second) {
            LOG << "already in game" << px.first->game_id();
            m_.players_.erase( px.first );
            px = m_.players_.insert( py );
            // BOOST_THROW_EXCEPTION( player_alreadyexist() << error_id(uid) );
        }
        char rk[128];
        snprintf(rk,sizeof(rk), "shk/%u/%lu", git->game_id(), git->tps_start_);
        redis::command("SADD", rk, uid);

        pit = px.first;
        auto& g = const_cast<gamebs&>(*git);
        g.join( pit->uid_, int(Mgamebs::Msg_newuser), json_player(*pit) );
    }

    if (auto c = sk.tag(uid, static_cast<cs_server*>(0))) {
        LOG << uid << "Multi";
        c.close(__LINE__,__FILE__);
    }
    sk.send(opx, "{\"error\":0}");
    return pit;
}

Main::Main(boost::asio::io_service& io_s, std::string h)
    : timer_loop_(io_s)
    , timer_chk_(io_s)
    , http_host_(h)
{
    // namespace placeholders = boost::asio::placeholders;
    io_s.post(boost::bind(&Main::game_loop, boost::system::error_code()));
    io_s.post(boost::bind(&Main::game_check, boost::system::error_code()));

//#define KK_HOST "http://product.api.moon.kklink.com"
// #define KK_HOST "http://api.kklinkoffice.com"
    url_bar_info_ = "/eauth/bar";
    url_user_info_ = "/eauth/user";
    url_end_ = "/eauth/msg";
    //"http://www.boost.org/LICENSE_1_0.txt";
    LOG << h;
}

void Main::game_loop(boost::system::error_code ec)
{
    static UInt loop_count = 0;
    if (ec) {
        return;
    }
    auto & self = instance();
    Mgamebs gm(self);

    Int64 tp_cur = milliseconds();
    UInt mils;
    while ( (mils = gm.next_loop_tpx(tp_cur)) == 0) {
        if (gm.loop(tp_cur)) {
            // on_game_end(eg);
        }
    }
    if (((loop_count++) & 0x07) == 0) {
        LOG << mils << tp_cur << self.games_online_.empty();
    }

    if (mils > 0) {
        if (mils > 10000) {
            LOG << mils << "bug";
        }
        self.timer_loop_.expires_from_now(boost::posix_time::milliseconds(mils));
        self.timer_loop_.async_wait(Main::game_loop);
    }
}

void Main::game_check(boost::system::error_code ec)
{
    if (ec) {
        return;
    }
    auto& self = instance();
    Mgamebs gm(self);

    // UInt mils = self.check_expire();
    int mils = 5000; // TODO

    self.timer_chk_.expires_from_now(boost::posix_time::milliseconds(mils));//(boost::posix_time::seconds(6));
    self.timer_chk_.async_wait(Main::game_check);
}

#include "msgh.h"

void msgh_player(io_context& sk, int opx, std::string const& data)
{
    UInt uid = sk.tag();
    Mplayer pm( Main::instance(), uid );

    try {
        player_list::iterator pit;

        switch (opx) {
            case Mplayer::Msg_sign: case Mplayer::Msg_sign2:
                pit = pm.sign_in(sk, json::decode<json::object>(data).value(), opx);
                LOG << pit->game_id() << uid << opx << data;
                return;
        }

        pit = pm.ref(uid);
        LOG << pit->game_id() << uid << opx << data;
        switch (opx) {
            case Mplayer::Msg_speedx:
                pm.speedx(sk, pit, json::decode<json::object>(data).value());
                break;
            case Connection_Lost:
                pm.disconnect(sk, pit);
                break;
        }
        return;
    } catch (myerror const& e) {
        LOG << e;
    } catch (boost::exception const& e) {
        LOG << e;
    } catch (std::exception const& e) {
        LOG << "=except:" << e.what();
    }
    sk.send(opx, ("{\"error\":4}"));
    sk.close(__LINE__,__FILE__);
}

void msgh_gamebs(io_context& sk, int opx, std::string const& data)
{
    LOG << sk.tag() << opx << data;
//void Main::message_gamebs(io_context& sk, int opx, std::string const& data)
    auto & m = Main::instance();
    Mgamebs gm( m );

    try {
        switch (opx) {
            case Mgamebs::Msg_connect: case Mgamebs::Msg_connect2:
                gm.connect(sk, json::decode<json::object>(data).value(), opx);
                break;
            case Mgamebs::Msg_end:
                gm.End(sk, itref<TagX>(m.games_,sk.tag()), json::decode<json::object>(data).value());
                break;
            case Connection_Lost:
                gm.disconnect(sk, itref<TagX>(m.games_,sk.tag()));
                break;
        }
        return;
    } catch (myerror const& e) {
        LOG << e;
        // errc = error_code::unknown;
    } catch (boost::exception const& e) {
        LOG << e;
        // errc = error_code::unknown;
    } catch (std::exception const& e) {
        LOG << "=except:" << e.what();
        // errc = error_code::unknown;
    }
    sk.send(opx, ("{\"error\":4}"));
    sk.close(__LINE__,__FILE__);
}

#include <urdl/read_stream.hpp>

boost::system::error_code http_query(std::string const& url, std::string * _body)
{
    boost::asio::io_service io_service;

    urdl::read_stream stream(io_service);
    stream.open(url);
    LOG << url;

    std::string body;
    for (std::size_t length = 0; ; )
    {
        boost::system::error_code ec;
        char data[1024];

        std::size_t nread = stream.read_some(boost::asio::buffer(data), ec);
        if (ec) {
            if (ec == boost::asio::error::eof) {
                if (stream.content_length() == std::numeric_limits<std::size_t>::max()) {
                    goto Pos_ok_;
                }
            }
            return ec;
        }
        if (length == 0) {
            length = stream.content_length();
            LOG << length << nread;
            if (length != std::numeric_limits<std::size_t>::max()) {
                body.reserve(length);
            }
        }
        //if (nread > length) {
        //    LOG << length << nread;
        //    return make_error_code(boost::system::errc::bad_message);
        //}

        if (_body) {
            body.insert(body.end(), &data[0], &data[0] + std::min(nread,length));
        }

        if (nread >= length) {
            goto Pos_ok_;
        }
        length -= nread;
    }

Pos_ok_:
    if (_body) {
        LOG << body;
        body.swap(*_body);
    }
    return boost::system::error_code();
}

json::object http_user_verify(std::string const& url, UInt uid, UInt gid, std::string const& tok)
{
    std::string q = url + str(boost::format("?uid=%1%&barid=%2%&token=%3%") % uid % gid % tok);
    std::string body;
    if (auto ec = http_query(q, &body)) {
        LOG << ec << ec.message() << body;
        BOOST_THROW_EXCEPTION( verify_fail() << error_id(uid) << errinfo_str(tok) );
    }
    return json::decode<json::object>(body).value();
}

json::object http_game_verify(std::string const& url, UInt gid, std::string const& tok)
{
    std::string q = url + str(boost::format("?barid=%1%&token=%2%") % gid % tok);
    std::string body;
    if (auto ec = http_query(q, &body)) {
        LOG << ec << ec.message();
        BOOST_THROW_EXCEPTION( verify_fail() << error_id(gid) << errinfo_str(tok) );
    }
    return json::decode<json::object>(body).value();
}

void http_send_message(std::string const& url, UInt gid, boost::array<UInt,3> const& av, UInt total)
{
    using namespace boost;
    std::string q = url + str(
            format("?n1=%1%&n2=%2%&n3=%3%&barid=%4%&np=%5%") % av[0] % av[1] % av[2] % gid % total);
    std::string body;
    if (auto ec = http_query(q, &body)) {
        LOG << ec << ec.message();
    }
}

