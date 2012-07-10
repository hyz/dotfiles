#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <ev.h>

#include <algorithm>

#define PORT 8000
#define ADDR_IP "127.0.0.1"

struct iobuf {
    char *begin, *ptr, *end;
};

struct connection {
    struct ev_io io;
    struct iobuf req, rsp;
};

static void *init_listener(struct ev_io *ls, char *ipaddr, int port);
static void cb_accept(struct ev_loop *loop, struct ev_io *ls, int revents);
static void cb_read(struct ev_loop *loop, struct ev_io *c, int revents);
static void cb_write(struct ev_loop *loop, struct ev_io *c, int revents);
static void finalize(struct ev_loop *loop, struct connection *c, int code);

int main(int argc, char **argv)
{
	struct ev_loop *loop = ev_loop_new(EVBACKEND_EPOLL);
	struct ev_io listener;

    if (!init_listener(&listener, 0, PORT))
        return 1;

	ev_io_start(loop, &listener);

	ev_loop(loop, 0);

    close(listener.fd);
	ev_loop_destroy(loop);

	return 0;

}

static void *init_listener(struct ev_io *ls, char *ipaddr, int port)
{
	struct sockaddr_in sa;
	int fd;

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

//setnonblocking(listener);

	bzero(&sa, sizeof(sa));
	sa.sin_family = PF_INET;
	sa.sin_port = htons(PORT);
	sa.sin_addr.s_addr = INADDR_ANY; //inet_addr(ADDR_IP);

	if (bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(fd, 1024) < 0) {
		perror("listen");
		exit(1);
	}

	ev_io_init(ls, cb_accept, fd, EV_READ);

	return ls;
}

static void cb_accept(struct ev_loop *loop, struct ev_io *ls, int revents)
{
	struct sockaddr_in sa;
	socklen_t salen = sizeof(sa);
	int newfd;
	struct connection *c;

	if ( (newfd = accept(ls->fd, (struct sockaddr *)&sa, &salen)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
	}

    if ( (c = (struct connection *)calloc(1, sizeof(struct connection))) == 0) {
        perror("calloc");
        close(newfd);
        return;
    }

	ev_io_init(&c->io, cb_read, newfd, EV_READ);
	ev_io_start(loop, &c->io);

	// printf("accept callback : fd :%d\n", c->fd);

}

static void *rebuf(struct iobuf *buf, int avail)
{
    if (buf->end - buf->ptr < avail) {
        char *p;
        int n = buf->ptr - buf->begin;

        if ( (p = (char*)realloc(buf->begin, n + avail)) == 0) {
            perror("realloc");
            return 0;
        }

        buf->begin = p;
        buf->ptr = p + n;
        buf->end = p + n + avail;
    }
    return buf;
}

static int completed(struct connection *c)
{
    int y = 0;
    const char *cr = "\r\n\r\n";
    char *eh, *p;

    if ( (eh = std::search(c->req.begin, c->req.ptr, cr, cr + 4)) < c->req.ptr) {
        int len;
        const char *m = "GET";

        eh += 4;

        if ( (y = std::equal(m, m+3, c->req.begin)) == 0) {
            const char *clen = "Content-Length";

            if ( (p = std::search(c->req.begin, c->req.ptr, clen, clen + strlen(clen))) < c->req.ptr) {
                while (p < c->req.ptr && !isdigit(*p))
                    ++p;
                if (p < c->req.ptr) {
                    len = atoi(p);
                    if (c->req.ptr - eh >= len) {
                        y = 1;
                    }
                }
            }
        }
    }

    return y;
}

static int init_rsp(struct connection *c)
{
    int len = c->req.ptr - c->req.begin;
    rebuf(&c->rsp, len);
    memcpy(c->rsp.begin, c->req.begin, len);
    return 1;
}

static void cb_read(struct ev_loop *loop, struct ev_io *_c, int revents)
{
    struct connection *c = (struct connection*)_c;
    int fd = _c->fd;
	int n;

	if (!rebuf(&c->req, 4096)) {
        return;
    }

    if ( (n = recv(fd, c->req.ptr, c->req.end - c->req.ptr, 0)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("recv");
        finalize(loop, c, n);
        return;
	} else if (n == 0) {
        printf("peer-closed %d\n", fd);
    }

    c->req.ptr += n;

    if (completed(c)) {
        printf("socket %d recv completed\n", fd);

        if (!init_rsp(c)) {
            finalize(loop, c, 201);
            return;
        }

        ev_io_stop(loop, &c->io);
        ev_io_init(&c->io, cb_write, fd, EV_WRITE);
        ev_io_start(loop, &c->io);

        printf("socket %d rsp start\n", fd);

    } else if (n == 0) {
        finalize(loop, c, 200);
    }
}

static void cb_write(struct ev_loop *loop, struct ev_io * _c, int revents)
{
    struct connection *c = (struct connection*)_c;
    int fd = _c->fd;
    int n;

	if ( (n = send(fd, c->rsp.ptr, c->rsp.end - c->rsp.ptr, 0)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("send");
        finalize(loop, c, n);
        return;
    }
    c->rsp.ptr += n;

    if (c->rsp.ptr >= c->rsp.end) {
        finalize(loop, c, 0);
    }
}

static void finalize(struct ev_loop *loop, struct connection *c, int code)
{
    ev_io_stop(loop, &c->io);

    close(c->io.fd);

    if (c->req.begin)
        free(c->req.begin);
    if (c->rsp.begin)
        free(c->rsp.begin);

    free(c);

    // ev_io_init(c, cb_read, fd, EV_READ);
    // ev_io_start(loop, c);
}

