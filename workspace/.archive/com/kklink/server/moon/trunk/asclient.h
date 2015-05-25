#ifndef ASCLIENT_H__
#define ASCLIENT_H__

#include <stdlib.h>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include "log.h"

template <typename T>
struct asclient : boost::noncopyable
{
    typedef asclient<T> this_type;

    struct stage {
        enum type { _
            , connecting = 1
            , idle
            , writing
        };
    };

    template <typename X, typename Y> void send(X& x, Y& y);
    template <typename A,typename B,typename C,typename D> void send(A& x, B& y, C& c, D& d);
    template <typename X> void send(X const & x);

    asclient(boost::asio::io_service & io_service, boost::asio::ip::tcp::endpoint const &ep);//(, T const & msgdo);
    static asclient<T> *instance(asclient<T> * c=0);

    void start()
    {
        ostage_ = stage::connecting;
        boost::system::error_code ec;
        timed_connect(ec);
    }

private:
    T message_do_;

    void timed_connect(boost::system::error_code const & ec);
    void handle_connect(boost::system::error_code const & ec);

    void handle_write(boost::system::error_code const & ec, size_t bytes);
    struct Handle_write;
    friend struct Handle_write;

    struct Handle_write
    {
        explicit Handle_write(asclient<T> * p) { self = p; }
        asclient<T> * self;
        void operator()(boost::system::error_code const & ec, size_t bytes) const { self->handle_write(ec, bytes); }
    };

    void handle_error(boost::system::error_code const & ec);

    boost::asio::ip::tcp::socket socket_;
    boost::asio::deadline_timer timer_;
    boost::asio::ip::tcp::endpoint endpoint_;

    void handle_read(boost::system::error_code const & ec, size_t bytes);

    typename stage::type ostage_;

    void check_obuf();
};

template <typename T>
template <typename X>
void asclient<T>::send(X const & x)
{
    message_do_(x);
    check_obuf();
}

template <typename T>
template <typename X, typename Y>
void asclient<T>::send(X& x, Y& y)
{
    message_do_(x, y);
    check_obuf();
}

template <typename T>
template <typename A,typename B,typename C,typename D>
void asclient<T>::send(A& x, B& y, C& c, D& d)
{
    message_do_(x, y, c, d);
    check_obuf();
}

//template <typename T>
//void asclient<T>::Handle_write::operator()(boost::system::error_code const & ec, size_t bytes) //const
//{
//    self->handle_write(ec, bytes);
//}

template <typename T>
asclient<T>* asclient<T>::instance(asclient<T>* c)
{
    static asclient* asc = 0;
    if (asc == 0)
    {
        BOOST_ASSERT(c);
        asc = c;
        asc->message_do_.event(0);
    }
    return asc;
}

//template <typename T>
//template <typename X, typename Y>
//void asclient<T>::send (const X& x, const Y& y)

template <typename T>
asclient<T>::asclient(boost::asio::io_service & io_service, boost::asio::ip::tcp::endpoint const &ep/*, T const & msgdo*/)
    : socket_(io_service)
    , timer_(io_service)
    , endpoint_(ep)
    // , message_do_(msgdo)
{
    ostage_ = stage::connecting;
}

template <typename T>
void asclient<T>::handle_write(boost::system::error_code const & ec, size_t bytes)
{
    if (ec)
    {
        LOG_I << ec <<" "<< ec.message() <<" " __FILE__ " "<< ostage_ <<" "<< endpoint_;
        return ; //handle_error(ec);
    }

    message_do_.consume(bytes);

    boost::asio::const_buffers_1 buf = message_do_.const_buffer() ;
    if (boost::asio::buffer_size(buf) == 0)
    {
        ostage_ = stage::idle;
        return;
    }
    boost::asio::async_write(socket_
            , buf
            , boost::bind(&this_type::handle_write, this
                , boost::asio::placeholders::error , boost::asio::placeholders::bytes_transferred));
}

template <typename T>
void asclient<T>::handle_error(boost::system::error_code const & ec)
{
    LOG_I << ec <<" "<< ec.message() <<" " __FILE__ " "<< ostage_ <<" "<< endpoint_;

    if (ec == boost::asio::error::operation_aborted)
    {
        // ostage_ = stage::error;
        return;
    }

    if (ostage_ == stage::connecting)
        return;

    ostage_ = stage::connecting;
    timer_.expires_from_now(boost::posix_time::seconds(1));
    timer_.async_wait(boost::bind(&this_type::timed_connect, this, boost::asio::placeholders::error));
}

template <typename T>
void asclient<T>::timed_connect(boost::system::error_code const & ec)
{
    LOG_I << ec <<" "<< ec.message() <<" " __FILE__ " "<< ostage_ <<" "<< endpoint_;
    if (ec)
    {
        return ; //handle_error(ec);
    }

    if (ostage_ == stage::connecting)
    {
        boost::system::error_code ec;
        socket_.close(ec);
        socket_.open(boost::asio::ip::tcp::v4());
        socket_.async_connect(endpoint_, boost::bind(&this_type::handle_connect, this
                    , boost::asio::placeholders::error));
    }
}

template <typename T>
void asclient<T>::handle_connect(boost::system::error_code const & ec)
{
    LOG_I << endpoint_ <<" "<< ec <<" "<< ec.message() <<" "<< ostage_;
    if (ec)
    {
        timer_.expires_from_now(boost::posix_time::seconds(5));
        timer_.async_wait(boost::bind(&this_type::timed_connect, this, boost::asio::placeholders::error));
        return;
    }
    message_do_.event(1);

    socket_.async_read_some(message_do_.prepare()
            , boost::bind(&this_type::handle_read, this
                , boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

    ostage_ = stage::idle;
    check_obuf();
}

template <typename T>
void asclient<T>::handle_read(boost::system::error_code const & erc, size_t bytes)
{
    if (erc)
    {
        LOG_I << __FILE__;
        return handle_error(erc);
    }

    boost::system::error_code ec;
    try {
        ec = message_do_.commit(bytes);
    } catch (my_exception const & ex) {
        ec = ex.error_code();
    } catch (std::exception const & ex) {
        LOG_I << ex.what();
        ec = make_error_code(boost::asio::error::connection_aborted);
    }
    if (ec)
    {
        LOG_I << ec <<" "<< ec.message();
        ostage_ = stage::connecting;
        return timed_connect(erc);
    }

    socket_.async_read_some(message_do_.prepare()
            , boost::bind(&this_type::handle_read, this
                , boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    check_obuf();
}

template <typename T>
void asclient<T>::check_obuf()
{
    if (ostage_ == stage::idle)
    {
        boost::asio::const_buffers_1 buf = message_do_.const_buffer() ;
        LOG_I << buf;
        if (boost::asio::buffer_size(buf) == 0)
            return;

        ostage_ = stage::writing;
        boost::asio::async_write(socket_
                , buf
                //, bind<void>(Handle_write(this), placeholders::error, placeholders::bytes_transferred)
                , boost::bind(&asclient<T>::handle_write, this
                    , boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
                );
    }
}

#endif

