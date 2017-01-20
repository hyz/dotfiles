#include <iostream>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
//#include "json.hpp"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
namespace posix = boost::asio::posix;

template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void err_msg_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
}
#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
template <typename... Args> void dbg_msg_(int lin_, char const* fmt, Args... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
    //fflush(stdout);
}
#define DBG_MSG(...) dbg_msg_(__LINE__, "%d: " __VA_ARGS__)
#define ensure(c, ...) if(!(c))ERR_EXIT(__VA_ARGS__)

//DepthMarketData CThostFtdcDepthMarketDataField
struct pstruct
{
    static int ready(char* beg,char* end) {
        int h = *reinterpret_cast<int*>(beg);
        int len = h & 0x0ffffff;
        if (end - beg < sizeof(int)+len)
            return 0;
        return (h>>24) & 0x0ff;
    }
    template <typename T> static T* body(char* beg) {
        int* b = reinterpret_cast<int*>(beg);
        return reinterpret_cast<T*>(b+1);
    }
    static char* next(char* beg) {
        int* b = reinterpret_cast<int*>(beg);
        int len = (*b) & 0x0ffffff;
        return reinterpret_cast<char*>(b + (sizeof(int)+ len+sizeof(int)-1)/sizeof(int));
    }

    static void pad(char* pos, char* end) {
        ensure(static_cast<ptrdiff_t>(end - (char*)0)%sizeof(int)==0, "align fault");
        if (pos < end) {
            int n = static_cast<ptrdiff_t>(end-(char*)0) % sizeof(int);
            end = pos + (sizeof(int) - n);
            while (pos != end)
                *pos++ = '\0';
        }
    }
};

struct fifo_buffer : std::array<int,8192>
{
    int* data_() const { return const_cast<fifo_buffer*>(this)->std::array<int,8192>::data(); }
    int  size_() const { return std::array<int,8192>::size(); }
    int bytes_len_ = 0;

    void clear() { bytes_len_ = 0; }
    void commit(int len) { bytes_len_ += len; }
    int consume_to(char* pc) {
        ensure(static_cast<ptrdiff_t>(pc-(char*)0)%sizeof(int)==0, "align fault");
        int* pos = reinterpret_cast<int*>(pc);
        int nb = (pos - data_())*sizeof(int);
        bytes_len_ -= nb; //sizeof(int)*(size_() - (pos - data_()));
        std::memmove(data_(), pos, bytes_len_);
        return nb;
    }
    char* data() const { return (char*)data_(); }
    int   size() const { return bytes_len_; }
    int size_avail() const { return size_()*sizeof(int) - bytes_len_; }
};

struct Minuted : std::vector<CThostFtdcDepthMarketDataField>
{
    Minuted() {
        reserve(60*4+30);
    }

    unsigned minute0 = 0;

    unsigned get_minute_index(CThostFtdcDepthMarketDataField* ptr) {
        unsigned min = minute_of_day(ptr->UpdateTime);
        if (minute0 == 0)
            minute0 = min;
        return min - minute0;
    }
    unsigned minute_of_day(char* p) {
        auto next_to = [](char* p, char x) {
            while (*p && *p++ != x)
                ;
            return p;
        };
        unsigned mx = atoi(p)*60;
        p = next_to(p, ':');
        mx += atoi(p);
        p = next_to(p, ':');
        if (atoi(p) > 0)
            ++mx;
        return mx;
    }
};


template <typename T> void json_dump(CThostFtdcDepthMarketDataField const& md, T& ret)
{
#if 0
    nlohmann::json j = {
        { "InstrumentID", md.InstrumentID }
        , { "TradingDay", md.TradingDay }
        , { "UpdateTime", md.UpdateTime }
        , { "LastPrice", md.LastPrice }
        , { "PreSettlementPrice", md.PreSettlementPrice }
        , { "SettlementPrice", md.SettlementPrice }
        , { "HighestPrice", md.HighestPrice }
        , { "LowestPrice", md.LowestPrice }
        , { "Volume", md.Volume }
        , { "Turnover", md.Turnover }
        , { "OpenInterest", md.OpenInterest }
        , { "AskPrice1", md.AskPrice1 }
        , { "AskVolume1", md.AskVolume1 }
        , { "BidPrice1", md.BidPrice1 }
        , { "BidVolume1", md.BidVolume1 }
    };
    ret = j.dump();
#else
    rapidjson::Writer<rapidjson::StringBuffer> writer(ret);
    writer.StartObject();               // Between StartObject()/EndObject(), 
        writer.Key("InstrumentID")      ; writer.String(md.InstrumentID);
        writer.Key("TradingDay")        ; writer.String(md.TradingDay);
        writer.Key("UpdateTime")        ; writer.String(md.UpdateTime);
        writer.Key("LastPrice")         ; writer.Double(md.LastPrice);
        writer.Key("PreSettlementPrice"); writer.Double(md.PreSettlementPrice);
        writer.Key("SettlementPrice")   ; writer.Double(md.SettlementPrice);
        writer.Key("HighestPrice")      ; writer.Double(md.HighestPrice);
        writer.Key("LowestPrice")       ; writer.Double(md.LowestPrice);
        writer.Key("Volume")            ; writer.Int(md.Volume);
        writer.Key("Turnover")          ; writer.Double(md.Turnover);
        writer.Key("OpenInterest")      ; writer.Double(md.OpenInterest);
        writer.Key("AskPrice1")         ; writer.Double(md.AskPrice1);
        writer.Key("AskVolume1")        ; writer.Int(md.AskVolume1);
        writer.Key("BidPrice1")         ; writer.Double(md.BidPrice1);
        writer.Key("BidVolume1")        ; writer.Int(md.BidVolume1);
        // writer.Key("a"); writer.StartArray(); writer.EndArray();
    writer.EndObject();
#endif
        //<<" 合约:"<<pDepthMarketData->InstrumentID
        //<<" 日期:"<<pDepthMarketData->TradingDay
        //<<" 时间:"<<pDepthMarketData->UpdateTime
        //<<" 毫秒:"<<pDepthMarketData->UpdateMillisec
        //<<" 现价:"<<pDepthMarketData->LastPrice
        //<<" 上次结算价:" << pDepthMarketData->PreSettlementPrice
        //<<" 本次结算价:" << pDepthMarketData->SettlementPrice
        //<<" 最高价:" << pDepthMarketData->HighestPrice
        //<<" 最低价:" << pDepthMarketData->LowestPrice
        //<<" 数量:" << pDepthMarketData->Volume
        //<<" 成交金额:" << pDepthMarketData->Turnover
        //<<" 持仓量:" << pDepthMarketData->OpenInterest
        //<<" 卖一价:" << pDepthMarketData->AskPrice1
        //<<" 卖一量:" << pDepthMarketData->AskVolume1
        //<<" 买一价:" << pDepthMarketData->BidPrice1
        //<<" 买一量:" << pDepthMarketData->BidVolume1
}

struct init_helper_ {
    template <typename T> init_helper_(T* thiz) {
        thiz->set_access_channels(websocketpp::log::alevel::all);
        thiz->clear_access_channels(websocketpp::log::alevel::frame_payload);
        thiz->init_asio();
    }
};

struct Main : websocketpp::server<websocketpp::config::asio>, init_helper_, boost::noncopyable
{
    struct connection_stat { int mdp=0; bool writing=0; };
    std::map<websocketpp::connection_hdl,connection_stat,std::owner_less<websocketpp::connection_hdl>> connections_;

    const char* fifo_path_;
    posix::stream_descriptor fifo_sd_;
    boost::asio::deadline_timer fifo_deadline_;
    fifo_buffer fifo_buffer_;

    std::map<std::string,Minuted> mds_; //Minuted mdvec_;

    ~Main(){}
    Main(int argc, char* const argv[]);

    void run(int argc, char* const argv[])
    {
        //fifo_start_read();
        start_accept();
        websocketpp::server<websocketpp::config::asio>::run();
    }

    void on_open(websocketpp::connection_hdl hdl)
    {
        connections_.emplace(hdl, connection_stat{}); // TODO: move
    }
    void on_close(websocketpp::connection_hdl hdl)
    {
        connections_.erase(hdl);
    }
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg)
    {
        // TODO
        std::cerr << "on_message:hdl msg: " << hdl.lock().get() <<" "<< msg->get_payload() << std::endl;
        //if (msg->get_payload() == "stop-listening") {
        //    this->stop_listening();
        //    return;
        //}
        //try {
        //    this->send(hdl, msg->get_payload(), msg->get_opcode());
        //} catch (const websocketpp::lib::error_code& e) {
        //    std::cerr << "Echo failed because: " << e
        //              << "(" << e.message() << ")" << std::endl;
        //}
    }

    void fifo_reopen(boost::system::error_code ec);
    void fifo_reset()
    {
        boost::system::error_code ec;
        fifo_sd_.close(ec);
        fifo_buffer_.clear();;
    }
    void fifo_start_read();
    void fifo_on_data_incoming();

    Minuted& newmd_save(CThostFtdcDepthMarketDataField* mdptr);
    void newmd_bcast(Minuted const& md);
};

void Main::fifo_start_read()
{
    fifo_sd_.async_read_some(boost::asio::buffer(fifo_buffer_.data()+fifo_buffer_.size(), fifo_buffer_.size_avail()),
        [this](boost::system::error_code ec, std::size_t length) {
            if (ec) {
                ERR_MSG("fifo read: %s", ec.message().c_str()); //std::clog << "fifo read: " << ec << ": " << ec.message() <<std::endl;
                fifo_reset();
                fifo_deadline_.expires_from_now(boost::posix_time::seconds(1));
                fifo_deadline_.async_wait([this](boost::system::error_code ec){ fifo_reopen(ec); });
            } else {
                // DBG_MSG("fifo read:", length);
                fifo_buffer_.commit(length);
                fifo_on_data_incoming();
                fifo_start_read();
            }
        });
}
void Main::fifo_on_data_incoming()
{
    char* beg = fifo_buffer_.data();
    char* end = fifo_buffer_.data() + fifo_buffer_.size();
    int dt;
    while (beg+sizeof(int) <= end && (dt = pstruct::ready(beg,end)) >= 0) {
        switch (dt) {
            case 11: //行情 | 合约:ag1606 日期:20160330 时间:22:35:44 毫秒:500 现价:3405 上次结算价:3408 本次结算价:1.79769e+308 最高价:3417 最低价:3392 数量:155906 成交金额:7.96867e+09 持仓量:594322 卖一价:3405 卖一量:136 买一价:3404 买一量:94 持仓量:594322
                newmd_bcast( newmd_save(pstruct::body<CThostFtdcDepthMarketDataField>(beg)) );
                break;
            case 0:
                // DBG_MSG("fifo: 0");
                break;
        }
        beg = pstruct::next(beg);
    }
    if (beg != fifo_buffer_.data()) {
        int nb = fifo_buffer_.consume_to(beg); (void)nb;
        DBG_MSG("fifo consume: %d", nb);
    }
}
Minuted& Main::newmd_save(CThostFtdcDepthMarketDataField* mdptr)
{
    auto it = mds_.find( mdptr->InstrumentID );
    if (it == mds_.end()) {
        auto p = mds_.emplace(mdptr->InstrumentID, Minuted());
        if (!p.second) ERR_EXIT("newmd_save");
        it = p.first;
    }
    auto& md = it->second;
    unsigned minx = md.get_minute_index(mdptr);
    if (minx >= md.size())
        md.resize(minx+1);
    memcpy(&md[minx], mdptr, sizeof(*mdptr));
    return md;
}
void Main::newmd_bcast(Minuted const& md)
{
    for (auto& c : connections_) {
        auto& hdl = c.first;
        auto& cs = c.second;
        for (unsigned i=cs.mdp; i < md.size(); ++i) {
            websocketpp::lib::error_code ec;
            rapidjson::StringBuffer jsb;
            json_dump(md[i], jsb);
            this->send(hdl, jsb.GetString(), jsb.GetSize(), websocketpp::frame::opcode::text, ec);
            if (ec) {
                std::cerr << "send: " << ec.message() << std::endl;
                //TODO: what-about on_close(hdl);
            } else {
                cs.mdp = i;
            }
        }
    }
}
void Main::fifo_reopen(boost::system::error_code ec)
{
    if (boost::asio::deadline_timer::traits_type::now() >= fifo_deadline_.expires_at()) {
        int fd = open(fifo_path_, O_NONBLOCK|O_RDONLY);
        //DBG_MSG("fifo_reopen:open: %d", fd);
        if (fd >= 0) {
            fifo_sd_ = std::move(posix::stream_descriptor(get_io_service(),fd));
            fifo_start_read();
            fifo_deadline_.expires_at(boost::posix_time::pos_infin);
        } else {
            fifo_deadline_.expires_from_now(boost::posix_time::seconds(2));
        }
        fifo_deadline_.async_wait([this](boost::system::error_code ec){ fifo_reopen(ec); });
    }
}

Main::Main(int argc, char* const argv[])
     : init_helper_(this), fifo_sd_(get_io_service()), fifo_deadline_(get_io_service())
{
    ensure(argc>1, "argv[1]");
    fifo_path_ = argv[1];
    fifo_deadline_.expires_at(boost::asio::deadline_timer::traits_type::now());
    fifo_deadline_.async_wait([this](boost::system::error_code ec){ fifo_reopen(ec); });

    set_message_handler(bind(&Main::on_message,this,::_1,::_2));
    set_open_handler(bind(&Main::on_open,this,::_1));
    set_close_handler(bind(&Main::on_close,this,::_1));
    int port = 9002;
    if (argc>2)
        port = atoi(argv[2]);
    listen(port);
}

static long Mem_[(sizeof(Main)+sizeof(long)-1)/sizeof(long)];
struct Wmain {
    Wmain(int argc, char* const argv[]) {
        new (&Mem_) Main(argc, argv);
    }
    ~Wmain() {
        reinterpret_cast<Main*>(&Mem_)->Main::~Main();
    }
    void run(int argc, char* const argv[]) {
        reinterpret_cast<Main*>(&Mem_)->run(argc, argv);
    }
};

int main(int argc, char* const argv[])
{
    try {
        Wmain wm(argc, argv); //static Main serv(argc, argv);
        wm.run(argc, argv);
    } catch (websocketpp::exception const & e) {
        ERR_MSG("%s", e.what());
    } catch (...) {
        ERR_MSG("exception");
    }
}

