#include <iostream>
#include <fstream>
#include <ostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

struct http_client : boost::asio::io_service
{
    http_client(const std::string& server, const std::string& path)
        : socket_(*this)
    {
        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        std::ostream request_stream(&request_buf_);
        request_stream << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << server << "\r\n"
            << "Accept: */*\r\n"
            << "Connection: close\r\n"
            << "\r\n";

        namespace ip = boost::asio::ip;
        using boost::asio::ip::tcp;
        namespace asio = boost::asio;

        tcp::resolver::query rq(server, "80");
        tcp::resolver resolver(*this);
        tcp::resolver::iterator iter = resolver.resolve(rq);

        //// Start a new Stackful Coroutine.
        ////boost::asio::spawn(io_service, [ q = std::move(rq), this ](auto yield) {});
        //boost::asio::spawn(io_service, [ rq, this ](boost::asio::yield_context yield) {
        //        do_resolve(rq, yield);
        //    });

        //boost::asio::async_connect(iter, ...);
        socket_.async_connect(*iter, [this](boost::system::error_code ec/*, tcp::resolver::iterator*/){
            if (ec) {
                std::cerr << "failed to connect: " << ec.message() << "\n";
                return;
            }
            this->query();
        });
    }

    int run() { return boost::asio::io_service::run(); }

private:
    struct transfer_at_least_one_line_t {
        typedef std::size_t result_type;
        template <typename Error> std::size_t operator()(const Error& err, std::size_t) {
            if (err)
                return 0;
            return 65536;
        }
        http_client* ptr_;
    };

    void query() {
        boost::asio::io_service& io_s = *this;
        boost::asio::spawn(io_s, [this](boost::asio::yield_context yield) {
                if (do_write_req(yield)
                        && do_read_status(yield)
                        && do_read_headers(yield))
                    do_content();
            });
    }

    bool do_write_req(boost::asio::yield_context yield) {
        boost::system::error_code ec;
        boost::asio::async_write(socket_, request_buf_, yield[ec]);
        if (ec) {
            std::cerr << "failed to do_write_req: " << ec.message() << "\n";
            return 0;
        }
        return 1;
    }

    bool do_read_status(boost::asio::yield_context yield) {
        boost::system::error_code ec;
        boost::asio::async_read_until(socket_, response_buf_, "\r\n", yield[ec]);
        if (ec) {
            std::cerr << "failed to do_read_status: " << ec.message() << "\n";
            return 0;
        }
        auto bufs = response_buf_.data();
        auto* beg = boost::asio::buffer_cast<const char*>(bufs);
        auto* end = beg + boost::asio::buffer_size(bufs);
        decltype(end) eol, p = beg;
        if ( (eol = std::find(p,end, '\n')) != end) {
            auto* e = eol++;
            while (e >= p && isspace(*e))
                --e;
            std::cout << boost::make_iterator_range(p,e+1) << "\n";
        }
        response_buf_.consume(eol - beg);
        return 1;
    }

    bool do_read_headers(boost::asio::yield_context yield) {
        boost::system::error_code ec;
        boost::asio::async_read_until(socket_, response_buf_, "\r\n\r\n", yield[ec]);
        if (ec) {
            std::cerr << "failed to do_read_headers: " << ec.message() << "\n";
            return 0;
        }
        size_t clen = size_t(-1);
        auto bufs = response_buf_.data();
        auto* beg = boost::asio::buffer_cast<const char*>(bufs);
        auto* end = beg + boost::asio::buffer_size(bufs);
        decltype(end) eol, p = beg;
        while ( (eol = std::find(p,end, '\n')) != end) {
            auto* e = eol++;
            while (e != p && isspace(*e))
                --e;
            auto line = boost::make_iterator_range(p,e+1);
            if (boost::starts_with(line, "Content-Length")) {
                boost::regex re_header_("^([^:]+):\\s+(.+)$");
                boost::cmatch m;
                if (boost::regex_match(p,e, m, re_header_)) {
                    clen = atoi(m[2].first);
            //std::clog << "Content-Length " << clen << "\n";
                }
            }
            std::cout << line << "\n";
            if (p == e)
                break;
            p = eol;
        }
        response_buf_.consume(eol - beg);
        return do_read_content(clen, yield);
    }

    bool do_read_content(size_t clen, boost::asio::yield_context yield)
    {
        if (response_buf_.size() < clen) {
            size_t n_bytes = clen - response_buf_.size();
            boost::system::error_code ec;
            if (clen == size_t(-1)) {
                boost::asio::async_read(socket_, response_buf_, boost::asio::transfer_all(), yield[ec]);
            } else {
                boost::asio::async_read(socket_, response_buf_.prepare(n_bytes), yield[ec]);
                if (!ec)
                    response_buf_.commit(n_bytes);
            }
            if (ec) {
                std::cerr << "failed to read_content: " << ec.message() << "\n";
                return 0;
            }
        }
        return 1;
    }

    void do_content() {
        std::ofstream ofs("content.html");
        ofs << &response_buf_;
    }

    //void do_resolve(const boost::asio::ip::tcp::resolver::query& query, boost::asio::yield_context yield) {
    //    boost::system::error_code ec;
    //    static boost::asio::ip::tcp::resolver resolver_(*this);
    //    auto iterator = resolver_.async_resolve(query, yield[ec]);
    //    if (ec) {
    //        std::cerr << "failed to resolve: " << ec.message() << "\n";
    //        return;
    //    }
    //    do_connect(iterator, yield);
    //}

    void do_connect(const boost::asio::ip::tcp::resolver::iterator& iterator, boost::asio::yield_context yield) {
        boost::system::error_code ec;
        boost::asio::async_connect(socket_, iterator, yield[ec]);
        if (ec) {
            std::cerr << "failed to do_connect: " << ec.message() << "\n";
            return;
        }

        do_write_req(yield);
    }

    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf request_buf_;
    boost::asio::streambuf response_buf_;
    // boost::asio::ip::tcp::resolver resolver_;
};

auto main(int argc, char* const argv[]) -> int {
  try {
    //boost::asio::io_service io_service;

    http_client hc(argv[1], argv[2]);
    //http_client hc(io_service, "myon.info", "/");

    hc.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}
