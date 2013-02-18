#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/format.hpp>
using boost::format;

using boost::bind;
using boost::shared_ptr;

using boost::asio::ip::tcp;
namespace asio = boost::asio;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

struct response
{
    response(socket_ptr socket)
        : socket_(socket)
    {
    }

    void handle_write_request(const boost::system::error_code& err)
    {
        if (err)
        {
            return handle_error(err, 9);
        }

        asio::async_read_until(socket_, streambuf_, "\r\n",
                bind(&response::handle_method_line, this,
                    asio::placeholders::error));
    }

    socket_ptr socket_;

    asio::streambuf streambuf_;
};

struct request
{
    socket_ptr socket_;
    asio::streambuf request_;

    int content_length_;
    std::string ckval_;

    typedef boost::shared_ptr<request> sh_ptr;

    request(socket_ptr socket) : socket_(socket)
    {
        content_length_ = 0;
    }

    static void start(sh_ptr obj)
    {
        // socket_->get_io_service();

        asio::async_read_until(*socket_, streambuf_, "\r\n",
                bind(&request::handle_method_line, obj, asio::placeholders::error));
    }

private:
    static void handle_method_line(sh_ptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            if (err == asio::error::eof)
            {
                ;
            }
            return handle_error(sh_ptr, err, 1);
        }

        // Check that response is OK.
        std::string method, uri, ver;

        std::istream istream(&streambuf_);
        istream >> method >> uri;
        // std::string status_message;
        std::getline(istream, ver);

        asio::async_read_until(*obj->socket_, obj->streambuf_, "\r\n\r\n",
                bind(&request::handle_read_headers, obj, asio::placeholders::error));
    }

    static void handle_read_headers(sh_ptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return handle_error(err, 2);
        }

        // Process obj response headers.
        asio::streambuf& sbuf = obj->streambuf_;
        std::istream istream(&sbuf);
        std::string line;
        while (std::getline(istream, line) && line != "\r")
        {
            std::string k;

            k = "Cookies";
            if (k.length() < line.length() && std::equal(k.begin(), k.end(), line.begin()))
            {
                boost::regex e("^Cookies:.*[\\s;]ck=([^ ;\t\r\n]+)\\s*$");
                boost::smatch res;
                if (boost::regex_match(line, res, e))
                {
                    obj->ckval_.assign(res[1].first, res[1].second);
                }
            }

            k = "Content-Length";
            if (k.length() < line.length() && std::equal(k.begin(), k.end(), line.begin()))
            {
                boost::regex e("^Content-[Ll]ength:\\s*(\\d+)\\s*$");
                boost::smatch res;
                if (boost::regex_match(line, res, e))
                {
                    obj->content_length_ = boost::lexical_cast<int>(res[1]);
                    if (obj->content_length_ < 0) // || obj->content_length_ > 1024*512
                    {
                        return handle_error(obj, err, 2);
                    }
                }
            }

            std::cout << line << "\n";
        }

        std::cout << format("content len %d - %d\n") % obj->content_length_ % sbuf.size()
            ;

        asio::async_read(*obj->socket_ , sbuf
                , asio::transfer_at_least(obj->content_length_)
                , bind(&request::handle_read_content, obj, asio::placeholders::error));
    }

    static void handle_read_content(sh_ptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return handle_error(obj, err, 3);
        }

        //(find/create object)
        //
        std::cout << &obj->streambuf_ << "\n";
        ///

        size_t left = obj->streambuf_.size() - obj->content_length_;

        shared_ptr<resposne> rsp(new response(obj->socket_));
        std::istream ins(&obj->streambuf_);
        std::ostream outs(&rsp->streambuf_);
        handle_request(outs, ins);
        response::start(rsp);

        if (obj->streambuf_.size() > left)
            obj->streambuf_.consume(obj->streambuf_.size() - left);

        // Next request
        asio::async_read_until(*obj->socket_, obj->streambuf_, "\r\n",
                bind(&request::handle_method_line, obj, asio::placeholders::error));
    }

    static void handle_error(sh_ptr obj, const boost::system::error_code& err, int w)
    {
        std::cout << "Error: " << err.message() << "\n";

        if (err == asio::error::eof)
        {
        }
    }
};

struct Main
{
    // boost::asio::io_service& io_service_;

    /// Acceptor used to listen for incoming connections.
    asio::ip::tcp::acceptor acceptor_;

    /// The connection manager which owns all live connections.
    connection_manager connection_manager_;

    /// The next connection to be accepted.
    connection_ptr new_request_;

    /// The handler for all incoming requests.
    request_handler request_handler_;

    std::map<std::string, std::string> glomap_;

    Main(asio::io_service& io_service, const char* addr, const char* port);
    void bye() { }

    void handle_accept(const boost::system::error_code& e);
};

Main::Main(asio::io_service& io_service, const char* addr, const char* port)
  : acceptor_(io_service),
    connection_manager_(),
    new_request_(),
    // request_handler_(doc_root)
{
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).

    asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
    asio::ip::tcp::resolver::query query(addr, port);
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    socket_.reset(new tcp::socket(acceptor_.get_io_service()));
    acceptor_.async_accept(*socket_,
            bind(&Main::handle_accept, this, asio::placeholders::error));
}

void Main::handle_accept(const boost::system::error_code& e)
{
    if (!acceptor_.is_open())
    {
        return;
    }

    if (e)
    {
        return handle_error(e);
    }

    shared_ptr<request> req(new request(socket_));
    request::start(req);

    socket_.reset(new tcp::socket(acceptor_.get_io_service()));
    acceptor_.async_accept(*socket_,
            bind(&Main::handle_accept, this, asio::placeholders::error));
}

// void sigexit(asio::io_service *io_service) { io_service->stop(); }

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cout << "Usage: ...\n";
      return 1;
    }

    asio::io_service io_service;

    /// The signal_set is used to register for process termination notifications.
    asio::signal_set sigstop;
    asio::signal_set sigw;

    sigstop(io_service),
    sigstop.add(SIGINT);
    sigstop.add(SIGTERM);
#if defined(SIGQUIT)
    sigstop.add(SIGQUIT);
#endif // defined(SIGQUIT)
    sigstop.async_wait(bind(asio::io_service::stop, &io_service));

    Main m(io_service, "0", argv[1]);

    io_service.run();
    m.bye();
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }

  return 0;
}
