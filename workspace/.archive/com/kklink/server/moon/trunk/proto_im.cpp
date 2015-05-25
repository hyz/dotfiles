#include "myconfig.h"
#include "log.h"
#include "proto_im.h"

using namespace std;
using namespace boost;

std::ostream & operator<<(std::ostream& out, socket_state const & s)
{
    return out << s.idx;
}
// extern monitor_socket monitor_socket_;

namespace imessage {

reader::reader(Socket_ptr soc, boost::function<bool (Socket_ptr, DataType&)> rsp)
    : socket_(soc)
    , fwdp_(rsp)
{
    static int sidx_=1;
    idx = sidx_++;
    // tp_alive_ = time(0);
    // stat[socket_->native_handle()] = socket_state(sidx_++);
    // monitor_socket_(socket_, time(0), "accept");
}


void reader::start()
{
    // LOG_I << "imessage::reader::start" << sizeof(len_);

    // stat[socket_->native_handle()].tp.sock_read = time(0);

    LOG_I << socket_; // <<" "<< socket_.use_count();

    //boost::asio::socket_base::keep_alive option(true);
    //socket_->set_option(option);
    // monitor_socket_(socket_, time(0), "read");

    asio::async_read(*socket_
            , asio::buffer(&len_.chrs[0], sizeof(len_))
            , bind(&This::handle_size, shared_from_this(), asio::placeholders::error));
}

void reader::handle_size(const boost::system::error_code& err)
{
    if (err)
    {
        return handle_error(err);
    }

    int np = sizeof(len_)-4; // char *p = &len_.chrs[4];
    while ( (int(len_.chrs[0]) & 0xff) == 0xff && np > 0)
    {
        memcpy(&len_.chrs[0], &len_.chrs[1], sizeof(len_) - np);
        --np;
    }
    len_.h[0] = ntohs(len_.h[0]);
    len_.h[1] = ntohs(len_.h[1]);
    LOG_I << socket_ << " size " << len_.h[0] << "," << len_.h[1] <<" "<< np;

    if (len_.h[0] >= 0x7f)
        return handle_error(make_error_code(boost::system::errc::bad_message));

    data_.first.resize(len_.h[0]);
    if (np > 0)
        memcpy(&data_.first[0], &len_.chrs[sizeof(len_)-np], np);
    data_.second.resize(len_.h[1]);

    // monitor_socket_(socket_, time(0), "read");

    string & buf = data_.first;
    asio::async_read(*socket_
            , asio::buffer(const_cast<char*>(buf.data() + np), buf.size() - np)
            , bind(&reader::handle_header, shared_from_this(), asio::placeholders::error));

    // tp_alive_ = time(0);
    // stat[socket_->native_handle()].tp.sock_read = time(0);
}

void reader::handle_header(const boost::system::error_code& err)
{
    if (err)
    {
        return handle_error(err);
    }
    // LOG_I << data_.first;
    // monitor_socket_(socket_, time(0), "read");

    string & buf = data_.second;
    asio::async_read(*socket_
            , asio::buffer(const_cast<char*>(buf.data()), buf.size())
            , bind(&reader::handle_body, shared_from_this(), asio::placeholders::error));
    // tp_alive_ = time(0);
    // stat[socket_->native_handle()].tp.sock_read = time(0);
}

void reader::handle_body(const boost::system::error_code& err)
{
    if (err)
    {
        return handle_error(err);
    }
    // LOG_I << data_.second;

    if (fwdp_(socket_, data_))
    {
        start();
    }
    // tp_alive_ = time(0);
}

void reader::handle_error(const boost::system::error_code& err)
{
    if (err)
    {
        LOG_I << socket_->native_handle() << " " << err;
        if (err == boost::asio::error::operation_aborted)
        {
            return;
        }
    }

    if (!socket_->is_open())
    {
        LOG_I << socket_ << " " << socket_->native_handle() << " " << err;
        return;
    }

    // monitor_socket_(socket_, time(0), "peer-close");
    DataType tmp;
    fwdp_(socket_, tmp);
}

} // namespace

