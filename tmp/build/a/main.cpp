//
// Copyright (c) 2013-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include "http_async_server.hpp"
#include "log.hpp"

struct Main : beast::http::http_async_server, boost::noncopyable
{
    using server_type = beast::http::http_async_server;

    struct Args {
        using address_type = boost::asio::ip::address;
        std::string root;
        std::uint16_t port;
        address_type ip; // std::string ip;
        std::size_t threads;
        // bool sync = vm.count("sync") > 0;

        Args(int ac, char* const av[]) {
            namespace po = boost::program_options;
            po::options_description desc("Options");

            desc.add_options()
                ("root,r", po::value<std::string>()->default_value(".")
                         , "Set the root directory for serving files")
                ("port,p", po::value<std::uint16_t>()->default_value(8080)
                         , "Set the port number for the server")
                ("ip"    , po::value<std::string>()->default_value("0.0.0.0")
                         , "Set the IP address to bind to, \"0.0.0.0\" for all")
                // ("sync,s",    "Launch a synchronous server")
                ("threads", po::value<std::size_t>()->default_value(4) , "Set the number of threads to use")
                ;
            po::variables_map vm;
            po::store(po::parse_command_line(ac, av, desc), vm);

            root =  vm["root"].as<std::string>();
            port = vm["port"].as<std::uint16_t>();
            ip = address_type::from_string( vm["ip"].as<std::string>() );
            threads = vm["threads"].as<std::size_t>();
        }
    };

    using endpoint_type = boost::asio::ip::tcp::endpoint;

    std::vector<std::thread> thread_;

    boost::asio::io_service& io_service() { return *this; }

    Main(Args const& a)
        : server_type(io_service(), endpoint_type{ a.ip, a.port }, a.root)
        , thread_(a.threads)
    {}
    void setup(Args const&) {}

    void teardown() {
        server_type::teardown(io_service());
        io_service().stop();
    }

    void run(int, char*const[])
    {
        /// setup(Args(ac, av));

        boost::asio::signal_set signals(io_service(), SIGINT, SIGTERM);
        signals.async_wait([this](boost::system::error_code const&, int) {
                this->teardown();
            });

        for (auto& t : thread_) {
            t = std::thread([this]{ io_service().run(); });
        }
        LOG(INFO) << thread_.size() << "threads";

        for (auto& t : thread_) {
            t.join();
            LOG(ERROR) << "joined" << int(&t - &thread_.front());
        }
        LOG(ERROR) << "bye.";
    }
};

int main(int ac, char* const av[])
{
    logging_init logging(ac, av);
    logging.samples();
    //logging.fatal();

    Main a( Main::Args(ac,av) );
    a.run(ac,av);

    return 0;
}

