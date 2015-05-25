#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <boost/lexical_cast.hpp>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>

#include "ios_push.h"

using namespace std;
int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
    strncpy(buf, (char *)(password), size);
    buf[size - 1] = '\0';
    return(strlen(buf));
}

bool ios_push::ssl_connect::connect_()
{
    int err;
    /* Load encryption & hashing algorithms for the SSL program */
    SSL_library_init();
    /* Load the error strings for SSL & CRYPTO APIs */
    SSL_load_error_strings();
    /* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
    meth = (SSL_METHOD*)SSLv3_method();
    /* Create an SSL_CTX structure */
    ctx = SSL_CTX_new(meth);                        
    if(!ctx) {
        std::cout<<"Could not get SSL Context"<<std::endl;
        return false;
    }

    // /* Load the CA from the Path */
    // if(SSL_CTX_load_verify_locations(ctx, NULL, capath) <= 0)
    // {
    //     /* Handle failed load here */
    //     cout<<"Failed to set CA location...";
    //     // ERR_print_errors_fp(stderr);
    //     exit(1);
    // }
    string pass = PERM_PASSWORD;
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (char*)pass.c_str());
    SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);

    string cf = config_.capath + config_.certfile;
    string kf = config_.capath + config_.keyfile;
    cout<<cf<<endl;
    cout<<kf<<endl;
    /* Load the client certificate into the SSL_CTX structure */
    if (SSL_CTX_use_certificate_file(ctx, cf.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cout<<"Cannot use Certificate File"<<std::endl;
        // ERR_print_errors_fp(stderr);
        return false;
    }
    /* Load the private-key corresponding to the client certificate */
    if (SSL_CTX_use_PrivateKey_file(ctx, kf.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cout<<"Cannot use Private Key"<<std::endl;
        // ERR_print_errors_fp(stderr);
        return false;
    }
    /* Check if the client certificate and private-key matches */
    if (!SSL_CTX_check_private_key(ctx)) {
        std::cout<<"Private key does not match the certificate public key"<<std::endl;
        return false;
    }

    /* Set up a TCP socket */
    sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);      
    if(sock == -1) {
        std::cout<<"Could not get Socket"<<std::endl;
        return false;
    }
    if(!host_info){
        memset (&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = htons(config_.port);       /* Server Port number */
        host_info = gethostbyname(config_.host.c_str());
        if(host_info) {
            /* Take the first IP */
            struct in_addr *address = (struct in_addr*)host_info->h_addr_list[0];
            server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */
        }
        else {
            cout<<"Could not resolve hostname:"<<config_.host;
            return false;
        }
    }
    /* Establish a TCP/IP connection to the SSL client */
    err = connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if(err == -1) {
        cout<<"Could not connect";
        return false;
    }    

    /* An SSL structure is created */
    ssl = SSL_new(ctx);
    if(!ssl) {
        cout<<"Could not get SSL Socket";
        return false;
    }    
    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
    SSL_set_fd(ssl, sock);
    /* Perform SSL Handshake on the SSL client */
    err = SSL_connect(ssl);
    if(err <= 0) {
        cout<<"Could not connect to SSL Server";
        return false;
    }
    
    useful = true;
    return true;
}

void ios_push::ssl_connect::close_()
{
    int err;
    if(!useful) return;
    /* Shutdown the client side of the SSL connection */
    err = SSL_shutdown(ssl);
    if(err == -1) {
        cout<<"Could not shutdown SSL";
        return;
    }    

    /* Terminate communication on a socket */
    err = close(sock);
    if(err == -1) {
        cout<<"Could not close socket";
        return;
    }    

    /* Free the SSL structure */
    SSL_free(ssl);
    /* Free the SSL_CTX structure */
    SSL_CTX_free(ctx);
    useful = false;
}

bool ios_push::push_message(const string& deviceTokenHex, const string& token, string& m)
{
    bool rtn = false;
    cout <<"deviceTokenHex:"<<deviceTokenHex<<endl;
    if(deviceTokenHex.empty()){
        cout<<"deviceTokenHex not available";
        return false;
    }
    if(!ssl_handle.ssl_available()){
        if(!ssl_handle.connect_()){
            return rtn;
        }
    }
    /* Convert the Device Token */
    int j = 0;
    char tmp[3];
    char deviceTokenBinary[DEVICE_BINARY_SIZE];
    for(std::string::const_iterator itr = deviceTokenHex.begin();
            itr != deviceTokenHex.end();) 

    {
        if( ' ' == *itr ) {
            ++itr;
        }
        else {
            tmp[0] = *itr++;
            tmp[1] = *itr++;
            tmp[2] = '\0';

            deviceTokenBinary[j++] =  strtoul(tmp, NULL, 16);
            cout<<"tmp:"<<tmp<<" tmpi:"<<strtoul(tmp, NULL, 16)<<endl;
        }
    }

    // json::object head = json::object() ("method", m.type)
    //         ("token", token)
    //         ("sequence", m.id())
    //         ;

    // json::object body = m.body + json::object()
    //         ("from", m.from)
    //         ("sid", m.gid)
    //         ;
    // json::object alert = json::object() ("head",head)
    //         ("body",body)
    //         ;
    // json::object aps = json::object() ("alert",alert)
    //         ("badge",1)
    //         ("sound","default")
    //         ;

    // string message = json::encode(json::object()
    //         ("aps",aps)
    //         );

    // std::cout<<"push message:"<<message<<endl;
    uint8_t command = 1; /* command number */
    uint32_t len = sizeof(uint8_t) + 2*sizeof(uint32_t)+ 2*sizeof(uint16_t) 
        + DEVICE_BINARY_SIZE + m.length();

    string binaryMessageBuff(len,0);
    /* message format is, |COMMAND|ID|EXPIRY|TOKENLEN|TOKEN|PAYLOADLEN|PAYLOAD| */
    char *binaryMessagePt = const_cast<char*>(binaryMessageBuff.c_str());
    uint32_t whicheverOrderIWantToGetBackInAErrorResponse_ID = 1234;
    // expire message if not delivered in 1 day
    uint32_t networkOrderExpiryEpochUTC = htonl(time(NULL)+86400);
    uint16_t networkOrderTokenLength = htons(DEVICE_BINARY_SIZE);
    uint16_t networkOrderPayloadLength = htons(m.length());

    /* command */
    *binaryMessagePt++ = command;
    /* provider preference ordered ID */
    memcpy(binaryMessagePt, &whicheverOrderIWantToGetBackInAErrorResponse_ID, sizeof(uint32_t));
    binaryMessagePt += sizeof(uint32_t);
    /* expiry date network order */
    memcpy(binaryMessagePt, &networkOrderExpiryEpochUTC, sizeof(uint32_t));
    binaryMessagePt += sizeof(uint32_t);
    /* token length network order */
    memcpy(binaryMessagePt, &networkOrderTokenLength, sizeof(uint16_t));
    binaryMessagePt += sizeof(uint16_t);
    /* device token */
    memcpy(binaryMessagePt, deviceTokenBinary, DEVICE_BINARY_SIZE);
    binaryMessagePt += DEVICE_BINARY_SIZE;
    /* payload length network order */
    memcpy(binaryMessagePt, &networkOrderPayloadLength, sizeof(uint16_t));
    binaryMessagePt += sizeof(uint16_t);
    /* payload */
    memcpy(binaryMessagePt, m.c_str(), m.length());
    binaryMessagePt += m.length();

    cout<<"binaryMessagePt-binaryMessageBuff:"<<binaryMessageBuff.length()<<endl;
    cout<<"m.length():"<<m.length()<<endl;
    cout<<"============================================================================="<<endl;
    for(int i= 0; i<binaryMessageBuff.length(); ++i){
            printf("%x ", binaryMessageBuff[i]);
            if(0==i%10 && 0!=i) cout<<endl;
    }
    cout<<endl;
    cout<<"============================================================================="<<endl;
    if (SSL_write(ssl_handle.ssl,binaryMessageBuff.c_str(),binaryMessageBuff.length()) > 0){
        rtn = true;
        cout<<"pushed message to IOS"<<endl;
    }
    else {
        ssl_handle.close_();
        cout<<"failed to push message to IOS"<<endl;
    }
    return rtn;
}

int main(int argc, char *argv[])
{

    /* Phone specific Payload message as well as hex formated device token */
    const char     *deviceTokenHex = NULL;
    if(argc == 1)
    {
        deviceTokenHex = "a642ccd0a54a82f6c02738674e1becc47d69ba6aed3c06b9fb745bf47a72b0e3";
    }
    else
    {
        deviceTokenHex = argv[1];
    }

    string token = "52299a9360af909f";
    // string msg = "{\"aps\":{\"alert\":{\"head\":{\"method\":\"chat/text\",\"token\":\"5b37c4e\",\"sequence\":54},\"body\":{\"from\":2578,\"content\":\"Conk\",\"sid\":\"KK1007A\"}},\"badge\":5,\"sound\":\"default\"},\"acme1\":\"bar\", \"acme2\":[\"bang\",\"whiz\"]}";
    // string msg = "{\"aps\":{\"alert\": \"body\" : \"Bob wants to play poker\",\"badge\":1,\"sound\":\"default\"}}";
    string msg="{\"aps\":{\"alert\":\"Message received from Bob\"}}";
    // string msg="{\"aps\":{\"alert\":\"SteveJ: Hello, world!\",\"sound\":\"default\"}}";
    // string msg="";
    // string msg="";
    ios_push::iso_push_inst().push_message(deviceTokenHex,token, msg);
    return 0;
}
