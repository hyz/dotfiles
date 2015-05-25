#include <string>
#include "log.h"
#include "ioctx.h"
#include "server.h"

template <typename T> inline T* Ensure(T* x) { BOOST_ASSERT(x); return x; }

void io_context::send(int cmd, std::string const & rsp)
{
    Ensure(srv_)->sendto(*Ensure(sk_), cmd, rsp);
}
void io_context::send(int cmd, json::object const & rsp)
{
    Ensure(srv_)->sendto(*Ensure(sk_), cmd, rsp);
}
void io_context::send(int cmd, json::array const & rsp)
{
    Ensure(srv_)->sendto(*Ensure(sk_), cmd, rsp);
}

io_context io_context::tag(UInt idx, cs_server*)
{
    io_context ret;
    if (socket_type* s = Ensure(srv_)->tag2(*Ensure(sk_), idx))
    {
        ret.srv_ = &cs_server::instance();
        ret.sk_ = s;
    }
    return ret;
}
io_context io_context::tag(UInt idx, bs_server*)
{
    io_context ret;
    if (socket_type* s = Ensure(srv_)->tag2(*Ensure(sk_), idx))
    {
        ret.srv_ = &bs_server::instance();
        ret.sk_ = s;
    }
    return ret;
}

UInt io_context::tag() const
{
    return Ensure(srv_)->get_tag(*Ensure(sk_));
}

void io_context::new_handler(message_handler_t h)
{
    Ensure(sk_)->message_handler = h;
}

void io_context::close(int ln, const char* fn)
{
    Ensure(srv_)->close(*Ensure(sk_), ln, fn);
}

bool io_context::is_closed() const
{
    return sk_->is_closed();
}

io_context io_context::find(UInt tag, bs_server*)
{
    io_context ctx;
    auto & srv = bs_server::instance();
    if (socket_type *s = srv.findtag(tag))
    {
        ctx.srv_ = &srv;
        ctx.sk_ = s;
    }
    return ctx;
}

io_context io_context::find(UInt tag, cs_server*)
{
    io_context ctx;
    auto & srv = cs_server::instance();
    if (socket_type *s = srv.findtag(tag))
    {
        ctx.srv_ = &srv;
        ctx.sk_ = s;
    }
    return ctx;
}


