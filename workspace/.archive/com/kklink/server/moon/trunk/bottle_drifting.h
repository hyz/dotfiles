#ifndef DRIFTING_BOTTLE_H__
#define DRIFTING_BOTTLE_H__

#include <string>
#include <set>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/identity.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include "user.h"
#include "dbc.h"
#include "util.h"
#include <boost/multi_index/random_access_index.hpp>

enum BottleMessage_Type
{
    BottleMessage_None = 0,
    BottleMessage_Audio = 1,
    BottleMessage_Image = 2,
    BottleMessage_Text = 3,
};

struct bottle
{
    UInt id;

    UID initiator;
    UID holder;
    std::string spot;
    time_t xtime;

    BottleMessage_Type type;
    std::string content;

    bottle(UID uid, std::string const & p)
        : spot(p)
    {
        id = alloc_id();
        initiator = uid;
        holder = uid;
        xtime = time(0);
    }

private:
    friend struct drifting;
    bottle() {}

    static int alloc_id() { return max_index<bottle>(+1); }
public:
    static const char *db_table_name() { return "bottle"; }
};
typedef boost::shared_ptr<bottle> bottle_ptr;

template <typename OutS> OutS & operator<<(OutS & out, bottle const & b)
{
    return out << b.id <<'/'<< b.initiator <<'/'<< b.holder <<'/'<< b.spot <<'/' << int(b.type);
}

struct TagSpot {};
struct TagHolder {};
struct TagBottleId {};

struct drifting : boost::noncopyable
{
    typedef boost::multi_index_container<
        boost::shared_ptr<bottle>,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<>, // boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<TagBottleId>, BOOST_MULTI_INDEX_MEMBER(bottle,UInt,id)
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagSpot>, BOOST_MULTI_INDEX_MEMBER(bottle,std::string,spot)
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagHolder>, BOOST_MULTI_INDEX_MEMBER(bottle,UID,holder)
            >
        >
    > drifting_bottles_type;

    boost::system::error_code Throw(bottle_ptr b);

    boost::system::error_code Rethrow(unsigned int bid);

    std::pair<bottle_ptr,boost::system::error_code> Catch(std::string const & spot);

    drifting_bottles_type::iterator holding_find(unsigned int bid);
    bool exist(unsigned bid) 
    {
        auto & idxs = boost::get<TagBottleId>(this->bottles_holding);
        auto i = idxs.find(bid) ;
        return (i!=idxs.end() && (*i)->holder==bottle_his_i_->userid);
    }

    drifting_bottles_type::iterator holding_begin()
    {
        return bottles_holding.begin();
    }

    drifting_bottles_type::iterator holding_end()
    {
        return bottles_holding.end();
    }

    int catch_count_left() const { return bottle_his_i_->count_catch;}
    int throw_count_left() const { return bottle_his_i_->count_throw;}

    void Delete(unsigned int bid);
    void deleteAll();

    typedef boost::multi_index::index<drifting_bottles_type, TagHolder>::type holder_index_t;

    std::pair<holder_index_t::iterator,holder_index_t::iterator> Bottles() const
    {
        auto & idxs = boost::get<TagHolder>(this->bottles_holding);
        return idxs.equal_range(bottle_his_i_->userid);
    }

    static drifting & inst(UID uid);

private:
    drifting();

    typedef drifting_bottles_type::iterator iterator;

    void _hold_bottle(bottle_ptr b);
    void _save_hold_his(bottle_ptr b);

    enum { AVAIL_COUNT = 20 };
    struct bottle_his : std::set<UInt>
    {
        UID userid;
        mutable signed short count_throw, count_catch;

        explicit bottle_his(UID uid)
        {
            userid = uid;
            count_throw = count_catch = AVAIL_COUNT;
        }
    };

    typedef boost::multi_index_container<
        bottle_his,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique< BOOST_MULTI_INDEX_MEMBER(bottle_his,UID,userid) >
        >
    > bottle_his_idx_t;

    drifting_bottles_type bottles_holding, bottles_drifting;

    bottle_his_idx_t bottle_his_idx_;
    bottle_his_idx_t::iterator bottle_his_i_;

    UInt xdate_;

    template <typename Int> bool limit_ok(Int & count) const
    {
#define SECONDS_PDAY (60*60*24)
        time_t t = time(0);
        if (t - xdate_ >= SECONDS_PDAY)
        {
            BOOST_FOREACH(auto & x, bottle_his_idx_)
                x.count_catch = x.count_throw = AVAIL_COUNT;
            const_cast<drifting*>(this)->xdate_ = t - t % SECONDS_PDAY;
            LOG_I << t << " limit reset";
        }
        return (count > 0 && count-- > 0);
    }
};

#endif // DRIFT_BOTTLE_H__

