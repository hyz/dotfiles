//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void listen(tcp::acceptor & a, boost::asio::ip::tcp::endpoint const & endpoint)
{
    a.open(endpoint.protocol());
    a.set_option(tcp::acceptor::reuse_address(true));
    a.bind(endpoint);

    a.listen();
    LOG_I << "Listen " << a.local_endpoint();
    //a.async_accept(t_.socket(), bind(&handle_accept, this, asio::placeholders::error));
}

tcp::endpoint resolve(boost::asio::io_service & io_service, const string& h, const string& p)
{
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(h, p);
    tcp::endpoint endpoint = *resolver.resolve(query);
    return endpoint;
}

//----------------------------------------------------------------------

template <typename connection>
struct relay_server : tcp::acceptor
{
    typedef relay_server<connection> this_type;
    typedef boost::shared_ptr<boost::asio::deadline_timer> timer;

    typedef boost::shared_ptr<connection> shared;
    typedef std::map<int,shared>::iterator iterator;
    std::map<int,shared> cs_;
    shared c_;

    typedef typename connection::message message;
    typedef typename connection::callback callback;
    typedef boost::function<void (message &, callback&)> messager_fn;
    typedef boost::function<messager_fn (connection&, message &, callback&)> messager_fn0;
    messager_fn0 messager_;

    struct connection_ptr
    {
        shared& operator->() { return i_->second; }
        shared::element_type& operator*() { return *i_->second; }

        connection_ptr(connection_ptr const & rhs) : i_(rhs.i_) {} // { s_ = rhs.s_; }
        connection_ptr & operator=(connection_ptr const & rhs)
        {
            i_ = rhs.i_;
            // s_ = rhs.s_;
            return *this;
        }

        connection_ptr(iterator i) : i_(i) {}

        //connection_ptr(this_type& s, iterator i) : i_(i) { s_ = &s; }
        // pclose() { s_.pclose(i_); }

        iterator i_;
        // this_type* s_;
    };

    relay_server(boost::asio::io_service & ios, messager_fn0 & fn)
        : tcp::acceptor(ios)
        , messager_(fn)
    {
    }

    void start(tcp::endpoint const & ep)
    {
        listen(*this, ep); 

        c_.reset(new connection(get_io_service()));
        acceptor_.async_accept(c_->socket(),
                bind(&handle_accept, this, boost::asio::placeholders::error));
    }

    void handle_accept(system::error_code const & ec)
    {
        if (ec)
        {
            return;
        }

        std::pair<iterator,bool> ret = cs_.insert( std::make_pair(c_->socket().native_handler(), c_) );
        c_->start( connection_ptr(ret.first) );

        c_.reset(new connection(get_io_service()));
        acceptor_.async_accept(c_->socket(),
                bind(&handle_accept, this, boost::asio::placeholders::error));
    }

    template <typename F> void post(F & fn) { get_io_service().post(fn); }

    relay_server::timer timer() { return timer(new timer::element_type(get_io_service())); }

    template <typename F> void post(relay_server::timer & t, int seconds, F const & fn)
    {
        t.expire_from_now();
        t.async_wait();
    }

    void pclose(connection_ptr p)
    {
        shared sp = p.i_->second;
        cs_.erase(p.i_);
        post(boost::bind(close_, sp));
    }

    messager_fn message(connection & c, message & m, callback & cb) { return messager_(c, m, cb); }
};

struct connection : tcp::socket
{
    struct message
    {
    };

    typedef boost::function<void (system::error_code const &)> callback;
    typedef boost::function<void (message&, callback&)> message_handler;

    typedef relay_server<connection> server;

    server* server_;
    server::connection_ptr thiz_;

    boost::asio::deadline_timer deadline_;

    message_handler messager_;
    message message_;

    connection(server & s)
        : tcp::socket(s.get_io_service())
        , deadline_(s.get_io_service())
    {
        server_ = &s;
    }

    connection::server & server() { return *server_; }

    void close() { server_->pclose(thiz_); }

    tcp::socket & socket() { return *this; }

    void start(server::connection_ptr thiz)
    {
        thiz_ = thiz;

        async_read;
    }

    void handle_message(system::error_code const & ec)
    {
        if (ec)
        {
            return;
        }

        message_;

        if (!messager_
                && !(messager_ = server_->message(*this
                        , message_ , bind(&connection::message_end, this))))
        {
            return;
        }

        messager_(message_ , bind(&connection::message_end, this));
    }

    void message_end(system::error_code const & ec)
    {
        ;
        async_read;
    }

    template <typename F>
    void send(asio::buffer bufs, F & fn)
    {
        ;
    }
};

struct apple_push
{
    tcp::socket socket_;

    apple_push(boost::asio::io_service & io_service)
        : socket_(io_service)
    {
    }

    connection::message_handler message(connection& c, connection::message& m, connection::callback& cb)
    {
        async_write, cb;

        return bind(&apple_push::message, this, boost::ref(c));
    }
};

int main(int argc, char* argv[])
{
    std::string addr = "127.0.0.1";
    std::string port = "9991";

    try
    {
        boost::asio::io_service io_service;

        apple_push ap(io_service);
        relay_server<connection<apple_push> > s(io_service, bind(&apple_push::message, &ap));

        s.start(resolve(io_service, addr, port));

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

