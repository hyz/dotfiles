#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <resolv.h>

#include <ev.h>

#define DEFAULT_PORT    8888

static int n_accept = 0, n_error = 0, n_ok = 0;

struct conn {
    struct ev_io ev_io;

    char *buf;
    int len, capacity, nwrite;
};

static void connfree(struct conn *c)
{
    if (c->buf)
        free(c->buf);
    free(c);
}

static int complete(struct conn *c, int n) {
    int ret = 0;

    if (n == 0) {

        return 1;
    }

    return ret;
}

static void on_send(struct ev_loop *loop, struct ev_io *w, int revents)
{
    struct conn *c = (struct conn*)w;
    int n;

    if ( (n = write(w->fd, c->buf + c->nwrite, c->len - c->nwrite)) < 0) {
        perror("write");
        close(w->fd);
        connfree(c);
        ++n_error;
        return;
    }
    c->nwrite += n;

    if (c->nwrite == c->len) {
        puts("send finished");
        close(w->fd);
        connfree(c);
        ++n_ok;
    }
}

static void on_read(struct ev_loop *loop, struct ev_io *w, int revents)
{
    struct conn *c = (struct conn*)w;
    int n;

    if (c->capacity - c->len <= 1024) {
        if ( (c->buf = (char*)realloc(c->buf, c->capacity + 4096)) == NULL) {
            perror("realloc");
            close(w->fd);
            connfree(c);
            ++n_error;
            return;
        }
        c->capacity += 4096;
    }

    if ( (n = read(w->fd, c->buf, c->capacity - c->len)) < 0) {
        perror("read");
        close(w->fd);
        connfree(c);
        ++n_error;
        return;
    }

    c->len += n;

    if ( (n = complete(c, n)) != 0) {
        ev_io_stop(loop, w);

        if (n < 0) {
            close(w->fd);
            connfree(c);
            ++n_error;
            return;
        } else {
            ev_io_init(w, on_send, w->fd, EV_WRITE);
            ev_io_start(loop, w);
        }
    }
}

static void on_accept(struct ev_loop *loop, struct ev_io *w, int revents)
{
    int fd;
    struct sockaddr_in saddr;
    socklen_t slen;

    if ( (fd = accept(w->fd, (struct sockaddr*)&saddr, &slen)) < 0) {
        perror("accept");
        return;
    }

    struct ev_io *client = (struct ev_io*)calloc(1, sizeof(struct conn));
    if (!client) {
        perror("calloc");
        close(fd);
        return;
    }

    ++n_accept;
    printf("new accept %d; %d %d", n_accept, n_ok, n_error);

    ev_io_init(client, on_read, fd, EV_READ);
    ev_io_start(loop, client);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in addr; // socklen_t addr_len = sizeof(addr);
    int lfd;

    lfd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int val = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    fcntl(lfd, F_SETFL, fcntl(lfd, F_GETFL, 0) | O_NONBLOCK);

    if (bind(lfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(lfd, 1024) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    // struct ev_loop *loop = ev_loop_new (EVBACKEND_EPOLL | EVFLAG_NOENV);
    // EVLOOP_NONBLOCK
    struct ev_loop *loop = ev_default_loop(EVLOOP_NONBLOCK);
    struct ev_io lw;
    ev_io_init(&lw, on_accept, lfd, EV_READ);
    ev_io_start(loop, &lw);

    ev_loop(loop, 0);

    // This point is never reached.
    close(lfd);

    return EXIT_SUCCESS;
}

