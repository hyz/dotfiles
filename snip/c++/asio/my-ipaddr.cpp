#include <iostream>
#include <boost/asio.hpp>

int main()
{
    using namespace boost::asio::ip;

    try {
        boost::asio::io_service io_s;
        udp::resolver   resolver(io_s);
        udp::resolver::query query(udp::v4(), "baidu.com", "");
        udp::resolver::iterator iter = resolver.resolve(query);
        udp::endpoint ep = *iter;
        udp::socket socket(io_s);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        std::cout << "My IP according to baidu is: " << addr.to_string() << std::endl;
    } catch (std::exception& e){
        std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;
    }
}

