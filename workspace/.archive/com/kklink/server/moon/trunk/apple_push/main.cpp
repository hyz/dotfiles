#include <unistd.h>
#include "http.cpp"
#include "ap.cpp"

static const char* host_ = "gateway.sandbox.push.apple.com";
static const char* port_ = "2195"; 
static unsigned short listen_port_ = 9991;

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
            if (++i >= argc)
                exit(3);
            host_ = argv[i];
        }
        else if (opt == "-p")
        {
            if (++i >= argc)
                exit(4);
            port_ = argv[i];
        }
        else if (opt == "-l")
        {
            if (++i >= argc)
                exit(5);
            listen_port_ = atoi(argv[i]);
        }
        else if (opt == "--help")
        {
            std::cout << argv[0] << " [-d] [-c] [-h gateway.sandbox.push.apple.com|gateway.push.apple.com] [-l 9991] etc/moon.d";
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

    //if (dae || !isatty(STDIN_FILENO))
    logging::setup( (dae || !isatty(STDIN_FILENO)) ? 0 : &std::clog
            , LOG_PID|LOG_CONS, 0); //(LOG_PID | LOG_CONS, LOG_LOCAL1)
    //{}logging::setup(LOG_PID|LOG_CONS, 0, std::clog); //logging::syslog(LOG_PID | LOG_CONS, LOG_LOCAL1, std::clog);

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

        apple_push ap(io_service, resolv(io_service, host_, port_));
        relay_server<connection> s(io_service);

        s.start(tcp::endpoint(tcp::v4(), listen_port_), boost::bind(&apple_push::message, &ap, _1, _2));

        io_service.run();
    }
    catch (std::exception& e)
    {
        LOG_E << "exception: " << e.what();
    }

    return 0;
}


