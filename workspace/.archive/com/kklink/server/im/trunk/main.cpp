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
#include <boost/thread/thread.hpp>
#include <boost/thread/scoped_thread.hpp>

#include "myerror.h"
#include "log.h"
#include "json.h"
#include "dbc.h"

#include "globals.h"
#include "user.h"
#include "message.h"
#include "msgh.h"
#include "ioctx.h"
#include "async.h"
#include "service.h"
#include "push.h"
#include "initialze.h"
#include "statis.h"
#include "apple_push.h"
#include "chatroom.h"

namespace ip = boost::asio::ip;

volatile bool bexit_ = 0;

boost::property_tree::ptree getconf(boost::filesystem::ifstream &cf, const char* sec)
{
    boost::property_tree::ptree conf;
    boost::property_tree::ini_parser::read_ini(cf, conf);
    if (sec) {
        // boost::property_tree::ptree empty;
        return conf.get_child(sec);
    }
    return conf;
}

struct instance : globals
{
    static instance* init(int argc, char* const argv[]);

    ims_server* get_ims() { return &ims_; }
private:
    static instance* inst0(boost::property_tree::ptree const& conf);

    static ip::tcp::endpoint endpoint(boost::property_tree::ptree const& conf, int defp)
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

    instance(boost::property_tree::ptree const & conf)
        : globals()
        , msgr_(io_service)
        , ims_(io_service, endpoint(conf.get_child("ims"), default_ims_port), msgh_ims_default)
        , imbs_(io_service, endpoint(conf.get_child("bs"), default_bs_port ), msgh_bs_default)
        , service_mgr_( server_setting(conf) )
        , message_mgr_check_post_online(io_service)
        , message_mgr_check_expire(io_service)
        , ims_timer_alive_(io_service)
        , ims_timer_tx_(io_service)
        , bs_check_expire_(io_service)
        , signals_(io_service, SIGHUP, SIGUSR1, SIGUSR2)
    {
        boost::property_tree::ptree tmp;
        push_mgr_.init( io_service, conf.get_child("push", tmp));

        msgh_bs_set_authcode( conf.get("bs.authcode", std::string()) );
        ioctx_set_ims(&ims_);
    }

    void start();

private:
    user_mgr user_mgr_;

    message_mgr msgr_;
    ims_server ims_;
    imbs_server imbs_;

    service_mgr service_mgr_;
    push_mgr push_mgr_;
    virtual_user_mgr virtuals_mgr_;
    PuppetAgent puppet_agent_mgr_;
    chatroom_mgr chatroom_mgr_;

    boost::asio::deadline_timer message_mgr_check_post_online;
    boost::asio::deadline_timer message_mgr_check_expire;
    boost::asio::deadline_timer ims_timer_alive_;
    boost::asio::deadline_timer ims_timer_tx_;
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
    std::string cf("/etc/yxim.conf");
    {
        namespace po = boost::program_options;

        std::string pidfile; //("/var/run/yxim-server.pid");

        po::options_description opt_desc("Options");
        opt_desc.add_options()
            ("help", "show this")
            ("daemon", "daemon") // ("daemon", po::value<int>(&dae)->implicit_value(1), "daemon")
            ("pid-file", po::value<std::string>(&pidfile)->default_value(""), "pid file")
            // ->required()
            ("conf", po::value<std::string>(&cf)->default_value(cf), "config file")
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

        LOG << "args" << cf << nochdir << pidfile;
    }

    print_buildinfo(0);

    boost::filesystem::ifstream confs(cf);
    if (!confs)
        BOOST_THROW_EXCEPTION( myerror() << boost::errinfo_file_name(cf) );

    boost::property_tree::ptree conf = getconf(confs,0);

    redis::init( conf.get_child("redis") );
    sql::dbc::init( sql::dbc::config(conf.get_child("mysql")) );
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

    imbs_.start();
    ims_.start();

    stloop_start(message_mgr_check_post_online, 180
            , boost::bind(&message_mgr::check_post_online, &message_mgr::instance()) );

    stloop_start(message_mgr_check_expire, 90
            , boost::bind(&message_mgr::check_expire, &message_mgr::instance()));

    stloop_start(bs_check_expire_, 30, boost::bind(&imbs_server::check_alive, &imbs_));

    stloop_start(ims_timer_alive_, 30, boost::bind(&ims_server::check_alive, &ims_));

    stloop_start(ims_timer_tx_, 10, boost::bind(&ims_server::check_tx, &ims_));

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
    statis::instance().flush(0);
}

//static UInt max_msgid()
//{
//    sql::datas datas("SELECT MAX(id) FROM message");
//    if (sql::datas::row_type row = datas.next()) {
//        return std::atoi(row[0]);
//    }
//    return 0;
//}

void thread_mysql()
{
//    while (!bexit_) {
//        auto reply = redis::command("LRANGE", "mq/body", -300, -1);
//        if (!reply || reply->type != REDIS_REPLY_ARRAY) {
//            LOG_ERR << "redis myq error" << bool(reply);
//            nslp = 10;
//            break;
//        }
//        if (reply->elements <= 0) {
//            nslp = 2;
//            break;
//        }
//
//        // AUTO_CPU_TIMER("sql:message");
//        std::vector<std::string> vals;
//        for (size_t i = reply->elements; i > 0; i--) {
//            vals.push_back(reply->element[i-1]->str);
//        }
//        if (!vals.empty()) {
//            sql::exec();
//        }
//        redis::command("LTRIM", "myq", 0, -int(reply->elements+1));
//        LOG << reply->elements;
//    }
//    if (!bexit_ && nslp > 0) {
//        while (!bexit_ && nslp-- > 0) {
//            this_thread::sleep_for(chrono::seconds(1));
//        }
//        goto Pos_entry_;
//    }
//}{
    int nslp;
Pos_entry_:
    nslp = 0;
    while (!bexit_) {
        auto reply = redis::command("BRPOP", "myq", 2);
        if (!reply) {
            LOG << "redis BRPOP myq error: reply";
            nslp = 10;
            break;
        }
        if (reply->type == REDIS_REPLY_NIL) {
            nslp = 2;
            break;
        }
        if (reply->type != REDIS_REPLY_ARRAY) {
            LOG_ERR << "redis BRPOP myq !REDIS_REPLY_ARRAY";
            nslp = 15;
            break;
        }
        if (reply->elements != 2 || !reply->element[1]->str) {
            nslp = 2;
            break;
        }
        boost::system::error_code ec;
        sql::exec(ec, reply->element[1]->str);
    }
    if (!bexit_ && nslp > 0) {
        do {
            boost::this_thread::sleep_for( boost::chrono::seconds(1) );
        } while (!bexit_ && --nslp > 0);
        goto Pos_entry_;
    }
    LOG << "exit" << bexit_;
}

int main(int argc, char* const argv[])
{
    try {
        boost::scoped_ptr<instance> inst(G = instance::init(argc, argv));

        boost::asio::signal_set sigstop(G->io_service, SIGINT, SIGTERM, SIGQUIT);
        sigstop.async_wait(&stop_io_service);

        statis stat;

        boost::asio::deadline_timer hourly(G->io_service);
        stloop_start(hourly, 60*5, &tf_hourly); // 60*60

        LOG << "tid" << gettid() << getenv("PATH");

        boost::scoped_thread<> thx( &thread_mysql );
        G->io_service.run();
        bexit_ = 1;
        thx.join();

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

