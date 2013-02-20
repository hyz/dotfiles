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
    socket_writer(socket_ptr socket)
        : socket_(socket)
    {
    }

    static void start(shared_ptr<socket_writer> obj)
    {
        if (obj->wbuf_.size() == 0 && obj->tmpbuf_.size() > 0)
        {
            std::ostream outs(&obj->wbuf_);
            outs << &obj->tmpbuf_;
            // obj->tmpbuf_.consume(obj->tmpbuf_.size()); // ?

            asio::async_write(*obj->socket_ , obj->wbuf_
                    , bind(&socket_writer::_write, obj, asio::placeholders::error)
                    );
        }
    }

    asio::streambuf* rdbuf() { return &tmpbuf_; }

private:
    socket_ptr socket_;
    asio::streambuf wbuf_;
    asio::streambuf tmpbuf_;

    typedef boost::shared_ptr<socket_writer> this_shptr;

private:
    static void _write(this_shptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(obj, err, 9);
        }

        // if (obj->wbuf_.size() == 0 && obj->tmpbuf_.size() > 0)
        start(obj);
    }

    static void _error(this_shptr obj, const boost::system::error_code& err, int w)
    {
        std::cout << "socket_writer error: " << err.message() << "\n";
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
    std::string method;
    std::string uri;
    std::string http_version;
    http_headers headers;
};

struct request_reader
{
    request_reader(socket_ptr socket)
        : socket_(socket)
        , writer_(new socket_writer(socket))
    {
        content_length_ = 0;
    }

    static void start(shared_ptr<request_reader> obj)
    {
        asio::async_read_until(*obj->socket_, obj->reqbuf_, "\r\n",
                bind(&request_reader::handle_method_line, obj, asio::placeholders::error));
    }

private:
    typedef boost::shared_ptr<request_reader> this_shptr;

    socket_ptr socket_;
    asio::streambuf reqbuf_;

    request req_;

    int content_length_;

    shared_ptr<socket_writer> writer_;

private:
    static void handle_method_line(this_shptr obj, const boost::system::error_code& err)
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
        std::istream ins(&obj->reqbuf_);
        ins >> obj->req_.method;
        boost::trim(obj->req_.method);
        if (obj->req_.method.empty())
        {
            asio::async_read_until(*obj->socket_, obj->reqbuf_, "\r\n",
                    bind(&request_reader::handle_method_line, obj, asio::placeholders::error));
            return;
        }

        ins >> obj->req_.uri >> obj->req_.http_version;

        std::cout << obj->req_.method << " " << obj->req_.uri << " " << obj->req_.http_version << "\n";
        std::string tmp;
        std::getline(ins, tmp);

        asio::async_read_until(*obj->socket_, obj->reqbuf_, "\r\n\r\n",
                bind(&request_reader::handle_headers, obj, asio::placeholders::error));
    }

    static void handle_headers(this_shptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(obj, err, 2);
        }

        obj->content_length_ = 0;

        asio::streambuf& sbuf = obj->reqbuf_;
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
                obj->req_.headers[k] = val;

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
                , bind(&request_reader::handle_content, obj, asio::placeholders::error));
    }

    static void handle_content(this_shptr obj, const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(obj, err, 3);
        }

        //(find/create object)
        //
        // std::cout << &obj->reqbuf_ << "\n";
        ///

        std::ostream outs(obj->writer_->rdbuf());

        std::vector<char> tmpbuf(obj->content_length_ + 1);
        obj->reqbuf_.sgetn(&tmpbuf[0], obj->content_length_);

        reqrsp_(outs, obj->req_, &tmpbuf[0], &tmpbuf[obj->content_length_]);
        socket_writer::start(obj->writer_);

        // Next request
        asio::async_read_until(*obj->socket_, obj->reqbuf_, "\r\n",
                bind(&request_reader::handle_method_line, obj, asio::placeholders::error));
    }

    static void reqrsp_(std::ostream& outs, const request& req, const char *cont, const char *end)
    {
        outs << req.method << " " << req.uri << " " << req.http_version << "\r\n"
            << req.headers << "\r\n"
            ;
        if (cont < end)
            outs.write(cont, end - cont);
    }

    static void _error(this_shptr obj, const boost::system::error_code& err, int w)
    {
        std::cout << "Error: " << err.message() << "\n";

        if (err == asio::error::eof)
        {
        }
    }
};

struct acceptor
{
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
    asio::ip::tcp::acceptor acceptor_;
    socket_ptr socket_;

    std::map<std::string, std::string> glomap_;

private:
    static void handle_accept(shared_ptr<acceptor> obj, const boost::system::error_code& e)
    {
        if (e)
        {
            return _error(obj, e);
        }

        std::cout << "accepted " << obj->socket_->local_endpoint() << "%" << obj->socket_->remote_endpoint() << "\n";

        shared_ptr<request_reader> req(new request_reader(obj->socket_));
        request_reader::start(req);

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

