#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include "statis.h"
#include "dbc.h"

static char const* const rk_statis = "statis";
enum { NONL_RESERVE=8 };
enum { NMSG_RESERVE=32 };

std::ostream& operator<<(std::ostream& outs, statis::msgs_t const& m)
{
    return outs << m.rx_n_ <<" "<< m.rx_nfin_
        <<" "<< m.rx_ndrop_ // <<" "<< m.rx_nsockwrite
        <<" "<< m.tx_n_ // <<" "<< m.tx_nrx_
        ;
}

static statis::msgs_t reads(UInt)
{
    auto reply = redis::command("GET", "msg/count/0");
    if (reply && reply->type == REDIS_REPLY_STRING)
    {
        std::stringstream ins(reply->str);
        statis::msgs_t m;
        if (ins >> m.rx_n_ >> m.rx_nfin_ >> m.rx_ndrop_ >> m.rx_nsockwrite
                >> m.tx_n_ >> m.tx_nrx_)
        {
            return m;
        }
    }
    return statis::msgs_t();
}
static void writes(UInt, statis::msgs_t const& m)
{
    std::stringstream outs;
    outs << m.rx_n_ <<" "<< m.rx_nfin_
            <<" "<< m.rx_ndrop_ <<" "<< m.rx_nsockwrite
        <<" "<< m.tx_n_ <<" "<< m.tx_nrx_;
    redis::command("SET", "msg/count/0", outs.str());
}

statis::~statis()
{
    LOG << gs_.msgs_;
    writes(0, gs_.msgs_);
    flush(1);
}

statis::statis()
    : ofs_("/home/yxim/onlmsg")
{
    tp_start_ = time(0);
    gs_.msgs_ = reads(0);
    LOG << gs_.msgs_;
}

template <typename Ct> inline bool is_full(Ct const& c)
{
    return (c.size() == c.capacity());
}

void statis::up_message_send(UInt fr, UInt msgid, std::vector<UInt>& dsts)
{
    if (!fr || !msgid || dsts.empty()) {
        return;
    }

    gs_.msgs_.tx_n_++;
    gs_.msgs_.rx_n_ += dsts.size();
    gs_.msgs_.tx_nrx_ = gs_.msgs_.rx_n_;

    {
        auto & su = access_(fr);
        su.msgs_.tx_n_++;
        su.msgs_.tx_nrx_ += dsts.size();
    }

    BOOST_FOREACH(UInt & dst, dsts) {
        auto & su = access_(dst);
        su.msgs_.rx_n_++;
    }
}

//void statis::up_message_add(UInt uid, UInt msgid)
//{
//    if (!uid || !msgid) {
//        return;
//    }
//
//    rx_n_++;
//    auto & su = access_(uid);
//
//    su.rx_n_++;
//}

void statis::up_message(UInt uid, UInt msgid, time_t t0, time_t t1, int stat)
{
    if (!uid || !msgid) {
        return;
    }

    auto & su = access_(uid);
    auto & recs = su.msgrecs_;

    gs_.msgs_.rx_nsockwrite++;
    su.msgs_.rx_nsockwrite++;

    if (stat == 0) {
        gs_.msgs_.rx_nfin_++;
        su.msgs_.rx_nfin_++;
    }
    recs.push_back( msgrec_t(msgid, t0, t1, stat) );

    if (is_full(recs)) {
        reportor rep(ofs_.ostream());
        rep(su); // su.report(ofs_.ostream());
    }
}

void statis::up_message_drop(UInt uid, UInt msgid)
{
    if (!uid || !msgid) {
        return;
    }

    auto & su = access_(uid);

    gs_.msgs_.rx_ndrop_++;
    su.msgs_.rx_ndrop_++;
}

struct sample_ol
{
    sample_ol()
    {
        ls_.emplace_back(0, time(0)-30);
    }

    std::vector<std::pair<int, time_t> > ls_;

    void push(int count, time_t tp);
};

void sample_ol::push(int olc, time_t tp)
{
    if (tp < ls_.back().second + 30) {
        return;
    }
    if (olc == ls_.back().first) {
        return;
    }

    ls_.emplace_back(olc, tp);

    if (ls_.size() >= 32)
    {
        std::vector<std::string> qs;
        qs.reserve(ls_.size());
        for (auto it=ls_.begin()+1, end=ls_.end(); it != end; ++it) {
            qs.push_back(str(boost::format("(%1%,%2%)") % it->second % it->first));
        }
        Sql_Exec("INSERT INTO sample_ol(time,n_online) VALUES" + boost::algorithm::join(qs,","));

        std::swap(ls_.front(), ls_.back());
        ls_.resize(1);
    }
}

static void olcount_append(int olc, time_t tp)
{
    static sample_ol olc_;
    olc_.push(olc, tp);
}

void statis::up_offline(UInt uid, socket_t& sk, time_t t0, time_t t1, int stat)
{
    if (!uid) {
        return;
    }

    auto & su = access_(uid);
    auto & recs = su.onlrecs_;

    gs_.onls_.off_++;
    su.onls_.off_++;
    recs.push_back( onlrec_t(t0, t1, stat) );

    if (is_full(recs)) {
        reportor rep(ofs_.ostream());
        rep(su); // su.report(ofs_.ostream());
    }

    olcount_append( gs_.onls_.on_ - gs_.onls_.off_, t1 );
}

void statis::up_online(UInt uid, socket_t& sk)
{
    if (!uid) {
        return;
    }

    auto & su = access_(uid);
    su.onls_.on_++;
    gs_.onls_.on_++;

    olcount_append( gs_.onls_.on_ - gs_.onls_.off_, time(0) );
}

statis::onlmsg_t& statis::access_(UInt uid)
{
    std::pair<onlmsg_list::iterator,bool> p = oml_.push_front( onlmsg_t(uid) );
    if (p.second) {
        // redis sync
        //
        auto it = p.first;
        const_cast<onlmsg_t&>(*it).loads(uid);
    } else {
        oml_.relocate(oml_.begin(), p.first);
    }

    auto & su = const_cast<onlmsg_t&>(*p.first);
    su.atime_ = time(0);
    return su;
}

template <typename Iter>
void statis::reportor::operator()(Iter it, Iter end)
{
    for (; it != end; ++it) {
        (*this)(*it);
    }
}

void statis::reportor::operator()(statis::msgrec_t const& m)
{
    (*os_) << uid_ <<"\t"<< m.msgid <<"\t"<< m.tp0 <<"\t"<< m.tp1 <<"\t"<< m.stat <<"\tMSG\n";
}

void statis::reportor::operator()(statis::onlrec_t const& m)
{
    (*os_) << uid_ <<"\t"<< m.tp0 <<"\t"<< m.tp1 <<"\t"<< m.stat <<"\tONL\n";
}

void statis::reportor::operator()(onls_t const& onls)
{
    (*os_) << uid_ <<"\t"<< onls.on_ <<"\t"<< onls.off_ <<"\t"<< tpc_ << "\tONLS\n";
}

void statis::reportor::operator()(msgs_t const& msgs)
{
    (*os_) << uid_
        <<"\t"<< msgs.rx_n_
        <<"\t"<< msgs.rx_nfin_
        <<"\t"<< msgs.rx_ndrop_
        <<"\t"<< msgs.rx_nsockwrite
        <<"\t"<< msgs.tx_n_
        <<"\t"<< msgs.tx_nrx_
        <<"\t"<< tpc_ <<"\tMSGS\n"
        ;
}

void statis::reportor::operator()(onlmsg_t const& oms)
{
    const_cast<reportor*>(this)->uid_ = oms.uid_;

    (*this)(oms.onls_);
    (*this)(oms.onlrecs_.begin(), oms.onlrecs_.end());
    (*this)(oms.msgs_);
    (*this)(oms.msgrecs_.begin(), oms.msgrecs_.end());

    {
        auto * su = const_cast<onlmsg_t*>(&oms);
        su->onlrecs_.clear();
        su->onlrecs_.reserve(NONL_RESERVE);
        su->msgrecs_.clear();
        su->msgrecs_.reserve(NONL_RESERVE);
    }

    const_cast<reportor*>(this)->uid_ = 0;
}

template <typename R>
static void report2(std::ostream& out, R const& rng)
{
    statis::reportor rep(out);
    rep(boost::begin(rng), boost::end(rng));
}

template <typename S1, typename S2>
static void summary2(std::ostream& out, S1 const& s1, S2 const& s2)
{
    statis::reportor rep(out);
    rep(s1);
    rep(s2);
}

void statis::on_day_end(std::ostream& out_f)
{
    time_t tpc = time(0);
    report2(out_f, oml_);
    summary2(out_f, gs_.onls_, gs_.msgs_);
    out_f <<"0\t"<< tp_start_ <<"\t"<< tpc << "\tTIMS\n";

    LOG << (tpc - tp_start_)/(60*60*24);
}

void statis::on_day_begin(std::ostream& out_f)
{
    time_t tpc = time(0);
    out_f <<"0\t"<< tp_start_ <<"\t"<< tpc << "\tTIMS\n";
}

void statis::flush(int fxa)
{
    auto day_end = boost::bind(&statis::on_day_end, this, _1);
    auto day_begin = boost::bind(&statis::on_day_begin, this, _1);
    auto& out_f = ofs_.make(&day_end, &day_begin);

    if (fxa) {
        report2(out_f, oml_);
    } else {
        gc(out_f);
    }

    writes(0, gs_.msgs_);

    summary2(out_f, gs_.onls_, gs_.msgs_);
    out_f.flush();
}

void statis::gc(std::ostream& out_f)
{
    time_t tpc = time(0);

    while (!oml_.empty())
    {
        auto & u = oml_.back();
        if (tpc - u.atime_ < 60*15) {
            break;
        }

        reportor rep(out_f);
        rep(u); // u.report(out_f);
        // s_init::save_msgs(u.uid, u);

        LOG << (tpc - u.atime_);
        oml_.pop_back();
    }
}

statis::onlmsg_t::onlmsg_t(UInt uid)
    : uid_( uid )
{
    onlrecs_.reserve(NONL_RESERVE);
    msgrecs_.reserve(NMSG_RESERVE);
}

void statis::onlmsg_t::loads(UInt uid)
{
    msgs_.rx_n_ = msgs_.rx_nfin_ = msgs_.rx_ndrop_ = msgs_.rx_nsockwrite = 0;
    msgs_.tx_n_ = msgs_.tx_nrx_ = 0;
}

void statis::onlmsg_t::saves(UInt uid)
{
    //rx_n_ , rx_nfin_ , rx_ndrop_ , rx_nsockwrite;
    //tx_n_ , tx_nrx_;
}

