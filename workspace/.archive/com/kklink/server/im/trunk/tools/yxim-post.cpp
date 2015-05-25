#include <string>
#include <list>
#include <iostream>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>
// #include <curl/curl.h>
#include "myerror.h"

// int count = 0;
boost::mutex mutex;
boost::mutex mutex_log_;

struct Log : boost::unique_lock<boost::mutex>
{
    template <typename X> Log const& operator<<(X const& x) const
    {
        *outs <<" "<< x;
        return *this;
    }
    Log(std::ostream & s)
        : boost::unique_lock<boost::mutex>(mutex_log_)
        , outs(&s)
    { *outs << idx_++; }
    ~Log() { *outs <<"\n"; }
    std::ostream * outs;
    static int idx_;
};
int Log::idx_ = 0;
using std::cout;
using std::clog;
using std::cerr;

struct request_info
{
    std::string data;
    size_t size() const { return data.size(); }

    static request_info make(std::string const& line);
};

request_info request_info::make(std::string const& line)
{
    request_info ret;
    ret.data.reserve(4 + line.size());

    auto it = line.begin();
    auto end = line.end();
    it = std::find(it, end, '\t');
    if (it != end)
        if ( (it = std::find(it+1, end, '\t')) != end)
            ++it;

    union { uint32_t len; char s[sizeof(uint32_t)]; } un;
    un.len = htonl(end - it);

    ret.data = std::string(un.s, sizeof(un)) + std::string(it, end);

    return ret;
}

std::list<request_info> requests_;

bool take_request(request_info& req)//(std::string & url, std::string& cook, std::string & cont)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    if (requests_.empty())
        return 0;
    std::swap(requests_.front(), req);
    requests_.pop_front();
    return 1;
}

namespace io = boost::asio;
namespace ip = boost::asio::ip;

ip::tcp::endpoint endpoint_;
std::string file_;
unsigned int n_thread_ = 10;
bool close_ = true;
int wait_seconds_ = 0;
size_t n_fail_ = 0;
size_t n_success_ = 0;
size_t n_rx_bytes_ = 0;
size_t n_tx_bytes_ = 0;

//typedef boost::error_info<struct rhd,size_t> errinfo_index;

int send_request(ip::tcp::socket& s, request_info const& req)
{
    io::write(s, io::buffer(req.data));

    union { uint32_t len; char s[sizeof(uint32_t)]; } un;

    size_t nread = io::read(s, io::buffer(un.s, sizeof(un)));
    if (nread == sizeof(un))
    {
        std::vector<char> buf(ntohl(un.len));
        size_t nbyte_ob = io::read(s, io::buffer(buf));
        if (nbyte_ob == buf.size())
        {
            Log(cout) << boost::make_iterator_range(&buf[0], &buf[0] + nbyte_ob);
            return 4+nbyte_ob;
        }

        Log(cerr) << "read body fail:" << nbyte_ob <<" expected:"<< buf.size();
    }
    else
    {
        Log(cerr) << "read head fail:" << nread <<" expected:"<< sizeof(un);
    }
    return -1;
}

void thread_loop()
{
    io::io_service io_s;

    request_info req;
    while (take_request(req))
    {
        try {
            ip::tcp::socket sk(io_s); // if (!sk.is_open())
            sk.connect(endpoint_); // io::connect( sk, endpoint_iter_ );
            int n;
            if ( (n = send_request(sk, req)) >= 0)
            {
                ++n_success_;
                n_rx_bytes_ += n;
                n_tx_bytes_ += req.size();
                continue;
            }
            // if (close_) sk.close();
            // if (wait_seconds_ > 0) sleep(wait_seconds_);
        } catch (myerror const& e) {
            Log(cerr) << e;
        } catch (std::exception const& e) {
            Log(cerr) << e.what();
        }
        ++n_fail_;
    }
}

size_t init_reqlist(std::list<request_info>& reqs, std::string const& file)
{
    boost::filesystem::ifstream infile(file);
    if (!infile)
        BOOST_THROW_EXCEPTION( myerror() << boost::errinfo_file_name(file) );

    size_t bytes = 0;

    std::string line;
    while (getline(infile, line))
    {
        auto it = reqs.insert(reqs.end(), request_info::make(line) );
        bytes += it->data.size();
    }

    return bytes;
}

int args(int argc, char *const argv[])
{
    namespace po = boost::program_options;
    std::string host;
    std::string port;

    po::options_description opt_desc("Options");
    opt_desc.add_options()
        ("help", "show this")
        ("threads", po::value<unsigned int>(&n_thread_)->default_value(n_thread_), "N threads")
        // ("close", po::value<bool>(&close_)->default_value(true), "close socket after request")
        // ("wait,w", po::value<int>(&wait_seconds_)->default_value(0), "wait seconds in thread loop")
        ("host,h", po::value<std::string>(&host)->required(), "host/ip-address")
        ("port,p", po::value<std::string>(&port)->required(), "port")
        ("file", po::value<std::string>(&file_)->required(), "data file")
        ;

    po::positional_options_description pos_desc;
    pos_desc.add("file", 1);
    // pos_desc.add("host", 1); pos_desc.add("port", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
            .options(opt_desc)
            .positional(pos_desc)
            .run(), vm);

    if (vm.count("help"))
    {
        std::cout << boost::format("Usage:\n  %1% [Options] <file> <host> <port>\n") % argv[0]
            << opt_desc
            ;
        exit(0);
    }

    po::notify(vm);

    {
        io::io_service io_s;
        ip::tcp::resolver resolver(io_s);
        ip::tcp::resolver::query query(host, port);
        endpoint_ = *resolver.resolve( query );
    }
    return 0;
}

int main(int argc, char *const argv[])
{
    using namespace boost::posix_time;

    args(argc, argv);
    std::cout << file_ <<" "<< endpoint_ <<" "<< n_thread_ <<"\n";

    size_t post_bytes = init_reqlist(requests_, file_);
    size_t n_request = requests_.size();

    ptime pt_begin = microsec_clock::local_time();

    boost::thread_group threads;
    if (n_thread_ > 0)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        for (unsigned int i = 0; i < n_thread_; ++i)
            threads.create_thread(&thread_loop);
        pt_begin = microsec_clock::local_time();
    }

    if (n_thread_ == 0)
        exit(0);
    // std::cout << "threads runing ...\n";

    threads.join_all();
    ptime pt_end = microsec_clock::local_time();

    Log(clog) << "done.";
    Log(clog) << "threads:" << n_thread_;
    Log(clog) << "milliseconds:" << (pt_end - pt_begin).total_milliseconds();
    Log(clog) << "n request:" << n_request;
    Log(clog) << "n fail:" << n_fail_;
    Log(clog) << "n success:" << n_success_;
    Log(clog) << "rx bytes:" << n_rx_bytes_;
    Log(clog) << "tx bytes:" << n_tx_bytes_;
    Log(clog) << "total bytes:" << post_bytes;

    return 0;
}

