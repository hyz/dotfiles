#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
using namespace std;

#define CA_CERT_PATH    "/home/lindu/ios_apns/"
// #define CA_CERT_PATH    "./"

// #if defined(IS_DEVELOPMENT_VERSION)
/* Development Certificates */
// #define RSA_CLIENT_CERT     "Certs/apns-dev-cert.pem"
// #define RSA_CLIENT_KEY      "Certs/apns-dev-key.pem"

/* Development Connection Infos */
#define APPLE_HOST          "gateway.sandbox.push.apple.com"
#define APPLE_PORT          2195

#define APPLE_FEEDBACK_HOST "feedback.sandbox.push.apple.com"
#define APPLE_FEEDBACK_PORT 2196
// #else
// /* Distribution Certificates */
#define RSA_CLIENT_KEY      "PushChatKey.pem"
#define RSA_CLIENT_CERT     "PushChatCert.pem"
// 
// /* Release Connection Infos */
// #define APPLE_HOST          "gateway.push.apple.com"
// #define APPLE_PORT          2195
// 
// #define APPLE_FEEDBACK_HOST "feedback.push.apple.com"
// #define APPLE_FEEDBACK_PORT 2196
// #endif

#define DEVICE_BINARY_SIZE  32
#define MAXPAYLOAD_SIZE     256

typedef struct {
    /* SSL Vars */
    SSL_CTX         *ctx;
    SSL             *ssl;
    SSL_METHOD      *meth;
    X509            *server_cert;
    EVP_PKEY        *pkey;

    /* Socket Communications */
    struct sockaddr_in   server_addr;
    struct hostent      *host_info;
    int                  sock;
} SSL_Connection;

typedef struct {
    /* The Message that is displayed to the user */
    char *message;

    /* The name of the Sound which will be played back */
    char *soundName;

    /* The Number which is plastered over the icon, 0 disables it */
    int badgeNumber;

    /* The Caption of the Action Key the user needs to press to launch the Application */
    char *actionKeyCaption;

    /* Custom Message Dictionary, which is accessible from the Application */
    char* dictKey[5];
    char* dictValue[5];
} Payload;

void ssl_disconnect(SSL_Connection *sslcon)
{
    int err;

    if(sslcon == NULL){
        return;
    }

    /* Shutdown the client side of the SSL connection */
    err = SSL_shutdown(sslcon->ssl);
    if(err == -1){
        printf("Could not shutdown SSL\n");
        exit(1);
    }    

    /* Terminate communication on a socket */
    err = close(sslcon->sock);
    if(err == -1){
        printf("Could not close socket\n");
        exit(1);
    }    

    /* Free the SSL structure */
    SSL_free(sslcon->ssl);

    /* Free the SSL_CTX structure */
    SSL_CTX_free(sslcon->ctx);

    /* Free the sslcon */
    if(sslcon != NULL)
    {
        free(sslcon);
        sslcon = NULL;
    }
}

int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
    strncpy(buf, (char *)(password), size);
    buf[size - 1] = '\0';
    return(strlen(buf));
}

SSL_Connection *ssl_connect(const char *host, int port, const char *certfile,
        const char *keyfile, const char* capath)
{
    int err;

    SSL_Connection *sslcon = NULL;
    sslcon = (SSL_Connection *)malloc(sizeof(SSL_Connection));
    if(sslcon == NULL){
        printf("Could not allocate memory for SSL Connection");
        exit(1);
    }

    SSL_library_init();
    SSL_load_error_strings();
    sslcon->meth = (SSL_METHOD*)SSLv3_method();
    sslcon->ctx = SSL_CTX_new(sslcon->meth);                        
    if(!sslcon->ctx){
        printf("Could not get SSL Context\n");
        exit(1);
    }

    char *password = "abc123";
    //SSL_CTX_set_default_passwd_cb(sslcon->ctx, pem_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(sslcon->ctx, password);

    /* Load the CA from the Path */
    // if(SSL_CTX_load_verify_locations(sslcon->ctx, NULL, capath) <= 0)
    // {
    //     /* Handle failed load here */
    //     printf("Failed to set CA location...\n");
    //     // ERR_print_errors_fp(stderr);
    //     exit(1);
    // }

    cout<<certfile<<endl;
    cout<<keyfile<<endl;
    string cf = capath;
    string kf = capath;
    cf += certfile;
    kf += keyfile;
    cout<<cf<<endl;
    cout<<kf<<endl;
    /* Load the client certificate into the SSL_CTX structure */
    if (SSL_CTX_use_certificate_file(sslcon->ctx, cf.c_str(), SSL_FILETYPE_PEM) <= 0) {
        printf("Cannot use Certificate File\n");
        // ERR_print_errors_fp(stderr);
        exit(1);
    }

    /* Load the private-key corresponding to the client certificate */
    if (SSL_CTX_use_PrivateKey_file(sslcon->ctx, kf.c_str(), SSL_FILETYPE_PEM) <= 0) {
        printf("Cannot use Private Key\n");
        // ERR_print_errors_fp(stderr);
        exit(1);
    }

    /* Check if the client certificate and private-key matches */
    // if (!SSL_CTX_check_private_key(sslcon->ctx)) {
    //     printf("Private key does not match the certificate public key\n");
    //     exit(1);
    // }

    /* Set up a TCP socket */
    sslcon->sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);      
    if(sslcon->sock == -1)
    {
        printf("Could not get Socket\n");
        exit(1);
    }

    memset (&sslcon->server_addr, '\0', sizeof(sslcon->server_addr));
    sslcon->server_addr.sin_family      = AF_INET;
    sslcon->server_addr.sin_port        = htons(port);       /* Server Port number */
    sslcon->host_info = gethostbyname(host);
    if(sslcon->host_info){
        /* Take the first IP */
        struct in_addr *address = (struct in_addr*)sslcon->host_info->h_addr_list[0];
        sslcon->server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */
    }
    else{
        printf("Could not resolve hostname %s\n", host);
        return NULL;
    }

    /* Establish a TCP/IP connection to the SSL client */
    err = connect(sslcon->sock, (struct sockaddr*) &sslcon->server_addr, sizeof(sslcon->server_addr));
    if(err == -1){
        printf("Could not connect\n");
        exit(1);
    }    

    /* An SSL structure is created */
    sslcon->ssl = SSL_new(sslcon->ctx);
    if(!sslcon->ssl){
        printf("Could not get SSL Socket\n");
        exit(1);
    }    

    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
    SSL_set_fd(sslcon->ssl, sslcon->sock);

    /* Perform SSL Handshake on the SSL client */
    err = SSL_connect(sslcon->ssl);
    if(err <= 0){
        printf("Could not connect to SSL Server\n");
        exit(1);
    }

    return sslcon;
}

bool sendPayload(SSL *sslPtr, char *deviceTokenBinary, char *payloadBuff, size_t payloadLength)
{
    bool rtn = false;
    if (sslPtr && deviceTokenBinary && payloadBuff && payloadLength)
    {
        uint8_t command = 1; /* command number */
        char binaryMessageBuff[sizeof(uint8_t)+sizeof(uint32_t)+sizeof(uint32_t)+sizeof(uint16_t)+DEVICE_BINARY_SIZE+sizeof(uint16_t)+MAXPAYLOAD_SIZE];
        /* message format is, |COMMAND|ID|EXPIRY|TOKENLEN|TOKEN|PAYLOADLEN|PAYLOAD| */
        char *binaryMessagePt = binaryMessageBuff;
        uint32_t whicheverOrderIWantToGetBackInAErrorResponse_ID = 1234;
        uint32_t networkOrderExpiryEpochUTC = htonl(time(NULL)+86400); // expire message if not delivered in 1 day
        uint16_t networkOrderTokenLength = htons(DEVICE_BINARY_SIZE);
        uint16_t networkOrderPayloadLength = htons(payloadLength);

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
        memcpy(binaryMessagePt, payloadBuff, payloadLength);
        binaryMessagePt += payloadLength;
        cout<<"============================================================================="<<endl;
        for(int i= 0; i<binaryMessagePt-binaryMessageBuff; ++i){
            printf("%x ",*(binaryMessageBuff+i));
            if(0==i%10 && 0!=i) cout<<endl;
        }

        cout<<endl;
        cout<<"============================================================================="<<endl;
        if (SSL_write(sslPtr, binaryMessageBuff, (binaryMessagePt - binaryMessageBuff)) > 0)
            rtn = true;
    }
    return rtn;
}

int send_payload(const char *deviceTokenHex, const char *payloadBuff, size_t payloadLength)
{
    int rtn = 0;

    SSL_Connection *sslcon = ssl_connect(APPLE_HOST, APPLE_PORT, RSA_CLIENT_CERT, RSA_CLIENT_KEY, CA_CERT_PATH);
    if(sslcon == NULL){
        printf("Could not allocate memory for SSL Connection");
        exit(1);
    }
    cout<<"connected"<<endl;
    int i = 0;
    int j = 0;
    int tmpi;
    char tmp[3];
    char deviceTokenBinary[DEVICE_BINARY_SIZE];
    while(i < strlen(deviceTokenHex))
    {
        if(deviceTokenHex[i] == ' '){
            i++;
        }
        else{
            tmp[0] = deviceTokenHex[i];
            tmp[1] = deviceTokenHex[i + 1];
            tmp[2] = '\0';

            sscanf(tmp, "%x", &tmpi);
            deviceTokenBinary[j] = tmpi;
            cout<<"tmp:"<<tmp<< " tmpi:"<<tmpi<<endl;

            i += 2;
            j++;
        }
    }
    string tmp_str("{\"aps\":{\"alert\":{\"head\":{\"method\":\"chat/remove\",\"token\":\"5226f248c41128f4\",\"sequence\":331},\"body\":{\"content\":2580,\"time\":\"2013-09-04 16:42:29\",\"from\":2580,\"sid\":\"KK1007A\"}},\"badge\":1,\"sound\":\"default\"}}");
    // if(sendPayload(sslcon->ssl, deviceTokenBinary,  (char*)payloadBuff, payloadLength))
    //     cout<<"send successed .."<<endl;
         cout<<"sending  ...."<<tmp_str<<endl;
    if(sendPayload(sslcon->ssl, deviceTokenBinary, (char*)tmp_str.c_str(), tmp_str.length())){
         cout<<"send successed .."<<endl;
    }
    ssl_disconnect(sslcon);

    return rtn;
}

// From the given Information JSON is generated (JSON is described here) and then sent. It is implemented as follows:


int send_remote_notification(const char *deviceTokenHex, Payload *payload)
{
    char messageBuff[MAXPAYLOAD_SIZE];
    char tmpBuff[MAXPAYLOAD_SIZE];
    char badgenumBuff[3];

    string msg("{\"aps\":{");
    // strcpy(messageBuff, "{\"aps\":{");

    if(payload->message != NULL)
    {
        // strcat(messageBuff, "\"alert\":");
        msg +="\"alert\":";
        if(payload->actionKeyCaption != NULL)
        {
            sprintf(tmpBuff, "{\"body\":\"%s\",\"action-loc-key\":\"%s\"},", payload->message, payload->actionKeyCaption);
            // strcat(messageBuff, tmpBuff);
            msg += tmpBuff;
        }
        else
        {
            sprintf(tmpBuff, "{\"%s\"},", payload->message);
            // strcat(messageBuff, tmpBuff);
            msg += tmpBuff;
        }
    }

    if(payload->badgeNumber > 99 || payload->badgeNumber < 0)
        payload->badgeNumber = 1;

    sprintf(badgenumBuff, "%d", payload->badgeNumber);
    // strcat(messageBuff, "\"badge\":");
    // strcat(messageBuff, badgenumBuff);
    msg += "\"badge\":";
    msg += badgenumBuff;

    // strcat(messageBuff, ",\"sound\":\"");
    // strcat(messageBuff, payload->soundName == NULL ? "default" : payload->soundName);
    // strcat(messageBuff, "\"},");
    msg += ",\"sound\":\"";
    msg += (payload->soundName == NULL ? "default" : payload->soundName);
    // strcat(messageBuff, "\"},");
    msg += "\"},";

    int i = 0;
    while(payload->dictKey[i] != NULL && i < 5)
    {
        sprintf(tmpBuff, "\"%s\":\"%s\"", payload->dictKey[i], payload->dictValue[i]);
        // strcat(messageBuff, tmpBuff);
        msg +=  tmpBuff;
        if(i < 4 && payload->dictKey[i + 1] != NULL)
        {
            // strcat(messageBuff, ",");
            msg +=  ",";
        }
        i++;
    }

    msg +=  "}";
    // printf("Sending %s\n", messageBuff);
    cout <<"Send:"<< msg<<endl;

    // send_payload(deviceTokenHex, messageBuff, strlen(messageBuff));
    send_payload(deviceTokenHex, msg.c_str(), msg.length());
}
/* MAIN Function */
int main(int argc, char *argv[])
{
    int     err;

    /* Phone specific Payload message as well as hex formated device token */
    const char     *deviceTokenHex = NULL;
    if(argc == 1)
    {
        deviceTokenHex = "3c89b45860bdddec2c2825d52f5190ecbcefa9e22bfb1dda582063b315d0ad9b";
    }
    else
    {
        deviceTokenHex = argv[1];
    }

    if(strlen(deviceTokenHex) < 64 || strlen(deviceTokenHex) > 70)
    {
        printf("Device Token is to short or to long. Length without spaces should be 64 chars...\n");
        exit(1);
    }

    Payload *payload = (Payload*)malloc(sizeof(Payload));
    memset(payload, 0, sizeof(Payload));

    payload->message = "Message to print out";

    // This is the red numbered badge that appears over the Icon
    payload->badgeNumber = 1;

    // This is the Caption of the Action key on the Dialog that appears
    payload->actionKeyCaption = "Caption of the second Button";

    // These are two dictionary key-value pairs with user-content
    payload->dictKey[0] = "Key1";
    payload->dictValue[0] = "Value1";

    payload->dictKey[1] = "Key2";
    payload->dictValue[1] = "Value2";

    /* Send the payload to the phone */
    // printf("Sending APN to Device with UDID: %s\n", deviceTokenHex);
    cout <<"Sending APN to Device with UDID: %s\n"<< deviceTokenHex<<endl;
    send_remote_notification(deviceTokenHex, payload);

    return 0;
}



