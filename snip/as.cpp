#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/format.hpp>
using boost::format;

using boost::bind;
using boost::shared_ptr;

using boost::asio::ip::tcp;
namespace asio = boost::asio;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

struct socket_writer
{
    socket_ptr socket_;
    asio::streambuf streambuf_;

    typedef boost::shared_ptr<socket_writer> this_shptr;

    socket_writer(socket_ptr socket)
        : socket_(socket)
    {
    }

    static void start(this_shptr obj, asio::sterambuf& sbuf)
    {
        bool wk = obj->streambuf_.size() > 0;
        std::ostream outs(&obj->streambuf_);
        outs << sbuf;

        if (!wk)
        {
            asio::async_write(*obj->socket_
                    , obj->streambuf_
                    , bind(&socket_writer::_write, obj, asio::placeholders::error)
                    );
        }
    }

private:
    static void _error(this_shptr obj, const boost::system::error_code& err, int w)
    {
        std::cout << "socket_writer error: " << err.message() << "\n";
    }

    static void _write(this_shptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(obj, err, 9);
        }
    }
};

struct http_headers : std::vector<std::pair<std::string, std::string> >
{
    using std::vector<std::pair<std::string, std::string> >::iterator;
    using std::vector<std::pair<std::string, std::string> >::const_iterator;

    iterator find(const std::string& k)
    {
        iterator i = begin();
        for (; i != end(); ++i)
            if (i->first == k)
                break;
        return i;
    }

    std::string& operator[](const std::string& k)
    {
        iterator i = find(k);
        if (i == end())
        {
            push_back(std::make_pair(k, std::string()));
            i = end() - 1;
        }
        return i->second;
    }

    const_iterator find(const std::string& k) const { return const_cast<http_headers*>(this)->find(k); }
    const std::string& operator[](const std::string& k) const { return const_cast<http_headers*>(this)->operator[](k); }
};

std::ostream& operator<<(std::ostream& outs, const http_headers& headers)
{
    for (http_headers::const_iterator i = headers.begin(); i != headers.end(); ++i)
        outs << i->first << ": " << i->second << "\r\n";
    return outs;
}

struct request
{
    socket_ptr socket_;
    asio::streambuf streambuf_;

    std::string method_;
    std::string uri_;
    std::string http_version_;
    http_headers headers_;

    int content_length_;
    std::string ckval_;

    shared_ptr<socket_writer> writer_;

    typedef boost::shared_ptr<request> sh_ptr;

    request(socket_ptr socket)
        : socket_(socket)
        , writer_(new socket_writer(socket));
    {
        content_length_ = 0;
    }

    static void start(sh_ptr obj)
    {
        asio::async_read_until(*obj->socket_, obj->streambuf_, "\r\n",
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
            return _error(obj, err, 1);
        }

        // Check that  is OK.
        std::istream ins(&obj->streambuf_);
        ins >> obj->method_;
        boost::trim(obj->method_);
        if (obj->method_.empty())
        {
            asio::async_read_until(*obj->socket_, obj->streambuf_, "\r\n",
                    bind(&request::handle_method_line, obj, asio::placeholders::error));
            return;
        }

        ins >> obj->uri_ >> obj->http_version_;

        std::cout << obj->method_ << " " << obj->uri_ << " " << obj->http_version_ << "\n";
        std::string tmp;
        std::getline(ins, tmp);

        asio::async_read_until(*obj->socket_, obj->streambuf_, "\r\n\r\n",
                bind(&request::handle_headers, obj, asio::placeholders::error));
    }

    static void handle_headers(sh_ptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(obj, err, 2);
        }

        asio::streambuf& sbuf = obj->streambuf_;
        std::istream ins(&sbuf);
        std::string line;
        boost::regex expr("^\\s*([\\w\\-]+)\\s*:\\s*(.*?)\\s*$");

        while (std::getline(ins, line) && line != "\r" && !line.empty())
        {
            boost::smatch res;
            if (boost::regex_match(line, res, expr))
            {
                std::string k(res[1].first, res[1].second);
                std::string val(res[2].first, res[2].second);
                obj->headers_[k] = val;

                if (k == "Content-Length" || k == "Content-length")
                {
                    obj->content_length_ = boost::lexical_cast<int>(val);
                }
            }

            std::cout << line << "\n";
        }

        std::cout << format("content len %d - %d\n") % obj->content_length_ % sbuf.size()
            ;

        asio::async_read(*obj->socket_ , sbuf
                , asio::transfer_at_least(obj->content_length_)
                , bind(&request::handle_content, obj, asio::placeholders::error));
    }

    static void handle_content(sh_ptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(obj, err, 3);
        }

        //(find/create object)
        //
        std::cout << &obj->streambuf_ << "\n";
        ///

        size_t left = obj->streambuf_.size() - obj->content_length_;

        // shared_ptr<socket_writer> rsp(new socket_writer(obj->socket_));
        asio::streambuf sbuf;
        std::ostream outs(&sbuf);
        std::istream ins(&obj->streambuf_);
        reqrsp_(outs, obj, ins);
        socket_writer::start(obj->writer_, sbuf);

        if (obj->streambuf_.size() > left)
        {
            obj->streambuf_.consume(obj->streambuf_.size() - left);
        }

        // Next request
        asio::async_read_until(*obj->socket_, obj->streambuf_, "\r\n",
                bind(&request::handle_method_line, obj, asio::placeholders::error));
    }

    static void reqrsp_(std::ostream& outs, sh_ptr obj, std::istream& ins)
    {
        outs << obj->method_ << " " << obj->uri_ << " " << obj->http_version_ << "\r\n"
            << obj->headers_ << "\r\n"
            << ins.rdbuf();

        // std::string k = "Cookies";
        // if (k.length() < line.length() && std::equal(k.begin(), k.end(), line.begin()))
        // {
        //     boost::regex e("^Cookies:.*[\\s;]ck=([^ ;\t\r\n]+)\\s*$");
        //     boost::smatch res;
        //     if (boost::regex_match(line, res, e))
        //     {
        //         obj->ckval_.assign(res[1].first, res[1].second);
        //     }
        // }
    }

    static void _error(sh_ptr obj, const boost::system::error_code& err, int w)
    {
        std::cout << "Error: " << err.message() << "\n";

        if (err == asio::error::eof)
        {
        }
    }
};

struct acceptor
{
    asio::ip::tcp::acceptor acceptor_;
    socket_ptr socket_;

    std::map<std::string, std::string> glomap_;

    acceptor(asio::io_service& io_service, const char* addr, const char* port);

    static void start(shared_ptr<acceptor> obj)
    {
        obj->acceptor_.listen();

        obj->socket_.reset(new tcp::socket(obj->acceptor_.get_io_service()));
        obj->acceptor_.async_accept(*obj->socket_,
                bind(&acceptor::handle_accept, obj, asio::placeholders::error)
                );
    }

private:
    static void handle_accept(shared_ptr<acceptor> obj, const boost::system::error_code& e)
    {
        if (e)
        {
            return _error(obj, e);
        }

        std::cout << "accepted " << obj->socket_->local_endpoint() << "%" << obj->socket_->remote_endpoint() << "\n";

        shared_ptr<request> req(new request(obj->socket_));
        request::start(req);

        obj->socket_.reset(new tcp::socket(obj->acceptor_.get_io_service()));
        obj->acceptor_.async_accept(*obj->socket_,
                bind(&acceptor::handle_accept, obj, asio::placeholders::error));
    }

    static void _error(shared_ptr<acceptor> obj, const boost::system::error_code& err)
    {
        std::cout << "acceptor error: " << err.message() << "\n";
    }
};

acceptor::acceptor(asio::io_service& io_service, const char* addr, const char* port)
  : acceptor_(io_service)
{
    asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
    asio::ip::tcp::resolver::query query(addr, port);
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
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
        asio::signal_set sigstop(io_service);
        asio::signal_set sigw(io_service);

        sigstop.add(SIGINT);
        sigstop.add(SIGTERM);
#if defined(SIGQUIT)
        sigstop.add(SIGQUIT);
#endif // defined(SIGQUIT)
        sigstop.async_wait(bind(&asio::io_service::stop, &io_service));

        shared_ptr<acceptor> a(new acceptor(io_service, "0", argv[1]));
        acceptor::start(a);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}

