#ifndef USER_H__
#define USER_H__

#include <vector>
#include <string>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/foreach.hpp>
#include "dbc.h"

#ifndef UINT
//# define UINT (unsigned int)
#endif
typedef unsigned int UInt ;
typedef UInt UID ;
typedef UInt MSGID ;
#define INVALID_UID ((UID)(0))
#define SYSADMIN_UID ((UID)(1000))
#define PRESERVED_UID ((UID)(10000))

inline bool is_normal_uid(UID u) { return u>PRESERVED_UID; }

std::string get_head_icon(UID uid);

    struct TagSrc {};
    struct TagDst {};

    struct xpair_t
    {
        UID src;
        UID dst;
        xpair_t(UID s=0, UID d=0) { src=s; dst=d; }
        bool operator<(xpair_t const & rhs) const { return dst<rhs.dst || (dst==rhs.dst && src<rhs.src); }
    };

struct relationship
{
    typedef boost::multi_index_container<
        xpair_t,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::identity<xpair_t> >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagSrc>, BOOST_MULTI_INDEX_MEMBER(xpair_t,UID,src)>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<TagDst>, BOOST_MULTI_INDEX_MEMBER(xpair_t,UID,dst)>
        >
    > rels_t;
    typedef boost::multi_index::index<rels_t,TagSrc>::type::iterator src_iterator;
    typedef boost::multi_index::index<rels_t,TagDst>::type::iterator dst_iterator;

    std::pair<src_iterator, src_iterator> get_by_src(UID src) const
    {
        auto & idxs = boost::get<TagSrc>(rels_);
        return idxs.equal_range(src);
    }

    std::pair<dst_iterator, dst_iterator> get_by_dst(UID dst) const
    {
        auto & idxs = boost::get<TagDst>(rels_);
        return idxs.equal_range(dst);
    }

    std::vector<xpair_t> bidirect(UID src) const
    {
        std::vector<xpair_t> ret;
        auto p = get_by_src(src);
        for (auto i = p.first; i != p.second; ++i)
        {
            if (rels_.find(xpair_t(i->dst, i->src)) != rels_.end())
                ret.push_back(*i);
        }
        return ret;
    }

    std::pair<rels_t::iterator,bool> insert(UID src, UID dst)
    {
        auto p = rels_.insert(xpair_t(src,dst));
        if (p.second)
        {
            boost::format fmt("INSERT INTO contacts(UserId,OtherId,relation) VALUES(%1%,%2%,1)");
            sql::exec(fmt % src % dst);
        }
        return p;
    }

    int erase(UID src, UID dst)
    {
        int n = rels_.erase(xpair_t(src,dst));
        if (n > 0)
        {
            boost::format fmt("DELETE FROM contacts WHERE UserId=%1% AND OtherId=%2%");
            sql::exec(fmt % src % dst);
        }
        return n;
    }

    void inflate_by_dst(UID dst)
    {
        return inflate(str(boost::format("WHERE OtherId='%1%' AND relation=1") % dst));
    }

    void inflate_by_src(UID src)
    {
        return inflate(str(boost::format("WHERE UserId='%1%' AND relation=1") % src));
    }

private:
    void inflate(std::string const& cond)
    {
        boost::format fmt("SELECT UserId,OtherId FROM contacts %1%");
        sql::datas datas( fmt % cond );
        while (sql::datas::row_type row = datas.next())
        {
            UID s = boost::lexical_cast<UID>(row.at(0));
            UID d = boost::lexical_cast<UID>(row.at(1));
            rels_.insert(xpair_t(s,d));
        }
    }

    rels_t rels_;
};

#endif // USER_H__

