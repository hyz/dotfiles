#include <string>
#include "log.h"
#include "ioctx.h"
#include "async.h"

static ims_server* ims_instance_ = 0;

template <typename T> inline T* ensure(T* x) { BOOST_ASSERT(x); return x; }

void io_context::send(json::object const & rsp)
{
    ims_->sendto(*socket_, rsp);
}

io_context io_context::tag(UInt idx)
{
    io_context ret;
    if (socket_type* s = ims_->tag2(*socket_, idx))
    {
        ret.ims_ = ims_instance_;
        ret.socket_ = s;
    }
    return ret;
}

UInt io_context::tag() const
{
    return ims_->get_tag(*socket_);
}

void io_context::new_handler(message_handler_t h)
{
    socket_->message_handler = h;
}

void io_context::close(int ln, const char* fn)
{
    ims_->close(*socket_, ln, fn);
}

bool io_context::is_closed() const
{
    return socket_->is_closed();
}

void ioctx_set_ims(ims_server* ims)
{
    BOOST_ASSERT(!ims_instance_);
    ims_instance_ = ims;
}

io_context io_context::find(UInt tag)
{
    BOOST_ASSERT(ims_instance_);

    io_context ctx;
    if (socket_type *s = ims_instance_->finds(tag))
    {
        ctx.ims_ = ims_instance_;
        ctx.socket_ = s;
    }
    return ctx;
}

