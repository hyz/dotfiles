#ifndef _TEST_MULTIINDEX_
#define _TEST_MULTIINDEX_
#include <string>
#include <vector>
#include <list>
#include <cmath>
#include <iterator>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/format.hpp>

#include "dbc.h"

struct advertising
{
    // static const char *db_table_name() { return "advertising"; }
    // static int alloc_id() { return max_index<advertising>(+1); }
    int id;
    std::string name;
    std::string A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2;
    time_t beginDate;
    time_t endDate;
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
    boost::format fmt("SELECT id,name,A_image1,A_size1,A_image2,A_size2,I_image1,I_size1,I_image2,I_size2,unix_timestamp(beginDate),unix_timestamp(endDate),city,zone,SessionIds FROM advertising WHERE city='南昌市'");
    // fmt = fmt % cond;
    sql::datas datas(fmt);
    while (sql::datas::row_type row = datas.next())
    {
        global_advertising a;
        a.id = boost::lexical_cast<unsigned int>(row.at(0));
        a.name = row.at(1,"");
        a.A_image1 = row.at(2);
        a.A_size1 = row.at(3);
        a.A_image2 = row.at(4);
        a.A_size2 = row.at(5);
        a.I_image1 = row.at(6);
        a.I_size1 = row.at(7);
        a.I_image2 = row.at(8);
        a.I_size2 = row.at(9);
        a.beginDate = boost::lexical_cast<time_t>(row.at(10));
        a.endDate = boost::lexical_cast<time_t>(row.at(11));
        a.city = row.at(12);
        a.zone = row.at(13);
        if ( row.at(14) ) {
            std::istringstream ins(row[14]);
            std::istream_iterator<std::string> i(ins), end;
            std::copy(i, end, std::back_inserter(a.SessionIds));
        }
        *it++ = a;
    }
};
struct AdvertisingId{};
struct AdvertisingCity{};

class global_mgr
{
    public:
        typedef boost::multi_index_container<
            global_advertising,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<AdvertisingId>, BOOST_MULTI_INDEX_MEMBER(advertising,int,id)
                >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<AdvertisingCity>, BOOST_MULTI_INDEX_MEMBER(global_advertising,std::string,city)
                >
                >
                > global_advertising_type;
        void get_advertising_bycity(const std::string& city)
        {
            global_advertising::load(boost::str(boost::format("city='%1%'")%city),std::back_inserter(g_advers_));
        }
        void print_advers_bycity(const std::string& city)
        {
            for ( global_advertising_type::iterator itr = g_advers_.begin();
                    itr != g_advers_.end(); ++itr)
            {
                std::cout<<itr->city <<" / "<< itr->beginDate << " / " <<itr->endDate;
            }
        }
    private:
        global_advertising_type g_advers_;
};
#endif
