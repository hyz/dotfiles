#ifndef IOCTX_H__
#define IOCTX_H__

#include <string>
#include "json.h"

enum { Connection_Lost = 0xff };
typedef unsigned int UInt;

struct racing_server;
struct cs_server;
struct bs_server;
// struct imbs_server;
struct socket_type;
struct io_context;

typedef void (*message_handler_t)(io_context& s, int cmd, std::string const& jv);

struct io_context
{
    void send(int cmd, std::string const & rsp);
    void send(int cmd, json::object const & rsp);
    void send(int cmd, json::array const & rsp);

    void close(int ln, const char* fn);
    bool is_closed() const;

    void new_handler(message_handler_t h);

    operator bool() const { return bool(sk_); }

    io_context tag(UInt tag, bs_server*);
    io_context tag(UInt tag, cs_server*);
    UInt tag() const;

    static io_context find(UInt tag, bs_server*);
    static io_context find(UInt tag, cs_server*);

public: // private:
    io_context(racing_server& s, socket_type& sock)
        : srv_(&s), sk_(&sock)
    {}

private:
    io_context()
        : srv_(0), sk_(0)
    {}

    racing_server *srv_;
    socket_type *sk_;
};

enum { default_cs_port=20011 };
enum { default_bs_port=20013 };
enum { default_admin_port=20015 };

#endif

