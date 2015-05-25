#include <string.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include "client.h"

#define CA_CERT_PATH    "./"

// #if defined(IOS_PROVIDE_VERSION)
// /* Development Certificates */
// #define RSA_CLIENT_CERT     "Certs/apns-dev-cert.pem"
// #define RSA_CLIENT_KEY      "Certs/apns-dev-key.pem"
// 
// /* Development Connection Infos */
// #define APPLE_HOST          "gateway.push.apple.com"
// #define APPLE_PORT          2195
// 
// #define APPLE_FEEDBACK_HOST "feedback.push.apple.com"
// #define APPLE_FEEDBACK_PORT 2196
// #else
/* Distribution Certificates */
#define RSA_CLIENT_KEY      "PushChatKey.pem"
#define RSA_CLIENT_CERT     "PushChatCert.pem"

/* Release Connection Infos */
#define APPLE_HOST          "gateway.sandbox.push.apple.com"
#define APPLE_PORT          2195

#define APPLE_FEEDBACK_HOST "feedback.sandbox.push.apple.com"
#define APPLE_FEEDBACK_PORT 2196
// #endif

#define DEVICE_BINARY_SIZE  32
#define MAXPAYLOAD_SIZE     256
#define PERM_PASSWORD "abc123"

// We're nearly done with the server part, the last (and hardest :-)) puzzle piece is the SSL connection, which is wrapped in a neat structure:
class ios_push
{
    public:
        struct cfg
        {
            cfg(std::string hostname = APPLE_HOST, int port_num = APPLE_PORT, 
                    std::string certfile_name = RSA_CLIENT_CERT, 
                    std::string keyfile_name = RSA_CLIENT_KEY, 
                    std::string capath_name = CA_CERT_PATH
               ):host(hostname),port(port_num),certfile(certfile_name),
            keyfile(keyfile_name),capath(capath_name){}

            std::string host;
            int port;
            std::string certfile;
            std::string keyfile;
            std::string capath;
        };
        struct ssl_connect
        {
            public:
                ssl_connect():useful(false){
                    connect_();
                }

                ssl_connect(cfg c):config_(c),useful(false){
                    connect_(); 
                }

                bool ssl_available(){return useful;}
                bool connect_();
                void close_();

                SSL_CTX         *ctx;
                SSL             *ssl;
                SSL_METHOD      *meth;
                cfg config_;
                int                  sock;
                bool    useful;
                // X509            *server_cert;
                // EVP_PKEY        *pkey;
                //
                struct sockaddr_in   server_addr;
                struct hostent      *host_info;

        };

        static ios_push iso_push_inst()
        {
            static ios_push instance;
            return instance;
        }
        bool push_message(const std::string& deviceTokenHex,const std::string& token, std::string& msg);

    private:
        ssl_connect ssl_handle;
};


