#include <cstdlib>
#include <fstream>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

using boost::asio::ip::udp;

struct Main
{
    Main(boost::asio::io_service& io_service, short port, boost::filesystem::path dir)
        : socket_(io_service, udp::endpoint(udp::v4(), port))
        , dir_(dir)
    {
        socket_.async_receive_from(
                boost::asio::buffer(data(), max_length), peer_endpoint_,
                boost::bind(&Main::handle_receive_from, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

private:
    void rtp_a() {
    }
    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
    {
        if (!error && bytes_recvd > 0) {
            boost::filesystem::ofstream ofs(dir_/std::to_string(dg_idx_++));
            ofs.write(data(), bytes_recvd);
        }
        using namespace boost::asio::placeholders;
        socket_.async_receive_from(
                boost::asio::buffer(data(), max_length), peer_endpoint_,
                boost::bind(&Main::handle_receive_from, this, error, bytes_transferred));
    }

    udp::socket socket_;
    udp::endpoint peer_endpoint_;
    enum { max_length = 4096 };
    int data_[max_length/sizeof(int)+1];
    char* data() const { return reinterpret_cast<char*>(&const_cast<Main*>(this)->data_); }

    boost::filesystem::path dir_;
    int dg_idx_ = 0;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            fprintf(stderr,"Usage: %s <port> <out-dir>\n", argv[0]);
            return 1;
        }

        boost::asio::io_service io_service;

        using namespace std; // For atoi.
        Main s(io_service, atoi(argv[1]), argv[2]);

        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
