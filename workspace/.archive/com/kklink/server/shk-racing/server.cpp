#include <string>
#include <list>
//#include <sstream>
#include <unordered_map>
//#include <unique_ptr>
//#include <boost/move/move.hpp>
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
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

//#include <boost/function.hpp>
//#include <boost/filesystem/fstream.hpp>
//#include <boost/container/deque.hpp>
//#include <boost/container/vector.hpp>
//#include <boost/container/static_vector.hpp>
//#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/ordered_index.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/identity.hpp>
//#include <boost/multi_index/composite_key.hpp>

// #include "message.h"
#include "log.h"
#include "ioctx.h"
// #include "statis.h"
#include "server.h"

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
// int protobuf::parse(boost::iterator_range<char*>& p2, PMsg const& pfx)
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

std::string protobuf::pack(int mid, std::string const& b)
{
    union {
        uint32_t len;
        char s[sizeof(uint32_t)];
    } u;
    u.len = htonl(b.size());
    // LOG << pfx << b.size() << b;
    return std::string(1, char(mid)) + std::string(u.s, u.s+sizeof(u)) + b;
}

std::string protobuf::pack(int mid, json::object const& jv)
{
    return protobuf::pack(mid, json::encode(jv));
}
std::string protobuf::pack(int mid, json::array const& jv)
{
    return protobuf::pack(mid, json::encode(jv));
}

racing_server::racing_server(boost::asio::io_service & io_s, ip::tcp::endpoint ep, message_handler_t h)
    : base_t(io_s, ep)
{
    message_handler_ = h;
}

void racing_server::check_alive()
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
    // emit(user_mgr::instance().ev_online, uid, onoff, extra);
}

socket_type* racing_server::untag(socket_type& sk)
{
    UInt uid;
    if ( (uid = sk.tagval_) == 0) {
        LOG << sk;
        return 0;
    }

    socket_type* sk_p = 0;

    auto it = tagidxs_.find(uid);
    if (it != tagidxs_.end()) {
        sk_p = *it;
        if (sk_p == &sk) {
            // emit(user_mgr::instance().ev_online, uid, 0, sk.is_closed());
            // statis::instance().up_offline(uid, sk, sk.tp0_, sk.tpa_, 0);
            tagidxs_.erase(it);
        } else {
            LOG << sk << "replaced, owner is" << *sk_p;
        }
    } else {
        LOG << sk << "not exist";
    }

    return sk_p;
}

socket_type* racing_server::tag2(socket_type& sk, UInt uid)
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

    auto i = tagidxs_.find(uid);
    if (i != tagidxs_.end()) {
        sk_p = *i;
        BOOST_ASSERT(sk_p != &sk);

        // statis::instance().up_offline(uid, *sk_p, sk_p->tp0_, sk_p->tpa_, 1);
        tagidxs_.erase(i);
        LOG << sk <<"replace"<< *sk_p;
    }

    tagidxs_.insert(&sk);
    // statis::instance().up_online(uid, sk);

    if (sk_p == 0) {
        io_s.post( boost::bind(&emit_delay, uid, 1, 0) );
        // emit(user_mgr::instance().ev_online, uid, 1, 0);
    }

    // warn: if sk_p not null, it should be closed by caller
    return sk_p;
}

// void racing_server::ack(socket_type& sk, UInt msgid, int real)
// {
//     if (sk.omsgbuf_.is_fin()) {
//         LOG << sk << "warn";
//         return;
//     }
// 
//     LOG << sk << sk.omsgbuf_ << msgid << real;
// 
//     if (sk.omsgbuf_.msgid != msgid) {
//         LOG << sk << sk.omsgbuf_ << msgid << "warn2";
//         return;
//     }
// 
//     sk.omsgbuf_.finalize(0);
// 
//     user_message p = message_completed(sk.tagval_, msgid);
// 
//     if (sk.obufs.empty()) {
//         if (p.second) {
//             inflate(sk, p.first, p.second);
//         }
//     }
// }

bool racing_server::on_connected(socket_type& s)
{
    namespace io = boost::asio;

    s.message_handler = message_handler_;

    s.protobuf_.reserve(256); // TODO 512
    auto buf = s.protobuf_.avail_buffer();
    // auto buf = io::buffer(b.first, b.second);
    async_read(s, buf, io::transfer_at_least(protobuf::bytes_of_header));

    return 1;
}

void racing_server::handle_message(socket_type& sk, int cmd, boost::iterator_range<char*> r)
{
    auto_cpu_timer_helper cput( str(boost::format(":handle_message(%1%)") % cmd) ); //AUTO_CPU_TIMER
    try
    {
        io_context ctx(*this, sk);
        (*sk.message_handler)(ctx, cmd, std::string(r.begin(), r.end()));

        cput.stop();
        cput.report();

    } catch(...) {}

    boost::timer::cpu_times es = cput.elapsed();
    LOG << sk << cmd << es.wall;
}

struct msgheader
{
    unsigned char cmd;
    size_t len;
    msgheader() { cmd=0; len=0; }
};

static int _parse(msgheader& h, boost::iterator_range<char*>& ret, boost::iterator_range<char*> const rpbuf)
{
    enum { bytes_of_header=5 };

    size_t rngsiz = size(rpbuf);

    if (rngsiz < bytes_of_header) {
        return int(bytes_of_header - rngsiz);
    }

    union { uint32_t len; char s[4]; } un;
    un.s[0] = rpbuf[1];
    un.s[1] = rpbuf[2];
    un.s[2] = rpbuf[3];
    un.s[3] = rpbuf[4];
    un.len = ntohl(un.len);

    size_t psiz = bytes_of_header + un.len;

    if (rngsiz < psiz) {
        return int(psiz - rngsiz);
    }

    ret = boost::make_iterator_range(rpbuf.begin()+bytes_of_header, rpbuf.begin()+psiz); // ret.advance_end( psiz );
    h.cmd = rpbuf[0];
    h.len = un.len;
    return 0;
}

boost::system::error_code racing_server::on_read(socket_type& sk, boost::system::error_code ec, size_t nbyte)
{
    namespace io = boost::asio;
    if (ec) {
        return ec;
    }

    BOOST_ASSERT(nbyte >= 4);
    LOG << sk << "bytes" << nbyte;
    sk.bytes_rx_ += nbyte;
    sk.protobuf_.commit(nbyte);

    try
    {
        int n_msg = 0;
        int nb_exp;
        boost::iterator_range<char*> rpbuf = sk.protobuf_.range();
        boost::iterator_range<char*> body;
        msgheader h;
        while ( (nb_exp = _parse(h, body, rpbuf)) == 0)
        {
            handle_message(sk, h.cmd, body);

            rpbuf = boost::make_iterator_range(boost::end(body), boost::end(rpbuf));
            ++n_msg;
        }

        if (nb_exp > (96*1024)) {
            LOG << sk << nb_exp << "gt 96k fail";
            return make_error_code(boost::system::errc::bad_message); // this->close(sk);
        } else if (nb_exp > 4096) {
            LOG << sk << nb_exp << "gt 4k";
        } else if (nb_exp < 0) {
            LOG << sk << nb_exp << "fail" << rpbuf;
            return make_error_code(boost::system::errc::bad_message); // this->close(sk);
        }

        if (sk.is_closed()) {
            return boost::system::error_code();
        }

        if (boost::begin(rpbuf) != sk.protobuf_.begin()) {
            sk.protobuf_.consume_to( boost::begin(rpbuf) );

            // sk.tpa_ = time(0);
            splice_front(&sk);

            if (n_msg > 1) {
                LOG << "n-msg" << n_msg;
            }
        }

        auto buf = sk.protobuf_.make_avail(nb_exp); // auto buf = sk.input.avail_buffer();
        BOOST_ASSERT(size_t(nb_exp) <= io::detail::buffer_size_helper(buf));
        LOG << sk << "async read" << nb_exp;
        async_read(sk, buf, io::transfer_at_least(nb_exp));

        return boost::system::error_code();
    } catch (myerror const& ex) {
        LOG << ex;
    } catch (std::exception const& ex) {
        LOG << "=except:" << ex.what();
    }

    return make_error_code(boost::system::errc::bad_message); // this->close(sk);
}

void racing_server::on_write(socket_type& sk, boost::system::error_code const& ec, size_t nbyte)
{
    namespace io = boost::asio;

    LOG << sk << "bytes" << nbyte << ec;
    if (ec) {
        return;
    }

    sk.bytes_tx_ += nbyte;

    BOOST_ASSERT (!sk.omsgbuf_.is_fin());
    BOOST_ASSERT (nbyte == sk.omsgbuf_.size());

    LOG << sk << sk.omsgbuf_;
}

// void racing_server::inflate(socket_type& sk, destination const& dst, message_ptr mptr)
// {
//     if (dst && mptr)
//     {
//         LOG << sk << mptr->id << dst << sk.obufs.empty();
// 
//         omessage_buf mx(dst, mptr->id, mptr->flags.ack_required);
//         mx.data = protobuf::pack(mptr->json(dst), sk);
//         sk.obufs.insert(sk.obufs.end(), mx);
//     }
// }

void racing_server::try_next_push(socket_type& sk)//(socket_type& sk, UInt msgid, std::string const& data)//(socket_type& sk, message_ptr mp)
{
    namespace io = boost::asio;

    if (sk.is_closed()) {
        LOG << sk << sk.stat() << "closed";
        return;
    }
    if (sk.is_writing()) {
        LOG << sk << sk.stat();
        return;
    }
    if (!sk.omsgbuf_.is_fin()) {
        LOG << sk << sk.omsgbuf_ << "sending";
        return;
    }

    //if (sk.obufs.empty()) {
    //    omessage_buf const& m = sk.front();
    //    sk.obufs.push_back(m);
    //}

    if (!sk.obufs.empty())
    {
        sk.omsgbuf_.init( sk.obufs.front() );
        sk.obufs.pop_front();

        LOG << sk << sk.omsgbuf_;
        async_write(sk, io::buffer(sk.omsgbuf_.data(), sk.omsgbuf_.size()));
    }
}

void racing_server::on_closed(socket_type& sk, boost::system::error_code const& ec)
{
    LOG << sk;
    if (UInt uid = get_tag(sk))
    {
        char* p=0;
        handle_message(sk, 0xff, boost::iterator_range<char*>(p,p));
    }
}

//int racing_server::check_tx()
//{
//    AUTO_CPU_TIMER("racing_server:check_tx");
//    time_t tpc = time(0);
//
//    while (!wacks_.empty())
//    {
//        auto it = wacks_.begin();
//        if (tpc - it->tp < 40) {
//            break;
//        }
//
//        ack_info a = *it;
//        wacks_.pop_front();
//
//        if (socket_type *sk = findtag(a.uid))
//        {
//            if (!sk->is_closed() && !sk->omsgbuf_.is_fin())
//            {
//                if (auto msgid = sk->omsgbuf_.msgid) {
//                    if (msgid == a.msgid) {
//                        LOG << *sk << sk->omsgbuf_ << "no ack close" << (tpc - sk->omsgbuf_.tp0_);
//                        close(*sk, __LINE__,__FILE__);
//                    }
//                }
//            }
//        }
//    }
//
//    return 10;
//}

socket_type::writing_msgbuf::~writing_msgbuf()
{
    if (!is_fin()) {
        finalize(1);
    }
}

void socket_type::writing_msgbuf::finalize(int err)
{
    socket_type* sk = boost::intrusive::detail::parent_from_member(this, &socket_type::omsgbuf_);

    //UInt uid = sk->tagval_;
    //time_t tpc = time(0);
    //statis::instance().up_message(uid, msgid, tp0_, tpc, err);

    std::string& self = *this;
    std::string().swap(self);
}

void socket_type::writing_msgbuf::init(omessage_buf const& m)
{
    BOOST_ASSERT( is_fin() );

    omessage_buf & base = *this;
    base = m;
    tp0_ = time(0);
}

// void cs_server::fev_regist(UInt uid, int regist)
// {
//     if (!regist) // on unregist
//     {
//         if (socket_type *sk = findtag(uid)) {
//             LOG << *sk << "unregist";
//             close(*sk, __LINE__,__FILE__);
//         }
//     }
// }
// void cs_server::fev_message(destination dst, message_ptr mp)
// {
//     BOOST_ASSERT(mp);
//     LOG << dst << *mp;
// 
//     if (socket_type* sk = findtag(dst.uid))
//     {
//         if (sk->obufs.empty()) {
//             inflate(*sk, dst, mp);
//             try_next_push(*sk); //(*sk, mp->id, mp->pack(dst), mp->flags.ack_required);
//         }
//     }
// }

