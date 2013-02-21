#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/format.hpp>
using boost::format;

using boost::bind;
using boost::function;
using boost::shared_ptr;

using boost::asio::ip::tcp;
namespace asio = boost::asio;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

struct socwriter : boost::noncopyable, boost::enable_shared_from_this<socwriter>
{
    socwriter(socket_ptr soc, function<void (socket_ptr)> fin) : soc_(soc), fin_(fin) {}

    ~socwriter() { fin_(soc_); }

    void send(const std::string& s)
    {
        bool empty = ls_.empty();

        ls_.push_back(s);

        if (empty)
        {
            asio::async_write(*soc_, asio::buffer(ls_.front())
                    , bind(&socwriter::writeb, shared_from_this(), asio::placeholders::error)
                    );
        }
    }

private:
    shared_ptr<tcp::socket> soc_;
    function<void (socket_ptr)> fin_;

    std::list<std::string> ls_;

    void writeb(const boost::system::error_code& err)
    {
        if (err)
        {
            return ;//_error(obj, err, 9);
        }

        ls_.pop_front();

        if (!ls_.empty())
        {
            asio::async_write(*soc_, asio::buffer(ls_.front())
                    , bind(&socwriter::writeb, shared_from_this(), asio::placeholders::error)
                    );
        }
    }
};

struct sender
{
    typedef socwriter worker;
    typedef shared_ptr<worker> worker_ptr;

    ~sender();

    function<void (const std::string&)> hire(shared_ptr<tcp::socket> soc);
    //worker_ptr hire(shared_ptr<tcp::socket> soc);

    void finish(socket_ptr soc)
    {
        hired_.erase(soc->native_handle());
    }

private:
    typedef std::map<tcp::socket::native_type, boost::weak_ptr<worker> > hired_t;
    typedef typename hired_t::iterator iterator;

    hired_t hired_;
};

sender::~sender()
{
    //for (iterator i = hired_.begin(); i != hired_.end(); ++i)
        //i->second->soc_->cancel();
}

function<void (const std::string&)> sender::hire(shared_ptr<tcp::socket> soc) //, std::streambuf* buf)
{
    sender::worker_ptr ker(new worker(soc, bind(&sender::finish, this, _1)));

    boost::weak_ptr<worker> _w(ker);
    std::pair<iterator, bool> ret = hired_.insert(std::make_pair(soc->native_handle(), _w));
    if (!ret.second)
    {
        ker = ret.first->second.lock();
    }

    return bind(&worker::send, ker, _1);
}

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

struct http_request
{
    std::string method;
    std::string uri;
    std::string http_version;
    http_headers headers;

    std::vector<char> cont;
};

struct httprsp_echo //struct http_responser
{
    typedef function<void (const std::string&)> send_fn_t;

    httprsp_echo(function<send_fn_t (socket_ptr)> hire)
        : hire_(hire)
    {
    }

    void query(shared_ptr<tcp::socket> soc, const http_request& req)
    {
        std::ostringstream outs;

        outs << req.method << " " << req.uri << " " << req.http_version << "\r\n"
            << req.headers << "\r\n"
            ;
        if (!req.cont.empty())
            outs.write(&req.cont[0], req.cont.size());

        // shared_ptr<sender::worker> ker = hire_(soc);
        send_fn_t sendfn = hire_(soc);
        sendfn(outs.str());
    }

private:
    function<send_fn_t (socket_ptr)> hire_;
};

struct protohttp_reader : boost::enable_shared_from_this<protohttp_reader>, boost::noncopyable
{
    protohttp_reader(shared_ptr<tcp::socket> soc
            , function<void (socket_ptr, const http_request&)> rsp)
        : socket_(soc), rsp_(rsp)
    {
        asio::async_read_until(*socket_, rsbuf_, "\r\n",
                bind(&protohttp_reader::handle_method_line, shared_from_this(), asio::placeholders::error));
    }

private:
    typedef boost::shared_ptr<protohttp_reader> this_shptr;

    socket_ptr socket_;
    asio::streambuf rsbuf_;
    http_request req_;

    function<void (socket_ptr, const http_request&)> rsp_;

private:
    void handle_method_line(const boost::system::error_code& err)
    {
        if (err)
        {
            if (err == asio::error::eof)
            {
                ;
            }
            return _error(err, 1);
        }

        req_ = http_request();

        // Check that  is OK.
        std::istream ins(&rsbuf_);
        ins >> req_.method;
        boost::trim(req_.method);
        if (req_.method.empty())
        {
            asio::async_read_until(*socket_, rsbuf_, "\r\n",
                    bind(&protohttp_reader::handle_method_line, shared_from_this(), asio::placeholders::error));
            return;
        }

        ins >> req_.uri >> req_.http_version;

        std::cout << req_.method << " " << req_.uri << " " << req_.http_version << "\n";
        std::string tmp;
        std::getline(ins, tmp);

        asio::async_read_until(*socket_, rsbuf_, "\r\n\r\n",
                bind(&protohttp_reader::handle_headers, shared_from_this(), asio::placeholders::error));
    }

    void handle_headers(const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(err, 2);
        }

        asio::streambuf& sbuf = rsbuf_;
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
                req_.headers[k] = val;

                if (k == "Content-Length" || k == "Content-length")
                {
                    req_.cont.resize(boost::lexical_cast<int>(val));
                }
            }

            std::cout << line << "\n";
        }

        std::cout << format("content len %d - %d\n") % req_.cont.size() % sbuf.size()
            ;

        asio::async_read(*socket_ , sbuf
                , asio::transfer_at_least(req_.cont.size())
                , bind(&protohttp_reader::handle_content, shared_from_this(), asio::placeholders::error));
    }

    void handle_content(const boost::system::error_code& err)
    {
        if (err)
        {
            return _error(err, 3);
        }

        rsbuf_.sgetn(&req_.cont[0], req_.cont.size());

        rsp_(socket_, req_);

        // Next http_request
        asio::async_read_until(*socket_, rsbuf_, "\r\n",
                bind(&protohttp_reader::handle_method_line, shared_from_this(), asio::placeholders::error));
    }

    static void _error(const boost::system::error_code& err, int w)
    {
        std::cout << "Error: " << err.message() << "\n";

        if (err == asio::error::eof)
        {
        }
    }
};

struct http_reader
{
    typedef protohttp_reader reader_t;

    http_reader(function<void (socket_ptr, const http_request&)> fn) : fn_(fn) {}

    void start(socket_ptr soc)
    {
        new reader_t(soc, fn_); // shared_ptr<reader_t> er = new reader_t(soc, fn_); // er->start();
    }

    function<void (socket_ptr, const http_request&)> fn_;
};

struct socket_acceptor
{
    socket_acceptor(asio::io_service& io_service, const char* addr, const char* port, function<void (socket_ptr)> fn);

    void start()
    {
        acceptor_.listen();

        socket_.reset(new tcp::socket(acceptor_.get_io_service()));
        acceptor_.async_accept(*socket_,
                bind(&socket_acceptor::handle_accept, this, asio::placeholders::error)
                );
    }

private:
    tcp::acceptor acceptor_;
    shared_ptr<tcp::socket> socket_;

    function<void (socket_ptr)> fn_;

private:
    void handle_accept(const boost::system::error_code& e)
    {
        if (e)
        {
            return _error(e);
        }

        std::cout << "accepted " << socket_->local_endpoint() << "%" << socket_->remote_endpoint() << "\n";

        fn_(socket_);

        socket_.reset(new tcp::socket(acceptor_.get_io_service()));
        acceptor_.async_accept(*socket_,
                bind(&socket_acceptor::handle_accept, this, asio::placeholders::error));
    }

    void _error(const boost::system::error_code& err)
    {
        std::cout << "acceptor error: " << err.message() << "\n";
    }
};

socket_acceptor::socket_acceptor(asio::io_service& io_service
        , const char* addr, const char* port
        , function<void (socket_ptr)> fn)
    : acceptor_(io_service)
    , fn_(fn)
{
    asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
    asio::ip::tcp::resolver::query query(addr, port);
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
}

// void sigexit(asio::io_service *io_service) { io_service->stop(); }

void func_socket_ready(socket_ptr socket)
{
}

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

        sender sender;
        httprsp_echo responser(bind(&sender::hire, &sender, _1));
        http_reader reader(bind(&httprsp_echo::query, &responser, _1, _2));
        socket_acceptor acceptor(io_service, "0", argv[1], bind(&http_reader::start, &reader, _1));

        acceptor.start();

        io_service.run();
        std::cout << "bye.\n";
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}

