#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <tuple>
#include <unordered_set>
//#include <functional>
#include <boost/locale.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/intrusive/parent_from_member.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include <boost/exception/errinfo_at_line.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scope_exit.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
//#include <boost/multi_index_container.hpp>
//#include <boost/multi_index/sequenced_index.hpp>
//#include <boost/multi_index/hashed_index.hpp>
//#include <boost/multi_index/member.hpp>
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
//#include <boost/archive/binary_oarchive.hpp>
#include "singleton.h"
#include "myerror.h"
#include "log.h"
#include "json.h"
#include "dbc.h"
#include "util.h"
#include "prettyprint.hpp"
#include "php_get.h"

#include "serialize_tuple.h"
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
typedef boost::archive::text_oarchive oArchive;
typedef boost::archive::text_iarchive iArchive;

#define NO_HEADER (boost::archive::no_header)
typedef unsigned int UInt;

using boost::asio::ip::tcp;
namespace placeholders = boost::asio::placeholders;
namespace multi_index = boost::multi_index;
namespace filesystem = boost::filesystem;
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

//template <typename T>
//inline std::ostream& operator<<(std::ostream& out, std::vector<T> const& v)
//{
//    typedef std::vector<T>::const_iterator iterator;
//    return out << boost::make_iterator_range<iterator>(v);
//}

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

    friend std::ostream& operator<<(std::ostream& out, Argv const& av)
    {
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

std::array<int,2> Pipe()
{
    std::array<int,2> v;
    if (pipe(&v[0]) < 0) {
        int errN = errno;
        char* es = strerror(errN);
        BOOST_THROW_EXCEPTION(error::fatal() << error::info(errN) << error::info(es));
    }
    return v;
}

static boost::property_tree::ptree make_ptree(boost::filesystem::path const &fp)
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

inline boost::format& fmt_fill(boost::format &fmt) { return fmt; }
template <typename T0, typename... T>
inline boost::format& fmt_fill(boost::format& fmt, T0&& t0, T&&... t)
{
    return fmt_fill(fmt % t0, std::forward<T>(t)...);
}

// "Host: gray_v4.api.moon.kklink.com",
// "X-API-VERSION: v4.0"
//// const char* url = "http://192.168.10.245/im/interaction?type=%1%&from=%2%&to=%3%&msg=%4%";
//auto q = str(boost::format("%1%?type=%2%&from=%3%&to=%4%&msg=%5%") % Url_sendmsg_ % type % fr % to % urlencode(msg));
//auto jv = php_get( q );

static int http_get_json(json::object* jv, std::string const& q)
{
    try {
        // auto q = str(boost::format("%1%?bid=%2") % url % bid_);
        auto v = php_get( q );
        if (jv) {
            *jv = std::move(v);
            //LOG << "GET" << q; // <<":"<< *jv;
        }
        return 0;
    } catch (myerror& ex) {
        LOG << ex;
    } catch (std::exception& ex) {
        LOG << ex.what();
    }
    LOG << "fail:" << q;
    return 1;
}

template <typename F, typename T0, typename... T>
static int http_get_json(json::object* jv, F&& fmt, T0&& t0, T&&... t)//(std::tuple<time_t,UInt,UInt,int> const& u)
{
    boost::format bfmt(std::forward<F>(fmt));
    auto && q = str( fmt_fill(bfmt, std::forward<T0>(t0), std::forward<T>(t)...) ); //std::string const& q = str(bfmt);
    BOOST_STATIC_ASSERT(std::is_same<std::string,typename std::decay<decltype(q)>::type>::value);
    return http_get_json(jv, q);
}

struct Main : boost::asio::io_service, singleton<Main>
{
    struct Args
    {
        int argc;
        char* const* argv;

        std::string host_; //("127.0.0.1");
        short unsigned int port_; // = "9999";
        std::string conf_; // = "9999";
        std::string pidfile_; // = "9999";
        std::string print_archive_file_;

        Args(int ac, char *const av[]);

        void save_pid(std::string const& fp) {
            boost::filesystem::ofstream out(fp);
            if (out) {
                out << getpid();
            }
        }
    };

    boost::asio::signal_set signals_exit_;
    boost::asio::signal_set signals_ig_;
    //tcp::acceptor acceptor_;
    tcp::socket socket_;
    // tcp::endpoint endp_;
    // fifo_stream_descriptor cast_;

    ~Main() {
        (this->*run_x_)(1);
        if (thx_.joinable()) {
            LOG << "joinable";
            thx_.join();
        }
    }
    Main(Args const& args); // (int argc, char *const argv[])

    int run() { return (this->*run_x_)(0); }

    template <typename Archive>
    void serialize(Archive& ar, unsigned int ver) {
        ar & xtq_;
        ar & bars_;
        ar & nfail_;
        ar & nreq_;
        ar & ntotal_;
        ar & timepx_start_;
    }
private:
    void handle_accept(boost::system::error_code ec);
    void handle_connect(boost::system::error_code ec) {}

private:
    void loop(boost::system::error_code ec);

#define FN_AR_TMP "/tmp/virtual-genchatmessage.s.4"
    void ar_save(time_t ct, bool barchive=1)
    {
        LOG << FN_AR_TMP << int(barchive);
        if (barchive && filesystem::exists(FN_AR_TMP)) {
            filesystem::remove(FN_AR_TMP "p");
            filesystem::rename(FN_AR_TMP, FN_AR_TMP "p");
        }
        filesystem::ofstream out(FN_AR_TMP);
        oArchive ar(out);
        ar & *this; // serialize(ar);
    }
    void ar_load()
    {
        filesystem::path fp(FN_AR_TMP);
        filesystem::ifstream in(fp);
        // filesystem::remove(fp);
        // time_t ct = time(0);
        if (in) {
            iArchive ar(in);
            ar & *this; // serialize(ar);
        }
    }
#undef FN_AR_TMP

    struct bar : std::vector<int> //tps_;
    {
        typedef std::vector<int> base;

        UInt bid_;
        int  bcat_;

        static inline struct tm* to_midnight(struct tm* tm) {
            tm->tm_hour = tm->tm_min = tm->tm_sec = 0; //tm->tm_wday = tm->tm_yday = 0;
            return tm; // mktime(tm)
        }

        static inline std::pair<time_t,time_t> time_range(int tm_hour0=14, int tm_hour1=24) {
            BOOST_STATIC_CONSTANT(unsigned int, seconds_phour=(60*60));
            //BOOST_STATIC_CONSTANT(int, tm_hour0=18);
            //BOOST_STATIC_CONSTANT(int, tm_hour1=24);
            BOOST_ASSERT(tm_hour0 < tm_hour1);
            time_t ct = time(0);
            struct tm tm;
            time_t tpb = mktime( to_midnight( localtime_r(&ct,&tm) ) );
            time_t tp0 = tpb + seconds_phour * tm_hour0;// + 60*15;
            time_t tp1 = tpb + seconds_phour * tm_hour1;// + 60*15;
            auto p = std::make_pair(std::max(tp0,ct), tp1);
            // LOG << p;
            return p;
        }

        void random(int n, int h0, int h1) {
            auto tpr = time_range(h0, h1);
            unsigned int dur = tpr.second - tpr.first;
            while (n-- > 0) {
                push_back(randuint(0, dur));
            }
            std::sort(begin(), end(), std::greater<int>());
        }

        bar(UInt b, int cat, int n, int h0, int h1) {
            bid_ = b;
            bcat_ = cat;
            random(n, h0, h1);
        }
        bar() {} // serialization required

        int pop_next_duration()
        {
            int d = back();
            pop_back();
            return d;
        }

        //bool empty() const { return tps_.empty(); }

        bool gen_msg(std::string const& url);

        friend std::ostream& operator<<(std::ostream& out, bar const& b) {
            return out << b.bid_ <<"\t"<< b.bcat_
                <<"\t"<< b.size() <<": "<< static_cast<base const&>(b);
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned int ver) {
            ar & bid_;
            ar & bcat_;
            ar & boost::serialization::base_object<base>(*this); //tps_;
        }
    };
    typedef bar value_type;

    enum {
        cat_1=0 // 慢摇吧/迪吧
        , cat_2 // 清吧
        , cat_3 // 演艺吧
        , cat_max
    };

    bar* add(UInt bid, int cat, int hot);

    std::multimap<time_t, value_type> xtq_; //this->xtq_.emplace(time(0) + mins * 60, uidp);
    std::unordered_set<UInt> bars_;
    int nfail_ = 0;
    int nreq_ = 0;
    int ntotal_ = 0;
    time_t timepx_start_ = 0;

    boost::asio::deadline_timer timer_;

private:
    int load_bars_start(std::string const& url);

    //int randmins();
    //std::tuple<time_t,UInt,UInt,int> pop_item();
    //void thx_loop();
    //void thx_after_seconds(int s=6);

    std::thread thx_;
    //boost::asio::io_service thx_io_service_;
    // boost::asio::deadline_timer timer_xth_;

    std::string Url_sendmsg_;// = "http://192.168.10.245/im/interaction";
    std::string Url_listbar_;// = "http://192.168.10.245/im/interaction";

    CurlHandle curl_wrapper;

    int run_print_archive_only_(bool bstop);
    int run_normal_(bool bstop);
    int (Main::*run_x_)(bool);
};

//template<int I=0, typename S, typename... Tp>
//inline typename std::enable_if<I==sizeof...(Tp)-1, S>::type& print(S& os, std::tuple<Tp...> const& t) {
//    return os << std::get<I>(t);
//}
//template<int I=0, typename S, typename... Tp>
//inline typename std::enable_if<I <sizeof...(Tp)-1, S>::type& print(S& os, std::tuple<Tp...> const& t) {
//    return print<I+1, S, Tp...>(os << std::get<I>(t) << " ", t);
//}
//template <int i, int l> struct tuple_print {
//    template <typename T> inline static std::ostream& print(std::ostream& os, T const& t) {
//        os << std::get<i>(t) <<" ";
//        return tuple_print<i+1,l>::print(os, t);
//    }
//};
//template <int i> struct tuple_print<i,i> {
//    template <typename T> inline static std::ostream& print(std::ostream& os, T const& t) {
//        return os << std::get<i>(t);
//    }
//};
//template <typename S, typename... T>
//inline S& operator<<(S& os, std::tuple<T...> const& t) {
//    return print(os, t);
//}

Main::Main(Args const& args) // (int argc, char *const argv[])
    : signals_exit_(*this, SIGINT, SIGTERM, SIGQUIT)
    , signals_ig_(*this, SIGHUP)
    //, acceptor_(*this)
    , socket_(*this)
    , timer_(*this)
    // , timer_xth_(*this)
    // , cast_(*this, args.cast_)
{
    if (args.print_archive_file_.empty()) {
        run_x_ = &Main::run_normal_;

        boost::property_tree::ptree conf = make_ptree(args.conf_);
        redis::init( conf.get_child("redis") );
        Url_sendmsg_ = conf.get("url.genmsg", "http://192.168.10.245/im/chatroom");
        Url_listbar_ = conf.get("url.listbar", "http://192.168.10.245/im/getbar");
        LOG << Url_sendmsg_;
        LOG << Url_listbar_;

    } else {
        run_x_ = &Main::run_print_archive_only_;
    }

    //sql::dbc::init( sql::dbc::config(conf.get_child("mysql")) );

    //= conf.get<UInt>("cache_max_size", 10000);

    // tcp::resolver resolv(*this); // resolv.resolve(tcp::resolver::query(args.host_, args.port_));
    //tcp::endpoint ep(tcp::v4(), args.port_);
    //acceptor_.open(ep.protocol());
    //acceptor_.set_option(tcp::acceptor::reuse_address(true));
    //acceptor_.bind(ep);
}

int Main::run_print_archive_only_(bool bstop)
{
    if (bstop) {
        return 0;
    }

    ar_load();
    time_t ct = time(0);
    BOOST_ASSERT(timepx_start_ < ct);

    std::cout <<"#time: "<< timepx_start_ <<" elapses "<< ct - timepx_start_ <<"\n";
    std::cout <<"#bars: "<< xtq_.size() <<" "<< bars_.size() <<"\n";
    std::cout <<"#msgs: "<< nfail_ <<" "<< nreq_ <<" "<< ntotal_ <<"\n";
    for (auto& p : xtq_) {
        std::cout << p.second <<" "<< int(p.first - timepx_start_) <<"\n";
    }
    return 0;
}
int Main::run_normal_(bool bstop)
{
    if (bstop) {
        ar_save( time(0), 0 );
        return 0;
    }

    signals_ig_.async_wait( [this](boost::system::error_code ec, int sig) {
                LOG << sig;
                this->ar_save( time(0), 0 );
            });
    signals_exit_.async_wait( [this](boost::system::error_code ec, int sig) {
                LOG << sig;
                this->stop();
            });

    ar_load();
    loop(boost::system::error_code());

    return boost::asio::io_service::run();
        //thx_ = std::thread(&Main::thx_loop, this);
        //if (acceptor_.is_open()) {
        //    acceptor_.listen();
        //    acceptor_.async_accept(socket_, [this](boost::system::error_code ec) { handle_accept(ec); });
        //}
}

bool Main::bar::gen_msg(std::string const& url)
{
    json::object ret;
    if (int x = http_get_json(&ret, "%1%?barid=%2%", url, bid_)) {
        LOG << "fail:" << x;
        return false;
    }
    auto ec = json::as<int>(ret, "code").value_or(-1);
    if (ec != 0) {
        LOG << "fail: ec" << ec;
        return false;
    }
    return true;
}

int Main::load_bars_start(std::string const& url)
{
    auto & q = url; //str(boost::format("%1%") % url);
    LOG << q;
    json::object jv;
    if (int x = http_get_json( &jv, q.c_str() )) {
        LOG <<"fail:"<< x;
        return 0;
    }
    auto _bars = json::as<json::array>(jv, "bars");
    if (!_bars) {
        LOG <<"fail:"<< jv;
        return 0;
    }
    auto& bars = _bars.value();
    if (bars.empty()) {
        LOG <<"fail: empty"<< jv;
        return 0;
    }
    int sum[cat_max] = {0};
    int ntot = 0;
    for (auto& x : bars) {
        auto& o = json::value<json::object>(x);
        UInt bid = json::value<UInt>(o, "id");
        // if (bid != 1 && bid != 43) continue; // TODO test-only
        int hot = 0;
        if (json::as<int>(o, "hot").value_or(0)) {
            hot = 1;
        }
        int cat = json::value<int>(o, "type") - 1;
        if (cat < 0 || cat >= cat_max || bid < 1) {
            LOG << "fail: bar data" << cat <<"bid"<< bid;
            continue;
        }
        if (auto* p = this->add(bid, cat, hot)) {
            ntot += p->size() +1;
            LOG << *p;
        }
        sum[cat]++;
    }
    LOG << "bars:" << bars.size() << sum;
    return ntot;
}

Main::bar* Main::add(UInt bid, int cat, int hot)
{
    if (!bars_.insert(bid).second) {
        LOG << "fail: exist" << bid;
        return 0;//bars_.end();
    }

    std::array<int,2> rNumb = {  3, 10 }; // 条数范围
    std::array<int,2> rHour = { 14, 24 }; // 时间范围
    int ra=0;

    if (!hot) switch (cat) {
    case cat_1: // 慢摇吧/迪吧
        ra=90; //y = (ra = randuint(0,100)) <= 90;
        rNumb = {5,15}; //{25, 40};
        rHour = {14, 24+2};
        break;
    case cat_2: // 清吧
    case cat_3: // 演艺吧
        ra=80; //y = (ra = randuint(0,100)) <= 80;
        break;
    default:
        LOG << "fail: bar type unknown";
        return 0;
    }
    int y;
    if ((y = randuint(0,100)) > ra) {
        LOG << bid << cat << ra << "skip" << y;
        return 0;
    }

    bar b(bid, cat, randuint(rNumb[0],rNumb[1]), rHour[0], rHour[1]);
    time_t tpx = timepx_start_ +  b.pop_next_duration();
    return &(xtq_.emplace(tpx, std::move(b))->second);
}

void Main::loop(boost::system::error_code ec)
{
    if (ec) {
        LOG << ec << ec.message();
        return;
    }

    UInt millis = 60000;
    auto && finally = yx::finally( [this,&millis]() noexcept {
            timer_.expires_from_now(boost::posix_time::milliseconds(millis));
            timer_.async_wait([this](boost::system::error_code ec){ loop(ec); });
        });

    time_t ct = time(0);
    auto tpr = bar::time_range(14,24+2); //struct tm tm; localtime_r(&ct, &tm); if (tm.tm_hour < 12 || tm.tm_hour  )
    if (ct < tpr.first || ct > tpr.second) {
        LOG << nfail_ << nreq_ << ntotal_ << ct << tpr;
        if (!bars_.empty()) {
            ar_save( ct );
        }
        xtq_.clear();
        bars_.clear();
        return;
    }
    if (bars_.empty()) {
        timepx_start_ = time(0);
        nfail_ = nreq_ = 0;
        ntotal_ = load_bars_start(Url_listbar_);
        if (ntotal_ > 0) {
            ar_save( ct );
        }
    }
    // BOOST_ASSERT(bars_.empty() == xtq_.empty());

    if (!xtq_.empty())
    {
        auto it = xtq_.begin();
        BOOST_ASSERT (it->first <= tpr.second && it->first >= tpr.first);

        if (it->first > ct) {
            int secs = it->first - ct;
            LOG << boost::format("%1%:%2%") % (secs/60) % (secs%60);
            if (secs < 60*60*3) {
                millis = std::min(60000, secs*1000);
                return;
            }
        } else {
            millis = 100;
            if (ct - it->first < 60*5) {
                nreq_++;
                if (!it->second.gen_msg(Url_sendmsg_)) {
                    nfail_++;
                }
                if (nreq_ % 100 == 0) {
                    LOG <<"summary:"<< nfail_ << nreq_ << ntotal_ << ct << tpr;
                }
            } else {
                millis = 1;
                LOG << "speed-up skip" << (ct - it->first) << it->second;
            }
        }

        auto b = std::move(it->second);
        xtq_.erase(it);

        if (!b.empty()) {
            time_t tpx = std::max(ct+5, timepx_start_+b.pop_next_duration());
            xtq_.emplace(tpx, std::move(b));
        }
    }

    static_cast<void>(finally); // silent warning
}

#if 0
void Main::thx_after_seconds(int s)
{
    timer_xth_.expires_from_now(boost::posix_time::seconds(s));
    timer_xth_.async_wait([this](boost::system::error_code ec){
            if (!ec)
                thx_ = std::thread(&Main::thx_loop, this);
        });
    LOG << s;
}

void Main::thx_loop()
{
    std::tuple<time_t,UInt,UInt,int> uidp = pop_item();
    auto ra = randuint(1, 100);
    int wseconds = 15;

    if (std::get<0>(uidp) > 0 && std::get<1>(uidp) > 0)
    {
        if (ra <= 70) {
            int mins = randmins();
            this->post([this,uidp,mins](){
                    thx_.join();
                    thx_after_seconds(1);
                    this->xtq_.emplace(time(0) + mins * 60, uidp);
                });
            LOG << uidp <<"mins"<< mins <<"ra"<< ra;
            return;
        }
        wseconds = 1;
    }
    LOG << uidp << ra <<"skip";// << (wseconds>1?"ra gt 70":"");

    this->post([this,wseconds](){
            thx_.join();
            thx_after_seconds(wseconds);
        });
}

static std::vector<std::pair<wchar_t const*,std::vector<char const*>>> words {
    { L"你好", {"哈喽","嗯哼？","你好","[呵呵]","[酷]","[挤眼]","[微笑]","[思考]","好啊","嗯？","[卖萌]","[呲牙大笑]","[爱你]","嘿嘿" }}
  , { L"hi",   {"哈喽","嗨","hi","hello","你好","[呵呵]","[微笑]","[思考]","好啊","嗯？","[卖萌]","[呲牙大笑]","[爱你]","嘿嘿" }}
  , { L"hello",{"哈喽","嗨","hi","hello","你好","[呵呵]","[微笑]","[思考]","好啊","嗯？","[卖萌]","[呲牙大笑]","[爱你]","嘿嘿" }}
  , { L"嗨",   {"哈喽","嗨","hi","hello","你好","[呵呵]","[微笑]","[思考]","好啊","嗯？","[卖萌]","[呲牙大笑]","[爱你]","嘿嘿" }}
  , { L"美女", {"哈喽","？","[呵呵]","[酷]","[挤眼]","[微笑]","[思考]","嗯？","[卖萌]","[呲牙大笑]","","[呲牙大笑]","[爱你]","嘿嘿" }}
  , { L"",     {"哈喽","嗨","hi","hello","你好","[呵呵]","[微笑]","[思考]","好啊","嗯？","[卖萌]","[呲牙大笑]","[爱你]","嘿嘿" }}
};

std::tuple<time_t,UInt,UInt,int> Main::pop_item()
{
    static char const* const redk = "yuexia:UsrPuppetList";

    auto reply = redis::command("LPOP", redk);
    if (!reply) {
        LOG << "redis BLPOP: no reply";
        return std::make_tuple(0,0,0,0);
    }
    if (reply->type != REDIS_REPLY_STRING) {
        return std::make_tuple(0,0,0,0);
    }
    //if (reply->type != REDIS_REPLY_ARRAY) {
    //    LOG_ERR << "redis BRPOP: !=REDIS_REPLY_ARRAY";
    //    return std::make_tuple(0,0,0,0);
    //}
    //if (reply->elements != 2 || !reply->element[1]->str) {
    //    LOG_ERR << "redis BRPOP";
    //    return std::make_tuple(0,0,0,0);
    //}
    if (words.empty()) {
        return std::make_tuple(0,0,0,0);
    }

    const char* p = reply->str; //(reply->element[1]->str);
    const char* ends = p + strlen(p);

    if (std::count(p, ends, '\t') < 2) {
        return std::make_tuple(0,0,0,0);
    }

    UInt src = atoi(p);
    p = std::find(p,ends, '\t') + 1;
    UInt dst = atoi(p);
    p = std::find(p,ends, '\t') + 1;

    if (src == 0 || dst == 0 || p >= ends) {
        return std::make_tuple(0,0,0,0);
    }

    int mcol = words.size()-1;

    std::wstring u32s = boost::locale::conv::utf_to_utf<wchar_t>(p, ends);
    for (int x = words.size()-1; x >= 0; --x) {
        auto r = boost::ifind_first(u32s, words[x].first);
        if (!boost::empty(r)) {
            mcol = x;
            break;
        }
    }
    LOG << src << dst << p << mcol;

    return std::make_tuple(time(0), src, dst, mcol);
}

//static inline struct tm* to_midnight(struct tm* tm)
//{
//    tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
//    //tm->tm_wday = tm->tm_yday = 0;
//    return tm; // mktime(tm)
//}
//static inline time_t mktime_midnight(struct tm const* ct)
//{
//    struct tm tm = *ct;
//    return mktime(to_midnight(&tm));
//}
//static inline time_t time_midnight(time_t ct)
//{
//    struct tm tm;
//    localtime_r(&ct, &tm);
//    return mktime(to_midnight(&tm));
//}

int Main::randmins()
{
    int mins = 15;
    time_t ct = time(0);
    struct tm tm;
    if (localtime_r(&ct, &tm)) {
        mins = randuint(10,70);
        if (tm.tm_hour >= 3 && tm.tm_hour < 10) {
            mins += (10 - tm.tm_hour) * 60;
        }
    }
    return mins; // / 60;//test
}
#endif

Main::Args::Args(int ac, char *const av[])
    : argc(ac), argv(av)
    , conf_("/etc/virtual-genchatmessage.conf")
{
    namespace opt = boost::program_options;

    opt::variables_map vm;
    opt::options_description opt_desc("Options");
    opt_desc.add_options()
        ("help", "show this")
        ("print", opt::value<std::string>(&print_archive_file_)->default_value(""), "show archive file")
        ("daemon", "daemon")
        ("pid-file", opt::value<std::string>(&pidfile_)->default_value(""), "pid file")
        //("listen,l", "listen mode")
        //("host,h", opt::value<std::string>(&host_)->default_value("0"), "host")
        //("port,p", opt::value<unsigned short>(&port_)->default_value(9999), "port")
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
        int chdir = (boost::starts_with(conf_,"/") && boost::starts_with(pidfile_,"/"));
        if (daemon(!chdir, 0) != 0) {
            LOG << "daemon" << strerror(errno);
        }
        logging::syslog(LOG_PID|LOG_CONS, 0);
        LOG << conf_ << chdir << pidfile_;
    }

    if (!pidfile_.empty()) {
        save_pid(pidfile_);
    }
}

int main(int argc, char* const argv[])
{
    try
    {
        Main app(Main::Args(argc, argv));
        app.run();
    } catch (error::signal& e) {
        LOG << e;
    } catch (myerror& ex) {
        LOG << ex;
    }
    LOG << "bye.";
    return 0;
}

// struct Handle_write : Iterator<1> 
// {
//     void operator()(boost::system::error_code ec, size_t bytes) const
//     {
//         Iterator<1> const& it = *this;
//         if (ec) {
//             LOG << ec << ec.message();
//             return;
//         }
//         if (it->outbufs_.empty()) {
//             return;
//         }
// 
//         auto & out = it->outbufs_;
// 
//         auto& sbuf = out.front();
//         out.len = htonl(sbuf.size());
// 
//         //std::array<boost::asio::const_buffer,2> bufs;
//         //bufs[0] = const_buffer(out.len);
//         //bufs[1] = sbuf.data();
//         std::array<boost::asio::const_buffer,2> bufs{ const_buffer(out.len), sbuf.data() };
// 
//         mark_live(it);
//         boost::asio::async_write(*it, bufs, *this);
//     }
// 
//     Handle_write(Iterator<1> it) : Iterator<1>(it)
//     {}
// };
// 
// void async_recv(Iterator<0> it);
// 
// int logik_entry(Iterator<0> it, boost::archive::binary_iarchive& ar);
// 
// struct Handle_read : Iterator<0>
// {
//     void operator()(boost::system::error_code ec, size_t bytes)
//     {
//         if (ec) {
//             LOG << ec << ec.message();
//             return;
//         }
//         Iterator<0>& it = *this;
//         auto & in = it->inbuf_;
// 
//         in.commit(bytes);
//         {
//             int x;
//             boost::archive::binary_iarchive ia(in, NO_HEADER);
//             if ( (x = logik_entry(it, ia)) < 0) {
//                 LOG << x;
//             } else {
//                 async_recv(*this);
//             }
//         }
//     }
// 
//     Handle_read(Iterator<0> it) : Iterator<0>(it)
//     {}
// };
// 
// struct Handle_read_size : Iterator<0>
// {
//     void operator()(boost::system::error_code ec, size_t bytes)
//     {
//         if (ec) {
//             LOG << ec << ec.message();
//             return;
//         }
// 
//         Iterator<0> & it = *this;
//         auto& in = it->inbuf_;
// 
//         in.len = htonl(in.len);
// 
//         mark_live(it);
//         boost::asio::async_read(*it, in.prepare(in.len), Handle_read(it));
//     }
// 
//     Handle_read_size(Iterator<0> it) : Iterator<0>(it)
//     {}
// };
// 
void Main::handle_accept(boost::system::error_code ec)
{
    if (ec) {
        LOG << ec << ec.message();
        return;
    }

    //Iterator<0> it( emplace(begin(), std::move(socket_)) );
    //async_recv(it);

    socket_.close(ec);
    //socket_.open(tcp::v4());
    //acceptor_.async_accept(socket_, [this](boost::system::error_code ec) { handle_accept(ec); });
}

#include "php_get.cpp"

