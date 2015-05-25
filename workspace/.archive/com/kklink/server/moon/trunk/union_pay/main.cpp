#include <unistd.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "http.cpp"
#include "receipt.cpp"
#include "dbc.h"

static const char* args(int argc, char* argv[], const char* conf)
{
    int no_changedir = 1;
    bool dae = false;
    std::ostream * slog = &std::clog;

    for (int i = 1; i < argc; ++i)
    {
        std::string opt = std::string(argv[i]);
        if (opt == "-d")
        {
            dae = true;
        }
        else if (opt == "-c")
        {
            no_changedir = 0;
            chdir("/");
        }
        else if (opt == "--help")
        {
            std::cout << argv[0] << " [-d] [-c] etc/moon.d";
            slog = &std::cout;
            exit(0);
        }
        else if ('-' != *argv[i])
        {
            conf = argv[i];
        }
    }

    if (dae)
    {
        if (daemon(no_changedir, 0) != 0)
        {
            std::cerr << "daemon " << strerror(errno);
        }
    }

    logging::setup( (dae || !isatty(STDIN_FILENO)) ? 0 : slog
            , LOG_PID|LOG_CONS, 0); //(LOG_PID | LOG_CONS, LOG_LOCAL1)

    char cwd[64];
    getcwd(cwd, sizeof(cwd));
    LOG_I << boost::format("Config %1%, %2% %3% %4%") % conf % dae % no_changedir % cwd;

    return conf;
}

boost::property_tree::ptree getcfg(std::string cfg, const char* sec)
{
    boost::property_tree::ptree empty, ini;
    std::ifstream ifs(cfg.c_str());
    boost::property_tree::ini_parser::read_ini(ifs, ini);
    return ini.get_child(sec, empty);
}

int main(int ac, char* av[])
{
    const char *cfg = args(ac, av, "etc/moon.conf");
    sql::initializer inisql( getcfg(cfg, "database") );
    try
    {
        boost::asio::io_service io_service;

        apple_push ap(io_service);
                //, resolv(io_service, "buy.itunes.apple.com", "https")
                //, resolv(io_service, "sandbox.itunes.apple.com", "https")
        http_server server(io_service);

        // server.start(resolv(io_service, "0", "9991"), boost::bind(&apple_push::message, &ap, _1, _2));
        server.start(tcp::endpoint(tcp::v4(), 9080)
                , boost::bind(&apple_push::message, &ap, _1, _2, _3));

        io_service.run();
    }
    catch (std::exception& e)
    {
        LOG_E << "exception: " << e.what();
    }

    return 0;
}


