#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h> 
#include <netdb.h> 
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <exception>
#include <boost/noncopyable.hpp>

#define BUFSIZE 1024

void exit127_(int,int) { exit(127); }
#define ERR_EXIT(...) exit127_( fprintf(stderr, __VA_ARGS__), fprintf(stderr, " error:%d: %s\n",__LINE__, strerror(errno)) )
#define ERR_MSG(...) (void)( fprintf(stderr, __VA_ARGS__), fprintf(stderr, " error:%d: %s\n",__LINE__, strerror(errno)) )
#define DBG_MSG(...) (void)( fprintf(stderr, __VA_ARGS__), fprintf(stderr, " debug:%d\n",__LINE__) )

typedef struct sockaddr SA;

struct Main : boost::noncopyable
{
    Main(char const* dst_ip, int dst_port, int idle)
    {
        singleton = this;
        runc_ = 0x7fffffff;
        gettimeofday(&tv0_, NULL);
        bzero((void*)&sent_,sizeof(sent_));
        bzero((void*)&recv_,sizeof(recv_));
        mils_idle_ = idle;
        socket_ = udp_connect(dst_ip, dst_port);
        run_ = &Main::do_send;
        printf("sending ...\n");
    }
    Main(int bind_port)
    {
        singleton = this;
        runc_ = 0x7fffffff;
        gettimeofday(&tv0_, NULL);
        bzero((void*)&sent_,sizeof(sent_));
        bzero((void*)&recv_,sizeof(recv_));
        socket_ = udp_bind("0", bind_port);
        run_ = &Main::do_receive;
        printf("receiving ...\n");
    }
    ~Main() { close(socket_); }

    void run() {
        srand(time(0));
        while ( runc_-- > 0 ) {
            (this->*run_)();
        }
    }

private:
    unsigned runc_; // = 0x7fffffff;
    void (Main::*run_)();
    static Main* singleton;

    int socket_;
    enum { max_length = 1024*128 };
    int32_t data_[max_length/4];

    void do_receive()
    {
        int len = recv(socket_, (void*)data_, sizeof(data_), 0);
        if (len < 8) {
            ERR_EXIT("recv %d", len);
        }
        recv_.incoming(ntohl(data_[0]), ntohl(data_[1]), len);
    }

    unsigned mils_idle_;
    void do_send()
    {
        // fd_set rfds;
        //FD_ZERO(&rfds);
        //FD_SET(socket_, &rfds);
        int randval = rand()%100;
        unsigned length = 1024*( (randval>90?15:5) + randval%5);
        data_[0] = htonl(length);
        data_[1] = htonl(sent_.seq);
        sent_.seq++;

        int retval = send(socket_, (void*)data_, length, 0);
        if (retval < 0) 
            ERR_EXIT("send");
        sent_.bytes_sent += retval;
        sent_.print();

        if (mils_idle_ > 0) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(mils_idle_));
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = mils_idle_*1000;
            retval = select(0, NULL, NULL, NULL, &tv);
        }
    }

private:
    struct timeval tv0_;
    static unsigned sMills() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (1000*1000*(tv.tv_sec - singleton->tv0_.tv_sec) + tv.tv_usec - singleton->tv0_.tv_usec)/1000;
    }

    struct recv_st
    {
        unsigned seq;
        unsigned pkgcount;
        unsigned bytes_recvd;
        unsigned bytes_exped;

        void incoming(unsigned len, unsigned seq, unsigned nbytes) {
			if (pkgcount == 0)
				gettimeofday(&Main::singleton->tv0_, NULL);
            recv_st prev = *this;

            this->pkgcount++;
            this->bytes_exped += len;
            this->seq = seq;
            this->bytes_recvd += nbytes;

            this->print(prev);
        }

        void print(recv_st const& prev) const {
            bool lost = (seq != prev.seq+1)
                || (bytes_recvd - prev.bytes_recvd != bytes_exped - prev.bytes_exped);
            unsigned ct = Main::sMills();
            if (lost || ((ct - prt_) > 5 && (seq&0x3f)==0) || (seq&0x7ff) == 0 || seq<5) {
                int nsec = (ct)/1000;
                if (nsec == 0)
                    nsec = 1;
                printf("%3u.%03u %4u %-4u %-4u %6.1f %7.1f %u%c", ct/1000,ct%1000
                        , prev.seq, seq, pkgcount, float(pkgcount)/nsec, bytes_recvd/1024.0/nsec, bytes_recvd, lost?'\t':'\n');
                if (lost)
                    printf("* %u\n", bytes_recvd-prev.bytes_recvd);
                prt_=ct;
            }
        }
        
        mutable unsigned prt_;
    } recv_;

    struct sent_st {
        unsigned seq;
        unsigned bytes_sent;

        //unsigned numb_psec_ = 30;
        //unsigned bytes_psec_ = 1024*300;

        void print() const {
            unsigned ct = Main::sMills();
            int nsec = ct/1000;
            if (nsec == 0)
                nsec = 1;
            if (((ct - prt_) > 5 && (seq&0x1f)==0) || (seq&0x1ff) == 0 || seq < 5) {
                printf("%3u.%03u %4u %4.1f %u\n", ct/1000,ct%1000, seq, bytes_sent/1024.0/nsec, bytes_sent);
                prt_ = ct;
            }
        }

        mutable unsigned prt_;
    } sent_;

private:
    static int udp_connect(char const* ip, short port)
    {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        struct sockaddr_in saddr;
        bzero((char *) &saddr, sizeof(saddr));
        saddr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &saddr.sin_addr);
        saddr.sin_port = htons( port );

        socklen_t slen = sizeof(saddr);
        if (connect(sockfd, (SA*)&saddr, slen) < 0) {
            ERR_EXIT("connect %s:%d", ip, port);
        }
        return sockfd;
    }
    static int udp_bind(char const* ip, short port)
    {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        struct sockaddr_in saddr;
        bzero((char *) &saddr, sizeof(saddr));
        saddr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &saddr.sin_addr);
        saddr.sin_port = htons( port );

        socklen_t slen = sizeof(saddr);
        if (bind(sockfd, (SA*)&saddr, slen) < 0) {
            ERR_EXIT("bind %s:%d", ip, port);
        }
        return sockfd;
    }
};
Main* Main::singleton;

int main(int argc, char* argv[])
{
    try {
        if (argc >= 3) {
            int idle = (argc>3 ? std::atoi(argv[3]) : 0);
            Main sender(argv[1], atoi(argv[2]), idle);
            sender.run();
        } else if (argc == 2) {
            Main receiver( atoi(argv[1]) );
            receiver.run();
        } else {
            fprintf(stderr,"Usage: a.out [host] <port> [idle-mills]\n");
            exit(1);
        }
    } catch (std::exception const& e) {
        ERR_EXIT("Exception: %s", e.what());
    }

    return 0;
}


//static int32_t buf_[1024*64/4];
//
//struct sent_status {
//    unsigned seq_numb, bytes_sent;
//};
//
//static time_t start_tp_ = 0;
//void print(struct sent_status* ss)
//{
//    static time_t print_tp = 0;
//    time_t ct = time(0);
//    if ( (ct - print_tp > 5 && (ss->seq_numb&0x1f)==0) || (ss->seq_numb & 0x1ff) == 0 || ss->seq_numb<5) {
//        int nsec = (ct - start_tp_);
//        if (nsec == 0)
//            nsec = 1;
//        printf("%u %.1f %u\n", ss->seq_numb, ss->bytes_sent/1024.0/nsec, ss->bytes_sent);
//        print_tp=ct;
//    }
//}
//
//int main(int argc, char *const argv[])
//{
//    int sockfd, port;
//    struct sockaddr_in saddr;
//    // struct hostent *server;
//    char *ip;
//    int mils = 0;
//    struct sent_status ss;
//
//    if (argc < 3) {
//       fprintf(stderr,"usage: %s <ip> <port> [milisecond]\n", argv[0]);
//       exit(0);
//    }
//    ip = argv[1];
//    port = atoi(argv[2]);
//    if (argc > 3)
//        mils = atoi(argv[3]);
//    srand(time(0));
//
//    ss.seq_numb = 0;
//    ss.bytes_sent = 0;
//
//    bzero((char *) &saddr, sizeof(saddr));
//    saddr.sin_family = AF_INET;
//    inet_pton(AF_INET, ip, &saddr.sin_addr);
//    saddr.sin_port = htons(port);
//
//    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//    if (sockfd < 0) 
//        ERR_EXIT("ERROR:socket");
//    start_tp_ = time(0);
//    for ( ;; ) {
//        struct timeval tv;
//        // fd_set rfds;
//        socklen_t slen = sizeof(saddr);
//        unsigned len = 1024*(rand()%24 + 6);
//        int retval;
//
//        buf_[0] = len;
//        buf_[1] = ss.seq_numb;
//        retval = sendto(sockfd, (void*)buf_, len, 0, (SA*)&saddr, slen);
//        if (retval < 0) 
//            ERR_EXIT("ERROR:sendto");
//        print(&ss);
//        ss.seq_numb++;
//        ss.bytes_sent += len;
//
//        //FD_ZERO(&rfds);
//        //FD_SET(sockfd, &rfds);
//        tv.tv_sec = 0;
//        tv.tv_usec = mils*1000;
//        retval = select(0, NULL, NULL, NULL, &tv);
//    }
//    
//    //n = recvfrom(sockfd, buf, strlen(buf), 0, (SA*)&saddr, &slen);
//    //if (n < 0) 
//    //  ERR_EXIT("ERROR in recvfrom");
//    return 0;
//}
