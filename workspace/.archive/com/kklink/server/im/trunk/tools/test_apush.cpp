#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/function/function0.hpp>  
#include <boost/thread/thread.hpp> 
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include <istream>

#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <curl/curl.h>

#include "push.h"
#include "json.h"
#include "log.h"
#include "dbc.h"
#include "user.h"
#include "client.h"

using namespace std;
using namespace boost;

class apush_msgr
{
    private:
    struct SslConnection{
        SSL_CTX         *ctx;
        SSL             *ssl;
        SSL_METHOD      *meth;
        X509            *server_cert;
        EVP_PKEY        *pkey;
        int sock;

        SslConnection()
        {
            ctx = NULL;
            ssl = NULL;
            meth = NULL;
            server_cert = NULL;
            pkey = NULL;
            sock = -1;
        }

        ~SslConnection()
        {
            /* Shutdown the client side of the SSL connection */
            if ( NULL != ssl ) { SSL_shutdown(ssl); }

            /* Terminate communication on a socket */
            if ( -1 != sock ) { close(sock); }

            /* Free the SSL structure */
            if ( NULL != ssl ) { SSL_free(ssl); }

            /* Free the SSL_CTX structure */
            if ( NULL != ctx ) { SSL_CTX_free(ctx); }
        }
    };

    struct ssl_config {
        string host;
        int port;

        string cf;
        string kf;
        string passwd;
        ssl_config( const string h, int p, const string& pa, 
                const string& cfn, const string& kfn, const string& ps)
        {
            host = h;
            port = p;
            cf = pa + cfn;
            kf = pa + kfn;
            passwd = ps;
        }
        ssl_config(){}
    };

    public:
    static apush_msgr& inst( ssl_config* pconf = NULL );
    static void init(const property_tree::ptree& ini);

    private:
    apush_msgr(const ssl_config& conf):conf_( conf ) {};
    ssl_config conf_;

    static const int DEVICE_BINARY_SIZE = 32;
    static const int MAXPAYLOAD_SIZE   = 256;
    boost::shared_ptr<SslConnection> ssl_connect(const ssl_config& conf)
    {
        boost::shared_ptr<SslConnection> pcon = boost::make_shared<SslConnection>();
        SSL_library_init();
        SSL_load_error_strings();
        // pcon->meth = (SSL_METHOD*)SSLv3_method();
        pcon->meth = (SSL_METHOD*)TLSv1_method(); // TLSv1
        pcon->ctx = SSL_CTX_new(pcon->meth);                        
        if(!pcon->ctx){
            LOG_I << "Could not get SSL Context";
            pcon.reset();
            return pcon;
        }

        SSL_CTX_set_default_passwd_cb_userdata(pcon->ctx, const_cast<char*>(conf.passwd.c_str()));

        /* Load the client certificate into the SSL_CTX structure */
        if (SSL_CTX_use_certificate_file(pcon->ctx, conf.cf.c_str(), SSL_FILETYPE_PEM) <= 0) {
            LOG_I << "Cannot use Certificate File";
            pcon.reset();
            return pcon;
        }

        /* Load the private-key corresponding to the client certificate */
        if (SSL_CTX_use_PrivateKey_file(pcon->ctx, conf.kf.c_str(), SSL_FILETYPE_PEM) <= 0) {
            LOG_I<<"Cannot use Private Key";
            pcon.reset();
            return pcon;
        }

        /* Set up a TCP socket */
        pcon->sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);      
        if(pcon->sock == -1) {
            LOG_I<< "Could not get Socket";
            pcon.reset();
            return pcon;
        }

        struct sockaddr_in   server_addr;
        memset (&server_addr, '\0', sizeof(server_addr));
        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = htons(conf.port);       /* Server Port number */
        struct hostent *host_info = gethostbyname(conf.host.c_str());
        if ( NULL == host_info ) { 
            LOG_I << " Could not get host_info by name";
            pcon.reset();
            return pcon;
        }

        struct in_addr *address = (struct in_addr*)host_info->h_addr_list[0];
        server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */

        /* Establish a TCP/IP connection to the SSL client */
        if ( -1 == connect(pcon->sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) ) {
            LOG_I << "Could not connect";
            pcon.reset();
            return pcon;
        }    

        /* An SSL structure is created */
        pcon->ssl = SSL_new(pcon->ctx);
        if(!pcon->ssl){
            LOG_I << "Could not get SSL Socket";
            pcon.reset();
            return pcon;
        }    

        /* Assign the socket into the SSL structure (SSL and socket without BIO) */
        SSL_set_fd(pcon->ssl, pcon->sock);

        /* Perform SSL Handshake on the SSL client */
        if ( SSL_connect(pcon->ssl) <= 0 ) {
            printf("SSL_connect error %s\n" , ERR_reason_error_string(ERR_get_error()));
            pcon.reset();
            return pcon;
        }

        return pcon;
    }

    bool sendPayload(SSL *sslPtr, char *deviceTokenBinary, const string& payload)
    {
        const int len = sizeof(uint8_t)+2*sizeof(uint32_t)+2*sizeof(uint16_t)+DEVICE_BINARY_SIZE+MAXPAYLOAD_SIZE;
        uint8_t command = 1; /* command number */
        char binaryMessageBuff[ len ] = { 0 };
        /* message format is, |COMMAND|ID|EXPIRY|TOKENLEN|TOKEN|PAYLOADLEN|PAYLOAD| */
        char *binaryMessagePt = binaryMessageBuff;
        uint32_t whicheverOrderIWantToGetBackInAErrorResponse_ID = 1234;
        uint32_t networkOrderExpiryEpochUTC = htonl(time(NULL)+86400); // expire message if not delivered in 1 day
        uint16_t networkOrderTokenLength = htons(DEVICE_BINARY_SIZE);
        uint16_t networkOrderPayloadLength = htons(payload.length());

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
        memcpy(binaryMessagePt, payload.c_str(), payload.length());
        binaryMessagePt += payload.length();
        if (SSL_write(sslPtr, binaryMessageBuff, (binaryMessagePt-binaryMessageBuff)) > 0) {
            return true;
        }

        return false;
    }

    int set_token(const char *deviceTokenHex, char* deviceTokenBinary)
    {
        unsigned int i = 0,j = 0;
        while( i < strlen(deviceTokenHex) ) {
            if(deviceTokenHex[i] == ' '){
                i++;
            } else{
                int tmpi = 0;
                char tmp[3];
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

        return 0;
    }

    public:
    void loop_send()
    {
        const char* palert = "{\"aps\":{\"alert\":\"%1%\",\"badge\":1,\"sound\":\"default\"}}";
        int i=100;
        try {
        while( --i ) {
            LOG<<"i:"<<i;
            // string tok("9e2cf707a227483d08fb7ed13733853d958b64a68dc05eea50372bd139aae94a");
            string tok("f39269e5f58c0c233353aa7996b5588aa90184b93976f0aca9868d3ed288eead");
            string m = "hello world";
            format fmt(palert);
            boost::shared_ptr<SslConnection> con;
            string content = (fmt % m).str();
            LOG_I << content;
            int ec = 0, retry=3;
            char deviceTokenBinary[DEVICE_BINARY_SIZE];
            set_token(tok.c_str(), deviceTokenBinary);
            do {
                if ( !con || NULL == con->ssl ) {
                    con = ssl_connect(conf_);
                    if ( !con || NULL == con->ssl ) {
                        LOG_I<<"failed to ssl_connect";
                        usleep(500);
                        continue;
                    }
                }

                LOG<<"Connected...";
                if ( sendPayload(con->ssl, deviceTokenBinary, content) ) {
                    LOG_I << "Successed to send payload.";
                    ec = 0;
                } else {
                    ec = -1;
                    LOG << "sendPayload fail" << retry;
                    usleep(500);
                }
            } while ( ec && --retry );

            // if ( 0 == ec ) { 
            //     break;
            // }
                    // usleep(500);
        }
    } catch( const std::exception& e) {
        LOG<<e.what();
    }
        return ;
        }
};

const int apush_msgr::DEVICE_BINARY_SIZE;
const int apush_msgr::MAXPAYLOAD_SIZE;

apush_msgr& apush_msgr::inst( ssl_config* pconf )
{
    static apush_msgr inst_( *pconf );

    return inst_;
}

void apush_msgr::init(const property_tree::ptree& ini)
{
    ssl_config cfg;

    // cfg.host = ini.get<std::string>("host", "gateway.sandbox.push.apple.com");
    cfg.host = ini.get<std::string>("host", "gateway.push.apple.com");
    cfg.port = ini.get<int>("port", 2195);

    string path = ini.get<std::string>("path", "./");
    cfg.cf = path + ini.get<std::string>("certfile", "PushChatCert.pem");
    cfg.kf = path + ini.get<std::string>("keyfile", "PushChatKey.pem");
    cfg.passwd = ini.get<std::string>("password","abc123");

    apush_msgr::inst(&cfg);

}

int main( int argc, char* argv[] )
{
    property_tree::ptree ini;
    apush_msgr::init(ini);

    boost::function0<void> f = boost::bind(&apush_msgr::loop_send, &apush_msgr::inst()); 
    thread t( f );
    t.join();

    return 0;
}
