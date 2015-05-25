#include<iostream>
#include<string>
#include<stdio.h> 
#include<string.h> 
#include<sys/socket.h> 
#include<netinet/in.h> 
#include<arpa/inet.h>
#include "../jss.h"
# define MAXDATASIZE 1024

# define SERVERIP "192.168.1.55" 
# define SERVERPORT 9900
using namespace std;

int main( int argc, char * argv[ ] ) 
{ 
    char buf[ MAXDATASIZE] = {0} ; 
    int sockfd, numbytes; 
    struct sockaddr_in server_addr; 
    if ( ( sockfd = socket ( AF_INET , SOCK_STREAM , 0) ) == - 1) { 
        perror ( "socket error" ) ; 
        return 1; 
    } 
    memset ( & server_addr, 0, sizeof ( struct sockaddr ) ) ; 
    server_addr.sin_family = AF_INET ; 
    server_addr.sin_port = htons ( SERVERPORT) ; 
    server_addr.sin_addr.s_addr = inet_addr( SERVERIP) ; 
    if ( connect ( sockfd, ( struct sockaddr * ) &server_addr, sizeof ( struct sockaddr ) ) == - 1) { 
        perror ( "connect error" ) ; 
        return 1; 
    } 

    char *head = "{\"method\":\"hello\",\"token\":\"5223f7488896fa26\"}";
    uint16_t headlen = strlen(head);
    uint16_t bodylen = 0;
    char *body;
    char *msg = new char[sizeof(uint16_t)*2 + headlen + bodylen + 1]();
    headlen = htons(headlen);
    bodylen = htons(bodylen);
    char *p = msg;
    memmove(p, &headlen, sizeof(uint16_t));
    memmove(p+sizeof(uint16_t), &bodylen, sizeof(uint16_t));
    memmove(p+sizeof(uint16_t)*2, head, strlen(head));

    cout <<"send:"<<head<<endl;
    if ( send ( sockfd, msg ,sizeof(uint16_t)*2 + strlen(head), 0) == - 1) { 
        perror ( "send error" ) ; 
        return 1; 
    } 
    delete [] msg;

    int i = 10;
    while(1){
        int len;
    cout<<"recving .."<<endl;
        if ( ( numbytes = recv ( sockfd, buf, MAXDATASIZE, 0) ) == - 1) { 
            perror ( "recv error" ) ; 
            return 1; 
        } 
        union {
            uint16_t h[2];
            char s[sizeof(uint16_t) * 2];
        } u;

        u.h[0] = ntohs(*(uint16_t*)buf);
        *(buf+numbytes) = 0;
        string out(buf+4,u.h[0]);
        cout << out<<endl;
        json::object jo = json::decode(out);
        int seq = jo.get<int>("sequence");
        json::object back = json::object()("method","ack")
            ("token","5223f7488896fa26")
            ("sequence",seq);

        string head = json::encode(back);
        string body;
        u.h[0] = htons(head.size());
        u.h[2] = htons(body.size());
        len = 2*sizeof(uint16_t)+head.size()+body.size();     
        memmove(buf,(string(&u.s[0], sizeof(u)) + head + body).c_str(),len);
        send(sockfd, buf, len,0);
        }

        cout<<"recved .."<<endl;
        close ( sockfd) ; 
    return 0; 
}
