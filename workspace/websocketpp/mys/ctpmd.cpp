#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
namespace posix = boost::asio::posix;

// pull out the type of messages sent by our config
//typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

struct client : websocketpp::client<websocketpp::config::asio_client>
{
    posix::stream_descriptor input_;
    std::array<char,512> input_buffer_;

    client() : input_(_io_s(),::dup(STDIN_FILENO)) {}

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg)
    {
        std::cout << "on_message called with hdl: " << hdl.lock().get()
                  << " and message: " << msg->get_payload()
                  << std::endl;
        this->read_input(hdl);
    }

    void on_open(websocketpp::connection_hdl hdl) { read_input(hdl); }

    void read_input(websocketpp::connection_hdl hdl)
    {
        input_.async_read_some(boost::asio::buffer(input_buffer_),
            [this,hdl](boost::system::error_code ec0, std::size_t length) {
                websocketpp::lib::error_code ec;
                this->send(hdl, input_buffer_.data(), length, websocketpp::frame::opcode::text, ec);
                if (ec) {
                    std::cout << "Echo failed because: " << ec.message() << std::endl;
                } else {
                }
            });
    }

private:
    boost::asio::io_service& _io_s() {
        init_asio();
        return get_io_service();
    }
};

int main(int argc, char* argv[]) {
    // Create a client endpoint
    client c;

    std::string uri = "ws://localhost:9002";

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        // Set logging to be pretty verbose (everything except message payloads)
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize ASIO

        // Register our message handler
        c.set_message_handler(bind(&client::on_message,&c,::_1,::_2));
        c.set_open_handler(bind(&client::on_open,&c,::_1));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        c.connect(con);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run()
        // will exit when this connection is closed.
        c.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}
