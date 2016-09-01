#include <iostream>
#include <boost/asio.hpp>

using namespace std;
using namespace boost;

void foo() { cout << "foo\n"; }

int main()
{
    asio::io_service ios;
    cout << "After construct: stopped="<< ios.stopped() << endl;

    ios.stop();
    cout << "After stop: stopped=" << ios.stopped() << endl;

    ios.reset();
    cout << "After reset: stopped=" << ios.stopped() << endl;

    ios.run();
    cout << "After run: stopped=" << ios.stopped() << endl;

    ios.post(foo);
    cout << "After post: stopped=" << ios.stopped() << endl;

    ios.run();
    cout << "After run: stopped=" << ios.stopped() << endl;

    ios.reset();
    cout << "After reset: stopped=" << ios.stopped() << endl;

    ios.run();
    cout << "After run: stopped=" << ios.stopped() << endl;

    ios.run();
    cout << "After run: stopped=" << ios.stopped() << endl;

    return 0;
}

