#ifndef IOCTX_H__
#define IOCTX_H__

#include <string>
#include "json.h"

typedef unsigned int UInt;

struct im_server;
struct ims_server;
struct imbs_server;
struct socket_type;
struct io_context;

typedef void (*message_handler_t)(io_context& s, int cmd, json::object const& jv);

struct io_context
{
    void send(json::object const & rsp);
    void close(int ln, const char* fn);
    bool is_closed() const;

    void new_handler(message_handler_t h);

    operator bool() const { return bool(socket_); }

    io_context tag(UInt tag);
    UInt tag() const;

    static io_context find(UInt tag);

public: // private:
    io_context(im_server& s, socket_type& sock)
        : ims_(&s), socket_(&sock)
    {}

private:
    io_context()
        : ims_(0), socket_(0)
    {}

    im_server *ims_;
    socket_type *socket_;
};

enum { default_ims_port=10011 };
enum { default_bs_port=10015 };
enum { default_admin_port=10015 };

void ioctx_set_ims(ims_server* ims);

#endif

