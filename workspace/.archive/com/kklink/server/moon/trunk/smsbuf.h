#ifndef SMSBUF_H__
#define SMSBUF_H__

#include <list>
#include <string>
#include <boost/asio/buffer.hpp>
#include "log.h"

struct smsbuf
{
    void operator()(const std::string & tok, const std::string & cont)
    {
        sl_.push_back( boost::str(
                boost::format("POST /sms?to=%1%&id=%2% HTTP/1.1\r\n"
                    "Host: 127.0.0.1\r\n"
                    "Content-Length: %3%\r\n"
                    "\r\n")
                % tok % id_++ % cont.length()) + cont );
    }

    boost::asio::mutable_buffers_1 prepare()
    {
        rbuf_.resize(1024);
        return boost::asio::buffer(const_cast<char*>(rbuf_.data()), rbuf_.size());
    }

    boost::system::error_code commit(size_t bytes)
    {
        LOG_I << boost::asio::const_buffer(rbuf_.data(), bytes);
        return boost::system::error_code();
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
        // LOG_I << boost::asio::const_buffer(rbuf_.data(), bytes);
    }

    smsbuf() { id_=0; }

    void event(int x) {}
private:
    unsigned int id_;
    std::string rbuf_;
    std::list<std::string> sl_;
};

#endif

