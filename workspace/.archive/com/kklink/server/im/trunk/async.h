#ifndef ASYNC_H__
#define ASYNC_H__

#include <string>
#include <list>
//#include <unordered_set>
//#include <boost/move/move.hpp>
//#include <boost/move/utility.hpp>
//#include <boost/filesystem/fstream.hpp>
//#include <boost/container/vector.hpp>
//#include <boost/container/static_vector.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/container/deque.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/identity.hpp>
//#include <boost/multi_index/composite_key.hpp>
//#include <boost/functional/hash.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/format.hpp>
#include "message.h"
#include "ioctx.h"
#include "json.h"
#include "log.h"
#include "util.h"

//// std::ostream & operator<<(std::ostream & out, boost::asio::ip::tcp::socket const & s);
// template <typename Out>
// Out & operator<<(Out & out, boost::asio::ip::tcp::socket const & s)
// {
//     boost::system::error_code ec;
//     boost::asio::ip::tcp::endpoint endp = s.remote_endpoint(ec);
//     return out << endp;
//     //return out << s.native_handle() <<","<< endp;
// }

template <typename protocol_t, typename socket_t>
struct async_nwk // : Protocol
{
    void start();

    void close(socket_t& s, int ln, const char* fn);

protected:
    async_nwk(boost::asio::io_service& io_s, boost::asio::ip::tcp::endpoint endpoint);

    template <typename Buf>
    void async_read(socket_t& s, Buf& buf)
    {
        using namespace boost;
        namespace placeholders = boost::asio::placeholders;
        // asio::buffer(&len_.chrs[0], sizeof(len_))
        // bind(&handle_size, shared_from_this(), asio::placeholders::error));
        s.reading(1);
        boost::asio::async_read(s, buf
                , bind(&async_nwk::handle_read, this
                    , &s, placeholders::error, placeholders::bytes_transferred)
                );
    }

    template <typename Buf, typename Pred>
    void async_read(socket_t& s, Buf& buf, Pred pred)
    {
        using namespace boost;
        namespace placeholders = boost::asio::placeholders;
        // asio::buffer(&len_.chrs[0], sizeof(len_))
        // bind(&handle_size, shared_from_this(), asio::placeholders::error));
        s.reading( 1 );
        asio::async_read(s, buf, pred
                , bind(&async_nwk::handle_read, this
                    , &s, placeholders::error, placeholders::bytes_transferred)
                );
    }

    template <typename Buf>
    void async_write(socket_t& s, Buf const & buf)
    {
        using namespace boost;
        namespace placeholders = boost::asio::placeholders;
        // asio::buffer(ls_.front())
        // bind(&writeb, s, asio::placeholders::error)
        s.writing( 1 );
        boost::asio::async_write(s, buf
                , bind(&async_nwk::handle_write, this
                    , &s, placeholders::error, placeholders::bytes_transferred));
    }

    // void read_until(socket_t s, buf, delim, fn)
    // {
    //     auto & sk = socket_index_[s.fd];
    //     asio::async_read_until(sk, buf, delim, fn);
    //     //bind(&handle, , asio::placeholders::error));
    // }

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    socket_t* sock0_;
    boost::object_pool<socket_t> pool_; 

protected:
    // boost::container::deque<socket_t*> sockets_;
    socket_t* head_alive_;

    void link_front(socket_t* nd)
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

    void unlink(socket_t* nd)
    {
        BOOST_ASSERT (nd);
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

    void splice_front(socket_t* nd)
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
    void handle_accept(boost::system::error_code const& ec);

    void handle_read(socket_t* s, boost::system::error_code const& ec, std::size_t bytes_transferred);

    void handle_write(socket_t* s, boost::system::error_code const& ec, std::size_t bytes_transferred);

    void loopend(boost::system::error_code const& ec, socket_t* s, int iofx);
};

template <typename protocol_t, typename socket_t>
void async_nwk<protocol_t,socket_t>::close(socket_t& sk, int ln, const char* fn)
{
    BOOST_ASSERT(!sk.is_closed());
    LOG << sk << sk.stat() << sk.tagval_ << boost::format("%1%:%2%") % ln % fn;

    if (sk.is_closed()) {
        LOG << "rep-close";
        return;
    }
    sk.closed(1);

        //LOG << sk.tagval_;
    unlink(&sk);

        //LOG << sk.tagval_;
    if (sk.tagval_ > 0) {
        static_cast<protocol_t*>(this)->untag(sk);
    }

        //LOG << sk.tagval_;
    if (sk.is_writing()) {
        return;
    }

        //LOG << sk.tagval_;
    if (sk.is_reading()) {
        LOG << sk << "cancel";
        boost::system::error_code ec;
        sk.cancel(ec);
    }
        //LOG << sk.tagval_;
}

template <typename protocol_t, typename socket_t>
void async_nwk<protocol_t,socket_t>::start()
{
    namespace placeholders = boost::asio::placeholders;
    using namespace boost;

    acceptor_.listen();
    LOG << "Listen" << acceptor_.local_endpoint();

    acceptor_.async_accept(*sock0_, bind(&async_nwk::handle_accept, this, placeholders::error));
}

template <typename protocol_t, typename socket_t>
async_nwk<protocol_t,socket_t>::async_nwk(boost::asio::io_service& io_s, boost::asio::ip::tcp::endpoint endpoint)
    : acceptor_(io_s)
{
    namespace ip = boost::asio::ip;

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);

    head_alive_ = 0;
    sock0_ = pool_.construct(io_s);

    //asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
    //asio::ip::tcp::resolver::query query(addr, port);
    //asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
}

template <typename protocol_t, typename socket_t>
void async_nwk<protocol_t,socket_t>::handle_accept(boost::system::error_code const& ec0)
{
    namespace ip = boost::asio::ip;
    namespace placeholders = boost::asio::placeholders;
    if (ec0)
    {
        LOG << "accepted" << ec0;
        return;
    }
    // AUTO_CPU_TIMER(":handle_accept");

    //boost::system::error_code ec;
    LOG << *sock0_ << "accepted";

    if (static_cast<protocol_t*>(this)->on_accepted(*sock0_))
    {
        sock0_->set_option(ip::tcp::socket::keep_alive(true));
        sock0_->set_option(ip::tcp::no_delay(true));

        // int fd = sock0_->native_handle();
        // if (sockets_.size() < fd) sockets_.resize(fd+512);

        // BOOST_ASSERT(sockets_[fd] == 0);
        // sockets_[fd] = sock0_;

        link_front(sock0_);
    }
    else
    {
        LOG << "not-accepted";
        pool_.destroy(sock0_);
    }

    sock0_ = pool_.construct(acceptor_.get_io_service());
    acceptor_.async_accept(*sock0_, bind(&async_nwk::handle_accept, this, placeholders::error));
}

template <typename protocol_t, typename socket_t>
void async_nwk<protocol_t,socket_t>::handle_read(socket_t* sk, boost::system::error_code const& erc, std::size_t bytes_transferred)
{
    AUTO_CPU_TIMER(":handle_read");
    sk->reading( 0 );

    try
    {
        boost::system::error_code ec = erc;
        if (!sk->is_closed()) {
            ec = static_cast<protocol_t*>(this)->on_read(*sk, erc, bytes_transferred);
            if (ec) {
                BOOST_ASSERT(!sk->is_reading());
                if (sk->is_reading()) {
                    LOG << *sk << "logic error, should not reach";
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

template <typename protocol_t, typename socket_t>
void async_nwk<protocol_t,socket_t>::handle_write(socket_t* s, boost::system::error_code const& ec, std::size_t bytes_transferred)
{
    s->writing( 0 );
    AUTO_CPU_TIMER(":handle_write");
    try
    {
        // LOG << *s << bytes_transferred;

        if (!s->is_closed()) {
            static_cast<protocol_t*>(this)->on_write(*s, ec, bytes_transferred);
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

template <typename protocol_t, typename socket_t>
void async_nwk<protocol_t,socket_t>::loopend(boost::system::error_code const& errc, socket_t* s, int iofx)
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
            static_cast<protocol_t*>(this)->untag(*s);
        }

        static_cast<protocol_t*>(this)->on_closed(*s, errc);
    }

    pool_.destroy(s);
}

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

struct imsprotocol : nwbuffer
{
    enum { bytes_of_header=4 };

    //template <typename PMsg> int parse(json::object & jv, boost::iterator_range<char*>& p2, PMsg const& pfx);

    template <typename JVal, typename PMsg>
    static std::string pack(JVal const& jv, PMsg const & pfx);

    imsprotocol()
    {}

    imsprotocol(imsprotocol && rhs)
        : nwbuffer(std::move(rhs))
    {}
};


template <typename socket_t>
struct socket_base : boost::asio::ip::tcp::socket, boost::noncopyable
{
    socket_base(boost::asio::io_service & io_s)
        : boost::asio::ip::tcp::socket(io_s)
    {
        next_ = prev_ = 0; //static_cast<socket_t*>(this);
        tp0_ = tpa_ = time(0);
    }

    struct _Flags {
        bool reading_;
        bool writing_;
        bool close_called_;
        bool _;
        _Flags() { memset(this, 0, sizeof(_Flags)); }
    } nwflags_;

    bool is_closed() const { return nwflags_.close_called_; }
    bool is_writing() const { return nwflags_.writing_; }
    bool is_reading() const { return nwflags_.reading_; }
    void reading(bool y) { nwflags_.reading_ = y; }
    void writing(bool y) { nwflags_.writing_ = y; }
    void closed(bool y) { nwflags_.close_called_ = y; }

    socket_t *next_;
    socket_t *prev_;
    time_t tp0_, tpa_;

    struct stat_print {
        socket_base<socket_t> const* thiz;
        stat_print(socket_base<socket_t> const*p) { thiz=p; }
        friend std::ostream& operator<<(std::ostream & out, stat_print const& s) {
            return out << s.thiz->is_closed() << s.thiz->is_reading() << s.thiz->is_writing();
        }
    };
    stat_print stat() const { return stat_print(this); }

private:
    void close();
};

struct message_info
{
    UInt msgid;
    std::string data;

    message_info(destination const& dst, UInt mid, bool ack)
    {
        msgid = mid;
        ackreq = ack;
        is_fin_ = 0;
    }

    message_info()
    {
        msgid=0;
        ackreq=0;
        is_fin_ = 1;
    }

    ~message_info() {
      //boost::intrusive::list_member_hook<>::node_algorithms::inited( &node_retx_ );
    }

    bool ackreq;
    bool is_fin_;
};

struct socket_type : socket_base<socket_type>
{
    socket_type(boost::asio::io_service & io_s)
        : socket_base(io_s)
    {
        tagval_ = 0;
        flags = 0;
        bytes_tx_ = bytes_rx_ = 0;
    }

    imsprotocol input;
    std::list<message_info> obufs;

    struct curmsg_t : message_info
    {
        ~curmsg_t();
        void init(message_info const& m);

        void finalize(int err);
        bool is_fin() const { return is_fin_; }

        time_t tp0_;

        friend std::ostream& operator<<(std::ostream & out, curmsg_t const& m) {
            if (m.is_fin_)
                return out;
            return out << m.msgid; // <<" "<< m.ackreq;
        }
    };
    curmsg_t curmsg;

    message_handler_t message_handler;
    UInt flags; // user

    UInt tagval_;
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

struct hash_sox {
    size_t operator()(socket_type* sk) const { return boost::hash_value(sk->tagval_); }
};

struct xkey_tag
{
    typedef UInt result_type; 
    result_type operator()(socket_type* sc) const { return sc->tagval_; }
};

struct im_server : async_nwk<im_server, socket_type> //, imsprotocol
{
    typedef async_nwk<im_server, socket_type> base_t;

    im_server(boost::asio::io_service & io_s, boost::asio::ip::tcp::endpoint ep, message_handler_t h);

public:
    void sendto(socket_type& sk, json::object const & rsp);

    socket_type* tag2(socket_type& sk, UInt idx);
    socket_type* untag(socket_type& sk);
    UInt get_tag(socket_type& sk) const { return sk.tagval_; }

    socket_type* finds(UInt uid) const
    {
        auto i = indices_.find(uid);
        if (i != indices_.end())
            return *i;
        return 0;
    }

public:
    void check_alive();

// private:
// private:
// private:
    bool on_accepted(socket_type& s);

    boost::system::error_code on_read(socket_type& sk, boost::system::error_code const& ec, size_t nbyte);

    void on_write(socket_type& sk, boost::system::error_code const& ec, size_t nbyte);

    // void pre_close(socket_type& sk) { tag(sk, 0); }
    void on_closed(socket_type& sk, boost::system::error_code const& ec);

protected:
    void sendto(socket_type& sk, std::string && rsp);

    void inflate(socket_type& sk, destination const& dst, message_ptr mptr);
    void try_next_push(socket_type& sk);

private:
    void handle_message(socket_type& sk, json::object const& jv, boost::iterator_range<char*> r);

    void ack(socket_type& sk, UInt msgid, int);

    message_handler_t message_handler_;

    boost::multi_index_container<
        socket_type*,
        boost::multi_index::indexed_by<boost::multi_index::hashed_unique<xkey_tag> >
    > indices_;

    struct ack_info
    {
        // int fd;
        UInt uid;
        UInt msgid;
        time_t tp;
        ack_info(UInt u, UInt mid) { uid=u; msgid=mid; tp=time(0); }
    };
    std::deque<ack_info> wacks_;
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
};

struct ims_server : im_server
{
    static ims_server* instance_;
    static ims_server& instance() { return *instance_; }

    ims_server(boost::asio::io_service & io_s, boost::asio::ip::tcp::endpoint ep, message_handler_t h)
        : im_server(io_s, ep, h)
    {
        BOOST_ASSERT(!instance_);
        instance_ = this;
    }

    void start()
    {
        user_mgr::instance().ev_regist.connect(
                boost::bind(&ims_server::fev_regist, this, _1, _2));
        message_mgr::instance().ev_newmsg.connect(
                boost::bind(&ims_server::fev_message, this, _1,_2) );

        return im_server::start();
    }

    void check_alive();

private:
    void fev_regist(UInt uid, int regist);
    void fev_message(destination dst, message_ptr mp);

};

struct imbs_server : im_server
{
    using im_server::im_server;

    void check_alive();
};

#endif

