#include <iostream>
#include <algorithm>

#include <boost/utility/string_ref.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/http/buffered_socket.hpp>
#include <boost/http/algorithm.hpp>

using namespace std;
using namespace boost;
namespace ip = boost::asio::ip;
using boost::asio::ip::tcp;

struct server : asio::io_service, tcp::acceptor
{
    server(int port=8080) : tcp::acceptor(*this, tcp::endpoint(tcp::v4(), port))
    {}

    void run() {
        spawn(io_service(), [this](asio::yield_context yield){ work(yield); });
        boost::asio::io_service::run();
    }

private:
    int id_ = 0;
    asio::io_service& acceptor() { return *this; }
    asio::io_service& io_service() { return *this; }

    void work(asio::yield_context yield)
    {
        int serve_id = id_++;
        try {
            serve(yield, serve_id);
        } catch (system::system_error &e) {
            if (e.code() != system::error_code{asio::error::eof}) {
                cerr << '[' << serve_id << "] Aborting on exception: " << e.what() << endl;
                std::exit(1);
            }
            cout << '[' << serve_id << "] Error: " << e.what() << endl;
        } catch (std::exception &e) {
            cerr << '[' << serve_id << "] Aborting on exception: " << e.what() << endl;
            std::exit(1);
        }
    }

    void accept(asio::yield_context yield, tcp::socket& s) {
        //if (n_concurent_ < 10) {
            this->async_accept(s, yield);
        //}
    };

    void serve(asio::yield_context yield, int serve_id)
    {
        http::buffered_socket socket(io_service());

        accept(yield, socket.next_layer());
        spawn(io_service(), [this](asio::yield_context yield){ work(yield); });

        while (socket.is_open()) {
            std::string method;
            std::string path;
            http::message message;

            cout << "--\n[" << serve_id << "] About to receive a new message" << endl;
            socket.async_read_request(method, path, message, yield);
            //message.body().clear(); // freeing not used resources

            if (http::request_continue_required(message)) {
                cout << '[' << serve_id << "] Continue required. About to send" " \"100-continue\"" << std::endl;
                socket.async_write_response_continue(yield);
            }

            while (socket.read_state() != http::read_state::empty) {
                cout << '[' << serve_id << "] Message not fully received" << endl;
                switch (socket.read_state()) {
                case http::read_state::message_ready:
                    cout << '[' << serve_id << "] About to receive some body" << endl;
                    socket.async_read_some(message, yield);
                    break;
                case http::read_state::body_ready:
                    cout << '[' << serve_id << "] About to receive trailers" << endl;
                    socket.async_read_trailers(message, yield);
                    break;
                default:;
                }
            }

            //cout << "BODY:==";
            //for (const auto &e: message.body()) {
            //    cout << char(e);
            //}
            //cout << "==" << endl;

            cout << '[' << serve_id << "] Message received. State = " << int(socket.read_state()) << endl;
            cout << '[' << serve_id << "] Method: " << method << endl;
            cout << '[' << serve_id << "] Path: " << path << endl;
            {
                auto host = message.headers().find("host");
                if (host != message.headers().end())
                    cout << '[' << serve_id << "] Host header: " << host->second << endl;
            }
            cout << '[' << serve_id << "] Write state = " << int(socket.write_state()) << std::endl;
            cout << '[' << serve_id << "] About to send a reply" << endl;

            http::message reply;
            //reply.headers().emplace("connection", "close");
            const char body[] = "Hello World\n";
            std::copy(body, body + sizeof(body) - 1, std::back_inserter(reply.body()));

            socket.async_write_response(200, string_ref("OK"), reply, yield);
        }
    }
};

int main()
{
    try {
        server s;
        s.run();
        cout << "running..." << endl;
    } catch (std::exception &e) {
        cerr << "Aborting on exception: " << e.what() << endl;
        std::exit(1);
    }
    return 0;
}

/// http://stackoverflow.com/questions/29322666/undefined-reference-to-cxa-thread-atexitcxxabi-when-compiling-with-libc
extern "C" int __cxa_thread_atexit(void (*func)(), void *obj, void *dso_symbol) {
    int __cxa_thread_atexit_impl(void (*)(), void *, void *);
    return __cxa_thread_atexit_impl(func, obj, dso_symbol);
}

