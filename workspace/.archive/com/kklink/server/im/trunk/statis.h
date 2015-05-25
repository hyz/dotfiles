#ifndef _STATIS_H__
#define _STATIS_H__

#include <vector>
#include <unordered_map>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include "globals.h"
#include "singleton.h"
#include "util.h"

namespace multi_index = boost::multi_index;

//inline bool day_of_year(time_t tp)
//{
//    struct tm tm = {};
//    localtime_r(&tp, &tm);
//    return tm.tm_yday;
//}
//inline int day_of_week(time_t tp)
//{
//    struct tm tm = {};
//    localtime_r(&tp, &tm);
//    return tm.tm_wday;
//}
//
//inline bool is_same_day(time_t x, time_t y)
//{
//    return (day_of_week(x) == day_of_week(y));
//    // enum { SECONDS_PDAY=(60*60*24) }; // return (x / SECONDS_PDAY) == (y / SECONDS_PDAY);
//}

struct statis : singleton<statis>
{
    typedef boost::asio::ip::tcp::socket socket_t;
    struct reportor;

    struct msgrec_t
    {
        UInt msgid;
        time_t tp0, tp1; //, tp2;
        int stat;
        msgrec_t(UInt mid, time_t t0, time_t t1, int st)
        {
            msgid = mid;
            tp0 = t0;
            tp1 = t1;
            stat = st;
        }
        friend struct reportor;
    };
    struct onlrec_t // : std::pair<time_t,time_t>
    {
        time_t tp0, tp1;
        int stat;
        // endpoint;
        onlrec_t(time_t t0, time_t t1, int st) //(, socket_t const& sk)
        {
            tp0 = t0;
            tp1 = t1;
            stat = st;
        }
        friend struct reportor;
    };

    struct onls_t
    {
        UInt off_, on_;
        onls_t() {
            on_ = off_ = 0;
        }
        friend struct reportor;
    };
    struct msgs_t
    {
        UInt rx_n_, rx_nfin_, rx_ndrop_, rx_nsockwrite;
        UInt tx_n_, tx_nrx_;
        msgs_t() {
            rx_n_ = rx_nfin_ = rx_ndrop_ = rx_nsockwrite = 0;
            tx_n_ = tx_nrx_ = 0;
        }
        friend struct reportor;
    };

    struct onlmsg_t
    {
        UInt const uid_;
        time_t atime_;

        onls_t onls_;
        msgs_t msgs_;
        std::vector<onlrec_t> onlrecs_;
        std::vector<msgrec_t> msgrecs_;

        onlmsg_t(UInt uid);

        void loads(UInt uid);
        void saves(UInt uid);
        friend struct reportor;
    };

    typedef boost::multi_index_container<
            onlmsg_t,
            multi_index::indexed_by<
                    multi_index::sequenced<>,
                    multi_index::hashed_unique<multi_index::member<onlmsg_t, UInt const, &onlmsg_t::uid_> >
                >
        > onlmsg_list;

    struct reportor
    {
        std::ostream* os_;
        time_t tpc_;
        UInt uid_;

        reportor(std::ostream& o )
            : os_(&o)
        {
            tpc_ = time(0);
            uid_ = 0;
        }
        void operator()(onlmsg_t const& );
        void operator()(onls_t const& );
        void operator()(msgs_t const& );
        void operator()(msgrec_t const& );
        void operator()(onlrec_t const& );
        template <typename Iter> void operator()(Iter it, Iter end);
    };

    void up_online(UInt uid, socket_t& sk);
    void up_offline(UInt uid, socket_t& sk, time_t t0, time_t t1, int stat);

    void up_message(UInt uid, UInt msgid, time_t t0, time_t t1, int stat);
    void up_message_send(UInt fr, UInt msgid, std::vector<UInt>& dsts);
    void up_message_drop(UInt uid, UInt msgid);
    //void up_message_empty(UInt uid, UInt msgid);

    onlmsg_t& access_(UInt uid);

    void on_day_end(std::ostream& out_f);
    void on_day_begin(std::ostream& out_f);
    void flush(int fxa);
    void gc(std::ostream& out_f);

    ~statis();
    statis();

    static void init(boost::filesystem::path out_dir);

    onlmsg_list oml_; // statis list
    time_t tp_start_;
    daily_rotate_ofstream<daily_filename_monthday,12> ofs_;

    struct {
        onls_t onls_;
        msgs_t msgs_;
    } gs_;
};

#endif // _STATIS_H__

