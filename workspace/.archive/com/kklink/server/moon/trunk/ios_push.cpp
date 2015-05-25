#include "myconfig.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/ssl.h>
#include <openssl/ssl23.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>

#include "proto_im.h"
#include "jss.h"
#include "ios_push.h"
#include "chat.h"
#include "log.h"

using namespace std;
using boost::asio::ip::tcp;
namespace sys = boost::system;
// namespace placeholders = boost::asio::placeholders;

const string giftmsg[] ={"","赠送一朵玫瑰给你","赠送一辆跑车给你","赠送一艘游艇给你","赠送一架飞机给你"};
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
        return false;
    }

    // /* Load the CA from the Path */
    // if(SSL_CTX_load_verify_locations(ctx, NULL, capath) <= 0)
    // {
    //     /* Handle failed load here */
    //     LOG_I<<"Failed to set CA location...";
    //     // ERR_print_errors_fp(stderr);
    //     exit(1);
    // }
    string pass = PERM_PASSWORD;
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (char*)pass.c_str());
    SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);

    string cf = config_.capath + "/" + config_.certfile;
    string kf = config_.capath + "/" + config_.keyfile;
    /* Load the client certificate into the SSL_CTX structure */
    if (SSL_CTX_use_certificate_file(ctx, cf.c_str(), SSL_FILETYPE_PEM) <= 0) {
        // LOG_I<<"Cannot use Certificate File";
        // ERR_print_errors_fp(stderr);
        return false;
    }
    /* Load the private-key corresponding to the client certificate */
    if (SSL_CTX_use_PrivateKey_file(ctx, kf.c_str(), SSL_FILETYPE_PEM) <= 0) {
        // LOG_I<<"Cannot use Private Key";
        // ERR_print_errors_fp(stderr);
        return false;
    }
    /* Check if the client certificate and private-key matches */
    if (!SSL_CTX_check_private_key(ctx)) {
        // LOG_I<<"Private key does not match the certificate public key";
        return false;
    }

    /* Set up a TCP socket */
    sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);      
    if(sock == -1) {
        // LOG_I<<"Could not get Socket";
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
            // LOG_I<<"Could not resolve hostname:";
            return false;
        }
    }
    /* Establish a TCP/IP connection to the SSL client */
    err = connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if(err == -1) {
        // LOG_I<<"Could not connect";
        return false;
    }    

    /* An SSL structure is created */
    ssl = SSL_new(ctx);
    if(!ssl) {
        // LOG_I<<"Could not get SSL Socket";
        return false;
    }    
    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
    SSL_set_fd(ssl, sock);
    /* Perform SSL Handshake on the SSL client */
    err = SSL_connect(ssl);
    if(err <= 0) {
        // LOG_I<<"Could not connect to SSL Server";
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
        // LOG_I<<"Could not shutdown SSL";
        return;
    }    

    /* Terminate communication on a socket */
    err = close(sock);
    if(err == -1) {
        // LOG_I<<"Could not close socket";
        return;
    }    

    /* Free the SSL structure */
    SSL_free(ssl);
    /* Free the SSL_CTX structure */
    SSL_CTX_free(ctx);
    useful = false;
}

static json::object pack_push_message(const imessage::message& m, UID uid)
{
    string message;
    if("chat/in-spot" == m.type){
        return json::object();
        //const string & session_name = m.body.get<string>("gname");
        //// chat_group & cg = chatmgr::inst().chat_group(get<string>());
        //// string session_name = cg.name();
        //message = "欢迎来到"+session_name+"现场";
    }
    else if("chat/out-spot" == m.type){
        return json::object();
        //const string & session_name = m.body.get<string>("gname");
        //// chat_group & cg = chatmgr::inst().chat_group(m.gid);
        //// string session_name = cg.name();
        //message = "您已离开"+session_name+"现场，月下期待与你的下一次相遇";
    }
    else
    {
        //client_ptr c = xindex::get(m.from);
        //if (!c){
        //    LOG_E << "ios push Not found " << m.from;
        //    return json::object();
        //}
        //json::object user = c->brief_user_info();

        std::string msgfmt;

        if("chat/image" == m.type){
            msgfmt ="%1%发来一张图片";
        }
        else if("chat/text" == m.type){
            msgfmt ="%1%：" + m.body.get<string>("content");
        }
        else if("chat/audio" == m.type){
            msgfmt ="%1%发来一段语音";
        }
        else if("chat/join" == m.type){
            const string & session_name = m.body.get<string>("gname");
            bool bar_code = m.body.get<bool>("bar_code",false);
            if(!bar_code){
                if(SYSADMIN_UID == m.from){
                    // msgfmt = "您已经加了聊吧" + session_name;
                    return json::object();
                }
                else{
                    msgfmt ="%1%邀请你加入群组"+session_name;
                }
            }
            // else{
            //     json::array membs = m.body.get<json::array>("content");
            //     if(!membs.empty()){
            //         json::object &user = boost::get<json::object>(membs.front());
            //         string userName = user.get<string>("userName","");
            //         if(!userName.empty())msgfmt = userName+"通过二维码扫描加入群组"+session_name;
            //     }
            // }
        }
        else if("chat/fans" == m.type){
            msgfmt ="%1%关注了你";
        }
        else if("chat/gift" == m.type){
            int giftid = m.body.get<int>("content");
            msgfmt = std::string("%1%") + giftmsg[giftid];
        }
        else{
            return json::object();
        }

        if("chat/pair" == m.type){
            if (m.body.get<bool>("success",false)) {
                msgfmt = "您有一条新的搭讪，快来看看吧";
            } else {
                json::object obj = m.body.get<json::object>("content",json::object());
                string userName= obj.get<string>("userName","");
                if ( userName.empty() ) return json::object();
                message = userName + "与您配对成功,快来看看吧";
            }
        } else {
            client_ptr cli = xindex::get(m.from);
            string nick_name = cli->user_info().get<std::string>("nick",  "");
            message = str(boost::format(msgfmt) % nick_name);
            // const json::object & user = m.body.get<json::object>("from");
            // string nick_name = user.get<string>("userName","");
        }
    }

    if(message.length()>200) //message.resize(200);
        message = utf32to8(utf8to32(message).substr(0,66)); 

    json::object alert;
    alert.put("alert", message);
    // alert.put("badge", 1);
    client_ptr c = xindex::get(uid);
    if(c){
        if(c->cache().get<bool>("enablePushAudio", true)){ 
            alert.put("sound", "default");
        }
    }

    return json::object()("aps",alert);
}

bool ios_push::push_message(const string& deviceTokenHex, const imessage::message& m, UID uid)
{
    bool rtn = false;

    if(deviceTokenHex.length() != 64) { // ( || len & 1)
        return false;
    }
    if(!ssl_handle.ssl_available()){
        if(!ssl_handle.connect_()){
            return rtn;
        }
    }

    // LOG_I <<"deviceTokenHex:"<<deviceTokenHex;
    /* Convert the Device Token */
    int j = 0;
    char tmp[3] = {0};
    char deviceTokenBinary[DEVICE_BINARY_SIZE];
    for(std::string::const_iterator itr = deviceTokenHex.begin();
            itr != deviceTokenHex.end();) 

    {
        if( !isxdigit(*itr) ) {
            return false;
            // ++itr;
        }
        else {
            tmp[0] = *itr++;
            tmp[1] = *itr++;
            // tmp[2] = '\0';
            deviceTokenBinary[j++] = strtoul(tmp, NULL, 16); 
        }
    }

    json::object aps = pack_push_message(m, uid);
    if (aps.empty())
    {
        LOG_I << m.type << " apns push ignored " <<  __FILE__ << __LINE__;
        return false;
    }
    string aps_message = json::encode(aps);
    // LOG_I<<"push aps_message:"<<aps_message;
    uint8_t command = 1; /* command number */
    uint32_t len = sizeof(uint8_t) + 2*sizeof(uint32_t)+ 2*sizeof(uint16_t) 
        + DEVICE_BINARY_SIZE + aps_message.length();

    string binaryMessageBuff(len, 0);
    /* aps_message format is, |COMMAND|ID|EXPIRY|TOKENLEN|TOKEN|PAYLOADLEN|PAYLOAD| */
    char *binaryMessagePt = const_cast<char*>(binaryMessageBuff.c_str());
    uint32_t whicheverOrderIWantToGetBackInAErrorResponse_ID = 1234;
    // expire aps_message if not delivered in 1 day
    uint32_t networkOrderExpiryEpochUTC = htonl(time(NULL)+86400);
    uint16_t networkOrderTokenLength = htons(DEVICE_BINARY_SIZE);
    uint16_t networkOrderPayloadLength = htons(aps_message.length());

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
    memcpy(binaryMessagePt, aps_message.c_str(), aps_message.length());
    binaryMessagePt += aps_message.length();

    if (SSL_write(ssl_handle.ssl,binaryMessageBuff.c_str(),binaryMessageBuff.length()) > 0){
        rtn = true;
    }
    else {
        ssl_handle.close_();
    }

    // LOG_I<<"pushed aps_message to IOS"<<endl;
    return rtn;
}

apple_push_c *apple_push_c::instance(apple_push_c* c)
{
    static apple_push_c* apc = c;
    return apc;
}

apple_push_c *apple_push_c::sandbox(apple_push_c* c)
{
    static apple_push_c* apc = c;
    return apc;
}

void apple_push_c::send(const imessage::message& m, const string& devtok, UID uid)
{
    try
    {
        json::object aps = pack_push_message(m, uid);
        if (aps.empty())
        {
            LOG_I << m.type << " apns push ignored " << __FILE__ << __LINE__;
            return;
        }

        ls_.push_back( makereq(devtok, m.id(), json::encode(aps), uid) );
        LOG_I << stage_ <<" " __FILE__ " " << m; // ls_.back();

        if (stage_ == stage::idle)
        {
            write_msg();
        }
    }
    catch (std::exception const & e)
    {
        LOG_I << e.what();
    }
}

std::string apple_push_c::makereq(const std::string & tk, int id, const std::string & aps, UID uid)
{
    std::string tok = find_if(tk.begin(),tk.end(), boost::is_upper())==tk.end() ? tk : boost::to_lower_copy(tk);
    return boost::str(
            boost::format("POST /ap?devtok=%1%&id=%2%&uid=%4% HTTP/1.1\r\n"
                "Host: 127.0.0.1\r\n"
                "Content-Length: %3%\r\n"
                "\r\n")
            % tok % id % aps.length() % uid) + aps;
}

apple_push_c::apple_push_c(boost::asio::io_service & io_service, boost::asio::ip::tcp::endpoint const &ep)
    : socket_(io_service)
    , timer_(io_service)
    , endpoint_(ep)
{
    stage_ = stage::connecting;
}

void apple_push_c::write_msg()
{
    if (stage_ != stage::idle || ls_.empty())
        return;
    stage_ = stage::writing;
    boost::asio::async_write(socket_
            , boost::asio::buffer(ls_.front())
            , boost::bind(&this_type::handle_write, this, boost::asio::placeholders::error));
}

void apple_push_c::handle_write(boost::system::error_code const & ec)
{
    BOOST_ASSERT (!ls_.empty());
    if (ec)
        return handle_error(ec);
    ls_.pop_front();
    stage_ = stage::idle;
    write_msg();
}

void apple_push_c::handle_error(boost::system::error_code const & ec)
{
    LOG_I << ec <<" "<< ec.message() <<" " __FILE__ " "<< stage_ <<" "<< endpoint_;

    if (ec == boost::asio::error::operation_aborted)
    {
        // stage_ = stage::error;
        return;
    }

    if (stage_ == stage::connecting)
        return;

    stage_ = stage::connecting;
    timer_.expires_from_now(boost::posix_time::seconds(1));
    timer_.async_wait(boost::bind(&apple_push_c::timed_connect, this, boost::asio::placeholders::error));
}

void apple_push_c::timed_connect(sys::error_code const & ec)
{
    LOG_I << ec <<" "<< ec.message() <<" " __FILE__ " "<< stage_ <<" "<< endpoint_;
    if (ec)
    {
        return ; //handle_error(ec);
    }

    if (stage_ == stage::connecting)
    {
        boost::system::error_code ec;
        socket_.close(ec);
        socket_.open(boost::asio::ip::tcp::v4());
        socket_.async_connect(endpoint_, boost::bind(&this_type::handle_connect, this
                    , boost::asio::placeholders::error));
    }
}

void apple_push_c::handle_connect(boost::system::error_code const & ec)
{
    LOG_I << ec <<" "<< ec.message() <<" " __FILE__ " "<< stage_ <<" "<< endpoint_;
    if (ec)
    {
        timer_.expires_from_now(boost::posix_time::seconds(5));
        timer_.async_wait(boost::bind(&apple_push_c::timed_connect, this, boost::asio::placeholders::error));
        return;
    }

    stage_ = stage::idle;
    write_msg();

    socket_.async_read_some(boost::asio::buffer(rbuf_,sizeof(rbuf_))
            , boost::bind(&this_type::handle_read, this
                , boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void apple_push_c::handle_read(sys::error_code const & ec, size_t bytes)
{
    if (ec)
    {
        LOG_I << __FILE__;
        return handle_error(ec);
    }

    LOG_I << boost::asio::const_buffer(rbuf_, bytes);

    socket_.async_read_some(boost::asio::buffer(rbuf_,sizeof(rbuf_))
            , boost::bind(&this_type::handle_read, this
                , boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

