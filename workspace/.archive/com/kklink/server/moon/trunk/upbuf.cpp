#include "myconfig.h"
#include <boost/format.hpp>
#include <boost/algorithm/string/find.hpp>
#include "upbuf.h"

using boost::format;
using boost::algorithm::ifind_first;
using boost::algorithm::find_first;

typedef unsigned int UID;

void union_pay_notify(UID uid, int pid
        , std::string const & merid
        , std::string const &trade_no, std::string const &out_trade_no);

void upbuf::operator()(int freq)
{
}

void upbuf::event(int v)
{
    if (v == 1) // connected
        rbuf_.clear();

    if (v > 0)
        sl_.push_back("GET /-pull HTTP/1.1\r\n"
                "Host: 127.0.0.1\r\n"
                "Content-Length: 0\r\n"
                "\r\n"
                );
}

boost::system::error_code upbuf::on_data(int lev)
{
    boost::iterator_range<std::string::iterator> eoh = find_first(rbuf_, "\r\n\r\n");
    if (boost::empty(eoh))
    {
        event(lev > 0 ? 9 : 0);
        return boost::system::error_code();
    }
    LOG_I << boost::asio::const_buffer(rbuf_.data(), eoh.end()-rbuf_.begin());

    boost::iterator_range<std::string::iterator> cl = ifind_first(rbuf_, "content-Length");
    if (boost::empty(cl))
    {
        return make_error_code(boost::system::errc::bad_message);
    }

    size_t clen = atoi(&cl.end()[2]);
    LOG_I << "content-Length " << clen <<" "<< rbuf_.size() - (eoh.end() - rbuf_.begin());
    if (eoh.end() + clen > rbuf_.end())
    {
        event(lev > 0 ? 9 : 0);
        return boost::system::error_code();
    }

    boost::iterator_range<std::string::iterator> body(eoh.end(), eoh.end() + clen);
    LOG_I << body;

    json::object js = json::decode(body.begin(), body.end());
    // {"_":"/bills","id_":24,"channel":2,"uid":10006,"pid":10,"merid":"880000000000652","trade_no":"201311111440450431331","out_trade_no":"18s2716sas"}

    //int id_ = j.get<int>("id_"); js.get<int>("channel", 2);
    int uid = js.get<int>("uid");
    int pid = js.get<int>("pid");
    std::string merid = js.get<std::string>("merid");
    std::string trade_no = js.get<std::string>("trade_no");
    std::string out_trade_no = js.get<std::string>("out_trade_no");

    LOG_I << uid <<" "<< pid;
    union_pay_notify(uid, pid, merid, trade_no, out_trade_no);
    LOG_I << __LINE__;

    LOG_I << boost::asio::buffer(rbuf_.data(), body.end() - rbuf_.begin());
    rbuf_.erase(rbuf_.begin(), body.end());

    return on_data(lev+1);
}

