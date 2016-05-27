#include <time.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>

namespace ip = boost::asio::ip;
using ip::udp;

struct Main
{
    Main(boost::asio::io_service& io_service, udp::endpoint ep, int idle)
        : socket_(io_service, udp::endpoint(udp::v4(),0))
        , endpoint_(ep)
    {
        std::srand(time(0));
        mils_idle_ = idle;
        do_send();
    }
    Main(boost::asio::io_service& io_service, short port)
        : socket_(io_service, udp::endpoint(udp::v4(), port))
    {
        do_receive();
    }

private:
    unsigned mils_idle_ = 0;
    time_t print_time_ = 0;
    struct recv_st {
        unsigned last_seq = 0;
        unsigned rnumb = 0;
        unsigned bytes_total = 0;

        void print(time_t& pt) {
            time_t ct = time(0);
            if (ct - pt > 5 && (last_seq & 0x3f) == 0) {
                printf("%u %u %u\n", last_seq, rnumb, bytes_total);
                pt=ct;
            }
        }
    } recv_;
    struct sent_st {
        unsigned snumb = 0;
        unsigned bytes_sent = 0;

        //unsigned numb_psec_ = 30;
        //unsigned bytes_psec_ = 1024*300;
        
        void print(time_t& pt) {
            time_t ct = time(0);
            if (ct - pt > 5 && (snumb & 0x3f) == 0) {
                printf("%u %u\n", snumb, bytes_sent);
                pt = ct;
            }
        }
    } sent_;

    void do_send()
    {
        unsigned length = 1024*(std::rand()%24 + 6);
        data_[0] = sent_.snumb++;;
        socket_.async_send_to( boost::asio::buffer((void*)data_, length)
            , endpoint_
            , [this](boost::system::error_code ec, std::size_t bytes_sent) {
                  if (!ec) {
                      sent_.bytes_sent += bytes_sent;
                      sent_.print(print_time_);
                      do_send();
                      if (mils_idle_ > 0)
                          std::this_thread::sleep_for(std::chrono::milliseconds(mils_idle_));
                  } else {
                      fprintf(stderr, "error %d:%s\n", ec.value(), ec.message().c_str());
                  }
            });
    }


    void do_receive()
    {
        socket_.async_receive_from( boost::asio::buffer((void*)data_, max_length)
            , endpoint_
            , [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    recv_.rnumb++;
                    recv_.last_seq = data_[0];
                    recv_.bytes_total += bytes_recvd;
                    recv_.print(print_time_);
                    do_receive();
                } else {
                    fprintf(stderr, "error %d:%s\n", ec.value(), ec.message().c_str());
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
        if (argc >= 3) {
            int idle = (argc>3 ? std::atoi(argv[3]) : 0);
            Main push(io_s, udp::endpoint(ip::address::from_string(argv[1]), std::atoi(argv[2])), idle);
            io_s.run();
        } else if (argc == 2) {
            Main recv(io_s, std::atoi(argv[1]));
            io_s.run();
        } else {
            fprintf(stderr,"Usage: a.out [host] <port>\n");
            return 1;
        }
    }
    catch (std::exception& e)
    {
        fprintf(stderr,"Exception: %s\n", e.what());
    }

    return 0;
}

