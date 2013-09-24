#include <cstdlib>
#include <deque>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>
#include "jss.h"
// #include "chat_message.hpp"

using boost::asio::ip::tcp;
namespace asio = boost::asio;

typedef std::deque<std::string> message_queue;

static std::string mac_ = "00:11:22";
static std::string gwid_ = "KKLink-001";
static std::string host_ = "127.0.0.1";
static std::string port_ = "9900";

class chat_client
{
public:
  chat_client(asio::io_service& io_service
      , tcp::resolver::iterator endpoint_iterator
      , std::string token
      )
    : io_service_(io_service)
      , socket_(io_service)
      , dl_alive_(io_service)
  {
    do_connect(endpoint_iterator);
    token_ = token;
    dl_alive_.expires_from_now(boost::posix_time::seconds(0));
  }

  void close(boost::system::error_code ec)
  {
    io_service_.post([this]() { socket_.close(); });
    std::cout << ec << " close\n";
  }

  void keep_alive()
  {
    bool expired = (dl_alive_.expires_at() <= asio::deadline_timer::traits_type::now());
    if (expired)
    {
        // dl_alive_.async_wait([this]() { this->keep_alive(); });
        dl_alive_.async_wait(std::bind(&chat_client::keep_alive, this));
        dl_alive_.expires_from_now(boost::posix_time::seconds(33));

        json::object hdobj;
        hdobj ("method","hello") ("token",token_) ;
        // if (!gwid_.empty() && !mac_.empty())
        // {
        //     hdobj ("mac",mac_) ("gwid",gwid_);
        // }

        std::string hds = json::encode(hdobj);

        decltype(read_size_) u;
        u.h[0] = htons(static_cast<short>(hds.size()));
        u.h[1] = htons(2);

        this->write(std::string(&u.s[0],sizeof(u)) + hds + "{}");
    }
  }

  bool is_open() const { return socket_.is_open(); }

private:
  void write(const std::string& msg)
  {
    io_service_.post(
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
      std::cout << "do read header " << sizeof(read_size_) << "\n";
    asio::async_read(socket_,
        asio::buffer(&read_size_.s[0], sizeof(read_size_)),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            read_msg_.resize( ntohs(read_size_.h[0]) + ntohs(read_size_.h[1]) );
            do_read_body();
          }
          else
          {
          close(ec); // socket_.close();
          }
        });
  }

  void do_read_body()
  {
      std::cout << "do read body " << read_msg_.size() << "\n";
    asio::async_read(socket_,
        asio::buffer(const_cast<char*>(read_msg_.data()), read_msg_.size()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::cout << read_msg_ << "\n";
            ack( json::decode(std::string(read_msg_.data(), ntohs(read_size_.h[0]))) );
            do_read_header();
          }
          else
          {
            close(ec); // socket_.close();
          }
        });
  }

  void ack(json::object jso)
  {
    jso.put("method", "ack");
    std::string hds = json::encode(jso);

    decltype(read_size_) u;
    u.h[0] = htons(static_cast<short>(hds.size()));
    u.h[1] = htons(2);

    this->write(std::string(&u.s[0],sizeof(u)) + hds + "{}");
  }

  void do_write()
  {
    asio::async_write(socket_,
        asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::cout << write_msgs_.front() << std::endl;
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            close(ec); // socket_.close();
          }
        });
  }

private:
  asio::io_service& io_service_;
  tcp::socket socket_;
  std::string token_;
  asio::deadline_timer dl_alive_;


  union { uint16_t h[2]; char s[4]; uint32_t len; } read_size_;
  std::string read_msg_;

  message_queue write_msgs_;
};

#include <boost/program_options.hpp>
namespace po = boost::program_options;

std::string args(int ac, char * const av[])
{
    std::string token;

    po::options_description desc("Options");
    desc.add_options()
        ("help", "display this help and exit")
        ("host,h", po::value<std::string>(&host_)->default_value(host_), "host to connect")
        ("port,p", po::value<std::string>(&port_)->default_value(port_), "port used when connect")
        ("mac", po::value<std::string>(&mac_)->default_value(mac_), "the macaddress")
        ("gwid", po::value<std::string>(&gwid_)->default_value(gwid_), "the gwid")
        ("token", po::value<std::string>(&token)->required(), "client id")
        ;
    po::positional_options_description pos_desc;
    pos_desc.add("token", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(pos_desc).run(), vm);
    if (vm.count("help"))
    {
        std::cout << "Usage: options_description [options] <token>\n"
            << desc;
        exit(0);
    }

    po::notify(vm);

    return token;
}

int main(int ac, char* const av[])
{
  try
  {
    std::string token = args(ac, av);

    asio::io_service io_service;

    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({ host_, port_ });

    chat_client c(io_service, endpoint_iterator, token);
    c.keep_alive();

    // std::thread t([&io_service](){ io_service.run(); });
    // while (c.is_open())
    //{
        // c.keep_alive();
        // std::this_thread::sleep_for(std::chrono::seconds(30));
    //}

    // char line[chat_message::max_body_length + 1];
    // while (std::cin.getline(line, chat_message::max_body_length + 1))
    // {
    //   chat_message msg;
    //   msg.body_length(std::strlen(line));
    //   std::memcpy(msg.body(), line, msg.body_length());
    //   msg.encode_header();
    //   c.write(msg);
    // }

    // boost::system::error_code ec;
    // c.close(ec);
    //
    // t.join();

    io_service.run();
    std::cout << "bye.\n";
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

