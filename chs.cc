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
#include <assert.h>
#include <stdint.h>
#include <iconv.h>
#include <fcntl.h>

#include <stdexcept>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "glex.hpp"

// #include <boost/algorithm/string.hpp>

enum {
    CN_SMBF = 0
        , CN_SMTX
        , CN_WAPQ
        , CN_WAPRSP_2SERV
        , CN_WAPINIT_2SERV
        , CN_PUSH_2SERV
        , CN_MAX
};

#define PORT 8000
// #define ADDR_IP "127.0.0.1"

typedef std::vector<std::pair<std::string, std::string> >::iterator head_iterator;

struct httprsp {
    // std::string method;
    // std::string path;
    std::string ver;
    int code;

    // std::string first;
    std::vector<std::pair<std::string, std::string> > head;

    std::string contenttype;
    std::string charset;
    int contentlength;

    std::string body;

    template <typename I_> bool initrng(I_ hd, I_ body, I_ end);

private:
    void request_line_parse(const std::string& l)
    {
        std::stringstream sl(l);
        std::string tmp;

        sl >> this->ver >> this->code >> tmp;
        std::cout << this->ver << " " << this->code << " " << tmp << "\n";
    }
};

struct httpreq {
    std::string method;
    std::string path;
    std::string ver;

    std::vector<std::pair<std::string, std::string> > head;

    // std::string contenttype;
    // std::string charset;
    int contentlength;

    std::string body;

    template <typename I_> bool initrng(I_ hd, I_ body, I_ end);
    bool init(const std::string &first, const std::string& head, const std::string& body);

private:
    bool method_line_parse(const std::string& l)
    {
        std::stringstream sl(l);

        sl >> this->method >> this->path >> this->ver;

        if (this->method != "GET" && this->method != "POST") {
            return false;
        }

        std::cout << this->method << " " << this->path << " " << this->ver << "\n";
    }
};

template <typename I_>
static std::string glex(const std::string& ex, I_ beg, I_ end)
{
    Range<std::string::const_iterator> _cont, _pat, _ret;

    _cont.begin = beg;
    _cont.end = end;
    _pat.begin = ex.begin();
    _pat.end = ex.end();

    if (globex(&_ret, &_pat, &_cont)) {
        return std::string(_ret.begin, _ret.end);
    }

    return "";
}

std::string unescape(const std::string& url)
{
    std::string esc;
    std::string::const_iterator it = url.begin();
    int pstart = 0, pos = 0;

    while ( (pos = url.find("&amp;", pstart)) != std::string::npos) {
        esc.insert(esc.end(), it + pstart, it + pos+1);
        pstart = pos + 5;
    }

    //if (it + pstart < url.end()) {
    esc.insert(esc.end(), it + pstart, url.end());
    //}

    return esc;
}

// // boost::algorithm::starts_with();
static bool starts_with(const std::string& s, const std::string& sub)
{
    return (s.size() >= sub.size() && std::equal(sub.begin(), sub.end(), s.begin()));
}

int split1(std::string &k, std::string &v, const std::string &s, const std::string &any)
{
    size_t pos = s.find_first_of(any);
    if (pos == std::string::npos) {
        k.assign(s.begin(), s.begin() + pos);
        return 1;
    }

    size_t pos2 = s.find_first_not_of(any, pos);
    v.assign(s.begin() + pos2, s.end());

    return 2;
}

static head_iterator headval(std::vector<std::pair<std::string, std::string> >& head, const std::string& k)
{
    head_iterator it = head.begin();
    for (; it != head.end(); ++it) {
        if (it->first == k) {
            break;
        }
    }
    return it;
}

template <typename H>
bool head_line_parse(H& head, std::string& l)
{
    std::string::iterator it, jt;

    if ( (jt = it = std::find(l.begin(), l.end(), ':')) >= l.end()) {
        return false;
    }

    if (*l.rbegin() == '\r') {
        l.resize(l.size() - 1);
    }

    while (++jt < l.end() && isspace(*jt))
        ;

    if (jt < l.end()) {
        head.push_back(make_pair(std::string(l.begin(), it), std::string(jt, l.end())));
        std::cout << head.back().first << " " << head.back().second << "\n";
    }
    return true;
}

bool httpreq::init(const std::string &first, const std::string& head, const std::string& body)
{
    method_line_parse(first);

    std::istringstream ss(head);
    std::string l;

    if (std::getline(ss, l)) {
        head_line_parse(this->head, l);
    }

    this->body = body;

    head_iterator hi = headval(this->head, "Content-Length");
    if (hi != this->head.end()) {
        this->contentlength = atoi(hi->second.c_str());
    }

    return true;
}

template <typename I_>
bool httpreq::initrng(I_ beg, I_ body, I_ end)
{
    std::string shd(beg, body);
    std::string l;

    std::istringstream ss(shd);

    if (std::getline(ss, l)) {
        method_line_parse(l);
    }

    while (std::getline(ss, l)) {
        head_line_parse(this->head, l);
    }

    this->body.assign(body, end);

    // std::transform(this->contenttype.begin(), this->contenttype.end(), this->contenttype.begin(), std::tolower);
    // std::transform(this->charset.begin(), this->charset.end(), this->charset.begin(), std::tolower);

    // lexical_cast;
    head_iterator hi = headval(this->head, "Content-Length");
    if (hi != this->head.end()) {
        this->contentlength = atoi(hi->second.c_str());
    }

    // std::cout << this->contenttype << " " << this->charset << " " << this->contentlength << "\n";

    return true;
}

template <typename I_>
bool httprsp::initrng(I_ beg, I_ body, I_ end)
{
    std::string shd(beg, body);
    std::string l;

    std::stringstream ss(shd);

    if (std::getline(ss, l)) {
        request_line_parse(l);
    }

    while (std::getline(ss, l)) {
        head_line_parse(this->head, l);
    }

    // this->charset = "UTF-8";
    head_iterator hi = headval(this->head, "Content-Type");
    if (hi != this->head.end()) {
        split1(this->contenttype, this->charset, hi->second, "; \t");
        std::transform(this->contenttype.begin(), this->contenttype.end(), this->contenttype.begin(), tolower);
        std::transform(this->charset.begin(), this->charset.end(), this->charset.begin(), toupper);
    }

    hi = headval(this->head, "Content-Type");
    if (hi != this->head.end()) {
        // lexical_cast;
        this->contentlength = atoi(hi->second.c_str());
    }

    this->body.assign(body, end);

    std::cout << this->contenttype << " " << this->charset << " " << this->contentlength << "\n";

    return true;
}

struct stepreq : httpreq {
    // std::string first;
    int seconds;
};

struct cstep {
    struct stepreq req;
    // std::string method, path;
    // std::string head;
    // std::string body;
    int idcode;

    std::vector<std::pair<std::string, std::string> > glex;
};

struct cstat {
    cstat() : stepx(0), nloop(0) { }

    std::map<std::string, std::string> cookies;

    std::map<std::string, std::string> vars;

    std::string smblock;

    std::vector<struct cstep> steps;

    int stepx, nloop;
    int idcode;

    std::string imsi, smsc;
};

static std::map<unsigned int, struct cstat> stats_;

struct connection {
    struct ev_io io;

    struct httpreq req;
    // int _contlen;

    int xpkg;
    unsigned int cid;

    std::string imsi;
    std::string smsc;

    std::string buf;

	struct ev_loop *ev_loop;

    connection(struct ev_loop *loop)
        : ev_loop(loop), xpkg(-1) {
        memset(&this->io, 0, sizeof(this->io));
    }

    ~connection() {
        if (this->io.fd > 0) {
            ev_io_stop(this->ev_loop, &this->io);
            close(this->io.fd);
        }
    }
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

inline int set_nonblocking(int fd)
{
    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
#if 1 //defined(O_NONBLOCK)
    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    /* Otherwise, use the old way of doing it */
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
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
    set_nonblocking(fd);

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

	while ( (newfd = accept(ls->fd, (struct sockaddr *)&sa, &salen)) >= 0) {
        struct connection *c = new connection(loop);

        set_nonblocking(newfd);

        ev_io_init(&c->io, cb_read, newfd, EV_READ);
        ev_io_start(loop, &c->io);

        std::cout << "from: " << sa.sin_addr.s_addr << "\n";
	}

    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("accept");
    }
}

// static const std::string& getk(const std::string &k, std::map<std::string, std::string> &hds)
// {
//     std::map<std::string, std::string>::iterator it;
// 
//     it = hds.find(k);
//     if (it == hds.end())
//         return "";
//     return it->second;
// }

// static int get_content_length(struct content *c)
// {
//     std::string val = getk("Content-Length", c->head);
//     if (val.empty())
//         return 0;
//     return atoi(val.c_str());
// }

static int completed(struct connection *c, struct httpreq *req, std::string &data)
{
    if (req->method.empty()) {
        if (data.size() > 1024 * 32) {
            return -1;
        }

        std::string::iterator it; //std::string::iterator it;
        std::string cr = "\r\n\r\n";

        if ( (it = std::search(data.begin(), data.end(), cr.begin(), cr.end())) >= data.end()) {
            return 0;
        }

        if (!req->initrng(data.begin(), it + 4, it + 4)) {
            return -1;
        }

        data.erase(data.begin(), it + 4);

        if (req->contentlength > 1024 * 32) {
            return -1;
        }

        //<---------------
        head_iterator hi = headval(req->head, "Cookie"); //getk("Cookie", req->head);
        if (hi != req->head.end()) {
            std::cout << hi->second << "\n";
            if (starts_with(hi->second, "ck=")) {

                std::string::iterator eq = hi->second.begin() + 3;
                if (std::count(eq, hi->second.end(), '/') == 3) {
                    std::string tmp;
                    std::replace(eq, hi->second.end(), '/', ' ');

                    std::istringstream ss(std::string(eq, hi->second.end()));
                    ss >> tmp >> c->imsi >> c->smsc >> c->xpkg;
                    c->cid = strtol(tmp.c_str(), 0, 16);
                }
            }
        }
        std::cout << "ck: " << c->cid << " " << c->imsi << " " << c->smsc << " " << c->xpkg << "\n";
        //<---------------
    }

    if (data.size() < req->contentlength) {
        return 0;
    }

    if (!data.empty()) {
        req->body.assign(data.begin(), data.end());
    }

    return 1;
}

template <typename C_, typename I_>
static C_& assign_rsp(C_& rsp, int code, const char *shd, I_ beg, I_ end) // (std::vector<char>& rsp, std::vector<char>& body)
{
    // "Date: Tue, 24 Jul 2012 07:53:48 GMT\r\n"
    std::ostringstream o;

    o << "HTTP/1.1 " << code << " " << shd << "\r\n"
        "Server: nginx/1.0.11\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Set-Cookie: ck=ff/130/135/0\r\n"
        << "Content-Length: " << std::distance(beg, end) << "\r\n\r\n";

    const std::string& s = o.str();

    std::cout << s << "\n";

    rsp.assign(s.begin(), s.end());

    if (beg < end) {
        rsp.insert(rsp.end(), beg, end);
    }

    return rsp;
}

inline std::string headline(const std::string& k, const std::string& val)
{
    return (k + ": " + val + "\r\n");
}
inline std::string headline(const std::string& k, unsigned int len)
{
    char slen[32];
    snprintf(slen, 32, "%u", len);
    return headline(k, slen);
}

template <typename I_>
void part_add(std::string &out, int chn, int seconds, I_ beg, I_ end)
{
    std::string s;

    {
        std::ostringstream o;
        o << chn << " " << seconds << "\r\n";
        s = o.str();
    }

    out.insert(out.end(), s.begin(), s.end());
    out.insert(out.end(), beg, end);

    s = "\r\n\r\n";
    out.insert(out.end(), s.begin(), s.end());
}

static std::string step_go(struct stepreq& step, struct cstat& cst)
{
    // method line
    std::string part = step.method + " " + subst(step.path, cst.vars) + " " + step.ver + "\r\n"; // s = subst(step.first, cst.vars);

    // heads
    head_iterator it = step.head.begin();
    for (; it != step.head.end(); ++it) {
        part += headline(it->first, subst(it->second, cst.vars));
    }

    if (!cst.cookies.empty()) {
        std::ostringstream o;
        std::map<std::string, std::string>::iterator i = cst.cookies.begin();
        o << i->first << "=" << i->second;
        for (++i; i != cst.cookies.end(); ++i) {
            o << "; " << i->first << "=" << i->second;
        }
        part += headline("Cookies", o.str()); // s = o.str(); // part.insert(part.end(), s.begin(), s.end());
    }

    std::string tmp = subst(step.body, cst.vars);

    part += headline("Content-Length", tmp.size()); // s = o.str(); // part.insert(part.end(), s.begin(), s.end());
    part += "\r\n";
    part += tmp;

    std::string body;
    part_add(body, CN_SMBF, 1, cst.smblock.begin(), cst.smblock.end());
    part_add(body, CN_WAPQ, step.seconds, part.begin(), part.end());

    std::string rspbuf;
    return assign_rsp(rspbuf, 200, "OK", body.begin(), body.end());
}

static std::string step_fwd(struct cstat& cst)
{
    std::string rspbuf;

    if (cst.stepx >= cst.steps.size()) {
        if (cst.nloop <= 0) {
            return assign_rsp(rspbuf, 200, "FIN", (char*)0, (char*)0);
            // throw std::logic_error("No steps");
        }
        --cst.nloop;
        cst.stepx = 0;
    }

    rspbuf = step_go(cst.steps[cst.stepx + 1].req, cst);
    ++cst.stepx;

    return (rspbuf);
}

template <typename I_>
static bool is_utf8_doc(struct httprsp &rsp, I_ beg, I_ end)
{
    if (!rsp.contenttype.empty() && rsp.charset == "UTF-8") {
        return true;
    }

    std::string s = "<?xml";
    if (std::distance(beg, end) > s.size() && std::equal(s.begin(), s.end(), beg)) {
        return true;
    }

    if (rsp.contenttype.find("vnd.wap") != std::string::npos) {
        return true;
    }

    return false;
}

template <typename C_>
bool _302_fwd(C_& rspbuf, struct httprsp& rsp, struct stepreq& sreq, struct cstat& cst)
{
    head_iterator hi = headval(rsp.head, "Location");
    if (hi == rsp.head.end() || hi->second.empty()) {
        return false;
    }

    struct stepreq a = sreq;
    a.path = unescape(hi->second);
    a.seconds = 1;
    rspbuf = step_go(a, cst);

    return true;
}

template <typename C_>
static C_* auto_fwd(C_& rspbuf, struct httprsp& rsp, struct stepreq& sreq, struct cstat& cst)
{
    const char *v[] = {
        "revalidate*</head>*中国移动*确认*href='$'"
            , "revalidate*</head>*<card*href=\"$\""
            , "<?xml*<card*onenterforward*href=\"$\""
    };

    for (int i = 0; i < 3; ++i) {
        try {
            std::string url = glex(v[i], rsp.body.begin(), rsp.body.end());
            if (!url.empty()) {
                struct stepreq a = sreq;
                a.path = unescape(url);
                a.seconds = 1;
                rspbuf = step_go(a, cst);
                return &rspbuf;
            }
        } catch (...) {
        }
    }

    return 0;
}

template <typename M_>
static void take_cookie(M_& map, struct httprsp& rsp)
{
    head_iterator hi = headval(rsp.head, "Content-Type");
    if (hi == rsp.head.end()) {
        return;
    }

    std::string tmp, k, v;

    split1(tmp, v, hi->second, ";");
    split1(k, v, tmp, "=");

    map[k] = v; // cks.push_back(std::make_pair(k, v));
}

static const char* rsp_xpkg1(struct connection *c)
{
    std::map<unsigned int, struct cstat>::iterator ic = stats_.find(c->cid);
    if (ic == stats_.end()) {
        return "Not found ck";
    }

    struct cstat& cst = ic->second;
    struct httprsp rsp;

    if (cst.stepx < 1) {
        return "Invalid step";
    }

    std::string::iterator it, jt;
    const char *cr = "\r\n\r\n";

    if ( (it = std::search(c->req.body.begin(), c->req.body.end(), cr, cr + 4)) == c->req.body.end()
            || (jt = std::search(it + 4, c->req.body.end(), cr, cr + 4)) == c->req.body.end()) {
        return "Invalid para";
    }
    it += 4;
    jt += 4;

    (c->req.body.begin(), it);

    if (!rsp.initrng(it, jt, c->req.body.end())) {
        return "Bad head";
    }

    take_cookie(cst.cookies, rsp); //it, jt);

    if (rsp.code == 302) {
// bool _302_fwd(C_& rspbuf, struct httprsp& rsp, struct stepreq& sreq, struct cstat& cst)
        if (!_302_fwd(c->buf, rsp, cst.steps[cst.stepx - 1].req, cst)) {
            return "E302";
        }
    }

    // if (rsp.code != 206 && is_utf8_doc(rsp, jt, c->req.body.end())) {
    if (rsp.code == 200 && is_utf8_doc(rsp, jt, c->req.body.end())) {
        if (std::distance(jt, c->req.body.end()) < 1024*8) {
            rsp.body.assign(jt, c->req.body.end());
            if (auto_fwd(c->buf, rsp, cst.steps[cst.stepx - 1].req, cst)) {
                return 0;
            }
        }
    }

    if (cst.stepx < cst.steps.size()) {
        struct cstep& step = cst.steps[cst.stepx - 1];

        for (head_iterator i = step.glex.begin()
                ; i != step.glex.end(); ++i) {
            cst.vars[i->first] = glex(i->second, it, c->req.body.end());
        }
    }

    c->buf = step_fwd(cst);

    return 0;
}

struct probuf {
    const char *head;
    const char *data;
    const char *end;

    probuf(int bufsiz, const void *buf, int reserved) {
        head = data = (char*)buf;
        end = data + reserved;
    }
};

int pop_raw(struct probuf *buf, int len, void *out)
{
    int n = (buf->end - buf->data);

    if (len <= n) {
        if (out)
            memcpy(out, buf->data, n);
        buf->data += len; //(len = std::min(n, len));
        return len;
    }

    return 0;
}

template <typename Ti>
Ti pop_int(struct probuf *buf)
{
    Ti i = 0;
    if (pop_raw(buf, sizeof(Ti), (void*)&i) == sizeof(Ti)) {
        switch (sizeof(Ti)) {
            case 4:
                return ntohl(i);
            case 2:
                return ntohs(i);
        }
    }

    return i;
}

inline int siz_buf(struct probuf *buf) { return (buf->end - buf->data); }

template <typename Ti, typename Tc>
std::string pop_string(struct probuf *buf)
{
    std::vector<Tc> s;
    Ti len;
    if ( (len = pop_int<Ti>(buf)) > 0 && len <= siz_buf(buf)) {
        s.resize(len / sizeof(Tc));

        pop_raw(buf, len, (void*)&s[0]);

        if (sizeof(Tc) == 2) {
            size_t inlen = len;
            char *inp = (char*) &s[0];

            std::string utf8;

            iconv_t cd;
            if ( (cd = iconv_open("UTF-8", "UTF-16LE")) == (iconv_t)-1) {
                perror("iconv_open");
            } else {
                utf8.resize(3 * s.size());

                size_t avail = utf8.size();
                char *outp = &utf8[0];

                // size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
                if (iconv(cd, &inp, &inlen, &outp, &avail) == (size_t)-1) {
                    perror("iconv");
                    utf8.clear();
                } else {
                    utf8.resize(utf8.size() - avail);
                }

                iconv_close(cd);
            }

            return utf8; //(s.begin(), s.end());
        }
    }

    return std::string(s.begin(), s.end());
}

static const char *rsp_xpkg0(struct connection *c)
{
    struct cstat& cst = stats_[c->cid];

    struct probuf pbuf(c->req.body.length(), c->req.body.data(), c->req.body.length());

    cst = cstat(); // reset
    cst.imsi = c->imsi;
    cst.smsc = c->smsc;

    cst.idcode = pop_int<int32_t>(&pbuf);
    cst.nloop = pop_int<uint8_t>(&pbuf);

    for (int i = 0; i < cst.nloop; ++i) {
        struct cstep step;

        ; pop_int<int16_t>(&pbuf);

        // http content
        std::string qfirst = pop_string<int16_t,int16_t>(&pbuf) + "\r\n";
        std::string qhead = pop_string<int16_t,int16_t>(&pbuf) + "\r\n\r\n";
        std::string qbody = pop_string<int16_t,int16_t>(&pbuf);
        step.req.init(qfirst, qhead, qbody);

        ; pop_string<uint8_t,int16_t>(&pbuf);

        // extracter
        {
            int n = pop_int<uint8_t>(&pbuf);
            for (int j = 0; j < n; ++j) {
                std::string kw = pop_string<uint8_t,int16_t>(&pbuf);
                std::string ex = pop_string<uint8_t,int16_t>(&pbuf);
                step.glex.push_back(std::make_pair(kw, ex));
            }
        }

        step.req.seconds = pop_int<int16_t>(&pbuf);

        cst.steps.push_back(step);
        std::cout << "step " << i << ":\n" << qfirst << qhead << qbody << "\n" << step.req.seconds << "\n";
    }

    // smblocker
    {
        int i, n;
        const char *begin = pbuf.data;

        n = pop_int<uint8_t>(&pbuf);
        for (i = 0; i < n; ++i) {
            pop_string<uint8_t,int8_t>(&pbuf); // num-pat
            pop_string<uint8_t,int16_t>(&pbuf); // cont-pat
            pop_int<int16_t>(&pbuf); // duration
        }

        cst.smblock.assign(begin, pbuf.data);
    }

    c->buf = step_fwd(cst);

    return 0;
}

static const char* setup_rsp(struct connection *c)
{
    const char* (*fp[])(struct connection*) = { rsp_xpkg0, rsp_xpkg1 };

    if (c->xpkg >= sizeof(fp)/sizeof(fp[0])) {
        return "Bad para";
    }

    return (fp[c->xpkg])(c);
}

static int recvbuf(std::string& buf, int fd)
{
    int n, siz;

    siz = buf.size();
    n = buf.capacity() - siz;

    n = std::max(n, 1024*8);
    buf.resize(siz + n);

    while ( (n = recv(fd, &buf[siz], n, 0)) > 0) {
        buf.resize(siz + n);
    }

    return n;
}

static void cb_read(struct ev_loop *loop, struct ev_io *_c, int revents)
{
    struct connection *c = (struct connection*)_c;
    int fd = _c->fd;
	int n;

    if ( (n = recvbuf(c->buf, fd)) < 0) {
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
        const char* bye = "bye.";
        printf("recv %d completed\n", fd);

        try {
            const char *err;

            if ( (err = setup_rsp(c)) == 0) {
                printf("rsp %d ok\n", fd);
            } else {
                // delete c;
                printf("rsp %d error\n", fd);
                assign_rsp(c->buf, 501, err, bye,bye+4);
            }
        } catch (const std::exception &e) {
            assign_rsp(c->buf, 501, e.what(), bye,bye+4);
        }

        ev_io_stop(loop, &c->io);
        ev_io_init(&c->io, cb_write, fd, EV_WRITE);
        ev_io_start(loop, &c->io);

    } else {
        if (n == 0) {
            delete c;
        }
    }
}

static int sendbuf(int fd, std::string &buf)
{
    int n;

	while ( (n = send(fd, &buf[0], buf.size(), 0)) > 0) {
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

