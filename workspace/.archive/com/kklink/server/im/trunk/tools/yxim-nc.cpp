#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3) {
            std::cerr << "Usage: yxim-nc <host> <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        tcp::socket sk(io_service);
        union { uint32_t len; char s[sizeof(uint32_t)]; } un;

        {
            tcp::resolver resolver(io_service);
            tcp::resolver::query query(tcp::v4(), argv[1], argv[2]);
            tcp::resolver::iterator iterator = resolver.resolve(query);

            boost::asio::connect(sk, iterator);
        } {
            std::string request(std::istream_iterator<char>(std::cin) , std::istream_iterator<char>());
            boost::algorithm::trim(request);

            un.len = htonl(request.size());
            boost::asio::write(sk, boost::asio::buffer(un.s));
            boost::asio::write(sk, boost::asio::buffer(request));
        } {
            std::vector<char> response;

            boost::asio::read(sk, boost::asio::buffer(un.s));
            response.resize(ntohl(un.len));
            boost::asio::read(sk, boost::asio::buffer(response));

            std::cout.write(&response[0], response.size());
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 2;
    }

    return 0;
}

