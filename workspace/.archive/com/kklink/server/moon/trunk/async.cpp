#include "myconfig.h"
#include <iostream>
#include <istream>
#include <ostream>
#include <boost/function.hpp>
#include "log.h"
#include "async.h"

// #include "serv.h"
// #include <boost/regex.hpp>
// #include <boost/lexical_cast.hpp>
// #include <boost/algorithm/string.hpp>
// 
// #include <boost/format.hpp>

// void sigexit(asio::io_service *io_service) { io_service->stop(); }

// using boost::format;

using namespace std;
using namespace boost;
// extern monitor_socket monitor_socket_;

void Acceptor::listen(const string& addr, const string& port, boost::function<void (Socket_ptr)> fn)
{
    fn_ = fn;

    asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
    asio::ip::tcp::resolver::query query(addr, port);
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);

    acceptor_.listen();
    LOG_I << "Listen " << acceptor_.local_endpoint();

    socket_.reset(new asio::ip::tcp::socket(acceptor_.get_io_service()));
    acceptor_.async_accept(*socket_,
            bind(&Acceptor::handle_accept, this, asio::placeholders::error)
            );
}

void Acceptor::handle_accept(const boost::system::error_code& e)
{
    if (e)
    {
        return _error(e);
    }

    socket_->set_option(asio::ip::tcp::socket::keep_alive(true));
    socket_->set_option(asio::ip::tcp::no_delay(true));

    LOG_I << "accepted " << socket_; //->local_endpoint() << "%" << socket_->remote_endpoint() << "\n";
    fn_(socket_);

    socket_.reset(new asio::ip::tcp::socket(acceptor_.get_io_service()));
    acceptor_.async_accept(*socket_,
            bind(&Acceptor::handle_accept, this, asio::placeholders::error));
}

void Acceptor::_error(const boost::system::error_code& err)
{
    LOG_I << "acceptor error: " << err.message() << "\n";
}

Acceptor::Acceptor(asio::io_service& io_service)
    : acceptor_(io_service)
{
}

Writer_impl::~Writer_impl()
{
    LOG_I << __FILE__ << __LINE__;
    fin_(soc_);
    LOG_I << soc_ <<" "<< this <<" "<< size_ <<" "<< cnt_ <<" "<< ls_.empty() <<" "<< soc_.use_count();
    soc_->close();
    LOG_I << __FILE__ << __LINE__;
}

Writer_impl::Writer_impl(Socket_ptr soc, boost::function<void (Socket_ptr)> fin)
    : soc_(soc)
    , fin_(fin)
{
    cnt_ = 0;
    size_ = 0;
    LOG_I << soc_ <<" "<< this <<" "<< soc_.use_count();
}

void Writer_impl::send(const std::string& s)
{
    bool empty = ls_.empty();

    if (s.empty())
    {
        if (!empty)
            ls_.erase(++ls_.begin(), ls_.end());
        // monitor_socket_(soc_, time(0), "local-close");
        return;
    }

    ls_.push_back(s);
    if (empty)
    {
        // monitor_socket_(soc_, time(0), "write");
        asio::async_write(*soc_, asio::buffer(ls_.front())
                , bind(&Writer_impl::writeb, shared_from_this(), asio::placeholders::error)
                );
    }

    LOG_I << soc_ <<" "<< size_ <<" "<< cnt_ <<" "<< ls_.empty() <<" "<< soc_.use_count();
}

void Writer_impl::writeb(const boost::system::error_code& err)
{
    if (err)
    {
        return ;//_error(obj, err, 9);
    }
    // monitor_socket_(soc_, time(0), "write-ok");

    ++cnt_;
    size_ += ls_.front().size();
    ls_.pop_front();

    if (!ls_.empty())
    {
        // monitor_socket_(soc_, time(0), "write");
        asio::async_write(*soc_, asio::buffer(ls_.front())
                , bind(&Writer_impl::writeb, shared_from_this(), asio::placeholders::error)
                );
    }

    LOG_I << soc_ <<" "<< size_ <<" "<< cnt_ <<" "<< ls_.empty() <<" "<< soc_.use_count();
}

Writer::~Writer()
{
    LOG_I << __FILE__ << __LINE__;
    //for (iterator i = workers_.begin(); i != workers_.end(); ++i)
        //i->second->soc_->cancel();
}

void Writer::finish(Socket_ptr soc)
{
    workers_.erase(soc->native_handle());
    LOG_I << soc;
}

boost::function<void (const std::string&)>
Writer::associate(Socket_ptr soc)
{
    Worker_ptr ker;

    boost::weak_ptr<Worker> wptr(ker);
    std::pair<iterator, bool> ret = workers_.insert(std::make_pair(soc->native_handle(), wptr));
    if (ret.second)
    {
        ker.reset(new Worker(soc, bind(&Writer::finish, this, _1)));
    }
    else
    {
        ker = ret.first->second.lock();
        BOOST_ASSERT(ker);
    }

    LOG_I << soc <<" "<< soc.use_count();
    return bind(&Worker::send, ker, _1);
}


