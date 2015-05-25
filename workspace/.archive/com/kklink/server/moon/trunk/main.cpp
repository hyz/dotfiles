#include "myconfig.h"
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <unistd.h>
#include <sys/types.h>

#include "async.h"
#include "client.h"
#include "service.h"
#include "dbc.h"
#include "log.h"
#include "mail.h"
#include "chat.h"
#include "ios_push.h"
#include "asclient.h"
#include "smsbuf.h"
#include "iapbuf.h"
#include "upbuf.h"
#include "bars.h"

using namespace std;
using namespace boost;
using boost::asio::ip::tcp;

time_t tp_startup_;
static std::string host_sms_ = "58.67.160.245";
static std::string port_ = "9000";
static std::string port_im_ = "9900";
static std::string addr_ = "0";

static std::string apple_push_addr_ = "127.0.0.1";

extern std::string debug_id_;
extern std::string read_file(std::string const & fnp);

static void lisig(asio::signal_set *sigs, const boost::system::error_code& er, int signal)
{
    debug_id_ = read_file("/tmp/debug_id_", true);
    LOG_I << debug_id_;
    sigs->async_wait(boost::bind(&lisig,sigs,_1,_2));
}

static void stop_io_service(boost::asio::io_service * ios, const boost::system::error_code& er, int signal)
{
    LOG_I << "sig " << signal;
    ios->stop();

    {
        pid_t pid = fork();
        if (pid == 0)
        {
            pid_t ppid = getppid();
            if (ppid > 1)
            {
                sleep(1);
                if (ppid == getppid())
                    kill(ppid, SIGKILL);
            }
            exit(0);
        }
    }
}

template <typename T> static boost::optional<T> sfaccess(T x)
{
    if (access(x, R_OK))
        return boost::optional<T>(x);
    return boost::optional<T>();
}
template <typename T, typename ...A> static boost::optional<T> sfaccess(T x, A... a)
{
    if (access(x, R_OK))
        return boost::optional<T>(x);
    return sfaccess(a...);
}

static const char* args(int argc, char* argv[])
{
    const char* conf = 0;
    int no_changedir = 1;
    bool dae = false;
    bool help = 0;
    std::ostream * slog = &std::clog;

    for (int i = 1; i < argc; ++i)
    {
        std::string opt = argv[i];
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
            help = 1;
            slog = &std::cout;
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

    extern void print_buildinfo();
    print_buildinfo();

    char cwd[64];
    getcwd(cwd, sizeof(cwd));
    LOG_I << boost::format("Config %1%, %2% %3% %4%") % conf % dae % no_changedir % cwd;
    if (help)
    {
        LOG_I << ".";
        LOG_I << argv[0] << " [-d] [-c] etc/moon.conf";
        exit(0);
    }
    // char const* v[] = { conf1, conf2, "." };
    return conf;
}

boost::property_tree::ptree getcfg(std::string cfg, const char* sec)
{
    boost::property_tree::ptree empty, ini;

    std::ifstream ifs(cfg.c_str());
    boost::property_tree::ini_parser::read_ini(ifs, ini);

    return ini.get_child(sec, empty);
}

// struct network_initializer {
//     network_initializer(const boost::property_tree::ptree & ini)
//     {
//         addr_ = ini.get<string>("listen", addr_);
//         port_ = ini.get<string>("port", port_);
//         port_im_ = ini.get<string>("portim", port_im_);
//         LOG_I << format("Listen %s %s,%s") % addr_ % port_ % port_im_;
//     }
// };

struct initializer : boost::noncopyable
{
    sql::initializer dbc_;

    mailer::initializer mailer_;

    Client::initializer client_;
    chatmgr::initializer chatmgr_;
    Service::initializer service_;

    initializer(std::string cfg, boost::asio::io_service& ios)
        : dbc_(getcfg(cfg, "database"))
        , mailer_(getcfg(cfg, "mail"))
        , client_(getcfg(cfg, "moon"), ios)
        , service_(getcfg(cfg, "moon"))
    {
        boost::property_tree::ptree ini = getcfg(cfg, "network");
        addr_ = ini.get<string>("listen", addr_);
        port_ = ini.get<string>("port", port_);
        port_im_ = ini.get<string>("portim", port_im_);
        host_sms_ = ini.get<string>("host_sms", host_sms_);
    }
};

void save_pid(const char* sfmt, std::string const& k)
{
    boost::filesystem::ofstream out(str(boost::format(sfmt) % k));
    out << getpid();
}

boost::asio::ip::tcp::endpoint resolv(boost::asio::io_service & io_service, const std::string& h, const std::string& p)
{
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(h, p);
    boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
    return *i;
}

int main(int argc, char* argv[])
{
    tp_startup_ = time(0);
    const char *cf = args(argc, argv);
    if (!cf)
        cf = sfaccess("etc/moon.conf", "etc/moon.d/lan.conf").get_value_or(".");

    boost::asio::io_service io_service;
    initializer initializer(cf, io_service);

    try
    {
        // 0
        asio::signal_set sigstop(io_service, SIGINT, SIGTERM
#               if defined(SIGQUIT)
                , SIGQUIT
#               endif
                );
        // sigstop.async_wait(bind(&asio::io_service::stop, &io_service));
        sigstop.async_wait(boost::bind(&stop_io_service, &io_service, _1, _2));

        asio::signal_set sigs(io_service, SIGHUP, SIGUSR1, SIGUSR2);
        sigs.async_wait(boost::bind(&lisig, &sigs, _1,_2)); // sigs.async_wait(&lisig);

        // 1
        net::writer writer; //(/*io_service*/);

        // 2
        ServiceEntry serv(bind(&net::writer::associate, &writer, _1));

        // 3
        typedef net::reader<http::reader> http_reader;
        http_reader reader(io_service, bind(&ServiceEntry::work, &serv, _1, _2));

        // 4
        net::acceptor acceptor(io_service);
        acceptor.listen(addr_.c_str(), port_.c_str(), bind(&http_reader::accept, &reader, _1));

        // 5 more
        typedef net::reader<imessage::reader> socket_reader;
        socket_reader imreader(io_service, bind(&ServiceEntry::imessage, &serv, _1, _2));
        //
        net::acceptor imacceptor(io_service);
        imacceptor.listen(addr_.c_str(), port_im_.c_str(), bind(&socket_reader::accept, &imreader, _1));

#if 1
        apple_push_c sandbox(io_service, tcp::endpoint(tcp::v4(), 9991));
        apple_push_c::sandbox(&sandbox);
        // sandbox.start();
        apple_push_c ap(io_service, tcp::endpoint(tcp::v4(), 9993));
        apple_push_c::instance(&ap);
        ap.start();

        asclient<smsbuf> sms(io_service, tcp::endpoint(boost::asio::ip::address::from_string(host_sms_), 9992));
        // asclient<smsbuf> sms(io_service, tcp::endpoint(tcp::v4(), 9992));

        asclient<smsbuf>::instance(&sms);
        sms.start();

        asclient<iapbuf> iap(io_service, tcp::endpoint(tcp::v4(), 9998));
        asclient<iapbuf>::instance(&iap);
        iap.start();

        asclient<upbuf> up(io_service, tcp::endpoint(tcp::v4(), 9080));
        up.start();
        up.send(1);
#endif

        boost::scoped_ptr<bars_mgr> bmgr(new bars_mgr(io_service) );
        bmgr->inst();

        save_pid("/tmp/pid.yx.%1%", port_);

        LOG_I << "start";
        io_service.run();
        LOG_I << "stopped";
    }
    catch (const std::exception& e)
    {
        LOG_E << "Exception: " << e.what();
        return 2;
    }

    return 0;
}

