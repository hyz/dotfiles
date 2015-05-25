#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include "myerror.h"
#include "bottle_drifting.h"

enum enum_error_drifting_bottle
{
    EN_Reach_Limit = 615,
    EN_Drifting_Not_Found = 616,
};

struct error_category_drifting_bottle : myerror_category
{
    error_category_drifting_bottle()
    {
        static code_string ecl[] = {
            { EN_Reach_Limit, "到达当日限制次数" },
            { EN_Drifting_Not_Found, "漂流瓶不存在" }
        };
        Init(this, &ecl[0], &ecl[sizeof(ecl)/sizeof(ecl[0])], "service");
    }
};
static error_category_drifting_bottle error_cat_;

void drifting::_save_hold_his(bottle_ptr b)
{
    sql::exec(boost::format("INSERT INTO bottle_his(UserId,id)"
                " VALUES(%1%,%2%)")
            % b->holder % b->id);
    const_cast<bottle_his&>(*bottle_his_i_).insert(b->id);
}

boost::system::error_code drifting::Throw(bottle_ptr b)
{
    if (!limit_ok(bottle_his_i_->count_throw))
        return boost::system::error_code(EN_Reach_Limit, error_cat_);

    _save_hold_his(b);

    bottles_drifting.push_back(b);

    sql::exec(boost::format("INSERT INTO bottle(id,initiator,spot,type,content)"
                " VALUES(%1%,%2%,'%3%',%4%,'%5%')")
            % b->id % b->initiator % b->spot % b->type % sql::db_escape(b->content));

    return boost::system::error_code();
}

boost::system::error_code drifting::Rethrow(unsigned int bid)
{
    auto & idxs = boost::get<TagBottleId>(this->bottles_holding);
    auto i = idxs.find(bid);
    if (i == idxs.end() || (*i)->holder != bottle_his_i_->userid)
        return boost::system::error_code();

    bottle_ptr b = *i;
    idxs.erase(i);
    sql::exec(boost::format("UPDATE bottle SET holder=0 WHERE id=%1%") % b->id);
    bottles_drifting.push_back(b);

    return boost::system::error_code();
}

void drifting::Delete(unsigned int bid)
{
    auto & idxs = boost::get<TagBottleId>(this->bottles_holding);
    auto i = idxs.find(bid);
    if (i != idxs.end()
            && (*i)->holder == bottle_his_i_->userid)
    {
        sql::exec(boost::format("DELETE FROM bottle WHERE id=%1%") % bid);
        idxs.erase(i);
    }
}

void drifting::deleteAll()
{
    auto & idxs = boost::get<TagBottleId>(this->bottles_holding);
    for( auto i=idxs.begin(); i!=idxs.end(); ++i) {
        if ( (*i)->holder != bottle_his_i_->userid){  continue; }
        sql::exec(boost::format("DELETE FROM bottle WHERE id=%1%") %(*i)->id);
        idxs.erase(i);
    }
}

drifting::drifting_bottles_type::iterator drifting::holding_find(unsigned int bid)
{
    auto & idxs = boost::get<TagBottleId>(this->bottles_holding);
    auto i = idxs.find(bid) ;
    if (i == idxs.end() || (*i)->holder != bottle_his_i_->userid)  {
        return this->bottles_holding.end();
    }

    return this->bottles_holding.project<0>( i );
}

void drifting::_hold_bottle(bottle_ptr b)
{
    b->holder = bottle_his_i_->userid;
    bottles_holding.push_back(b);

    if (b->holder != b->initiator)
        sql::exec(boost::format("UPDATE bottle SET holder=%2% WHERE id=%1%") % b->id % b->holder);

    _save_hold_his(b);
}

std::pair<bottle_ptr,boost::system::error_code> drifting::Catch(std::string const & spot)
{
    if (!limit_ok(bottle_his_i_->count_catch))
        return std::make_pair(bottle_ptr(), boost::system::error_code(EN_Reach_Limit,error_cat_));

    if (bottles_drifting.empty()
            || randuint(1,100) < 5)
        return std::make_pair(bottle_ptr(), boost::system::error_code());

    // if (!spot.empty())
    {
        auto & idxs = boost::get<TagSpot>(bottles_drifting);
        for (auto p = idxs.equal_range(spot); p.first != p.second; ++p.first)
        {
            auto b = *p.first;
            if (bottle_his_i_->find(b->id) == bottle_his_i_->end())
            {
                idxs.erase(p.first);
                _hold_bottle(b);
                return std::make_pair(b, boost::system::error_code()); //goto Pos_ret_;
            }
        }
        return std::make_pair(bottle_ptr(), boost::system::error_code());
    }

    // for (iterator i = bottles_drifting.begin(); i != bottles_drifting.end(); ++i)
    // {
    //     bottle_ptr b = *i;
    //     if (bottle_his_i_->find(b->id) == bottle_his_i_->end())
    //     {
    //         bottles_drifting.erase(i);
    //         _hold_bottle(b);
    //         return std::make_pair(b, boost::system::error_code());
    //     }
    // }

    // return std::make_pair(bottle_ptr(), boost::system::error_code());
}

drifting::drifting()
{
    sql::datas datas("SELECT id,initiator,holder,UNIX_TIMESTAMP(xtime),spot,type,content FROM bottle");
    while (sql::datas::row_type row = datas.next())
    {
        bottle_ptr b( new bottle() );
        b->id = boost::lexical_cast<unsigned int>(row.at(0));
        b->initiator = boost::lexical_cast<UID>(row.at(1));
        b->holder = boost::lexical_cast<UID>(row.at(2));
        b->xtime = boost::lexical_cast<time_t>(row.at(3));
        b->spot = row.at(4);
        b->type = BottleMessage_Type( boost::lexical_cast<int>(row.at(5)) );
        b->content = row.at(6);
        if (b->holder)
            bottles_holding.push_back(b);
        else
            bottles_drifting.push_back(b);
    }
}

drifting & drifting::inst(UID uid)
{
    static drifting d;
    auto p = boost::get<0>(d.bottle_his_idx_).insert( bottle_his(uid) );
    d.bottle_his_i_ = p.first;
    if (p.second)
    {
        boost::format fmt("SELECT id FROM bottle_his WHERE UserId=%1%");
        sql::datas datas(fmt % uid);
        while (sql::datas::row_type row = datas.next())
        {
            UInt id = boost::lexical_cast<UInt>(row.at(0));
            const_cast<bottle_his&>(*d.bottle_his_i_).insert(id);
        }
    }
    return d;
}

