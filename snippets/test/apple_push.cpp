//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

// "a642ccd0a54a82f6c02738674e1becc47d69ba6aed3c06b9fb745bf47a72b0e3"

enum { max_length = 1024 };

    template <typename Int> std::string& pkint(std::string & pkg, Int x)
    {
        Int i;
        switch (sizeof(Int)) {
            case sizeof(uint32_t): i = htonl(x); break;
            case sizeof(uint16_t): i = htons(x); break;
            default: i = x; break;
        }
        char *pc = (char*)&i;
        pkg.insert(pkg.end(), &pc[0], &pc[sizeof(Int)]);
        return pkg;
    }
    std::string & pkstr(std::string & pkg, const std::string & s)
    {
        uint16_t len = s.length();
        len = htons(len);
        char *pc = (char*)&len;
        pkg.insert(pkg.end(), &pc[0], &pc[sizeof(uint16_t)]);
        return (pkg += s);
    }

    std::string unhex(const std::string & hs)
    {
        std::string buf;
        const char* s = hs.data();
        for (unsigned int x = 0; x+1 < hs.length(); x += 2)
        {
            char h = s[x];
            char l = s[x+1];
            h -= (h <= '9' ? '0': 'a'-10);
            l -= (l <= '9' ? '0': 'a'-10);
            buf.push_back( char((h << 4) | l) );
        }
        return buf;
    }

    std::string apack(std::string const & tok, int id, std::string const & aps)
    {
        BOOST_ASSERT(tok.length() == 32);

        std::string buf; //(len, 0);
        pkstr(pkstr(pkint(pkint(pkint( buf
                , uint8_t(1)) // command
                , uint32_t(id)) // /* provider preference ordered ID */
                , uint32_t(time(NULL)+(60*60*6))) // /* expiry date network order */
                , tok) // binary 32bytes device token
                , aps)
                ;
        return buf;
    }


class client
{
public:
  client(boost::asio::io_service& io_service,
      boost::asio::ssl::context& context,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
    : socket_(io_service, context)
  {
    //socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    //socket_.set_verify_callback(boost::bind(&client::verify_certificate, this, _1, _2));

    boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
        boost::bind(&client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  bool verify_certificate(bool preverified,
      boost::asio::ssl::verify_context& ctx)
  {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cout << "Verifying " << subject_name << "\n";

    return preverified;
  }

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_handshake(boost::asio::ssl::stream_base::client,
          boost::bind(&client::handle_handshake, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Connect failed: " << error.message() << "\n";
    }
  }

  void handle_handshake(const boost::system::error_code& error)
  {
    if (!error)
    {
      request_ = apack(unhex("a642ccd0a54a82f6c02738674e1becc47d69ba6aed3c06b9fb745bf47a72b0e3")
              , 321
              , "{\"aps\":{\"alert\":\"hello world\"}}");

      // request_ = "{\"aps\":{\"alert\":\"This is the alert text\",\"badge\":1,\"sound\":\"default\"}}";
      std::cout << request_;
      // std::cout << "Enter message: ";
      // std::cin.getline(request_, max_length);
      size_t request_length = request_.size();

      boost::asio::async_write(socket_,
          boost::asio::buffer(request_.data(), request_length),
          boost::bind(&client::handle_write, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      std::cout << "Handshake failed: " << error.message() << "\n";
    }
  }

  void handle_write(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(reply_, bytes_transferred),
          boost::bind(&client::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      std::cout << "Write failed: " << error.message() << "\n";
    }
  }

  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      std::cout << "Reply: ";
      std::cout.write(reply_, bytes_transferred);
      std::cout << "\n";
    }
    else
    {
      std::cout << "Read failed: " << error.message() << "\n";
    }
  }

private:
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
  std::string request_;
  // char request_[max_length];
  char reply_[max_length];
};

#define PERM_PASSWORD "abc123"
#define RSA_CLIENT_KEY      "PushChatKey.pem"
#define RSA_CLIENT_CERT     "PushChatCert.pem"

//std::string password_callback( std::size_t max_length, password_purpose purpose) { return PERM_PASSWORD; }
std::string password_callback( std::size_t max_length, boost::asio::ssl::context::password_purpose purpose) { return PERM_PASSWORD; }

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(argv[1], argv[2]);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv3);
    // ctx.set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use);

    ctx.set_password_callback(password_callback);
    ctx.use_certificate_file(RSA_CLIENT_CERT, boost::asio::ssl::context::pem);
    ctx.use_private_key_file(RSA_CLIENT_KEY, boost::asio::ssl::context::pem);

    client c(io_service, ctx, iterator);

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

// ./a.out gateway.sandbox.push.apple.com 2195


