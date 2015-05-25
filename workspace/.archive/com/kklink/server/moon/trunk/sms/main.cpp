#include <unistd.h>
#include "http.cpp"
#include "sms.cpp"

static std::string args(int argc, char* argv[], std::string conf)
{
    int no_changedir = 1;
    bool dae = false;

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
        else if (opt == "-h")
        {
            std::cout << argv[0] << " [-d] [-c] etc/moon.d";
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
    // if (dae || !isatty(STDIN_FILENO))
    // {
    //     logging::syslog(LOG_PID|LOG_CONS, 0, std::clog); //logging::syslog(LOG_PID | LOG_CONS, LOG_LOCAL1, std::clog);
    // }
    logging::setup( (dae || !isatty(STDIN_FILENO)) ? 0 : &std::clog
            , LOG_PID|LOG_CONS, 0); //(LOG_PID | LOG_CONS, LOG_LOCAL1)

    char cwd[64];
    getcwd(cwd, sizeof(cwd));
    LOG_I << boost::format("Config %1%, %2% %3% %4%") % conf % dae % no_changedir % cwd;

    return conf;
}

int main(int ac, char* av[])
{
    certs_dir_ = args(ac, av, certs_dir_);

    try
    {
        boost::asio::io_service io_service;

        apple_push ap(io_service, resolv(io_service, APPLE_HOST, APPLE_PORT));
        http_server server(io_service);

        // server.start(resolv(io_service, "0", "9991"), boost::bind(&apple_push::message, &ap, _1, _2));
        server.start(tcp::endpoint(tcp::v4(), 9992), boost::bind(&apple_push::message, &ap, _1, _2));

        io_service.run();
    }
    catch (std::exception& e)
    {
        LOG_E << "exception: " << e.what();
    }

    return 0;
}


