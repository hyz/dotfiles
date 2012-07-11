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
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

// #include <boost/algorithm/string.hpp>

#define PORT 8000
// #define ADDR_IP "127.0.0.1"

// struct iobuf { char *begin, *ptr, *end; };

struct stats {
    int step;
};

static std::map<unsigned int, struct stats> stats_;

struct content {
    std::string method;
    std::string path;

    std::string first;
    std::map<std::string, std::string> head;
    // std::vector<std::pair<std::string,std::string> > head;
    std::vector<char> body;
};

struct connection {
    struct ev_io io;

    struct content req;
    int _contlen;

    int chn;
    unsigned int sn;

    std::string imsi;
    std::string smsc;

    std::vector<char> buf;

	struct ev_loop *ev_loop;

    connection(struct ev_loop *loop)
        : ev_loop(loop) {
        memset(&this->io, 0, sizeof(this->io));
    }

    ~connection() {
        if (this->io.fd > 0) {
            ev_io_stop(ev_loop, &this->io);
            close(this->io.fd);
        }
    }

    // static char _buf[1024*8];
};

static void *init_listener(struct ev_io *ls, char *ipaddr, int port);
static void cb_accept(struct ev_loop *loop, struct ev_io *ls, int revents);
static void cb_read(struct ev_loop *loop, struct ev_io *c, int revents);
static void cb_write(struct ev_loop *loop, struct ev_io *c, int revents);
// static void finalize(struct ev_loop *loop, struct connection *c, int code);

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

	struct connection *c = new connection(loop);

	if ( (newfd = accept(ls->fd, (struct sockaddr *)&sa, &salen)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        delete c;
        return;
	}

	ev_io_init(&c->io, cb_read, newfd, EV_READ);
	ev_io_start(loop, &c->io);

	// printf("accept callback : fd :%d\n", c->fd);
}

// static void *rebuf(struct iobuf *buf, int avail)
// {
//     if (buf->end - buf->ptr < avail) {
//         char *p;
//         int n = buf->ptr - buf->begin;
// 
//         if ( (p = (char*)realloc(buf->begin, n + avail)) == 0) {
//             perror("realloc");
//             return 0;
//         }
// 
//         buf->begin = p;
//         buf->ptr = p + n;
//         buf->end = p + n + avail;
//     }
//     return buf;
// }

template <typename Iter>
static void *consume_head(struct content *cont, Iter h, Iter hend)
{
    std::string shd(h, hend);
    std::string l;
    std::stringstream ss(shd);

    if (std::getline(ss, cont->first))
    {
        std::stringstream sl(cont->first);
        std::string s;

        sl >> cont->method >> cont->path >> s;

        if (cont->method != "GET" && cont->method != "POST") {
            return 0;
        }

        std::cout << cont->method << " " << cont->path << " " << s << std::endl;
    }

    while (std::getline(ss, l)) {
        std::string sep = ": "; // const char *sep = ": ";
        std::string::iterator it;

        if ( (it = std::search(l.begin(), l.end(), sep.begin(), sep.end())) >= l.end())
            break;

        std::string k(l.begin(), it);
        cont->head[k] = std::string(it+2, l.end());

        std::cout << k << ": " << cont->head[k] << std::endl;
    }

    return cont;
}

static std::string getk(const std::string &k, std::map<std::string, std::string> &hds)
{
    std::map<std::string, std::string>::iterator it;

    it = hds.find(k);
    if (it == hds.end())
        return "";
    return it->second;

    // for (std::vector<std::string>::iterator it = hd->head.begin()
    //         ; it < hd->head.end()
    //         ; ++it) {
    //         return *it;
    //     }
    // }
    // return "";
}

static int get_content_length(struct content *c)
{
    std::string val = getk("Content-Length", c->head);
    if (val.empty())
        return 0;
    return atoi(val.c_str());
    // char *it;
    // for (it = lin.begin(); it < lin.end(); ++it) {
    //     if (isdigit(*it)) {
    //         return atoi(it);
    //     }
    // }
    // return 0;
}

static int completed(struct connection *c, struct content *req, std::vector<char> &data)
{
    if (req->method.empty()) {
        if (data.size() > 1024 * 32) {
            return -1;
        }

        std::vector<char>::iterator it; //std::string::iterator it;
        std::string cr = "\r\n\r\n";

        if ( (it = std::search(data.begin(), data.end(), cr.begin(), cr.end())) >= data.end()) {
            return 0;
        }

        if (!consume_head(req, data.begin(), it + 4)) {
            return -1;
        }

        1;// data.erase(data.begin(), it + 4); // TODO

        c->_contlen = get_content_length(req);
        if (c->_contlen > 1024 * 32) {
            return -1;
        }

        //         "Cookie: cid=\tff/ffffffffxx/1...2...3...4.../1...2...3...4...\r\n"

        std::string cookie = getk("Cookie", req->head);
        std::string::iterator ieq = std::find(cookie.begin(), cookie.end(), '=');
        if (ieq == cookie.end() || std::count(ieq+1, cookie.end(), '/') != 3) {
            return -1;
        }
        cookie.erase(cookie.begin(), ieq+1);
        std::replace(cookie.begin(), cookie.end(), '/', ' ');

        std::string sn;
        std::stringstream ss(cookie);

        ss >> c->chn >> sn >> c->imsi >> c->smsc;
        c->sn = strtol(sn.c_str(), 0, 16);

        std::cout << "COOKIE: " << c->chn << " " << sn << ":" << c->sn << " " << c->imsi << " " << c->smsc << "\n";
    }

    if (data.size() < c->_contlen) {
        return 0;
    }

    if (!data.empty()) {
        req->body.assign(data.begin(), data.end());
    }

    return 1;
}

static void *init_rsp(struct connection *c)
{
    int len;
    // c->buf.assign();

    return c;
}

static int recvbuf(std::vector<char>& buf, int fd)
{
    int n, siz;

    siz = buf.size();
    n = buf.capacity() - siz;

    n = std::max(n, 1024*8);
    buf.resize(siz + n);

    if ( (n = recv(fd, &buf[siz], n, 0)) > 0) {
        buf.resize(siz + n);
    }

    return n;
}

static void cb_read(struct ev_loop *loop, struct ev_io *_c, int revents)
{
    struct connection *c = (struct connection*)_c;
    int fd = _c->fd;
	int n;

    if ( (n = recvbuf(c->buf, fd)) < 0) { // if ( (n = recv(fd, c->_buf, sizeof(c->_buf), 0)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recv");
            delete c;
            return;
        }
	} else if (n == 0) {
        printf("peer-closed %d\n", fd);
    }

    int y;
    if ( (y = completed(c, &c->req, c->buf)) < 0) {
        delete c;

    } else if (y) {
        printf("socket %d recv completed\n", fd);

        if (!init_rsp(c)) {
            delete c;
            return;
        }

        ev_io_stop(loop, &c->io);
        ev_io_init(&c->io, cb_write, fd, EV_WRITE);
        ev_io_start(loop, &c->io);

        printf("socket %d rsp start\n", fd);

    } else {
        if (n == 0) {
            delete c;
        }
    }
}

static int sendbuf(int fd, std::vector<char> &buf)
{
    int n;

	if ( (n = send(fd, &buf[0], buf.size(), 0)) > 0) {
        buf.erase(buf.begin(), buf.begin() + n);
    }

    return n;
}

static void cb_write(struct ev_loop *loop, struct ev_io * _c, int revents)
{
    struct connection *c = (struct connection*)_c;
    int fd = _c->fd;
    int n;

    if ( (n = sendbuf(fd, c->buf)) < 0) { // if ( (n = send(fd, c->rsp.ptr, c->rsp.end - c->rsp.ptr, 0)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        perror("send");
        delete c;
        return;
    }

    if (c->buf.empty()) { //c->rsp.ptr >= c->rsp.end) {
        delete c;
        std::cout << "______\n\n";
    }
}

// static void finalize(struct ev_loop *loop, struct connection *c, int code)
// {
//     ev_io_stop(loop, &c->io);
// 
//     close(c->io.fd);
// 
//     // if (c->req.begin) free(c->req.begin);
//     // if (c->rsp.begin) free(c->rsp.begin);
// 
//     delete c; // free(c);
// 
//     // ev_io_init(c, cb_read, fd, EV_READ);
//     // ev_io_start(loop, c);
// }

