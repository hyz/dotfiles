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

#define BUFSIZE 1024

void err_exit(char const*msg) {
    perror(msg);
    exit(0);
}

typedef struct sockaddr SA;

static int32_t buf_[1024*64/4];

struct sent_status {
    unsigned seq_numb, bytes_sent;
};

static time_t start_tp_ = 0;
void print(struct sent_status* ss)
{
    static time_t print_tp = 0;
    time_t ct = time(0);
    if ( (ct - print_tp > 5 && (ss->seq_numb&0x1f)==0) || (ss->seq_numb & 0x1ff) == 0 || ss->seq_numb<5) {
        int nsec = (ct - start_tp_);
        if (nsec == 0)
            nsec = 1;
        printf("%u %.1f %u\n", ss->seq_numb, ss->bytes_sent/1024.0/nsec, ss->bytes_sent);
        print_tp=ct;
    }
}

int main(int argc, char *const argv[])
{
    int sockfd, port;
    struct sockaddr_in saddr;
    // struct hostent *server;
    char *ip;
    int mils = 0;
    struct sent_status ss;

    if (argc < 3) {
       fprintf(stderr,"usage: %s <ip> <port> [milisecond]\n", argv[0]);
       exit(0);
    }
    ip = argv[1];
    port = atoi(argv[2]);
    if (argc > 3)
        mils = atoi(argv[3]);
    srand(time(0));

    ss.seq_numb = 0;
    ss.bytes_sent = 0;

    bzero((char *) &saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &saddr.sin_addr);
    saddr.sin_port = htons(port);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        err_exit("ERROR:socket");
    start_tp_ = time(0);
    for ( ;; ) {
        struct timeval tv;
        // fd_set rfds;
        socklen_t slen = sizeof(saddr);
        unsigned len = 1024*(rand()%24 + 6);
        int retval;

        buf_[0] = len;
        buf_[1] = ss.seq_numb;
        retval = sendto(sockfd, (void*)buf_, len, 0, (SA*)&saddr, slen);
        if (retval < 0) 
            err_exit("ERROR:sendto");
        print(&ss);
        ss.seq_numb++;
        ss.bytes_sent += len;

        //FD_ZERO(&rfds);
        //FD_SET(sockfd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = mils*1000;
        retval = select(0, NULL, NULL, NULL, &tv);
    }
    
    //n = recvfrom(sockfd, buf, strlen(buf), 0, (SA*)&saddr, &slen);
    //if (n < 0) 
    //  err_exit("ERROR in recvfrom");
    return 0;
}

