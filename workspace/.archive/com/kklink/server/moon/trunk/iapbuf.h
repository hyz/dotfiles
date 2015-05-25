#ifndef IAPBUF__
#define IAPBUF__

#include <list>
#include <string>
#include <boost/asio/buffer.hpp>
#include "log.h"
#include "jss.h"

struct iap_receipt
{
    int id;
    std::string productid;
    std::string content;
    bool sandbox;

    iap_receipt(int id_, const std::string & productid_, const std::string & cont_, bool sandbox_)
        : productid(productid_)
        , content(cont_)
    {
        id = id_;
        sandbox = sandbox_;
    }

    void result(const char* err, const json::object & receipt);
    void save(const char* tab, const char* err);

    static iap_receipt load(int id_, boost::system::error_code & ec);
};

std::ostream & operator<<(std::ostream & out, iap_receipt const & iap);

struct iapbuf
{
    void operator()(int id, const std::string & productid, const std::string & cont, bool sandbox);

    boost::asio::mutable_buffers_1 prepare()
    {
        int n = rbuf_.size();
        rbuf_.resize(n + 1024);
        return boost::asio::buffer(const_cast<char*>(rbuf_.data()+n), 1024);
    }

    boost::system::error_code commit(size_t bytes)
    {
        LOG_I << boost::asio::const_buffer(rbuf_.data(), bytes);
        boost::system::error_code ec = on_data();
        rbuf_.resize(rbuf_.size() - (1024 - bytes));
        return ec;
    }

    boost::asio::const_buffers_1 const_buffer()
    {
        if (sl_.empty())
            return boost::asio::buffer(static_cast<const char*>(0), 0);
        return boost::asio::buffer(sl_.front());
    }

    void consume(size_t bytes)
    {
        LOG_I << (sl_.front().size() == bytes) <<" "<< sl_.front();
        BOOST_ASSERT (sl_.front().size() == bytes);
        sl_.pop_front();
    }

    iapbuf() { }

    void event(int x) {}
private:
    std::string rbuf_;
    std::list<std::string> sl_;
    // std::list<iap_receipt> iaps_;

    boost::system::error_code on_data();
};

#endif

