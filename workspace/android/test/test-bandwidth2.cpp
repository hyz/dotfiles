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
#include <algorithm>
#include <boost/noncopyable.hpp>

inline void exit127_(int,int) { exit(127); }
#define ERR_EXIT(...) exit127_( fprintf(stderr, __VA_ARGS__), fprintf(stderr, " error:%d: %s\n",__LINE__, strerror(errno)) )
#define ERR_MSG(...) (void)( fprintf(stderr, __VA_ARGS__), fprintf(stderr, " error:%d: %s\n",__LINE__, strerror(errno)) )
#define DBG_MSG(...) (void)( fprintf(stderr, __VA_ARGS__), fprintf(stderr, " debug:%d\n",__LINE__) )

typedef struct sockaddr SA;

struct Main : boost::noncopyable
{
    Main(char const* dst_ip, int dst_port, int Nps, int Kps)
    {
        singleton = this;
        gettimeofday(&tv0_, NULL);
        bzero((void*)&sent_,sizeof(sent_));
        bzero((void*)&recv_,sizeof(recv_));

        numbps_ = numbpx_ = Nps;
        numbrush_ = numbrx_ = 0;
        bytes1_ = std::max(Kps*1024/Nps, 1500);

        sockaddr_init(&saddr_, dst_ip, dst_port); // socket_ = udp_connect(dst_ip, dst_port);
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        runc_ = 0x7fffffff;
        run_ = &Main::do_send;
        printf("sending ...\n");
    }
    Main(short bind_port)
    {
        singleton = this;
        gettimeofday(&tv0_, NULL);
        bzero((void*)&sent_,sizeof(sent_));
        bzero((void*)&recv_,sizeof(recv_));
        socket_ = udp_bind("0", bind_port);
        runc_ = 0x7fffffff;
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
    struct timeval tv0_;
    static unsigned sMills() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (1000*1000*(tv.tv_sec - singleton->tv0_.tv_sec) + tv.tv_usec - singleton->tv0_.tv_usec)/1000;
    }

    struct recv_st
    {
        unsigned seq;
        unsigned n_dgram;
        unsigned bytes_recvd;
        unsigned bytes_exped;

        void incoming(unsigned len, unsigned seq, unsigned nbytes) {
			if (n_dgram == 0)
				gettimeofday(&Main::singleton->tv0_, NULL);
            recv_st prev = *this;

            this->n_dgram++;
            this->bytes_exped += len;
            this->seq = seq;
            this->bytes_recvd += nbytes;

            this->print(prev);
        }

        void print(recv_st const& prev) const {
            char const* seqv = 0;
            if (seq != prev.seq+1) {
                char const* legv[3] = { "misorder", "repeated", "lost" };
                seqv = legv[(seq<prev.seq ? -1 : (seq==prev.seq ? 0 : 1)) +1];
            }

            unsigned ct = Main::sMills();
            if (seqv || ((ct - tpr_) > 5 && (seq&0x7f)==0) || (seq&0x7ff) == 0 || seq<3) {
                int nsec = (ct)/1000;
                if (nsec == 0)
                    nsec = 1;
                printf("%3u.%03u %4u %-4u %-4u %6.1f %7.1f %u%c", ct/1000,ct%1000
                        , prev.seq, seq, seq+1-n_dgram, float(n_dgram)/nsec, bytes_recvd/1024.0/nsec, bytes_recvd, seqv?' ':'\n');
                if (seqv)
                    printf("* %s\n", seqv);//("* %u a %u\n", bytes_recvd-prev.bytes_recvd, bytes_recvd);
                tpr_=ct;
            }
        }
        
        mutable unsigned tpr_;
    } recv_;

    struct sent_st {
        unsigned seq;
        unsigned bytes_sent;

        void outgoing(unsigned, unsigned bytes) {
            sent_st const& prev = *this;
            seq++;
            bytes_sent += bytes;
            this->print(prev);
        }
        //unsigned numb_psec_ = 30;
        //unsigned bytes_psec_ = 1024*300;

        void print(sent_st const&) const {
            unsigned ct = Main::sMills();
            int nsec = ct/1000;
            if (nsec == 0)
                nsec = 1;
            if (((ct - tpr_) > 5 && (seq&0x7f)==0) || (seq&0x7ff) == 0 || seq < 3) {
                printf("%3u.%03u %4u %4.1f %4.1f %u\n", ct/1000,ct%1000, seq, float(seq+1)/nsec, bytes_sent/1024.0/nsec, bytes_sent);
                tpr_ = ct;
            }
        }

        mutable unsigned tpr_;
    } sent_;

private:
    unsigned char numbps_, numbpx_, numbrush_, numbrx_;
    unsigned short bytes1_;

    unsigned runc_; // = 0x7fffffff;
    void (Main::*run_)();
    static Main* singleton;

    int socket_;
    enum { max_length = 1024*80 };
    int32_t data_[max_length/4];

    void do_receive()
    {
        int len = recv(socket_, (void*)data_, sizeof(data_), 0);
        if (len < 32) {
            ERR_EXIT("recv %d", len);
        }
        recv_.incoming(ntohl(data_[0]), ntohl(data_[1]), len);
    }

    void do_send()
    {
        if (numbrx_ >= numbrush_ && numbpx_ >= numbps_) {
            numbrx_ = numbpx_ = 0;
            numbrush_ = 3+rand()%3;
        }

        int wmills = 1000/(numbps_+1);//int raval = rand()%100;
        unsigned length = unsigned(bytes1_) + rand()%3000 - 1500; //= 1024*( (raval>90?15:5) + raval%5);
        if (numbrx_ < numbrush_) {
            numbrx_++;
            wmills = 0;
            //length = length * (1 + rand()%200/100.0);
        } else if (numbpx_ < numbps_) {
            numbpx_++;
        }
        if (length > 1024*56) {
            DBG_MSG("%u > 56K", length/1024);
            length = std::min(56*1024, int(length));
        }

        data_[0] = htonl(length);
        data_[1] = htonl(sent_.seq);
        //data_[2] = htonl(1);

        // fd_set fds; //FD_ZERO(&fds); //FD_SET(socket_, &fds);
        int retval = sendto(socket_, (void*)data_, length, 0, (SA*)&saddr_, (socklen_t)sizeof(saddr_));
        if (retval < 0) 
            ERR_EXIT("sendto");
        sent_.outgoing(sent_.seq, length);

        if (wmills > 0) {
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * wmills;//(numbex_>0 ? 1 : 1000/numbps_)*1000;
            select(0, NULL, NULL, NULL, &tv);
            //std::this_thread::sleep_for(std::chrono::milliseconds(mils_idle_));
        }
    }

private:
    struct sockaddr_in saddr_;

    static void sockaddr_init(struct sockaddr_in* sa, char const* ip, short port)
    {
        bzero((char *) sa, sizeof(struct sockaddr_in));
        sa->sin_family = AF_INET;
        inet_pton(AF_INET, ip, &sa->sin_addr);
        sa->sin_port = htons( port );
    }
    static int udp_connect(char const* ip, short port)
    {
        struct sockaddr_in saddr;
        socklen_t slen = sizeof(struct sockaddr_in);
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        sockaddr_init(&saddr,ip, port);
        if (connect(sockfd, (SA*)&saddr, slen) < 0) {
            ERR_EXIT("connect %s:%d", ip, port);
        }
        return sockfd;
    }
    static int udp_bind(char const* ip, short port)
    {
        struct sockaddr_in saddr;
        socklen_t slen = sizeof(struct sockaddr_in);
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        sockaddr_init(&saddr,ip, port);
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
        if (argc == 5) {
            Main sender(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
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

