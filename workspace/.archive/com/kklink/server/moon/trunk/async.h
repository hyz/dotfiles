#ifndef ASYNC_H__
#define ASYNC_H__

#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> Socket_ptr;
typedef Socket_ptr socket_ptr;

inline std::ostream & operator<<(std::ostream & out, const socket_ptr & s)
{
    if (!s)
    {
        return out << "Null";
    }
    if (!s->is_open())
    {
        return out << "sa:" << s.get();
    }

    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endp = s->remote_endpoint(ec);
    if (ec)
        return out << s->native_handle() <<",error:" << ec.value();
    return out << s->native_handle() <<","<< endp;
}

// std::ostream & operator<<(std::ostream & out, boost::asio::ip::tcp::socket const & s);
template <typename Out>
Out & operator<<(Out & out, boost::asio::ip::tcp::socket const & s)
{
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endp = s.remote_endpoint(ec);
    return out << s.native_handle() <<","<< endp;
}


struct Writer_impl : boost::noncopyable, boost::enable_shared_from_this<Writer_impl>
{
    Writer_impl(Socket_ptr soc, boost::function<void (Socket_ptr)> fin);

    ~Writer_impl();

    void send(const std::string& s);

private:
    Socket_ptr soc_;
    std::list<std::string> ls_;

    unsigned int cnt_;
    unsigned int size_;
    boost::function<void (Socket_ptr)> fin_;

    void writeb(const boost::system::error_code& err);
};

struct Writer
{
    typedef Writer_impl Worker;
    typedef boost::shared_ptr<Writer_impl> Worker_ptr;

    Writer(/*boost::asio::io_service& io_service*/) {}
    ~Writer();

    boost::function<void (const std::string&)> associate(Socket_ptr soc);

    void finish(Socket_ptr soc);

private:
    typedef std::map<boost::asio::ip::tcp::socket::native_type, boost::weak_ptr<Worker> > Map;
    typedef Map::iterator iterator;

    Map workers_;
};


struct Acceptor
{
    Acceptor(boost::asio::io_service& io_service);

    void listen(const std::string& addr, const std::string& port, boost::function<void (Socket_ptr)> fn);

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    Socket_ptr socket_;

    boost::function<void (Socket_ptr)> fn_;

private:
    void handle_accept(const boost::system::error_code& e);

    void _error(const boost::system::error_code& err);
};

namespace net {

    typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

    typedef Acceptor acceptor;
    typedef Writer writer;

    template <typename Protoc>
    struct reader
    {
        reader(boost::asio::io_service& io_service
                , boost::function<bool (Socket_ptr, typename Protoc::DataType&)> fn)
            : handler_(fn)
        {}

        void accept(Socket_ptr soc)
        {
            boost::shared_ptr<Protoc> wk(new Protoc(soc, handler_));
            wk->start();
        }

        boost::function<bool (Socket_ptr, typename Protoc::DataType&)> handler_;

        std::vector<Socket_ptr> soclist_;
    };

}

struct monitor_socket
{
    static const char *filename() { return "/tmp/yx-monitor-socket"; }
    mutable boost::shared_ptr<boost::filesystem::ofstream> outfile_;

    monitor_socket()
        : outfile_(new boost::filesystem::ofstream(filename(), std::ios::app))
    { //{ list_last(); }
    }

    template <typename ...A> void operator()(A... a)
    {
        // std::vector<std::string> v;
        // cat(v, a...);
        // BOOST_FOREACH(auto const & x , v)
        //     *outfile_ << x << "\t";
        // *outfile_ << "\n";
        // outfile_->flush();
    }

    template <typename C, typename X> void cat(C& v, X const & x)
    {
        std::ostringstream oss;
        oss << x;
        v.push_back(oss.str()); //( boost::lexical_cast<std::string>(x) );
    }

    template <typename C, typename X, typename ...A> void cat(C& v, X const& x, A... a)
    {
        cat(v, x);
        cat(v, a...);
    }

#if 0
    std::string list_last() const
    {
        if (outfile_)
        {
            outfile_->flush();
            outfile_.reset();
        }

        std::map<int, std::string> lines;
        {
            std::ifstream xf(filename());
            std::string line;
            while (getline(xf, line))
            {
                UID uid = atoi(line.c_str());
                if (!uid)
                    continue;

                std::istringstream fis(line);
                std::string tds; // = "<td>" + time_string() + "</td>";
                std::string tmps;
                while (getline(fis, tmps, '\t'))
                    tds += "<td>" + tmps + "</td>";

                lines[uid] = tds;
            }
        }

        outfile_.reset(new boost::filesystem::ofstream(filename(), std::ios::app));

extern time_t tp_startup_;
        std::string hds = "<table><tr>"
            "<td>服务器时间：</td>"
            "<td>" + time_string() + "</td>"
            "<td>服务器启动时间：</td>"
            "<td>" + time_string(tp_startup_) + "</td>"
            "</tr></table>";
        std::string tbody = "<tr>"
            "<td>用户ID</td>"
            "<td>用户昵称</td>"
            "<td>酒吧ID</td>"
            "<td>是否后台</td>"
            "<td>是否socket在线</td>"
            "<td>最后socket ip:port</td>"
            "<td>最后socket创建</td>"
            "<td>最后socket活跃</td>"
            "<td>最后socket发送</td>"
            "<td>最后活跃</td>"
            "<td>apple device id</td>"
            "</tr>";
        BOOST_FOREACH(auto const & l , lines)
            tbody += "<tr>" + l.second + "</tr>";
        return hds + "<table border=\"1\" bordercolor=\"#000000\">" + tbody + "</table>";
    }
#endif
};

#endif

