#ifndef PROTO_IM_H__
#define PROTO_IM_H__

#include <arpa/inet.h>
#include <stdint.h>
#include <utility>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include "async.h"
#include "jss.h"

struct socket_state
{
    boost::asio::ip::address addr;
    struct time_points
    {
        time_t sock_open, sock_close;
        time_t sock_read;
        time_t sock_write;
        time_points() { memset(this, 0, sizeof(*this)); }
    } tp;
    struct wrapflags
    {
        bool reading;
        bool writing;
        wrapflags() { memset(this, 0, sizeof(*this)); }
    } flags;

    int idx;
    unsigned int uid;

    socket_state(int x)
    {
        idx = x;
        tp.sock_open = time(0);
        uid = 0;
    }
};

namespace imessage {

struct reader : boost::enable_shared_from_this<reader> //, boost::noncopyable
{
    int idx;
    // socket_state stat;

    typedef std::pair<std::string, std::string> DataType;
    typedef reader This;

    reader(Socket_ptr soc, boost::function<bool (Socket_ptr, DataType&)> rsp);
    ~reader() {}

    void start();

private:
    Socket_ptr socket_;

    union { uint16_t h[4]; char chrs[sizeof(uint16_t)*4]; } len_;
    DataType data_;

    boost::function<bool (Socket_ptr, DataType&)> fwdp_;

    // time_t tp_alive_;
private:
    void handle_size(const boost::system::error_code& err);

    void handle_header(const boost::system::error_code& err);

    void handle_body(const boost::system::error_code& err);

    void handle_error(const boost::system::error_code& err);
    // boost::timer::auto_cpu_timer auto_timer_;
};

} // namespace imessage


inline boost::asio::ip::tcp::endpoint r_endpoint(socket_ptr & s)
{
    boost::system::error_code ec;
    return s->remote_endpoint(ec);
}

#endif

