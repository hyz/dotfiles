#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>
#include <string>
#include <list>
//#include <boost/thread.hpp>
//#include <boost/pool/object_pool.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
//#include <boost/container/flat_map.hpp>
//#include <boost/container/static_vector.hpp>
//#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/hashed_index.hpp>
//#include <boost/multi_index/member.hpp>
//namespace multi_index = boost::multi_index;
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
//#include <boost/archive/binary_oarchive.hpp>
//#define NO_HEADER (boost::archive::no_header)
//#include <boost/array.hpp>
//#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
//#include <boost/exception/errinfo_at_line.hpp>
//#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/iterator_range.hpp>
//#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/format.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
#include "singleton.h"
#include "log.h"
#include "myerror.h"
//#include "json.h"
//#include "dbc.h"
#include <iostream>

//#include "message.h"

typedef unsigned int UInt;

namespace ip = boost::asio::ip;
using boost::asio::ip::tcp;

namespace posix = boost::asio::posix;

namespace error {
    struct fatal : myerror { using myerror::myerror; };
    struct signal : myerror { using myerror::myerror; };
    struct not_found : myerror { using myerror::myerror; };
    struct size : myerror { using myerror::myerror; };
    struct io : myerror { using myerror::myerror; };
    struct output_not_exist : myerror { using myerror::myerror; };

    // typedef boost::error_info<struct I,size_t> info_int;
    // typedef boost::error_info<struct S,std::string> info_str;
    // typedef boost::error_info<struct E,boost::system::error_code> info_ec;
    template <typename T> inline boost::error_info<T,T> info(T const& x) { return boost::error_info<T,T>(x); }
    inline boost::error_info<char const*,char const*> info(char const* x) {
        return boost::error_info<char const*,char const*>(x);
    }

} /// namespace error

template <typename T,int N>
struct auto_close_fdvec : boost::container::static_vector<T,N>
                    , boost::noncopyable
{
    ~auto_close_fdvec() {
        BOOST_FOREACH(int fd, *this) {
            if (fd > 0)
                close(fd);
        }
    }

    template <typename R> R operator()(R&& a)
    {
        BOOST_FOREACH(T fd, a)
            this->push_back(fd);
        return boost::move(a);
    }
    T operator()(T fd)
    {
        this->push_back(fd);
        return fd;
    }
};

std::string basename(std::string pf)
{
    auto x = pf.find_last_of('/');
    if (x != std::string::npos) {
        pf.erase(0, x+1);
    }
    return boost::move(pf);
}

struct Argv : std::vector<std::string>
{
    template <typename ...Params>
    Argv(std::string const& p, Params... args) : path_(p)
        { append(args...); }

    Argv(std::string const& p) : path_(p) {
        if (!p.empty())
            append(basename(p));
    }

    int execute(int iofd[2]) const;

    std::string const& path() const { return path_; }
    // bool empty() const { return path_.empty(); }

private:
    void append(std::string const& a)
    {
        push_back(a);
        BOOST_ASSERT(size() < 16);
    }
    template <typename ...Params>
    void append(std::string const& a, Params&... args)
    {
        push_back(a);
        append(args...);
    }

    std::string path_;

    friend std::ostream& operator<<(std::ostream& out, Argv const& av) {
        std::vector<std::string> const& cv = av;
        return out << av.path() <<" "<< cv;
    }
};

int Argv::execute(int iofd[2]) const
{
    BOOST_ASSERT(size() > 0 && size() < 16);
    typedef boost::container::static_vector<char*,16> Aps;
    Aps av;

    av.push_back( const_cast<char*>(path_.c_str()) );
    BOOST_FOREACH(std::string const& x, *this) {
        av.push_back( const_cast<char*>(x.c_str()) );
    }
    av.push_back(0);

    int mfd = 100;
    if (iofd[1] != STDIN_FILENO) {
        dup2(iofd[1], STDIN_FILENO);
        mfd = std::max(mfd, iofd[1]);
    }
    if (iofd[0] != STDOUT_FILENO) {
        dup2(iofd[0], STDOUT_FILENO);
        mfd = std::max(mfd, iofd[0]);
    }
    for (int fd=3; fd < mfd; fd++) {
        if (close(fd) == 0)
            mfd = std::max(fd+100, mfd);
    }

    execvp(av[0], &av[1]);
    int errN = errno;
    if (errN == EACCES) {
        // LOG << "exec EACCES" << errN << strerror(errN) << av[0];
        std::swap(av[0], av[1]);
        execv("/bin/sh", &av[0]);
    }

    return errN;
}

//static pid_t Forkex(int iofd[2], Argv const& av)
//{
//    pid_t pid = fork();
//    if (pid < 0) {
//        char* es = strerror(errno);
//        LOG << "fork" << pid << es;
//        BOOST_THROW_EXCEPTION(error::fatal() << error::info(es));
//    }
//
//    LOG << av << iofd[0] << iofd[1]; // << errN;
//
//    if (pid == 0) // child
//    {
//        int errN;
//        auto bname = basename(av.path());
//        if (bname == "cat") {
//            Argv av2 ( av.path(), av[0] ); // av.resize(1);
//            errN = av2.execute(iofd); // errN = exec_av(iofd, v);
//        } else {
//            errN = av.execute(iofd); // errN = exec_av(iofd, v);
//        }
//        kill(getppid(), SIGUSR1);
//        LOG << "exec" << strerror(errN) << av;
//        exit(errN);
//    }
//
//    return pid;
//}
//
//static int open_tty_output() { return open("/proc/self/fd/0", O_NOCTTY, O_WRONLY); } //readlink("/proc/self/fd/0");

boost::array<int,2> Pipe()
{
    boost::array<int,2> v;
    if (pipe(&v[0]) < 0) {
        int errN = errno;
        char* es = strerror(errN);
        BOOST_THROW_EXCEPTION(error::fatal() << error::info(errN) << error::info(es));
    }
    return v;
}

static void save_pid(std::string const& fp) //(const char* sfmt)
{
    boost::filesystem::ofstream out(fp);
    if (out) {
        out << getpid();
    }
}

static boost::property_tree::ptree make_pt(boost::filesystem::path const &fp)
{
    boost::filesystem::ifstream cfs(fp);
    if (!cfs) {
        BOOST_THROW_EXCEPTION(error::io() << error::info(fp));
    }
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(cfs, pt);
    return boost::move(pt);
}

//#define DECL_TAG(X) struct X{ friend std::ostream& operator<<(std::ostream& out,X const& x) {return out<< #X;} }
//DECL_TAG(Soc); DECL_TAG(Dec); DECL_TAG(Enc); DECL_TAG(App);

struct fifo_stream_descriptor : posix::stream_descriptor, boost::noncopyable
{
    std::string fp_;

    ~fifo_stream_descriptor()
    {
        if (!fp_.empty()) {
            remove(fp_.c_str());
        }
    }

    fifo_stream_descriptor(boost::asio::io_service& io_s, std::string const& fp)
        : posix::stream_descriptor(io_s)
        , fp_(fp)
    {
        if (!fp_.empty()) {
            remove(fp_.c_str());
            if (mkfifo(fp_.c_str(), 0660) < 0) {
                LOG << "mkfifo" << fp_ << strerror(errno);
                BOOST_THROW_EXCEPTION(error::fatal());
            }

            int fd = open(fp_.c_str(), O_RDONLY|O_NDELAY);
            if (fd < 0) {
                LOG << "open" << fp_ << strerror(errno);
                BOOST_THROW_EXCEPTION(error::fatal());
            }
            this->assign(fd);
        }
    }
};

inline boost::asio::mutable_buffers_1 as_buffer(uint32_t& n)
{
    return boost::asio::mutable_buffers_1(static_cast<void*>(&n), sizeof(uint32_t));
}
inline boost::asio::const_buffers_1 as_buffer(uint32_t const& n)
{
    return boost::asio::const_buffers_1(static_cast<void const*>(&n), sizeof(uint32_t));
}

inline bool _Success(boost::system::error_code const& ec, int line, char const* s)
{
    if (ec) {
        LOG << ec << ec.message() <<"#"<< line << s;
    }
    return (!ec);
}

namespace msm = boost::msm;
namespace mpl = boost::mpl;

template <typename Derived>
struct State : msm::front::state_machine_def<State<Derived>>
{
#define _SUCCESS(ec) _Success(ec, __LINE__,__FUNCTION__)
#define STEP_OK(m, ec) m.step_ok(m,ec,__LINE__,__FUNCTION__)

    struct Ev_idle_end {};
    struct Ev_connect_ok {};
    struct Ev_write_ok {};
    struct Ev_size_ok {};
    struct Ev_content_ok {};
    // struct Ev_error {};

    struct Idle : public msm::front::state<> {
        template <class Ev, class M> void on_entry(Ev const&, M& m)
        {
            time_t tp_chk = m.last_ok_time_ + 45;
            time_t ct = time(0);

            if (tp_chk < ct && ct - tp_chk > 10) {
                m.restart_server(m, __LINE__);
                return;
            }

            int secs = 3;
            if (tp_chk > ct) {
                secs = std::min(45, int(tp_chk - ct));
            }
            LOG << "Idle" << secs;
            m.timer_.expires_from_now(boost::posix_time::seconds(secs));
            m.timer_.async_wait([&m](boost::system::error_code ec) { if(_SUCCESS(ec)){ m.idle_end(m); } });
        }
        template <class Ev, class M> void on_exit(Ev const&, M&) {}
    };
    struct Connecting : public msm::front::state<> {
        template <class Ev, class M> void on_entry(Ev const&, M& m)
        {
        //LOG << "Connecting:on_entry" << time(0);
            boost::system::error_code ec;
            m.socket_.close(ec);
            m.socket_.async_connect(m.endpx_, [&m](boost::system::error_code ec){
                    if (STEP_OK(m,ec)){ m.process_event(Ev_connect_ok()); }
                });
        }
        template <class Ev, class M> void on_exit(Ev const&, M&) {}
    };
    struct Writing : public msm::front::state<> {
        template <class Ev, class M> void on_entry(Ev const&, M& m)
        {
        //LOG << "Writing:on_entry" << time(0);
            boost::asio::async_write(m.socket_, m.wbuf_.const_buffers(),
                    [&m](boost::system::error_code ec, size_t) {
                        if (STEP_OK(m,ec)) { m.process_event(Ev_write_ok()); }
                    });
        }
        template <class Ev, class M> void on_exit(Ev const&, M&) {}
    };
    struct Reading_size : public msm::front::state<> {
        template <class Ev, class M> void on_entry(Ev const&, M& m)
        {
        //LOG << "Reading_size:on_entry" << time(0);
            boost::asio::async_read(m.socket_, as_buffer(m.rbuf.len),
                    [&m](boost::system::error_code ec, size_t) {
                        if (STEP_OK(m,ec)) {
                            m.rbuf.len = ntohl(m.rbuf.len);
                            m.rbuf.resize(m.rbuf.len);
                            m.process_event(Ev_size_ok());
                        }
                    });
        }
        template <class Ev, class M> void on_exit(Ev const&, M&) {}
    };
    struct Reading_content : public msm::front::state<> {
        template <class Ev, class M> void on_entry(Ev const&, M& m)
        {
        //LOG << "Reading_content:on_entry" << time(0);
            boost::asio::async_read(m.socket_, boost::asio::buffer(m.rbuf),
                    [&m](boost::system::error_code ec, size_t) {
                        if (STEP_OK(m,ec)) {
                            m.last_ok_time_ = time(0);
                            m.process_event(Ev_content_ok());
                        }
                        m.socket_.close(ec);
                    });
        }
        template <class Ev, class M> void on_exit(Ev const&, M&) {}
    };
    struct Getpid {
        template <class Ev, class M, class S, class T>
        void operator()(Ev const& ev, M& m, S&, T&)
        {
            auto r = boost::make_iterator_range(std::find(m.rbuf.begin(), m.rbuf.end(), ':'), m.rbuf.end());
            if (!boost::empty(r)) {
                r = boost::algorithm::find_token(r, boost::algorithm::is_digit(), boost::algorithm::token_compress_on);
                if (!boost::empty(r)) {
                    LOG << m.pid_ <<":="<< r;
                    m.pid_ = boost::lexical_cast<int>(r);
                    return;
                }
            }
            LOG << "Getpid fail" << m.rbuf;
        }
    };

    typedef Idle initial_state; //typedef mpl::vector<Idle, > initial_state;
    template <typename... T> using Row = msm::front::Row<T...> ;
    typedef msm::front::none None;

    struct transition_table : mpl::vector<
        Row< Idle            , Ev_idle_end   , Connecting      , None   , None >
      , Row< Connecting      , Ev_connect_ok , Writing         , None   , None >
      , Row< Writing         , Ev_write_ok   , Reading_size    , None   , None >
      , Row< Reading_size    , Ev_size_ok    , Reading_content , None   , None >
      , Row< Reading_content , Ev_content_ok , Idle            , Getpid , None >
    > {};

    State(boost::asio::io_service* io_s, tcp::endpoint px, std::string sc)
        : timer_(*io_s)
        , socket_(*io_s)
        , endpx_(px)
        , script_(sc)
        , wbuf_( "{\"cmd\":73}" )
    {
        LOG << endpx_;
        last_ok_time_ = time(0);
    }
    boost::asio::io_service& get_io_service() { return timer_.get_io_service(); }

    boost::asio::deadline_timer timer_;
    tcp::socket socket_;
    tcp::endpoint endpx_;
    std::string script_;
    pid_t pid_ = 0;
    time_t last_ok_time_ = 0;

    struct buffer_read : std::vector<char> {
        uint32_t len = 0;
    } rbuf;

    struct buffer_write : boost::string_ref {
        mutable uint32_t len = 0;
        buffer_write(char const* s) : boost::string_ref(s) {}
        std::array<boost::asio::const_buffer,2> const_buffers() const {
            len = ntohl( size() );
            return { as_buffer(len), boost::asio::buffer(data(), size()) };
        }
    } wbuf_;

    template <class M>
    void idle_end(M& m) {
        LOG << "seconds(10)" << pid_;
        timer_.expires_from_now(boost::posix_time::seconds(10)); //10
        timer_.async_wait([&m](boost::system::error_code ec) {
                if (!ec) {
                    LOG << "Timeout";
                    m.reset_state(m);
                }
            });
        m.process_event(Ev_idle_end());
    }

    template <typename M>
    bool step_ok(M& m, boost::system::error_code const& ec, int line, char const* fn)
    {
        if (!_Success(ec,line,fn)) {
            m.reset_state(m);
            return false;
        }
        return true;
    }

    template <class M>
    void restart_server(M& m, int line)
    {
        boost::system::error_code ec;
        timer_.cancel(ec);

        LOG <<"warning, restart"<< pid_ <<"#"<< line;
        if (pid_ > 0) {
            // kill(pid_, SIGHUP);
            // kill(pid_, SIGTERM);
            // sleep(5); //this_thread::sleep_for();
            // kill(pid_, SIGKILL);
            pid_ = 0;
        }

        m.last_ok_time_ = time(0);
        if (!m.script_.empty()) {
            auto sc = str(boost::format("%1% restart") % m.script_);
            LOG << sc;
            std::system(sc.c_str()); // RESTART
        }

        m.reset_state(m);
    }

    template <class M>
    void reset_state(M& m)
    {
        auto& io_s = get_io_service();
        io_s.post([&m](){
                m.stop();
                m.start();
            });
    }
};

struct Main : boost::asio::io_service , msm::back::state_machine<State<Main>>, singleton<Main>
{
    struct Args
    {
        int argc;
        char* const* argv;

        std::string host_; //("127.0.0.1");
        short unsigned int port_; // = "9999";
        std::string script_; // = "9999";
        std::string conf_; // = "9999";
        std::string pidfile_; // = "9999";

        Args(int ac, char *const av[]);
        tcp::endpoint endpx() const { return { ip::address::from_string(host_), port_ }; }
    };

    Main(Args const& args); // (int argc, char *const argv[])
    int run();

    boost::asio::signal_set signals_exit_;
    boost::asio::signal_set signals_ignore_;
};

Main::Main(Args const& args)
    : msm::back::state_machine<State<Main>>(this, args.endpx(), args.script_)
    , signals_exit_(*this, SIGINT, SIGTERM, SIGQUIT)
    , signals_ignore_(*this, SIGHUP)
    //, acceptor_(*this)
{
    //boost::property_tree::ptree conf = make_pt(args.conf_);
    //auto port = conf.get<short>("ims.port");

    //redis::init( conf.get_child("redis") );
    //sql::dbc::init( sql::dbc::config(conf.get_child("mysql")) );

    //cache_max_size_ = conf.get<UInt>("cache_max_size", 10000);
    //LOG << cache_max_size_;

    // tcp::resolver resolv(*this); // resolv.resolve(tcp::resolver::query(args.host_, args.port_));
    //tcp::endpoint ep(tcp::v4(), args.port_);
    //acceptor_.open(ep.protocol());
    //acceptor_.set_option(tcp::acceptor::reuse_address(true));
    //acceptor_.bind(ep);
}

int Main::run()
{
    signals_ignore_.async_wait(
            [this](boost::system::error_code ec, int sig) {
                LOG << sig;
            });
    signals_exit_.async_wait(
            [this](boost::system::error_code ec, int sig) {
                LOG << sig;
                this->boost::asio::io_service::stop();
            });

    //if (acceptor_.is_open()) {
    //    acceptor_.listen();
    //    acceptor_.async_accept(socket_, boost::bind(&Main::handle_accept, this, placeholders::error));
    //// } else { socket_.async_connect(endp_, boost::bind(&Main::handle_connect, this, placeholders::error));
    //}

    //timer_.expires_from_now(boost::posix_time::seconds());
    //timer_.async_wait(Main::timer_fn);
    //thread_ = boost::thread(&Main::fn_thread);
    msm::back::state_machine<State<Main>>::start();
    return boost::asio::io_service::run();
}

Main::Args::Args(int ac, char *const av[])
    : argc(ac), argv(av)
    , conf_("/etc/yxim.conf")
{
    namespace opt = boost::program_options;

    opt::variables_map vm;
    opt::options_description opt_desc("Options");
    opt_desc.add_options()
        ("help", "show this")
        ("daemon", "daemon")
        ("pid-file", opt::value<std::string>(&pidfile_)->default_value(pidfile_), "pid file")
        //("listen,l", "listen mode")
        ("host,h", opt::value<std::string>(&host_)->default_value("127.0.0.1"), "host")
        ("port,p", opt::value<unsigned short>(&port_)->default_value(8443), "port")
        ("script", opt::value<std::string>(&script_)->required(), "rc script")
        ("conf", opt::value<std::string>(&conf_)->default_value(conf_), "config file")
        ;
    opt::positional_options_description pos_desc;
    pos_desc.add("conf", 1);

    opt::store(opt::command_line_parser(argc, argv).options(opt_desc).positional(pos_desc).run(), vm);

    if (vm.count("help")) {
        std::cout << boost::format("Usage:\n  %1% [Options]\n") % argv[0]
            << opt_desc
            ;
        exit( 0 );
    }

    opt::notify(vm);

    if (vm.count("daemon")) {
        BOOST_ASSERT(!conf_.empty());
        int chdir = boost::starts_with(conf_,"/"); // && boost::starts_with(pidfile_,"/");
        if (daemon(!chdir, 0) != 0) {
            LOG << "daemon" << strerror(errno);
        }
        logging::syslog(LOG_PID|LOG_CONS, 0);
        LOG << conf_ << chdir << pidfile_;
    }

    if (!boost::filesystem::exists(script_)) {
        LOG << "fatal:" << script_ << "not exist";
    }

    if (pidfile_.size() > 1) {
        save_pid(pidfile_);
    }
}

int main(int argc, char* const argv[])
{
    try
    {
        Main app(Main::Args(argc, argv));
        //Mstart();
        app.run();
    } catch (error::signal& e) {
        LOG << e;
    } catch (myerror& e) {
        LOG << "Exception:" << e.what();
    }

    LOG << "bye.";
    return 0;
}

