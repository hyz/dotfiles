#include <string>
#include <list>
#include <iostream>
//#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>
//#include <curl/curl.h>
#include "myerror.h"
#include "json.h"
#include "async.h"

typedef unsigned int UInt;

struct Log : boost::noncopyable
{
    template <typename X> Log const& operator<<(X const& x) const
    {
        *outs <<" "<< x;
        return *this;
    }
    Log(std::ostream & s) : outs(&s) { *outs << idx_++; }
    ~Log() { *outs <<"\n"; }
    std::ostream * outs;
    static UInt idx_;
};
UInt Log::idx_ = 0;
using std::cout;
using std::clog;
using std::cerr;

namespace io = boost::asio;
namespace ip = boost::asio::ip;
using namespace boost::posix_time;

io::io_service & io_service()
{
    static io::io_service io_s;
    return io_s;
}

struct tx_data
{
    UInt cmd;
    std::string data;
    tx_data(UInt x, std::string const& d) : data(d) { cmd=x; }
};

struct connection : ip::tcp::socket
{
    connection(io::io_service& io_s, UInt id)
        : ip::tcp::socket(io_s)
    { uid = id; }

    connection(connection && rhs)
        : ip::tcp::socket(std::move(rhs))
        , txbufs(std::move(rhs.txbufs))
        , rxbuf(std::move(rhs.rxbuf))
    { uid = rhs.uid; }

    boost::asio::const_buffers_1 const_buffer() const { return io::buffer(txbufs.begin()->data); }
    tx_data const& front() const { return txbufs.front(); }
    void consume_front() { txbufs.pop_front(); }
    bool is_empty() const { return txbufs.empty(); }

    bool is_ok() const { return !err_c; }

    boost::system::error_code err_c;
    UInt uid;
    std::list<tx_data> txbufs;
    std::vector< std::list<tx_data> > stages_data_;
    imsprotocol rxbuf;

    unsigned short cmdexp_;
    bool cmdexp_waiting_;
    bool writing;

    template <typename Rng>
    void stage_data(int x, Rng const& r)
    {
        stages_data_.resize(x+1);
        stages_data_[x].assign(r.begin(), r.end()); // = std::list<tx_data>(r.begin(), r.end());
    }

    void stage_at(size_t x)
    {
        if (x < stages_data_.size())
        {
            stages_data_[x].swap(txbufs); // txbufs = stages_data_[x]; // 
        }
    }

    void append_tx(int cmd, std::string const& tx)
    {
        txbufs.push_back(tx_data(cmd, tx));
    }
};

ip::tcp::endpoint endpoint_; // std::list<std::string> files_;
bool msg_ack_ = 0;
int heartbeat_tv_ = 90;

std::vector<connection> connections_;
size_t idx2connect_ = 0;
size_t n_bad_connection_ = 0;
size_t n_busy_ = 0;

boost::unordered_map<UInt,UInt> m_uid_c_;

typedef boost::error_info<struct x,std::string> errinfo_str;
typedef boost::error_info<struct u,UInt> errinfo_int;

struct cmdstat_t
{
    ptime pt_open, pt_close;
    UInt count;

    cmdstat_t() { count=0; }

    void increment()
    {
        ++count;

        if (pt_open.is_not_a_date_time())
            pt_open = microsec_clock::local_time();
        pt_close = microsec_clock::local_time();
    }

    template <typename ...Args>
    void report(std::ostream& out, Args... params) const
    {
        if (pt_open.is_not_a_date_time())
            return;

        _print_h(out, "", params...);

        auto dur = pt_close - pt_open;
        out << ":"
            << " count " << count
            << " milliseconds " << dur.total_milliseconds()
            ;
    }

    template <typename T> static void _print_h(std::ostream& out, const char* space, T x)
    { out << space << x; }
    template <typename T, typename ...Args> static void _print_h(std::ostream& out, char const* space, T x, Args... params)
    {
        out << space << x;
        _print_h(out, " ", params...);
    }
};

extern void loop_end(UInt idx, int io_f);

struct handle_write
{
    void operator()(boost::system::error_code ec, std::size_t bytes_transferred);

    UInt const idx_;
    handle_write(UInt x) : idx_(x) {}
};

struct handle_connect
{
    void operator()(boost::system::error_code ec) const;

    handle_connect(UInt x) : idx_(x) {}

    UInt const idx_;
};

struct handle_read
{
    void operator()(boost::system::error_code ec, std::size_t bytes_transferred) const;

    handle_read(UInt x) : idx_(x) {}
private:
    UInt const idx_;

    int parse_cmds(connection& cnn) const;
    boost::iterator_range<char*> parse(boost::iterator_range<char*> rng, size_t* nb_exp) const;

    void ack(UInt msgid, std::string const& sid) const;
};

struct stage_status
{
    static UInt ver_count_;
    std::string name_;
    ptime pt_open, pt_close;
    UInt n_bad;
    UInt tx_bytes_transferred;
    UInt rx_bytes_transferred;

    boost::unordered_map<int, cmdstat_t> rxcmds;
    boost::unordered_map<int, cmdstat_t> txcmds;

    stage_status(std::string const& name)
        : name_(name)
    {
        init();
    }

    void init()
    {
        n_bad = 0;
        tx_bytes_transferred = 0;
        rx_bytes_transferred = 0;
        pt_open = microsec_clock::local_time();
    }

    bool is_done() const { return (n_busy_ == 0); }

    void update_tx(connection& cnn, int cmd, size_t bytes)
    {
        txcmds[cmd].increment();
        tx_bytes_transferred += bytes;
        ++ver_count_;
        pt_close = microsec_clock::local_time();
    }

    void update_rx_cmd(connection& cnn, int cmd)
    {
        rxcmds[cmd].increment();
        ++ver_count_;
        pt_close = microsec_clock::local_time();
    }

    void update_rx(connection& cnn, size_t bytes)
    {
        rx_bytes_transferred += bytes;
        ++ver_count_;
        pt_close = microsec_clock::local_time();
    }

    void update_fail(connection& cnn)
    {
        ++n_bad;
        ++n_bad_connection_;
        ++ver_count_;
        pt_close = microsec_clock::local_time();
    }

    void report(std::ostream& out) const;

    static void s_report(std::ostream& out);
};

UInt stage_status::ver_count_;

void stage_status::report(std::ostream& out) const
{
    static UInt s_ver = 0;
    if (s_ver == ver_count_)
        return;
    s_ver = ver_count_;

    auto dur = pt_close - pt_open;
    out << "[" << name_ << "] " << second_clock::local_time()
        << "\n  total milliseconds " << dur.total_milliseconds()
        << "\n  connections fail " << n_bad <<"("<< n_bad_connection_ <<")"
        << "\n  connections avail "<< connections_.size() - n_bad_connection_
        << "\n  bytes tx " << tx_bytes_transferred
        << "\n  bytes rx " << rx_bytes_transferred
        ;
    BOOST_FOREACH(auto & p, txcmds)
        p.second.report(out, "\n  tx command", p.first);
    BOOST_FOREACH(auto & p, rxcmds)
        p.second.report(out, "\n  rx command", p.first);
    out << "\n";
}

std::vector<stage_status> stages_;
std::vector<stage_status>::iterator iterator_;

void stage_status::s_report(std::ostream& out)
{
    return;
    BOOST_FOREACH(auto & s, stages_)
        s.report(out);
}

template <typename Iter> std::string pack(Iter it, Iter end)
{
    union { uint32_t len; char s[sizeof(uint32_t)]; } un;
    un.len = htonl(std::distance(it, end));
    return std::string(un.s,sizeof(un)) + std::string(it, end);
}

std::string pack(std::string const& js)
{
    union { uint32_t len; char s[sizeof(uint32_t)]; } un;
    un.len = htonl(js.size());
    return std::string(un.s,sizeof(un)) + js;
}

boost::unordered_map<UInt, std::list<tx_data> > parse_input_file(std::string const& fn)
{
    using namespace std;
    boost::unordered_map<UInt, list<tx_data> > datal;
    int nline = 0;

    boost::filesystem::ifstream ifile(fn);
    if (!ifile)
        BOOST_THROW_EXCEPTION( myerror() << boost::errinfo_file_name(fn) );

    std::string line;
    while (getline(ifile, line))
    {
        auto begin = line.begin();
        auto end = line.end();

        auto it1 = std::find(begin, end, '\t');
        if (it1 == end)
            BOOST_THROW_EXCEPTION( myerror() << errinfo_str(line) );
        auto it2 = std::find(it1+1, end, '\t');
        if (it2 == end)
            BOOST_THROW_EXCEPTION( myerror() << errinfo_str(line) );

        UInt uid = boost::lexical_cast<UInt>(std::string(begin, it1));
        int cmd = boost::lexical_cast<int>(std::string(it1+1, it2));
        if (it2 != end)
            ++it2;

        auto & l = datal.insert( make_pair(uid, list<tx_data>()) ).first->second;
        l.emplace_back( cmd, pack(it2, end) );

        ++nline;
    }
    // Log(clog) << nline; // return nline;
    return datal;
}

void handle_read::operator()(boost::system::error_code ec, std::size_t bytes_transferred) const
{
    connection & cnn = connections_[idx_];
    if (ec) {
        Log(cerr) << cnn.uid <<"read error"<< ec << ec.message();
        cnn.err_c = ec;
    } else {
        iterator_->update_rx(cnn, bytes_transferred);

        cnn.rxbuf.commit(bytes_transferred);

        int min_exp = parse_cmds(cnn);
        if (min_exp > 0) {
            cnn.rxbuf.reserve(min_exp);
            io::async_read(cnn, cnn.rxbuf.avail_buffer(), io::transfer_at_least(min_exp), handle_read(idx_));
        }
    }

    loop_end(idx_, std::ios::in);
}

int handle_read::parse_cmds(connection& cnn) const
{
    try
    {
        boost::iterator_range<char*> rng, s_rng = cnn.rxbuf.range();
        size_t min_exp = 0;
        while ( !empty(rng = parse(s_rng, &min_exp)) )
        {
            BOOST_ASSERT(size(rng) >= 4);
            s_rng.advance_begin( size(rng) );
            rng.advance_begin(4);
            if (empty(rng)) {
                continue;
            }
            Log(clog) << cnn.uid <<" "<< rng;

            boost::optional<json::object> oo = json::decode<json::object>(rng);
            if (!oo || boost::empty(oo.value())) {
                continue;
            }
            auto const& obj = oo.value();

            UInt cmd = json::as<UInt>(obj, "cmd").value();
            UInt err = json::as<UInt>(obj, "error").value_or(0);

            iterator_->update_rx_cmd(cnn, cmd);

            if (cnn.cmdexp_ == cmd && cmd>0) {
                cnn.cmdexp_waiting_ = 0;
            }

            if (!err) {
                if (cmd == 200) {
                    json::object const& body = json::as<json::object>(obj,"body").value();
                    UInt msgid = json::as<UInt>(body,"msgid").value();
                    std::string sid = json::as<std::string>(body,"sid").value();

                    this->ack(msgid, sid);
                }
            }

            cnn.rxbuf.consume_to( s_rng.begin() );
        }
        BOOST_ASSERT(min_exp > 0);
        return min_exp;

    } catch (myerror const& ex) {
        boost::system::error_code ec;
        Log(cerr) << cnn.local_endpoint(ec) << ex;
    } catch (std::exception const& ex) {
        boost::system::error_code ec;
        Log(cerr) << cnn.local_endpoint(ec) << "=except:" << ex.what();
    }
    return -1;
}

boost::iterator_range<char*> handle_read::parse(boost::iterator_range<char*> rng, size_t* nb_exp) const
{
    enum { bytes_of_header=4 };
    boost::iterator_range<char*> ret(rng.begin(),rng.begin());
    // BOOST_ASSERT(n_exp);

    if (size(rng) < bytes_of_header)
    {
        *nb_exp = bytes_of_header - size(rng);
        return ret;
    }

    union { uint32_t len; char s[sizeof(uint32_t)]; } un;
    un.s[0] = rng[0];
    un.s[1] = rng[1];
    un.s[2] = rng[2];
    un.s[3] = rng[3];

    size_t nbyte = bytes_of_header + ntohl(un.len);
    if (size(rng) < nbyte)
    {
        *nb_exp = nbyte - size(rng);
        return ret;
    }

    ret.advance_end( nbyte );
    return ret;
}

void handle_write::operator()(boost::system::error_code ec, std::size_t bytes_transferred)
{
    connection & cnn = connections_[idx_];
    cnn.writing = 0;
    if (ec)
    {
        Log(cerr) << cnn.uid <<"write"<< ec;
        cnn.err_c = ec;
    }
    else
    {
        int cmd = cnn.front().cmd;
        iterator_->update_tx(cnn, cmd, bytes_transferred);
    }

    loop_end(idx_, std::ios::out);
}

void handle_connect::operator()(boost::system::error_code ec) const
{
    connection & cnn = connections_[idx_];
    cnn.writing = 0;
    if (ec) {
        Log(cerr) << cnn.uid << ec << "connect fail";
        cnn.err_c = ec;
    } else {
        Log(clog) << cnn.uid << cnn.local_endpoint(ec) <<"connected";
        iterator_->update_tx(cnn, 1, 0);

        cnn.rxbuf.reserve(4096);
        io::async_read(cnn, cnn.rxbuf.avail_buffer(), io::transfer_at_least(4), handle_read(idx_));
    }

    extern void async_connect();
    async_connect();
    loop_end(idx_, std::ios::trunc);
}

void handle_read::ack(UInt msgid, std::string const& sid) const
{
    if (msg_ack_)
    {
        connection & cnn = connections_[idx_];
        bool bempty = cnn.is_empty();

        std::string abuf = str(boost::format("{\"cmd\":95,\"body\":{\"msgid\":%1%,\"sid\":\"%2%\"}}") % msgid % sid);
        cnn.append_tx( 95, pack(abuf) );
        if (bempty)
        {
            extern void async_write(UInt idx);
            async_write(idx_);
        }
    }
}

void async_connect()
{
    if (idx2connect_ >= connections_.size())
        return;

    connection & cnn = connections_[idx2connect_];

    cnn.cmdexp_waiting_ = 0;
    cnn.cmdexp_ = 0;
    ++n_busy_;
    cnn.writing = 1;

    cnn.stage_at(0);
    cnn.async_connect(endpoint_, handle_connect(idx2connect_));
    ++idx2connect_;
}

void async_write(UInt idx)
{
    connection & cnn = connections_[idx];
    if (cnn.is_empty())
        return;

    cnn.cmdexp_waiting_ = 0;
    cnn.cmdexp_ = 0;

    int cmd = cnn.front().cmd;
    if (cmd >= 99)
    {
        cnn.cmdexp_waiting_ = 1;
        cnn.cmdexp_ = cmd;
        ++n_busy_;
    }

    ++n_busy_;
    cnn.writing = 1;
    io::async_write(cnn, cnn.const_buffer(), handle_write(idx));
}

void loop_end(UInt idx, int io_f)
{
    connection & cnn = connections_[idx];

    if (io_f == std::ios::in)
    {
        if (cnn.cmdexp_)
            if (!cnn.cmdexp_waiting_)
                --n_busy_;
    }
    else
    {
        --n_busy_;
    }

    if (!cnn.is_empty())
    {
        if (!cnn.writing && !cnn.cmdexp_waiting_) {
            cnn.consume_front();
            if (!cnn.is_empty()) {
                async_write(idx);
                return;
            }
        }
    }

    if (cnn.err_c)
    {
        iterator_->update_fail(cnn);
    }

    extern void stage_next();
    if (n_busy_ == 0)
        stage_next();
}

void stage_next()
{
    if (n_busy_ > 0)
        return;

    iterator_->report(cout);

    if (iterator_->name_.empty())
        return;

    ++iterator_;
    iterator_->init();

    // if (iterator_ != stages_.end())
    {
        int nc = connections_.size();
        for (int x=0; x < nc; ++x)
        {
            connection & cnn = connections_[x];
            if (!cnn.is_ok())
                continue;
            cnn.stage_at(std::distance(stages_.begin(), iterator_));
            async_write(x);
        }
    }
}

void stage_connect(UInt const nc)
{
    iterator_ = stages_.begin();
    n_busy_ = 0;
    n_bad_connection_ = 0;

    idx2connect_=0;

    iterator_->init();
    for (UInt x=0, n=std::min(nc, UInt(connections_.size()))
            ; x < n; ++x)
        async_connect();
}

void stage_data_init(int stx, std::string const& fn)
{
    using namespace std;

    boost::unordered_map<UInt, list<tx_data> > xl = parse_input_file(fn);

    BOOST_FOREACH(auto &p, xl)
    {
        auto it = m_uid_c_.find(p.first);
        if (it == m_uid_c_.end())
            BOOST_THROW_EXCEPTION( myerror() << errinfo_int(p.first) );
        connection & cnn = connections_[it->second];
        cnn.stage_data(stx, p.second);
    }
}

template <typename Fp> void stage_init(Fp beg, Fp end)
{
    using namespace std;

    boost::unordered_map<UInt, list<tx_data> > xl = parse_input_file(*beg);
    if (xl.empty())
        return;

    int stx = 0;
    connections_.reserve(xl.size());

    stages_.emplace_back(*beg);
    BOOST_FOREACH(auto & p, xl)
    {
        m_uid_c_.insert(std::make_pair(p.first, connections_.size()));
        connections_.emplace_back(io_service(), p.first);
        connection & cnn = connections_.back();

        list<tx_data> lx;
        lx.push_back(tx_data(1, std::string()));
        cnn.stage_data(stx, lx);
    }

    ++stx;
    ++beg;
    while (beg != end)
    {
        stages_.emplace_back(*beg);
        stage_data_init(stx, *beg);
        ++beg;
        ++stx;
    }
}

int init(int argc, char *const argv[])
{
    namespace opt = boost::program_options;

    std::string host("127.0.0.1");
    std::string port;
    std::vector<std::string> files;

    opt::options_description opt_desc("Options");
    opt_desc.add_options()
        ("help", "show this")
        //("threads", opt::value<unsigned int>(&n_thread_)->default_value(n_thread_), "N threads")
        //("close", opt::value<bool>(&close_)->default_value(true), "close socket after request")
        ("ack", opt::value<bool>(&msg_ack_)->implicit_value(1), "ack message")
        ("heartbeat,b", opt::value<int>(&heartbeat_tv_)->default_value(heartbeat_tv_), "heartbeat interval")
        ("host,h", opt::value<std::string>(&host)->default_value(host), "host/ip-address")
        ("port,p", opt::value<std::string>(&port)->required(), "port")
        ("file", opt::value<std::vector<std::string> >(&files)->required(), "files")
        ;

    opt::positional_options_description pos_desc;
    pos_desc.add("file", -1);

    opt::variables_map vm;
    opt::store(opt::command_line_parser(argc, argv)
            .options(opt_desc)
            .positional(pos_desc)
            .run(), vm);

    if (vm.count("help"))
    {
        std::cout << boost::format("Usage:\n  %1% [Options] --host=<host> --port=<port> <ids> <cmds files...>\n") % argv[0]
            << opt_desc
            ;
        exit(0);
    }

    opt::notify(vm);

    if (files.empty())
        BOOST_THROW_EXCEPTION( myerror() << errinfo_str("args error") );

    stages_.reserve(boost::size(files) + 1);
    stage_init(boost::begin(files), boost::end(files) );
    stages_.emplace_back(std::string());

    {
        ip::tcp::resolver resolver(io_service());
        ip::tcp::resolver::query query(host, port);
        endpoint_ = *resolver.resolve( query );
    }
    Log(clog) << files <<" "<< endpoint_;

    return 0;
}

std::ostream &operator<<(std::ostream& out, std::list<std::string> const& l)
{
    BOOST_FOREACH(auto & x, l)
        out << x;
    return out;
}

void timed_report()
{
    stage_status::s_report(cout);

    cout.flush();
    clog.flush();
    cerr.flush();
}

void timed_heartbeat()
{
    if (!iterator_->name_.empty())
        return;

    size_t nc = connections_.size();
    for (size_t x = 0; x < nc; ++x)
    {
        connection & cnn = connections_[x];
        if (!cnn.is_ok())
            continue;
        if (!cnn.is_empty())
            continue;
        cnn.append_tx( 91, pack("{\"cmd\":91}") );
        async_write(x);
    }
}

int main(int argc, char *const argv[])
{
    // std::clog.rdbuf(std::cout.rdbuf());
    init(argc, argv);

    boost::asio::signal_set signals(io_service(), SIGHUP, SIGUSR1);
    signals.async_wait([](boost::system::error_code, int) {
                std::cout.flush();
                std::cerr.flush();
            });

    boost::asio::signal_set sigstop(io_service(), SIGINT, SIGTERM, SIGQUIT);
    sigstop.async_wait([](boost::system::error_code, int) { io_service().stop(); });

    stage_connect(128);

    boost::asio::deadline_timer timer_report(io_service());
    {
        simple_timer_loop stl(timer_report, 45, &timed_report );
        stl.wait(45);
    }
    boost::asio::deadline_timer timer_hbeat(io_service());
    {
        simple_timer_loop stl(timer_hbeat, heartbeat_tv_, &timed_heartbeat);
        stl.wait(heartbeat_tv_);
    }

    io_service().run();

    stage_status::s_report(cout);

    std::cout << "bye.\n";
    std::cout.flush();
    std::cerr.flush();

    // ptime pt_end = microsec_clock::local_time();

    //std::clog << "[done]\n";
    //std::clog << "total requests: " << "" <<"\n";
    //std::clog << "total milliseconds used: " << (pt_end - st_.pt_begin).total_milliseconds() <<"\n";

    //if (post_bytes > 0) std::clog << "total bytes posted: " << post_bytes <<"\n";

    return 0;
}

// void send_notification(UInt uid, UInt msgid, std::string const& alert, time_t tpex) {}

