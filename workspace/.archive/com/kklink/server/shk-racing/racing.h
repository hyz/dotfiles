#ifndef RACING_H__
#define RACING_H__

#include <time.h>
#include <list>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include "singleton.h"
#include "ioctx.h"

namespace multi_index = boost::multi_index;

typedef int64_t Int64;
typedef unsigned int UInt;
struct TagX {};
struct TagS {};
struct TagG {};
Int64 milliseconds();

struct gamebs
{
    gamebs(UInt gid, std::string const& tok)
        : token_(tok)
    {
        // sk_ = sk;
        gid_ = gid;
        tps_start_ = time(0);
        tp_sync_ = 0;
        n_player_ = 0;
        // S_ = 0xffff; // distance; S = VT
    }
    ~gamebs() { reset(0); }
    // void speed_up(UInt uid, UInt spx) { uspx_[uid] = spx; }

    void reset(boost::array<UInt,3>* endv);

    void count_player(int n);
    int  n_player() const { return n_player_; }

    bool is_avail() const {
        return (time(0) - tps_start_ < 60*3);
    }

    UInt game_id() const { return gid_; }

    void join(UInt uid, int opx, std::string const& data);
    //void exit(UInt uid)
    //{
    //    if (ingames_.find(uid) != ingames_.end()) {
    //        exgames_.insert(uid);
    //    }
    //}

    UInt gid_;
    boost::unordered_set<UInt> ingames_;
    // boost::unordered_set<UInt> exgames_;

    int n_player_;
    //boost::unordered_set<UInt> uspx_;

// private:
    std::vector<std::pair<int,std::string> > resp_;

    std::string token_;

    time_t tps_start_;
    Int64 tp_sync_;

    friend std::ostream& operator<<(std::ostream& out, gamebs const& g) {
        return out << g.game_id();
    }
};

typedef boost::multi_index_container<
        gamebs,
        multi_index::indexed_by<
                multi_index::hashed_unique<
                    multi_index::tag<TagX>, multi_index::member<gamebs, UInt, &gamebs::gid_>
                 >,
                 multi_index::sequenced<multi_index::tag<TagS> >
            >
    > game_list;
// typedef multi_index::index<game_list, TagX>::type::iterator game_list_iterator;
//typedef multi_index::index<game_list, TagX>::type game_idx_t;

typedef boost::multi_index_container<
        game_list::iterator,
        multi_index::indexed_by<
                multi_index::hashed_unique<multi_index::tag<TagX>,multi_index::identity<game_list::iterator> >,
                multi_index::sequenced<multi_index::tag<TagS> >
            >
    > game_list_online;

struct player
{
    player(game_list::iterator it, UInt uid
            , std::string const& tok
            , char gender, std::string const& nick, std::string const& photo);
    // void join(game_list::iterator gs) { gs_ = gs; }

    inline UInt last_speed() const { return spxs_[(speeds_ix_-1) % spxs_.size()]; }
    UInt speed() const;
    void speed_up(uint8_t spx);
    void speed_down();
    game_list::iterator git() const { return git_; }
    UInt game_id() const { return git_->game_id(); }

    void reset()
    {
        spxs_[0] = spxs_[1] = spxs_[2] = 0;
        tp_spx_ = 0;
    }

    UInt uid_;
    // UInt pos_;

    // struct player_brief {};
    std::string token_;

    char gender_; //char _[2];

    std::array<uint8_t, 3> spxs_;
    unsigned int speeds_ix_;
    Int64 tp_spx_;

    std::string nick_;
    std::string photo_;

    game_list::iterator git_;
};

typedef boost::multi_index_container<
        player,
        multi_index::indexed_by<
                multi_index::hashed_unique<
                    multi_index::tag<TagX>, multi_index::member<player, UInt, &player::uid_>
                >,
                multi_index::ordered_non_unique<
                    multi_index::tag<TagG>, multi_index::member<player, game_list::iterator, &player::git_>
                >
                // multi_index::sequenced<multi_index::tag<TagS> >
            >
    > player_list;
//typedef multi_index::index<player_list, TagG>::type player_gidx_t;

struct Main : singleton<Main>
{
    Main(boost::asio::io_service& io_s, std::string h);

public:
    static void message_player(io_context& sk, int opx, std::string const& data);
    static void message_gamebs(io_context& sk, int opx, std::string const& data);

    game_list games_;
    player_list players_;
    game_list_online games_online_;

public: // private:
    static void game_check(boost::system::error_code ec);
    static void game_loop(boost::system::error_code ec);

    boost::asio::deadline_timer timer_loop_;
    boost::asio::deadline_timer timer_chk_;

    std::string url_bar_info_;
    std::string url_user_info_;
    std::string url_end_;
    std::string http_host_;
};

#endif // RACING_H__

