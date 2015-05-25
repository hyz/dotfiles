#ifndef SERVER_H__
#define SERVER_H__

#include <string>
#include <list>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/container/deque.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/format.hpp>
#include "singleton.h"
#include "json.h"
#include "log.h"
#include "util.h"
#include "ioctx.h"

template <typename Derived, typename Socket>
struct server_base // : Protocol
{
    void start();

    void close(Socket& s, int ln, const char* fn);

protected:
    server_base(boost::asio::io_service& io_s, boost::asio::ip::tcp::endpoint endpoint);

    template <typename Buf>
    void async_read(Socket& s, Buf& buf)
    {
        using namespace boost;
        namespace placeholders = boost::asio::placeholders;
        s.reading(1);
        boost::asio::async_read(s, buf
                , bind(&server_base::handle_read, this
                    , &s, placeholders::error, placeholders::bytes_transferred)
                );
    }

    template <typename Buf, typename Pred>
    void async_read(Socket& s, Buf& buf, Pred pred)
    {
        using namespace boost;
        namespace placeholders = boost::asio::placeholders;
        s.reading( 1 );
        asio::async_read(s, buf, pred
                , bind(&server_base::handle_read, this
                    , &s, placeholders::error, placeholders::bytes_transferred)
                );
    }

    template <typename Buf>
    void async_write(Socket& s, Buf const & buf)
    {
        using namespace boost;
        namespace placeholders = boost::asio::placeholders;
        s.writing( 1 );
        boost::asio::async_write(s, buf
                , bind(&server_base::handle_write, this
                    , &s, placeholders::error, placeholders::bytes_transferred));
    }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    Socket* sock0_;
    boost::object_pool<Socket> pool_; 

protected:
    // boost::container::deque<Socket*> sockets_;
    Socket* head_alive_;

    void link_front(Socket* nd)
    {
        BOOST_ASSERT(nd->prev_ == 0 && nd->next_ == 0);
        if (head_alive_)
        {
            nd->prev_ = head_alive_->prev_;
            nd->next_ = head_alive_;
            head_alive_->prev_->next_ = nd;
            head_alive_->prev_ = nd;
        }
        else
        {
            nd->next_ = nd->prev_ = nd;
        }
        head_alive_ = nd;
    }

    void unlink(Socket* nd)
    {
        if (!nd->next_)
            return;

        BOOST_ASSERT (head_alive_);
        nd->next_->prev_ = nd->prev_;
        nd->prev_->next_ = nd->next_;

        if (head_alive_ == nd)
            if ( (head_alive_ = nd->next_) == nd)
                head_alive_ = 0;
        nd->next_ = nd->prev_ = 0;
    }

    void splice_front(Socket* nd)
    {
        BOOST_ASSERT (head_alive_);
        BOOST_ASSERT (nd && nd->prev_ && nd->next_);
        if (nd != head_alive_)
        {
            unlink(nd);
            link_front(nd);
        }
    }

private:
    void handle_connected(boost::system::error_code const& ec);
    void handle_read(Socket* s, boost::system::error_code const& ec, std::size_t bytes_transferred);
    void handle_write(Socket* s, boost::system::error_code const& ec, std::size_t bytes_transferred);
    void loopend(boost::system::error_code const& ec, Socket* s, int iofx);
};

template <typename Derived, typename Socket>
void server_base<Derived,Socket>::close(Socket& sk, int ln, const char* fn)
{
    BOOST_ASSERT(!sk.is_closed());
    LOG << sk << sk.stat() << boost::format("%1%:%2%") % ln % fn;

    if (sk.is_closed()) {
        LOG << "rep-close";
        return;
    }
    sk.closed(1);

    unlink(&sk);

    if (sk.tagval_ > 0) {
        static_cast<Derived*>(this)->untag(sk);
    }

    if (sk.is_writing()) {
        return;
    }

    if (sk.is_reading()) {
        LOG << sk << "cancel";
        boost::system::error_code ec;
        sk.cancel(ec);
    }
}

template <typename Derived, typename Socket>
void server_base<Derived,Socket>::start()
{
    namespace placeholders = boost::asio::placeholders;
    using namespace boost;

    acceptor_.listen();
    LOG << "Listen" << acceptor_.local_endpoint();

    acceptor_.async_accept(*sock0_, bind(&server_base::handle_connected, this, placeholders::error));
}

template <typename Derived, typename Socket>
server_base<Derived,Socket>::server_base(boost::asio::io_service& io_s, boost::asio::ip::tcp::endpoint endpoint)
    : acceptor_(io_s)
{
    namespace ip = boost::asio::ip;

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);

    head_alive_ = 0;
    sock0_ = pool_.construct(io_s);
}

template <typename Derived, typename Socket>
void server_base<Derived,Socket>::handle_connected(boost::system::error_code const& ec0)
{
    namespace ip = boost::asio::ip;
    namespace placeholders = boost::asio::placeholders;

    if (ec0) {
        LOG << "accepted" << ec0;
        return;
    }
    // AUTO_CPU_TIMER(":handle_connected");

    LOG << *sock0_ << "accepted";

    if (static_cast<Derived*>(this)->on_connected(*sock0_))
    {
        sock0_->set_option(ip::tcp::socket::keep_alive(true));
        sock0_->set_option(ip::tcp::no_delay(true));

        link_front(sock0_);
    } else {
        LOG << "not-accepted";
        pool_.destroy(sock0_);
    }

    sock0_ = pool_.construct(acceptor_.get_io_service());
    acceptor_.async_accept(*sock0_, bind(&server_base::handle_connected, this, placeholders::error));
}

template <typename Derived, typename Socket>
void server_base<Derived,Socket>::handle_read(Socket* sk, boost::system::error_code const& erc, std::size_t bytes_transferred)
{
    AUTO_CPU_TIMER(":handle_read");
    sk->reading( 0 );

    try
    {
        boost::system::error_code ec = erc;
        if (!sk->is_closed()) {
            ec = static_cast<Derived*>(this)->on_read(*sk, erc, bytes_transferred);
            if (ec) {
                BOOST_ASSERT(!sk->is_reading());
                if (sk->is_reading()) {
                    LOG << *sk << "should not reach";
                }
                if (ec == boost::asio::error::eof) {
                    ec = boost::system::error_code();
                }
            }
        }

        loopend(ec, sk, 0);
        return;

    } catch (myerror const& ex) {
        LOG << ex;
    } catch (std::exception const& ex) {
        LOG << "=except:" << ex.what();
    }

    loopend(make_error_code(boost::system::errc::bad_message), sk, 0);
}

template <typename Derived, typename Socket>
void server_base<Derived,Socket>::handle_write(Socket* s, boost::system::error_code const& ec, std::size_t bytes_transferred)
{
    s->writing( 0 );
    AUTO_CPU_TIMER(":handle_write");
    try
    {
        // LOG << *s << bytes_transferred;

        if (!s->is_closed()) {
            static_cast<Derived*>(this)->on_write(*s, ec, bytes_transferred);
        }

        if (!s->omsgbuf_.is_fin()) {
            s->omsgbuf_.finalize(0);
            // return;
        }
        if (!s->is_closed()) {
            static_cast<Derived*>(this)->try_next_push(*s); //(sk, 0, std::string());
        }

        loopend(ec, s, 1);
        return;

    } catch (myerror const& ex) {
        LOG << ex;
    } catch (std::exception const& ex) {
        LOG << "=except:" << ex.what();
    }

    loopend(make_error_code(boost::system::errc::bad_message), s, 1);
}

template <typename Derived, typename Socket>
void server_base<Derived,Socket>::loopend(boost::system::error_code const& errc, Socket* s, int iofx)
{
    BOOST_ASSERT(s);
    if (errc) { // (io::error::eof)
        LOG << *s << errc << errc.message() << iofx;

        if (s->is_reading() || s->is_writing()) {
            LOG << *s << "cancel" << errc;
            boost::system::error_code ec;
            s->cancel(ec);
            return;
        }
    }
    LOG << *s << s->stat();

    s->tpa_ = time(0);
    if (s->is_writing()) {
        return;
    }

    if (s->is_reading()) {
        if (s->is_closed()) {
            LOG << *s << "cancel";
            boost::system::error_code ec;
            s->cancel(ec);
        }
        return;
    }

    LOG << *s << "... destroy";

    if (!s->is_closed()) {
        if (s->next_) {
            unlink(s);
        }

        if (s->tagval_ > 0) {
            static_cast<Derived*>(this)->untag(*s);
        }

        static_cast<Derived*>(this)->on_closed(*s, errc);
    }

    pool_.destroy(s);
}

template <typename Socket, typename UserTag>
struct socket_base : boost::asio::ip::tcp::socket, boost::noncopyable
{
    typedef socket_base<Socket, UserTag> ThisType;

    socket_base(boost::asio::io_service & io_s)
        : boost::asio::ip::tcp::socket(io_s)
        , tagval_()
    {
        tp0_ = tpa_ = time(0);
        next_ = prev_ = 0; //static_cast<Socket*>(this);
    }

    enum { XF_Closed = 0x01 , XF_Reading = 0x02 , XF_Writing = 0x04 };
    /// little-endian
    enum { XF_b = (sizeof(int)-1) };

    inline bool is_closed() const { return (xflag_.v[XF_b] & XF_Closed); }
    inline bool is_writing() const { return (xflag_.v[XF_b] & XF_Writing); }
    inline bool is_reading() const { return (xflag_.v[XF_b] & XF_Reading); }
    inline void reading(bool y) { y ? (xflag_.v[XF_b] |= XF_Reading) : (xflag_.v[XF_b] &= ~XF_Reading); }
    inline void writing(bool y) { y ? (xflag_.v[XF_b] |= XF_Writing) : (xflag_.v[XF_b] &= ~XF_Writing); }
    inline void closed (bool y) { y ? (xflag_.v[XF_b] |= XF_Closed) : (xflag_.v[XF_b] &= ~XF_Closed); }

    inline void reference_incr() { xflag_.count++; }
    inline UInt reference_decr() { return ((xflag_.count--) & 0x00ffffff); }

    union XFlag {
        unsigned int count;
        unsigned char v[sizeof(int)];
        XFlag() { count=0; }
    } xflag_;

    UserTag tagval_;
    time_t tp0_, tpa_;

    Socket *next_;
    Socket *prev_;

    struct stat_print {
        ThisType const* thiz;
        stat_print(ThisType const*p) { thiz=p; }
        friend std::ostream& operator<<(std::ostream & out, stat_print const& s) {
            return out << s.thiz->is_closed() << s.thiz->is_reading() << s.thiz->is_writing();
        }
    };
    stat_print stat() const { return stat_print(this); }

private:
    void close();
};

struct nwbuffer : boost::noncopyable
{
    explicit nwbuffer(size_t capacity=0);

    void commit(size_t n)
    {
        size_t navail = capacity_ - siz_;
        siz_ += std::min(navail, n);
    }

    boost::asio::mutable_buffers_1 avail_buffer() const
    {
        return boost::asio::buffer(beg_ + siz_, capacity_ - siz_);
    }
    boost::asio::mutable_buffers_1 make_avail(size_t nb)
    {
        if (!reserve(siz_ + nb))
            ; // THROW
        return avail_buffer();
    }

    bool reserve(size_t nc);

    char *begin() const { return beg_; }
    char *end() const { return beg_ + siz_; }

    boost::iterator_range<char*> range() const { return boost::make_iterator_range(begin(), end()); }

    void consume(size_t n);
    void consume_to(char *pos)
    {
        BOOST_ASSERT(begin() <= pos && pos <= end());
        return consume(pos - begin()); //(end() - pos);
    }

protected:
    ~nwbuffer();

    nwbuffer(nwbuffer && rhs)
    {
        beg_ = rhs.beg_;
        siz_ = rhs.siz_;
        capacity_ = rhs.capacity_;
        rhs.beg_ = 0;
        rhs.siz_ = 0;
        rhs.capacity_ = 0;
    }

    size_t bytes(int x=0) const { return (siz_ + x); }

private:
    char *beg_;
    size_t siz_;
    size_t capacity_;

   // BOOST_MOVABLE_BUT_NOT_COPYABLE(file_descriptor)
};

struct protobuf : nwbuffer
{
    enum { bytes_of_header=4 };

    //template <typename PMsg> int parse(json::object & jv, boost::iterator_range<char*>& p2, PMsg const& pfx);

    static std::string pack(int mid, json::object const& jv);
    static std::string pack(int mid, json::array const& jv);
    static std::string pack(int mid, std::string const& buf);

    protobuf()
    {}

    protobuf(protobuf && rhs)
        : nwbuffer(std::move(rhs))
    {}
};

struct omessage_buf : std::string
{
    omessage_buf()
    {}
    omessage_buf(std::string const& d)
        : std::string(d)
    {}
    // ~omessage_buf() { boost::intrusive::list_member_hook<>::node_algorithms::inited( &node_retx_ ); }

    char const* data() const { return this->std::string::data(); }
    size_t size() const { return this->length(); }
};

struct socket_type : socket_base<socket_type,UInt>
{
    socket_type(boost::asio::io_service & io_s)
        : socket_base(io_s)
    {
        flags = 0;
        bytes_tx_ = bytes_rx_ = 0;
    }

    //omessage_buf front() const { ;; }
    //void pop_front() { ;; }

    struct writing_msgbuf : omessage_buf
    {
        time_t tp0_;

        ~writing_msgbuf();

        void init(omessage_buf const& m);
        void finalize(int err);
        bool is_fin() const { return empty(); }

        friend std::ostream& operator<<(std::ostream & out, writing_msgbuf const& m) {
            if (m.is_fin() || m.size() <= 5) {
                return out;
            }
            out.write(m.data()+5, m.size()-5);
            return out;
        }
    };
    writing_msgbuf omsgbuf_;

    protobuf protobuf_;
    std::list<omessage_buf> obufs;

    message_handler_t message_handler;
    UInt flags; // user
    UInt bytes_tx_, bytes_rx_;

    struct lnode_retx : boost::intrusive::list_member_hook<>
    {
        time_t tp;
    } node_retx_;

    friend std::ostream& operator<<(std::ostream & out, socket_type const& s)
    {
        boost::system::error_code ec; // boost::asio::ip::tcp::endpoint ep = ;
        return out << s.tagval_ <<"/"<< s.remote_endpoint(ec); //const_cast<socket_type&>(s).native_handle();
    }
};
// void intrusive_ptr_add_ref(socket_type * m);
// void intrusive_ptr_release(socket_type * m);
inline void intrusive_ptr_add_ref(socket_type * m)
{
    m->reference_incr();
}
inline void intrusive_ptr_release(socket_type * m)
{
    if (m->reference_decr() == 0) {
        LOG << "free" << *m;
    }
}
typedef boost::intrusive_ptr<socket_type> socket_ptr;

struct hash_sox {
    inline size_t operator()(socket_type* sk) const { return boost::hash_value(sk->tagval_); }
};

struct xkey_tag
{
    typedef UInt result_type; 
    result_type operator()(socket_type* sc) const { return sc->tagval_; }
};

struct racing_server : server_base<racing_server, socket_type>, boost::noncopyable
{
    typedef server_base<racing_server, socket_type> base_t;

    racing_server(boost::asio::io_service & io_s, boost::asio::ip::tcp::endpoint ep, message_handler_t h);

    socket_type* tag2(socket_type& sk, UInt idx);
    socket_type* untag(socket_type& sk);
    UInt get_tag(socket_type& sk) const { return sk.tagval_; }

    socket_type* findtag(UInt uid) const
    {
        auto i = tagidxs_.find(uid);
        if (i != tagidxs_.end())
            return *i;
        return 0;
    }

    void sendto(socket_type& sk, int mid, json::array const& rsp)
    {
        return mysend(sk, protobuf::pack(mid, rsp));
    }
    void sendto(socket_type& sk, int mid, json::object const& rsp)
    {
        return mysend(sk, protobuf::pack(mid, rsp));
    }
    void sendto(socket_type& sk, int mid, std::string const& rsp)
    {
        return mysend(sk, protobuf::pack(mid, rsp));
    }

protected:
private:
    friend base_t;

    void mysend(socket_type& sk, std::string && rsp)
    {
        sk.obufs.push_back( omessage_buf(rsp) );
        try_next_push(sk);
    }

    bool on_connected(socket_type& s);
    void on_write(socket_type& sk, boost::system::error_code const& ec, size_t nbyte);
    void on_closed(socket_type& sk, boost::system::error_code const& ec);
    boost::system::error_code on_read(socket_type& sk, boost::system::error_code ec, size_t nbyte);

private:
    void try_next_push(socket_type& sk);
    void handle_message(socket_type& sk, int cmd, boost::iterator_range<char*> r);

    message_handler_t message_handler_;

    boost::multi_index_container<
        socket_type*,
        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<xkey_tag> >
    > tagidxs_;

    // struct ack_info
    // {
    //     // int fd;
    //     UInt uid;
    //     UInt msgid;
    //     time_t tp;
    //     ack_info(UInt u, UInt mid) { uid=u; msgid=mid; tp=time(0); }
    // };
    // std::deque<ack_info> wacks_;
    // boost::asio::deadline_timer timer_tx_;

    typedef boost::intrusive::list<
            socket_type, boost::intrusive::member_hook<
                socket_type, socket_type::lnode_retx, &socket_type::node_retx_
                // socket_type, boost::intrusive::list_member_hook<>, &socket_type::node_retx_
            >
        > list_retx_t;

    list_retx_t l_retx_;

public:
    int check_tx();
    void check_alive();
};

//
// yuexia iPhone/Android client server
//
struct cs_server : singleton<cs_server>, racing_server
{
    cs_server(boost::asio::io_service & io_s, boost::asio::ip::tcp::endpoint ep, message_handler_t h)
        : racing_server(io_s, ep, h)
    {}

    void start()
    {
        return this->racing_server::start();
    }

    void check_alive()
    {
        // AUTO_CPU_TIMER("as:check_alive");
        this->racing_server::check_alive();
    }
};

//
// browser/flash server
//
struct bs_server : singleton<bs_server>, racing_server
{
    bs_server(boost::asio::io_service & io_s, boost::asio::ip::tcp::endpoint ep, message_handler_t h)
        : racing_server(io_s, ep, h)
    {}

    void start()
    {
        return this->racing_server::start();
    }

    void check_alive()
    {
        // AUTO_CPU_TIMER("bs:check_alive");
        this->racing_server::check_alive();
    }
};

#endif

