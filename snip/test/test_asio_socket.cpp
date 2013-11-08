#include <list>
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace boost;
using boost::asio::ip::tcp;

void foo(int x, int y) {}

tcp::endpoint resolve(boost::asio::io_service & io_service, const string& h, const string& p)
{
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(h, p);
    tcp::resolver::iterator i = resolver.resolve(query);
    return *i;
}

void handle_connect(const boost::system::error_code& ec)
{
    cout << ec << endl;
}


int main(int ac, char *const av[])
{
    if (ac < 3)
        exit (3);
    const char *h = av[1];
    const char *p = av[2];

    asio::io_service ios;

    tcp::socket soc(ios);

    soc.async_connect(resolve(ios, h, p), bind(&handle_connect, boost::asio::placeholders::error));
    ios.run();
    soc.close();
    ios.reset();

        soc.open(boost::asio::ip::tcp::v4());
        soc.async_connect(resolve(ios, h, p), bind(&handle_connect, boost::asio::placeholders::error));
        soc.close();

        ios.run();
        ios.reset();

    // soc.open(boost::asio::ip::tcp::v4());
    for (int n=3; n > 0; --n)
    {
        soc.open(boost::asio::ip::tcp::v4());
        soc.async_connect(resolve(ios, h, p), bind(&handle_connect, boost::asio::placeholders::error));

        ios.run();
        soc.close();
        ios.reset();
    }

    return 0;
}

