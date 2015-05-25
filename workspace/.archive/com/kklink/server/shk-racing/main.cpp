#include <sys/types.h>
#include <linux/unistd.h>
#define gettid() syscall(__NR_gettid)  
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>

#include "myerror.h"
#include "log.h"
#include "json.h"
#include "dbc.h"

#include "globals.h"
#include "msgh.h"
// #include "statis.h"
#include "ioctx.h"
#include "server.h"
#include "racing.h"

namespace ip = boost::asio::ip;

bool cf_init(boost::property_tree::ptree& cf, boost::filesystem::path const &fp)
{
    try {
        boost::filesystem::ifstream confs(fp);
        if (!confs) {
            return 0;
        }
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(confs, pt);
        cf.swap(pt);
        return 1;
    } catch (...) {
        LOG << fp << "fail";
    }
    return 0;
}
boost::property_tree::ptree cf_child(boost::property_tree::ptree const& cf, std::string const& sec, boost::property_tree::ptree defs)
{
    return cf.get_child(sec, defs);
}
boost::property_tree::ptree cf_child(boost::property_tree::ptree const& cf, std::string const& sec)
{
    boost::property_tree::ptree defs;
    return cf.get_child(sec, defs);
}

struct instance : globals
{
    static instance* init(int argc, char* const argv[]);

private:
    static instance* inst0(boost::property_tree::ptree const& conf);

    static ip::tcp::endpoint make_endpoint(boost::property_tree::ptree const& conf, int defp)
    {
        std::string port = conf.get("port", boost::lexical_cast<std::string>(defp));
        std::string host = conf.get("host", std::string("0"));

        auto & io_s = globals::instance().io_service;

        ip::tcp::resolver resolver(io_s);
        ip::tcp::resolver::query query(host, port);
        ip::tcp::endpoint ep = *resolver.resolve(query);
        return ep;
        // ip::tcp::endpoint(ip::address::from_string(h), p)
    }

    typedef boost::property_tree::ptree ptree;

    instance(boost::property_tree::ptree const & conf)
        : globals()
        , m_(io_service, conf.get<std::string>("http.host"))
        , cs_(io_service, make_endpoint(cf_child(conf,"cs",ptree()), default_cs_port), msgh_player)
        , bs_(io_service, make_endpoint(cf_child(conf,"bs",ptree()), default_bs_port), msgh_gamebs)
        // , msgr_(io_service)
        // , service_mgr_( server_setting(conf) )
        // , message_mgr_check_post_online(io_service)
        // , message_mgr_check_expire(io_service)
        , cs_timer_alive_(io_service)
        // , cs_timer_tx_(io_service)
        , bs_check_expire_(io_service)
        , signals_(io_service, SIGHUP, SIGUSR1, SIGUSR2)
    {}

    void start();

private:
    // user_mgr user_mgr_;
    // message_mgr msgr_;
    Main m_;
    cs_server cs_;
    bs_server bs_;

    // service_mgr service_mgr_;
    // push_mgr push_mgr_;

    //boost::asio::deadline_timer message_mgr_check_post_online;
    //boost::asio::deadline_timer message_mgr_check_expire;
    boost::asio::deadline_timer cs_timer_alive_;
    //boost::asio::deadline_timer cs_timer_tx_;
    boost::asio::deadline_timer bs_check_expire_;

    boost::asio::signal_set signals_;
    static void signal(const boost::system::error_code& er, int sig);
    // ip::tcp::endpoint(boost::asio::ip::address::from_string(host_sms_), 9992)
} *G = 0;

extern void print_buildinfo(int);

static void save_pid(std::string const& fp) //(const char* sfmt)
{
    boost::filesystem::ofstream out(fp);
    if (out) {
        out << getpid();
    }
}

instance* instance::init(int ac, char* const av[])
{
    std::string cfp("/etc/shk-racing.conf");
    {
        namespace po = boost::program_options;

        std::string pidfile; //("/var/run/yxim-server.pid");

        po::options_description opt_desc("Options");
        opt_desc.add_options()
            ("help", "show this")
            ("daemon", "daemon") // ("daemon", po::value<int>(&dae)->implicit_value(1), "daemon")
            ("pid-file", po::value<std::string>(&pidfile)->default_value(""), "pid file")
            // ("cs", po::value<>(&cs)->default_value("20011"), "pid file")
            // ->required()
            ("conf", po::value<std::string>(&cfp)->default_value(cfp), "config file")
            ;
        po::positional_options_description pos_desc;
        pos_desc.add("conf", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(ac, av)
                .options(opt_desc)
                .positional(pos_desc)
                .run(), vm);
        if (vm.count("help"))
        {
            std::cout << boost::format("Usage:\n  %1% [Options]\n") % av[0]
                << opt_desc
                ;
            print_buildinfo(0);
            exit(0);
        }
        po::notify(vm);

        int nochdir = (vm.count("conf")>0);
        if (vm.count("daemon")) {
            if (daemon(nochdir, 0) != 0) {
                LOG << "daemon" << strerror(errno);
            }
            logging::syslog(LOG_PID|LOG_CONS, 0);
        }
        if (!pidfile.empty()) {
            //pidfile = std::string(nochdir ? "yxim-server.pid" : "/var/run/yxim-server.pid");
            save_pid(pidfile); //("/tmp/yx.pid");
        }

        LOG << "args" << cfp << nochdir << pidfile;
    }

    print_buildinfo(0);

    boost::property_tree::ptree conf;
    if (!cf_init(conf, cfp)) {
        // BOOST_THROW_EXCEPTION( myerror() << boost::errinfo_file_name(cfp) );
    }

    redis::init( conf.get_child("redis", conf) );
    // sql::dbc::init( sql::dbc::config(conf.get_child("mysql")) );
    // MyMongo::mongo_conn_mgr::init( conf.get_child("mongo") );

    auto inst = instance::inst0(conf);
    inst->start();

    return inst;
}

instance* instance::inst0(boost::property_tree::ptree const& conf)
{
    return (new instance(conf));
    //static instance g(conf);
    //return &g;
}

void instance::start()
{
    LOG << sizeof(*this);

    bs_.start();
    cs_.start();

    // stloop_start(message_mgr_check_post_online, 180, boost::bind(&message_mgr::check_post_online, &message_mgr::instance()) );
    // stloop_start(message_mgr_check_expire, 90, boost::bind(&message_mgr::check_expire, &message_mgr::instance()));

    stloop_start(bs_check_expire_, 30, boost::bind(&bs_server::check_alive, &bs_));
    stloop_start(cs_timer_alive_, 30, boost::bind(&cs_server::check_alive, &cs_));

    // stloop_start(cs_timer_tx_, 10, boost::bind(&cs_server::check_tx, &cs_));

    signals_.async_wait(&instance::signal);
}

void instance::signal(const boost::system::error_code& er, int sig)
{
    //debug_id_ = read_file("/tmp/debug_id_", true);
    //LOG << debug_id_;
    G->signals_.async_wait(&instance::signal);
}

static void stop_io_service(const boost::system::error_code& ec, int sig)
{
    LOG << "sig" << sig;
    G->io_service.stop();

    {
        pid_t pid = fork();
        if (pid == 0) // child
        {
            for (int f=0; f<1024; ++f) {
                close(f);
            }
            pid_t ppid = 0;
            for (int i=0; i < 6; ++i) {
                if ( (ppid = getppid()) > 1) {
                    sleep(1);
                }
            }
            if ( (ppid = getppid()) > 1) {
                kill(ppid, SIGKILL);
            }
            exit(0);
        }
    }
}

void tf_hourly()
{
    // statis::instance().flush(0);
}

int main(int argc, char* const argv[])
{
    try {
        boost::scoped_ptr<instance> inst(G = instance::init(argc, argv));

        boost::asio::signal_set sigstop(G->io_service, SIGINT, SIGTERM, SIGQUIT);
        sigstop.async_wait(&stop_io_service);

        // statis stat;

        boost::asio::deadline_timer hourly(G->io_service);
        stloop_start(hourly, 60*5, &tf_hourly); // 60*60

        LOG << "tid" << gettid();

        G->io_service.run();

        tf_hourly();
        inst.reset();
        LOG << "bye";

    } catch (myerror const& ex) {
        LOG << ex;
    } catch (std::exception const& ex) {
        LOG << "=except:" << ex.what();
    }

    return 0;
}

// ip::tcp::endpoint(boost::asio::ip::address::from_string(host_sms_), 9992)

//void send_notification(UInt uid, UInt msgid, std::string const& alert, time_t tp_exp)
//{
//    // push_mgr::instance().push(uid, msgid, tp_exp, alert);
//}

