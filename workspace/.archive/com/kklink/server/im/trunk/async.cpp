#include <string>
#include <list>
//#include <sstream>
#include <unordered_map>
//#include <unique_ptr>
//#include <boost/move/move.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/functional/hash.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/intrusive/detail/parent_from_member.hpp>

//#include <boost/function.hpp>
//#include <boost/smart_ptr.hpp>
//#include <boost/filesystem/fstream.hpp>
//#include <boost/container/deque.hpp>
//#include <boost/container/vector.hpp>
//#include <boost/container/static_vector.hpp>
//#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/ordered_index.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/identity.hpp>
//#include <boost/multi_index/composite_key.hpp>
#include "message.h"
#include "log.h"
#include "ioctx.h"
#include "async.h"
#include "statis.h"

// std::list<std::string> keyws_; // filter keywords

//struct socket_t { int fd; socket_t(int x) : fd(x) {} };
typedef boost::asio::ip::tcp::socket socket_t;
namespace ip = boost::asio::ip;

template <typename Int> Int hton(Int x)
{
    switch (sizeof(x)) {
        case 4: return htonl(x);
        case 2: return htons(x);
    }
    return x;
}
template <typename Int> Int ntoh(Int x) { return hton(x); }

template <typename IntIt, typename CharIt>
static void hex_copy(IntIt beg, IntIt end, CharIt iter)
{
    for (; beg != end; ++beg)
    {
        auto val = hton(*beg);
        char * p = static_cast<char*>(&val);
        char * ep = &p[sizeof(val)];
        for (; p != ep; ++p)
        {
            char h = (*p >> 4) & 0x0f;
            char l = (*p) & 0x0f;
            *iter++ = char((h) + ((h) < 10 ? '0' : 'a'-10));
            *iter++ = char((l) + ((l) < 10 ? '0' : 'a'-10));
        }
    }
}

template <typename I>
struct hex_output_iterator : boost::iterator_range<I>
{
    template <typename R>
    hex_output_iterator(R const& r)
        : boost::iterator_range<I>(r) {}

    friend std::ostream& operator<<(std::ostream& out, hex_output_iterator<I> r)
    {
        hex_copy(r.begin(), r.end(), std::ostream_iterator<char>(out));
        return out;
    }
};

template <typename I>
hex_output_iterator<I> make_xo_range(I beg, I end)
{
    return hex_output_iterator<I>(boost::make_iterator_range(beg,end));
}

nwbuffer::nwbuffer(size_t ncap)
{
    beg_ = 0;
    siz_ = capacity_ = 0;

    if (ncap > 0)
    {
        if (!reserve(ncap))
            ; // TODO
    }
}

bool nwbuffer::reserve(size_t ncap)
{
    if (ncap <= capacity_) {
        return 1;
    }

    size_t realcap = (ncap<=0x3ff) ? 0x400 : ((ncap + 0x3ff) & ~0x3ff);

    if (siz_ > 0) {
        LOG << ncap << realcap << (void*)beg_ << siz_ << capacity_;
        // LOG << make_xo_range(beg_, beg_ + std::min(siz_, size_t(16)));
    }

    if (void *ptr = realloc(beg_, realcap))
    {
        capacity_ = realcap;
        beg_ = static_cast<char*>(ptr);

        if ((beg_ - static_cast<char*>(0)) % sizeof(uint32_t)) {
            LOG << "bad align";
            LOG << "bad align";
            LOG << "bad align";
            // sleep(1); abort();
        }

        // if (siz_ > 0) LOG << make_xo_range(beg_, beg_ + std::min(siz_, size_t(16)));

        return 1;
    }

    LOG << realcap << "realloc fail";
    return 0;
}

nwbuffer::~nwbuffer() 
{
    if (beg_) {
        free(beg_);
    }
}

void nwbuffer::consume(size_t n)
{
    BOOST_ASSERT(n <= siz_);
    // n = std::min(n,siz_);
    if (n > 0) {
        siz_ -= n;
        memmove(beg_, beg_+n, siz_);
    }
}

// template <typename PMsg>
// int imsprotocol::parse(boost::iterator_range<char*>& p2, PMsg const& pfx)
// {
//     size_t nbyte, nbyte_ob = 0;
// 
// pos_parse_nxt_:
//     nbyte = bytes();
//     if (nbyte < bytes_of_header)
//     {
//         LOG << pfx << nbyte;
//         return bytes_of_header - nbyte;
//     }
// 
//     nbyte_ob = ntohl(*reinterpret_cast<uint32_t*>(begin()));
//     if (nbyte_ob == 0)
//     {
//         LOG << pfx;
//         consume(bytes_of_header);
//         goto pos_parse_nxt_;
//     }
// 
//     if (nbyte_ob > 1024)
//     {
//         LOG << pfx << nbyte_ob << "long msg";
//         if (nbyte_ob > 1024 * 96) {
//             LOG << pfx << "gt 96k fail";
//             return -16;
//         }
//     }
// 
//     if (nbyte < nbyte_ob + bytes_of_header)
//     {
//         LOG << pfx << nbyte_ob << nbyte;
//         reserve(nbyte_ob + bytes_of_header);
//         return (nbyte_ob + bytes_of_header) - nbyte;
//     }
// 
//     auto p = begin() + bytes_of_header;
//     LOG << pfx << nbyte_ob << boost::make_iterator_range(p, p + std::min(nbyte_ob,size_t(128)));
//     // jv = json::decode<json::object>(p, p + nbyte_ob).value();
// 
//     consume(bytes_of_header+nbyte_ob);
//     return 0;
// }

template <typename JVal, typename PMsg>
std::string imsprotocol::pack(JVal const& jv, PMsg const & pfx)
{
    // AUTO_CPU_TIMER(":pack:json:encode");
    union {
        uint32_t len;
        char s[sizeof(uint32_t)];
    } u;

    std::string tmp = json::encode(jv);
    u.len = htonl(tmp.size());

    LOG << pfx << tmp.size() << tmp;

    return std::string(u.s, u.s+sizeof(u)) + tmp;
}

im_server::im_server(boost::asio::io_service & io_s, ip::tcp::endpoint ep, message_handler_t h)
    : base_t(io_s, ep)
{
    message_handler_ = h;
}

void im_server::check_alive()
{
    time_t tpc = time(0);
    int cnt = 0;

    while (head_alive_)
    {
        socket_type* nd = head_alive_->prev_;
        if (tpc - nd->tpa_ < 60*3) {
            LOG << *nd << (tpc - nd->tpa_);
            break;
        }

        LOG << *nd <<"timeout"<< (nd->tpa_ - tpc);
        ++cnt;
        this->close(*nd, __LINE__,__FILE__); // unlinked
        BOOST_ASSERT(nd != head_alive_);
    }

    if (cnt > 1)
    {
        LOG << tpc << "close n" << cnt;
    }
}

static void emit_delay(UInt uid, bool onoff, bool extra)
{
    // LOG << uid << "signal" << onoff;
    emit(user_mgr::instance().ev_online, uid, onoff, extra);
}

socket_type* im_server::untag(socket_type& sk)
{
    UInt uid;
    if ( (uid = sk.tagval_) == 0) {
        LOG << sk;
        return 0;
    }

    socket_type* sk_p = 0;

    auto it = indices_.find(uid);
    if (it != indices_.end()) {
        sk_p = *it;
        if (sk_p == &sk) {
            emit(user_mgr::instance().ev_online, uid, 0, sk.is_closed());
            statis::instance().up_offline(uid, sk, sk.tp0_, sk.tpa_, 0);
            indices_.erase(it);
        } else {
            LOG << sk << "replaced by" << *sk_p;
        }
    } else {
        LOG << sk << "not exist";
    }

    return sk_p;
}

socket_type* im_server::tag2(socket_type& sk, UInt uid)
{
    BOOST_ASSERT(sk.tagval_ == 0);
    BOOST_ASSERT(uid > 0);
    if (uid == 0) {
        LOG << sk << uid << "fault";
        return 0;
    }
    if (sk.tagval_ > 0) {
        LOG << sk << uid << "fault 2";
        return 0;
    }
    sk.tp0_ = sk.tpa_ = time(0);

    boost::asio::io_service& io_s = sk.get_io_service();

    socket_type* sk_p = 0;

    sk.tagval_ = uid;

    auto i = indices_.find(uid);
    if (i != indices_.end()) {
        sk_p = *i;
        BOOST_ASSERT(sk_p != &sk);

        statis::instance().up_offline(uid, *sk_p, sk_p->tp0_, sk_p->tpa_, 1);
        indices_.erase(i);
        LOG << sk <<"replace"<< *sk_p;
    }

    indices_.insert(&sk);
    statis::instance().up_online(uid, sk);

    if (sk_p == 0) {
        io_s.post( boost::bind(&emit_delay, uid, 1, 0) );
        // emit(user_mgr::instance().ev_online, uid, 1, 0);
    }

    // warn: if sk_p not null, it should be closed by caller
    return sk_p;
}

void im_server::sendto(socket_type& sk, std::string && rsp)
{
    BOOST_ASSERT(!sk.is_closed());
    //LOG << sk << sk.stat();

    message_info mx;
    mx.data = boost::move(rsp); //imsprotocol::pack(rsp, sk);
    sk.obufs.insert(sk.obufs.end(), mx);

    try_next_push(sk); //(sk, 0, std::string());
}

void im_server::sendto(socket_type& sk, json::object const & rsp)
{
    return sendto(sk, imsprotocol::pack(rsp, sk));
}

void im_server::ack(socket_type& sk, UInt msgid, int real)
{
    if (sk.curmsg.is_fin()) {
        LOG << sk << "warn";
        return;
    }

    LOG << sk << sk.curmsg << msgid << real;

    if (sk.curmsg.msgid != msgid) {
        LOG << sk << sk.curmsg << msgid << "warn2";
        return;
    }

    sk.curmsg.finalize(0);

    std::pair<destination,message_ptr> p = message_mgr::instance().completed(sk.tagval_, msgid);

    if (sk.obufs.empty()) {
        if (p.second) {
            inflate(sk, p.first, p.second);
        }
    }
}

bool im_server::on_accepted(socket_type& s)
{
    namespace io = boost::asio;

    s.message_handler = message_handler_;

    s.input.reserve(256); // TODO 512
    auto buf = s.input.avail_buffer();
    // auto buf = io::buffer(b.first, b.second);
    async_read(s, buf, io::transfer_at_least(imsprotocol::bytes_of_header));

    return 1;
}

void im_server::handle_message(socket_type& sk, json::object const& jv, boost::iterator_range<char*> r)
{
    enum { Msgcmd_heartbeat = 91, Msgcmd_ack = 95, Msgcmd_auth = 99, Msgcmd_sendmsg=205 };
    enum { Msgcmd_listonline=71, Msgcmd_countonlines=72, Msgcmd_ping=73 };

    int cmd = json::as<int>(jv,"cmd").value_or(0);
    auto_cpu_timer_helper cput( str(boost::format(":handle_message(%1%)") % cmd) ); //AUTO_CPU_TIMER

    if (cmd < 90)
    {
        im_server const & ims = ims_server::instance();
        switch (cmd) {
            case Msgcmd_countonlines: {
                    json::object jo;
                    jo.emplace("count", boost::size(ims.indices_));
                    sendto(sk, imsprotocol::pack(jo, sk));
                }
                break;
            case Msgcmd_listonline: {
                    json::array ja;
                    BOOST_FOREACH(auto & x, ims.indices_) {
                        ja.push_back( boost::lexical_cast<std::string>(*x) );
                    }
                    sendto(sk, imsprotocol::pack(ja, sk));
                }
                break;
            case Msgcmd_ping: {
                    json::object jo;
                    jo.emplace("pid", int(getpid()));
                    sendto(sk, imsprotocol::pack(jo, sk));
                }
                break;
        }
        this->close(sk, __LINE__,__FILE__);
    }
    else if (cmd < 99)
    {
        switch (cmd)
        {
            case Msgcmd_ack: {
                    auto & body = json::as<json::object>(jv, "body").value();
                    auto msgid = json::as<UInt>(body,"msgid").value();
                    this->ack(sk, msgid, 1);
                    try_next_push(sk);
                }
                break;
        }
        if (sk.tagval_ == 0 && !sk.is_closed()) {
            LOG << sk << "would close";
        }
    }
    else
    {
        io_context ctx(*this, sk);
        (*sk.message_handler)(ctx, cmd, jv);
    }

    try {
        cput.stop();
        cput.report();
        auto es = cput.elapsed();
        LOG << sk << cmd << es.wall;
    } catch(...) {}
}

static int _parse(boost::iterator_range<char*>& ret, boost::iterator_range<char*> const s_rng)
{
    enum { bytes_of_header=4 };

    size_t rsiz = size(s_rng);
    ret = boost::make_iterator_range(s_rng.begin(),s_rng.begin());

    if (rsiz < bytes_of_header) {
        return int(bytes_of_header - rsiz);
    }

    union { uint32_t len; char s[4]; } un;
    un.s[0] = s_rng[0];
    un.s[1] = s_rng[1];
    un.s[2] = s_rng[2];
    un.s[3] = s_rng[3];
    size_t psiz = bytes_of_header + ntohl(un.len);

    if (rsiz < psiz) {
        return int(psiz - rsiz);
    }

    ret.advance_end( psiz );
    return 0;
}

boost::system::error_code im_server::on_read(socket_type& sk, boost::system::error_code const& ec, size_t nbyte)
{
    namespace io = boost::asio;
    if (ec) {
        return ec;
    }

    BOOST_ASSERT(nbyte >= 4);
    //LOG << sk << "bytes" << nbyte;
    sk.bytes_rx_ += nbyte;
    sk.input.commit(nbyte);

    try
    {
        int n_msg = 0;
        int nb_exp;
        boost::iterator_range<char*> pr, s_rng = sk.input.range();
        while ( (nb_exp = _parse(pr, s_rng)) == 0)
        {
            BOOST_ASSERT(size(pr) >= 4);
            pr.advance_begin(4);
            if (!empty(pr)) {
                boost::optional<json::object> jv = json::decode<json::object>( pr );
                if (!jv) {
                    LOG << sk << "json fail" << pr;
                    nb_exp = -9;
                    break;
                }
                LOG << sk << pr;
                handle_message(sk, jv.value(), pr);
            }
            s_rng = boost::make_iterator_range(boost::end(pr), boost::end(s_rng));
            ++n_msg;
        }

        if (nb_exp > (96*1024)) {
            LOG << sk << nb_exp << "gt 96k fail";
            return make_error_code(boost::system::errc::bad_message); // this->close(sk);
        } else if (nb_exp > 4096) {
            LOG << sk << nb_exp << "gt 4k";
        } else if (nb_exp < 0) {
            LOG << sk << nb_exp << "fail" << s_rng;
            return make_error_code(boost::system::errc::bad_message); // this->close(sk);
        }
        if (sk.is_closed()) {
            return ec;
        }

        if (boost::begin(s_rng) != sk.input.begin()) {
            sk.input.consume_to( boost::begin(s_rng) );

            sk.tpa_ = time(0);
            splice_front(&sk);

            if (n_msg > 1) {
                LOG << "n-msg" << n_msg;
            }
        }

        auto buf = sk.input.make_avail(nb_exp); // auto buf = sk.input.avail_buffer();
        BOOST_ASSERT(size_t(nb_exp) <= io::detail::buffer_size_helper(buf));
        // LOG << sk << nb_exp << "async read";
        async_read(sk, buf, io::transfer_at_least(nb_exp));

        return ec;
    } catch (myerror const& ex) {
        LOG << ex;
    } catch (std::exception const& ex) {
        LOG << "=except:" << ex.what();
    }

    return make_error_code(boost::system::errc::bad_message); // this->close(sk);
}

void im_server::on_write(socket_type& sk, boost::system::error_code const& ec, size_t nbyte)
{
    namespace io = boost::asio;

    if (ec) {
        LOG << sk << ec;
        return;
    }

    sk.bytes_tx_ += nbyte;

    BOOST_ASSERT (!sk.curmsg.is_fin());
    BOOST_ASSERT (nbyte == sk.curmsg.data.size());

    // LOG << sk << sk.curmsg;
    if (sk.curmsg.msgid)
    {
        if (sk.curmsg.ackreq) {
            LOG << sk << "ack need";
            wacks_.emplace_back(sk.tagval_, sk.curmsg.msgid);
            return;
        }

        this->ack(sk, sk.curmsg.msgid, 0);
        try_next_push(sk);
        // LOG << boost::posix_time::microsec_clock::local_time() << "TRACE";
        return;
    }

    sk.curmsg.finalize(0);
    try_next_push(sk); //(sk, 0, std::string());
}

void im_server::inflate(socket_type& sk, destination const& dst, message_ptr mptr)
{
    if (dst && mptr)
    {
        LOG << sk << mptr->id << dst << sk.obufs.empty();

        message_info mx(dst, mptr->id, mptr->flags.ack_required);
        mx.data = imsprotocol::pack(mptr->json(dst), sk);
        sk.obufs.insert(sk.obufs.end(), mx);
    }
}

void im_server::try_next_push(socket_type& sk)//(socket_type& sk, UInt msgid, std::string const& data)//(socket_type& sk, message_ptr mp)
{
    namespace io = boost::asio;

    if (sk.is_closed() || sk.is_writing()) {
        LOG << sk << sk.stat();
        return;
    }
    if (!sk.curmsg.is_fin()) {
        LOG << sk << sk.curmsg << "sending";
        return;
    }

    if (sk.obufs.empty()) {
        std::pair<destination,message_ptr> p = message_mgr::instance().front(sk.tagval_);
        if (p.second)
            inflate(sk, p.first, p.second);
    }

    if (!sk.obufs.empty())
    {
        sk.curmsg.init( sk.obufs.front() );
        sk.obufs.pop_front();

        LOG << sk << sk.curmsg;
        async_write(sk, io::buffer(sk.curmsg.data));
    }
}

void im_server::on_closed(socket_type& sk, boost::system::error_code const& ec)
{
    LOG << sk;
    if (UInt uid = sk.tagval_)
    { }
}

int im_server::check_tx()
{
    AUTO_CPU_TIMER("im_server:check_tx");
    time_t tpc = time(0);

    while (!wacks_.empty())
    {
        auto it = wacks_.begin();
        if (tpc - it->tp < 40) {
            break;
        }

        ack_info a = *it;
        wacks_.pop_front();

        if (socket_type *sk = finds(a.uid))
        {
            if (!sk->is_closed() && !sk->curmsg.is_fin())
            {
                if (auto msgid = sk->curmsg.msgid) {
                    if (msgid == a.msgid) {
                        LOG << *sk << sk->curmsg << "no ack close" << (tpc - sk->curmsg.tp0_);
                        close(*sk, __LINE__,__FILE__);
                    }
                }
            }
        }
    }

    return 10;
}

socket_type::curmsg_t::~curmsg_t()
{
    if (!is_fin_) {
        finalize(1);
    }
}

void socket_type::curmsg_t::finalize(int err)
{
    socket_type* sk = boost::intrusive::detail::parent_from_member(this, &socket_type::curmsg);
    UInt uid = sk->tagval_;
    time_t tpc = time(0);
    statis::instance().up_message(uid, msgid, tp0_, tpc, err);

    std::string().swap(data);
    is_fin_ = 1;
}

void socket_type::curmsg_t::init(message_info const& m)
{
    BOOST_ASSERT( this->is_fin_ );

    message_info & bm = *this;
    bm = m;

    tp0_ = time(0);
    this->is_fin_ = 0;
}

ims_server* ims_server::instance_;

void ims_server::check_alive()
{
    AUTO_CPU_TIMER("ims:check_alive");
    this->im_server::check_alive();
}

void ims_server::fev_regist(UInt uid, int regist)
{
    if (!regist) // on unregist
    {
        if (socket_type *sk = finds(uid)) {
            LOG << *sk << "unregist";
            close(*sk, __LINE__,__FILE__);
        }
    }
}

void ims_server::fev_message(destination dst, message_ptr mp)
{
    BOOST_ASSERT(mp);
    LOG << dst << *mp;

    if (socket_type* sk = finds(dst.uid))
    {
        if (sk->obufs.empty()) {
            inflate(*sk, dst, mp);
            try_next_push(*sk); //(*sk, mp->id, mp->pack(dst), mp->flags.ack_required);
        }
    }
}

void imbs_server::check_alive()
{
    AUTO_CPU_TIMER("imbs:check_alive");
    this->im_server::check_alive();
}

