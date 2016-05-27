#include <stdlib.h>
#include <time.h>

#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>

namespace ip = boost::asio::ip;
using ip::udp;

struct Main
{
    Main(boost::asio::io_service& io_service, int x, udp::endpoint ep)
        : socket_(io_service, udp::endpoint(udp::v4(),0))
        , endpoint_(ep)
    {
        srand(time(0));
        do_send(x);
    }
    Main(boost::asio::io_service& io_service, short port)
        : socket_(io_service, udp::endpoint(udp::v4(), port))
    {
        do_receive();
    }

private:
    struct recv_st {
        unsigned last_seq = 0;
        unsigned rnumb = 0;
        unsigned bytes_total = 0;

        time_t print(time_t pt) {
            time_t ct = time(0);
            //if (ct - pt < 5) return pt;
            printf("%u %u %u\n", last_seq, rnumb, bytes_total);
            return ct;
        }
    } recv_;

    struct sent_st {
        unsigned snumb = 0;
        unsigned bytes_sent = 0;

        //unsigned numb_psec_ = 30;
        //unsigned bytes_psec_ = 1024*300;
        
        time_t print(time_t pt) {
            time_t ct = time(0);
            if (ct - pt < 5)
                return pt;
            printf("%u %u\n", snumb, bytes_sent);
            return ct;
        }
    } sent_;

    time_t print_time_ = 0;

    void do_receive()
    {
        socket_.async_receive_from( boost::asio::buffer((void*)data_, max_length)
            , endpoint_
            , [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    recv_.rnumb++;
                    recv_.last_seq = data_[0];
                    recv_.bytes_total += bytes_recvd;
                    do_receive();
                    print_time_ = recv_.print(print_time_);
                } else {
                    fprintf(stderr, "error %d:%s\n", ec.value(), ec.message().c_str());
                }
            });
    }

    void do_send(unsigned length)
    {
        //unsigned length = rand()/(1024*24) + 1024*6;
        data_[0] = sent_.snumb++;;
        socket_.async_send_to( boost::asio::buffer((void*)data_, length)
            , endpoint_
            , [this,length](boost::system::error_code ec, std::size_t bytes_sent) {
                  if (!ec) {
                      sent_.bytes_sent += bytes_sent;
                      //do_send();
                      //print_time_ = recv_.print(print_time_);
                  } else {
                      printf("len %u\n", length);
                      printf("sent: %d:%s\n", ec.value(), ec.message().c_str());
                  }
            });
    }

private:
    udp::socket socket_;
    udp::endpoint endpoint_;

    enum { max_length = 1024*128 };
    int32_t data_[max_length/4];
};

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_service io_s;
    if (argc == 3) {
        for (int i=100; i > 60; --i) {
            Main push(io_s, 1024*i, udp::endpoint(ip::address::from_string(argv[1]), std::atoi(argv[2])));
            io_s.run();
            io_s.reset();
        }
    } else if (argc == 2) {
        Main recv(io_s, std::atoi(argv[1]));
        io_s.run();
    } else {
      std::cerr << "Usage: a.out [host] <port>\n";
      return 1;
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

