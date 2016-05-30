#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>

#if defined(BOOST_MSVC)
#include < time.h >
#include < windows.h >

struct timezone
{
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

namespace ip = boost::asio::ip;
using ip::udp;

struct Main : boost::noncopyable
{
    Main(boost::asio::io_service& io_service, udp::endpoint ep, int idle)
        : socket_(io_service, udp::endpoint(udp::v4(),0))
        , endpoint_(ep)
    {
        singleton = this;
        std::srand(time(0));
        gettimeofday(&tv0_, NULL);
        mils_idle_ = idle;
        do_send();
        printf("sending ...\n");
    }
    Main(boost::asio::io_service& io_service, short port)
        : socket_(io_service, udp::endpoint(udp::v4(), port))
    {
        singleton = this;
        std::srand(time(0));
        gettimeofday(&tv0_, NULL);
        do_receive();
        printf("receiving ...\n");
    }

private:
    struct timeval tv0_;
    static unsigned sMills() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (1000*1000*(tv.tv_sec - singleton->tv0_.tv_sec) + tv.tv_usec - singleton->tv0_.tv_usec)/1000;
    }

    unsigned mils_idle_ = 0;
    struct recv_st
    {
        unsigned seq = 0;
        unsigned pkgcount = 0;
        unsigned bytes_recvd = 0;
        unsigned bytes_exped = 0;

        void incoming(unsigned len, unsigned seq, unsigned nbytes) {
			if (pkgcount == 0)
				gettimeofday(&Main::singleton->tv0_, NULL);
            recv_st prev = *this;

            this->pkgcount++;
            this->bytes_exped += len;
            this->seq = seq;
            this->bytes_recvd += nbytes;

            this->print(prev);
        }

        void print(recv_st const& prev) const {
            bool lost = (seq != prev.seq+1)
                || (bytes_recvd - prev.bytes_recvd != bytes_exped - prev.bytes_exped);
            unsigned ct = Main::sMills();
            if (lost || ((ct - pt_) > 5 && (seq&0x3f)==0) || (seq&0x7ff) == 0 || seq<5) {
                int nsec = (ct)/1000;
                if (nsec == 0)
                    nsec = 1;
                printf("%3u.%03u %4u %-4u %-4u %6.1f %7.1f %u%c", ct/1000,ct%1000
                        , prev.seq, seq, pkgcount, float(pkgcount)/nsec, bytes_recvd/1024.0/nsec, bytes_recvd, lost?'\t':'\n');
                if (lost)
                    printf("* %u\n", bytes_recvd-prev.bytes_recvd);
                pt_=ct;
            }
        }
        
        mutable unsigned pt_ = 0;
    } recv_;
    struct sent_st {
        unsigned seq = 0;
        unsigned bytes_sent = 0;

        //unsigned numb_psec_ = 30;
        //unsigned bytes_psec_ = 1024*300;

        void print() const {
            unsigned ct = Main::sMills();
            int nsec = ct/1000;
            if (nsec == 0)
                nsec = 1;
            if (((ct - pt_) > 5 && (seq&0x1f)==0) || (seq&0x1ff) == 0 || seq < 5) {
                printf("%3u.%03u %4u %4.1f %u\n", ct/1000,ct%1000, seq, bytes_sent/1024.0/nsec, bytes_sent);
                pt_ = ct;
            }
        }

        mutable unsigned pt_;
    } sent_;

    void do_receive()
    {
        socket_.async_receive_from( boost::asio::buffer((void*)data_, max_length)
            , endpoint_
            , [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    recv_.incoming(ntohl(data_[0]), ntohl(data_[1]), bytes_recvd);
                    do_receive();
                } else {
                    fprintf(stderr, "error %d:%s\n", ec.value(), ec.message().c_str());
                }
            });
    }
    void do_send()
    {
        int rv = rand()%100;
        unsigned length = 1024*( (rv>90?15:5) + rv%5);
        data_[0] = htonl(length);
        data_[1] = htonl(sent_.seq);
        sent_.seq++;
        socket_.async_send_to( boost::asio::buffer((void*)data_, length)
            , endpoint_
            , [this](boost::system::error_code ec, std::size_t bytes_sent) {
                  if (!ec) {
                      sent_.bytes_sent += bytes_sent;
                      sent_.print();
                      if (mils_idle_ > 0)
                          std::this_thread::sleep_for(std::chrono::milliseconds(mils_idle_));
                      do_send();
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
    static Main* singleton;
};
Main* Main::singleton;

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

#if defined(BOOST_MSVC)
#include < time.h >
#include < windows.h >

//struct timezone
//{
//	int  tz_minuteswest; /* minutes W of Greenwich */
//	int  tz_dsttime;     /* type of dst correction */
//};
//
//int gettimeofday(struct timeval *tv, struct timezone *tz);


#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)

{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		tmpres /= 10;  /*convert into microseconds*/
					   /*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}

#endif
