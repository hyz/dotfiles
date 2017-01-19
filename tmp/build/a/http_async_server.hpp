#ifndef HTTP_ASYNC_SERVER_HPP__
#define HTTP_ASYNC_SERVER_HPP__

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "beast/core/placeholders.hpp"
#include "beast/core/streambuf.hpp"
#include "beast/http.hpp"
#include "mime_type.hpp"
#include "file_body.hpp"
#include "log.hpp"

namespace beast { namespace http {

struct http_async_server : boost::asio::io_service
{
    using io_service = boost::asio::io_service;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using address_type = boost::asio::ip::address;
    using socket_type = boost::asio::ip::tcp::socket;

    using request_type = request<string_body>;
    using response_type = response<file_body>;

    //std::mutex mutex_;
    boost::asio::ip::tcp::acceptor acceptor_;
    socket_type sock_;
    std::string root_;

public:
    http_async_server(io_service& io_s, endpoint_type const& ep, std::string const& root)
        : acceptor_(io_s)
        , sock_(io_s)
        , root_(root)
    {
        acceptor_.open(ep.protocol());
        acceptor_.bind(ep);
        acceptor_.listen(boost::asio::socket_base::max_connections);
        acceptor_.async_accept(sock_,
            std::bind(&http_async_server::on_accept, this, beast::asio::placeholders::error));
    }

    void teardown(io_service& io_s)
    {
        error_code ec;
        io_s.dispatch( [&]{ acceptor_.close(ec); } );
    }

    //void run(unsigned threads) {}

private:
    //template<class Stream, class Handler, bool isRequest, class Body, class Fields>
    //class write_op
    //{
    //    using alloc_type = handler_alloc<char, Handler>;
    //    struct Data
    //    {
    //        Stream& s;
    //        http::message<isRequest, Body, Fields> msg;
    //        Handler h;
    //        bool bcontinuation;
    //        template<class DeducedHandler>
    //        Data(DeducedHandler&& h_, Stream& s_, http::message<isRequest, Body, Fields>&& m_)
    //            : s(s_)
    //            , msg(std::move(m_))
    //            , h(std::forward<DeducedHandler>(h_))
    //            , bcontinuation( boost_asio_handler_cont_helpers::is_continuation(h_) )
    //        {}
    //    };
    //    std::shared_ptr<Data> data_;
    //public:
    //    write_op(write_op&&) = default;
    //    write_op(write_op const&) = default;
    //    template<class DeducedHandler, class... Args>
    //    write_op(DeducedHandler&& h, Stream& s, Args&&... args)
    //        : data_(std::allocate_shared<Data>(
    //                    alloc_type{h}, std::forward<DeducedHandler>(h), s, std::forward<Args>(args)...))
    //    {
    //        (*this)(error_code{}, false);
    //    }
    //    void operator()(error_code ec, bool again = true)
    //    {
    //        auto& d = *data_;
    //        d.bcontinuation = d.bcontinuation || again;
    //        if (!again) {
    //            beast::http::async_write(d.s, d.msg, std::move(*this));
    //            return;
    //        }
    //        d.h(ec);
    //    }
    //    //friend
    //    static void* asio_handler_allocate(std::size_t size, write_op* op)
    //    {
    //        return boost_asio_handler_alloc_helpers::allocate(size, op->data_->h);
    //    }
    //    //friend
    //    static void asio_handler_deallocate( void* p, std::size_t size, write_op* op)
    //    {
    //        return boost_asio_handler_alloc_helpers::deallocate(p, size, op->data_->h);
    //    }
    //    //friend
    //    static bool asio_handler_is_continuation(write_op* op)
    //    {
    //        return op->data_->bcontinuation;
    //    }
    //    template<class Function>
    //    //friend
    //    static void asio_handler_invoke(Function&& f, write_op* op)
    //    {
    //        return boost_asio_handler_invoke_helpers::invoke(f, op->data_->h);
    //    }
    //};
    //template<class Stream, bool isRequest, class Body, class Fields, class DeducedHandler>
    //static void
    //async_write(Stream& stream, message<isRequest, Body, Fields>&& msg, DeducedHandler&& handler)
    //{
    //    write_op<Stream, typename std::decay<DeducedHandler>::type,isRequest, Body, Fields>
    //        {std::forward<DeducedHandler>(handler), stream, std::move(msg)};
    //}

    class Peer : public std::enable_shared_from_this<Peer>
    {
        int id_;
        streambuf sbuf_;
        socket_type sock_;
        http_async_server& server_;
        boost::asio::io_service::strand strand_;
        request_type req_;

    public:
        Peer(Peer&&) = default;
        Peer(Peer const&) = default;
        Peer& operator=(Peer&&) = delete;
        Peer& operator=(Peer const&) = delete;

        Peer(socket_type&& sock, http_async_server& server)
            : sock_(std::move(sock))
            , server_(server)
            , strand_(sock_.get_io_service())
        {
            static int n = 0;
            id_ = ++n;
        }

        void run()
        {
            do_read();
        }

        void do_read()
        {
            async_read(sock_, sbuf_, req_
                    , strand_.wrap(
                        std::bind(&Peer::on_read, shared_from_this(), asio::placeholders::error)));
        }

        void on_read(error_code const& ec)
        {
            if (ec) {
                LOG(ERROR) << id_ << ec.message();
                return;
            }
            static auto* const HTTP_HEADER_Server = "Simple-Beast";

            auto path = req_.url;
            if (path == "/")
                path = "/index.html";
            path = server_.root_ + path;

            if (!boost::filesystem::exists(path)) {
                response<string_body> res;
                res.status = 404;
                res.reason = "Not Found";
                res.version = req_.version;
                res.fields.insert("Server", HTTP_HEADER_Server);
                res.fields.insert("Content-Type", "text/json");
                res.body = "The file '" + path + "' was not found";
                prepare(res);
                async_write(sock_, std::move(res),
                    std::bind(&Peer::on_write, shared_from_this(), asio::placeholders::error));
                return;
            }

            try {
                response_type res;
                res.status = 200;
                res.reason = "OK";
                res.version = req_.version;
                res.fields.insert("Server", HTTP_HEADER_Server);
                res.fields.insert("Content-Type", mime_type(path));
                res.body = path;
                prepare(res);
                async_write(sock_, std::move(res),
                    std::bind(&Peer::on_write, shared_from_this(), asio::placeholders::error));
            } catch(std::exception const& e) {
                response<string_body> res;
                res.status = 500;
                res.reason = "Internal Error";
                res.version = req_.version;
                res.fields.insert("Server", HTTP_HEADER_Server);
                res.fields.insert("Content-Type", "text/html");
                res.body = std::string{"An internal error occurred"} + e.what();
                prepare(res);
                async_write(sock_, std::move(res),
                    std::bind(&Peer::on_write, shared_from_this(), asio::placeholders::error));
            }
        }

        void on_write(error_code ec)
        {
            if(ec) {
                LOG(ERROR) << id_ << ec.message();
                return;
            }
            do_read();
        }
    };

    void on_accept(error_code ec)
    {
        if (ec) {
            LOG(ERROR) << ec.message();
            exit(127);
        }

        //socket_type sock(std::move(sock_));
        std::make_shared<Peer>(std::move(sock_), *this)->run();

        acceptor_.async_accept(sock_,
            std::bind(&http_async_server::on_accept, this, asio::placeholders::error));
    }
};

} } // http // beast

#endif
