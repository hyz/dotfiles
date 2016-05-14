// #include <sys/types.h> #include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/signals2/signal.hpp>
//#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <chrono> //#include <boost/chrono.hpp>
namespace chrono = std::chrono;
#if defined(__ANDROID__)
#  include <regex> //<boost/regex.hpp>
namespace re = std;
#else
#  include <boost/regex.hpp>
namespace re = boost;
#endif

#if defined(__ANDROID__)
#  include <android/log.h>
#  define  LOG_TAG    "HGSC"
#  define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#  define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#  define ERR_EXIT(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#  define NOT_PRINT_PROTO 1
#else

template <typename... As> void err_exit_(int lin_, char const* fmt, As... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... As> void err_msg_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
}
#  define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
#  define LOGD(...) err_msg_(__LINE__, "D:%d: " __VA_ARGS__)
#  define LOGE(...) err_msg_(__LINE__, "E:%d: " __VA_ARGS__)
#endif

enum { BUFFER_FLAG_CODEC_CONFIG=2 };
extern "C" {
    void hgs_h264slice_inflate(int timestamp, char* p, size_t len);
    void hgs_h264slice_commit(int timestamp, int flags);

    void hgs_poll_once();
    void hgs_exit(int);
    void hgs_init();
}

template <typename I1, typename I2>
void base64dec(I1 beg, I1 end, I2 out_it)
{
    while (end != beg && *(end-1) == '=')
        --end;
    using namespace boost::archive::iterators;
    using Iter = transform_width<binary_from_base64<I1>,8,6>;
    std::copy(Iter(beg), Iter(end), out_it);
    //return boost::algorithm::trim_right_copy_if( std::string(Iter(beg), Iter(end)), [](char c) { return c == '\0'; });
}
template <typename I1, typename I2>
void base64enc(I1 beg, I1 end, I2 out_it) {
    using namespace boost::archive::iterators;
    using Iter = base64_from_binary<transform_width<I1,6,8>>;
    std::copy(Iter(beg), Iter(end), out_it);
}

template <typename Int,int> struct Ntoh_;
template <typename Int> struct Ntoh_<Int,4> { static Int cast(Int val) { return ntohl(val); } };
template <typename Int> struct Ntoh_<Int,2> { static Int cast(Int val) { return ntohs(val); } };
//template <typename Int> struct Ntoh_<Int,1> { static Int cast(Int val) { return (val); } };
template <typename Int> Int Ntoh(uint8_t* data,uint8_t* end)
{
    if (size_t(end-data) < sizeof(Int)) {
        ERR_EXIT("Ntoh"); return 0;
    }
    Int val;
    memcpy(&val, data, sizeof(Int));
    return Ntoh_<Int,sizeof(Int)>::cast(val);
}

namespace ip = boost::asio::ip;

static char* xsfmt(char xs[],unsigned siz, uint8_t*data,uint8_t*end)
{
    int len=std::min(16,int(end-data));
    for (int j=0, i=0; j < (int)siz && i < len; ++i)
        j += snprintf(&xs[j],siz-j, ((i>1&&i%2==0)?" %02x":"%02x"), (int)data[i]);
    return xs;
}

struct rtp_header
{
    BOOST_STATIC_ASSERT(__BYTE_ORDER == __LITTLE_ENDIAN);

    uint8_t cc:4;         // CSRC count
    uint8_t x:1;          // header extension flag
    uint8_t p:1;          // padding flag
    uint8_t version:2;    // protocol version

    uint8_t pt:7;         // payload type
    uint8_t m:1;          // marker bit
    uint16_t seq;         // sequence number, network order

    uint32_t timestamp;     // timestamp, network order
    uint32_t ssrc;          // synchronization source, network order
    uint32_t csrc[1];        // optional CSRC list

    static rtp_header* cast(uint8_t* data, uint8_t* end) {
        if (data + sizeof(rtp_header)-4 > end)
            return nullptr;
        rtp_header* h = reinterpret_cast<rtp_header*>(data);
        if (data + h->length() > end)
            return nullptr;
        h->seq = ntohs(h->seq);
        h->timestamp = ntohl(h->timestamp) / 10; //XXX:  millseconds
        h->ssrc = ntohl(h->ssrc);
        for (unsigned i=0; i < h->cc; ++i)
            h->csrc[i] = ntohl(h->csrc[i]);
        return h;
    }
    unsigned length() const { return sizeof(rtp_header)-4 + 4*this->cc; }

    void print(uint8_t* data, uint8_t* end) {
#if !defined(NOT_PRINT_PROTO)
        char xs[128] = {};
        printf("%4u:%u: version %d p %d x %d cc %d pt %d seq %d: %s\n"
                , int(end-data), this->length()
                , this->version, this->p, this->x, this->cc, this->pt, this->seq
                , xsfmt(xs,sizeof(xs), data,end));
#endif
    }
};

struct nal_unit_header
{
    uint8_t type:5;
    uint8_t nri:2;
    uint8_t f:1;

    static nal_unit_header* cast(uint8_t* data, uint8_t* end) {
        if (data+1 > end)
            return nullptr;
        return reinterpret_cast<nal_unit_header*>(data);
    }
    unsigned length() const { return 1; }

    void print(uint8_t* data, uint8_t* end) {
#if !defined(NOT_PRINT_PROTO)
        char xs[128] = {};
        printf("%4u:%u: f %d nri %d type %d: %s\n"
                , int(end-data), this->length()
                , this->f, this->nri, this->type
                , xsfmt(xs,sizeof(xs), data,end));
#endif
    }
};

struct fu_header
{
    uint8_t type:5;
    uint8_t r:1;
    uint8_t e:1;
    uint8_t s:1;

    static fu_header* cast(uint8_t* data, uint8_t* end) {
        if (data + 1 > end)
            return nullptr;
        return reinterpret_cast<fu_header*>(data);
    }
    unsigned length() const { return 1; }

    void print(uint8_t* data, uint8_t* end) {
#if !defined(NOT_PRINT_PROTO)
        char const* se = "";
        if (this->s)
            se = "\t:FU-A START";
        else if (this->e)
            se = "\t:FU-A END";
        else if (this->s && this->e)
            se = "\t:FU-A START&END";
        char xs[128] = {};
        printf("%4u:%u: s %d e %d type %d: %s%s\n"
                , int(end-data), this->length()
                , this->s, this->e, this->type
                , xsfmt(xs,sizeof(xs), data,end), se);
#endif
    }
};

///rtcp////////////////////////////////////
enum
{
    RTCP_SR     = 200,
    RTCP_RR     = 201,
    RTCP_SDES   = 202,
    RTCP_BYE    = 203,
    RTCP_APP    = 204,
};

struct rtcp_header
{
    uint8_t rc:5;      // reception report count
    uint8_t p:1;       // padding
    uint8_t v:2;       // version

    uint8_t pt;      // packet type
    uint16_t length; /* pkt len in words, w/o this word */

    static rtcp_header* cast(uint8_t* data, uint8_t* end) {
        if (end-data < 4)
            return 0;
        rtcp_header* h = reinterpret_cast<rtcp_header*>(data);
        h->length = ntohs(h->length);
        if (h->length > end-data-4)
            return 0;
        return h;
    }
};

struct rtcp_sr_sender_info // sender report
{
    uint32_t ssrc;
    uint32_t ntpmsw; // ntp timestamp MSW(in second)
    uint32_t ntplsw; // ntp timestamp LSW(in picosecond)
    uint32_t rtpts;  // rtp timestamp
    uint32_t spc;    // sender packet count
    uint32_t soc;    // sender octet count

    static rtcp_sr_sender_info* cast(uint8_t* data, uint8_t* end) {
        rtcp_sr_sender_info* h = reinterpret_cast<rtcp_sr_sender_info*>(data);
        h->ssrc = ntohl(h->ssrc);
        h->ntpmsw = ntohl(h->ntpmsw);
        h->ntplsw = ntohl(h->ntplsw);
        h->rtpts = ntohl(h->rtpts);
        h->spc = ntohl(h->spc);
        h->soc = ntohl(h->soc);
        return h;
    }
};

struct rtcp_rr // receiver report
{
    uint32_t ssrc;
};

struct rtcp_report_block // report block
{
    uint32_t ssrc;
    uint32_t fraction:8; // fraction lost
     int32_t cumulative:24; // cumulative number of packets lost
    uint32_t exthsn; // extended highest sequence number received
    uint32_t jitter; // interarrival jitter
    uint32_t lsr; // last SR
    uint32_t dlsr; // delay since last SR
};

struct rtcp_sdes_item // source description RTCP packet
{
    uint8_t pt; // chunk type
    uint8_t len;
    uint8_t data[1];
};

static int64_t stimestamp(int init=0)
{
    static chrono::system_clock::time_point base = chrono::system_clock::now();
    if (init)
        base = chrono::system_clock::now()-chrono::milliseconds(30);
    return chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - base).count();
}

struct h264nal : boost::noncopyable
{
    //struct Fclose { void operator()(FILE* fp) const { if(fp)fclose(fp); } };
    // std::string sps_, pps_;
    FILE* dump_fp_; //std::unique_ptr<FILE,decltype(&fclose)> fp_;

    explicit h264nal(FILE* fp=0) : dump_fp_(fp) {}

    ~h264nal() {
        if (dump_fp_)
            fclose(dump_fp_);
    }

    void sprop_parameter_sets(std::string const& sps, std::string const& pps)
    {
        BOOST_STATIC_ASSERT(sizeof(int)==4);
        if (sps.empty())
            return;
        //sps_ = std::move(sps); pps_ = std::move(pps);
        std::vector<int> buf(1 + (sps.length()+4+pps.length())/sizeof(int)+1 );
        uint8_t* beg = reinterpret_cast<uint8_t*>( &buf[1] );

        memcpy(beg, sps.data(), sps.length());
        int len = sps.length();

        if (!pps.empty()) {
            static uint8_t sbytes[4] = {0,0,0,1};
            memcpy(beg+len, sbytes, 4);
            memcpy(beg+len+4, pps.data(), pps.length());
            len += 4+pps.length();
        }

        output_helper out(dump_fp_);
        out.put(1, beg,beg+len);
        out.commit(BUFFER_FLAG_CODEC_CONFIG);
    }

    size_t totsiz_ = 0, sizprint_=0;
    void data_incoming(uint8_t* data, uint8_t* end) //const
    {
        output_helper out(dump_fp_);
        nal_unit_header* h1 = nal_unit_header::cast(data, end);
        if (!h1)
            return;

        if (totsiz_-sizprint_ > 1024*1024) {
            LOGD("totsiz %u", totsiz_);
            sizprint_=totsiz_;
        }
        h1->print(data,end);

        switch (h1->type) {
            case 24: // STAP-A
                data += 1; //h1->length();
                while (end-data >= 2) {
                    uint16_t len = Ntoh<uint16_t>(data,end);
                    data += 2;
                    if (len > end - data)
                        break;
                    if (nal_unit_header* h2 = nal_unit_header::cast(data,data+len)) {
                        h2->print(data,data+len);
                        out.put(1, data, data+len); totsiz_ += len;
                        out.commit(0);
                        data += len; //
                    }
                }
                break;
            case 28: // FU-A
                data += 1; //h1->length();
                if (fu_header* h2 = fu_header::cast(data,end)) {
                    h2->print(data,end);
                    fu_header fuh = *h2;
                    if (fuh.s) {
                        nal_unit_header* h = nal_unit_header::cast(data,end);
                        h->nri = h1->nri;
                        h->f = h1->f;
                        h->print(data,end);
                    }
                    if (fuh.e) {
                        out.put(fuh.s, data + !fuh.s, end); totsiz_ += (end-data);
                        out.commit(0);
                    } else {
                        out.put(fuh.s, data + !fuh.s, end); totsiz_ += (end-data);
                    }
                }
                break;
            default:
                if (h1->type < 24) {
                    out.put(1, data,end); totsiz_ += (end-data);
                    out.commit((h1->type==7 || h1->type==8) ? BUFFER_FLAG_CODEC_CONFIG : 0);
                } else {
                    LOGE("nal-unit-type %d", h1->type);
                }
                break;
        }
    }

    struct output_helper
    {
        FILE* fp_;
        //uint8_t* opos_; uint8_t* oend_; uint8_t* ohead_;

        output_helper(FILE* fp, uint8_t* b=0, uint8_t* e=0) {
            fp_ = fp;
            //ohead_ = opos_ = b; oend_ = e;
        }
        ~output_helper() {
            ;
        }
        // unsigned avails() const { return unsigned(oend_ - opos_); }

        void put(bool start_bytes, uint8_t const* data, uint8_t const* end)
        {
            static uint8_t sbytes[4] = {0,0,0,1};

            if (fp_) {
                if (start_bytes)
                    fwrite(sbytes, sizeof(sbytes), 1, fp_);
                fwrite(data, end-data, 1, fp_);
            }
            if (start_bytes) {
                data -= 4;
                memcpy((void*)data, sbytes, sizeof(sbytes));
            }
            hgs_h264slice_inflate(0x0ff, (char*)data, end-data);

            //if (avails() < 4u*start_bytes + (end - data)) {
            //    ERR_MSG("size %u<%u", avails(), 4u*start_bytes + (end - data));
            //    return;
            //}
            // if (start_bytes) {
            //     std::copy(&start_bytes4[0], &start_bytes4[4], opos_);
            //     opos_ += 4;
            // }
            // std::copy(data, end, opos_);
            // opos_ += (end - data);
        }
        // void put(uint8_t const* data, uint8_t const* end) { return put(1, data, end); }
        void commit(int flags) {
            hgs_h264slice_commit(0x0ff, flags);

            static unsigned ncommit_ = 0; // test-only
            static unsigned ts_ = 0;

            ++ncommit_;
            unsigned ts = stimestamp();
            if (ts - ts_ >= 1000) {
                LOGD("F-RATE %u/%u", ncommit_, (ts-ts_)); //("BUFFER_FLAG_CODEC_CONFIG");
                ts_ = ts;
                ncommit_ = 0;
            }
        }
        // void commit(uint8_t const* data, uint8_t const* end, int flags) { return commit(1, data, end, flags); }
    };
};

struct h264file_mmap 
{
    typedef std::pair<uint8_t*,uint8_t*> range;
    uint8_t *begin_, *end_;

    h264file_mmap(int fd) {
        struct stat st; // fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);
        begin_ = (uint8_t*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        end_ = begin_ + st.st_size;
        assert(begin_[0]=='\0'&& begin_[1]=='\0'&& begin_[2]=='\0'&& begin_[3]==1);
    }
    //~h264file_mmap() { close(fd); }

    range begin() const { return find_(begin_); }
    range next(range const& prev) const { return find_(prev.second); }

    range find_(uint8_t* e) const {
        uint8_t* b = e;
        if (e+4 < end_) {
            uint8_t dx[] = {0, 0, 0, 1};
            b = e + 4;
            e = std::search(b, end_, &dx[0], &dx[4]);
        }
        return std::make_pair(b,e);
    }
};

struct h264file_printer : boost::noncopyable
{
    h264nal nalu_;
    h264file_mmap h264f_;

    h264file_printer(int fd, FILE* dumpfd) : nalu_(dumpfd), h264f_(fd)
    {}
    int setup(int, char*[])
    {
        for (auto p = h264f_.begin(); p.first < p.second; p = h264f_.next(p)) {
            nalu_.data_incoming(p.first+4, p.second);
        }
        return 0;
    }
};

template <typename Derived>
struct rtsp_connection
{
    typedef rtsp_connection<Derived> This;

    ip::tcp::socket tcpsock_;
    ip::tcp::endpoint endpoint_;
    std::string path_;
    std::string session_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    int cseq_ = 1;

    rtsp_connection(boost::asio::io_service& io_s, ip::tcp::endpoint ep, std::string path)
        : tcpsock_(io_s), endpoint_(ep), path_(path)
    {
        BOOST_STATIC_ASSERT(std::is_base_of<This, Derived>::value);
        LOGD("rtsp_connection");
    }

    struct Options {
        void operator()(Derived* d) const { d->on_success(*this); }
    };
    struct Describe {
        void operator()(Derived* d) const { d->on_success(*this); }
    };
    struct Setup {
        void operator()(Derived* d) const { d->on_success(*this); }
    };
    struct Play {
        void operator()(Derived* d) const { d->on_success(*this); }
    };
    struct Teardown {
        void operator()(Derived* d) const { d->on_success(*this); }
    };

    struct Connect {};

    void connect()
    {
        auto handler = [this](boost::system::error_code ec) {
            LOGD("connect: %d:%s", ec.value(), ec.message().c_str());
            if (ec) {
                derived()->on_error(ec, Connect{});
            } else {
                derived()->on_success(Connect{});
            }
        };
        LOGD("connect %s:%d", endpoint_.address().to_string().c_str(), endpoint_.port());
        tcpsock_.async_connect( endpoint_, handler );
    }

    void options()
    {
        {
        std::ostream outs(&request_);
        outs << "OPTIONS " << path_ << " RTSP/1.0\r\n"
            << "CSeq: " << cseq_++ << "\r\n"
            << "\r\n";
        }
        boost::asio::async_write(tcpsock_, request_, Action_helper<Options>{derived()} );
    }

    void describe()
    {
        {
        std::ostream outs(&request_);
        outs << "DESCRIBE " << path_ << " RTSP/1.0\r\n"
            << "CSeq: " << cseq_++ << "\r\n"
            << "Accept: application/sdp\r\n"
            << "\r\n";
        }
        boost::asio::async_write(tcpsock_, request_, Action_helper<Describe>{derived()} );
    }

    void setup(std::string streamid, std::string transport) //Setup
    {
        LOGD("setup: %s/%s\n\t%s", path_.c_str(), streamid.c_str(), transport.c_str());
        {
        std::ostream outs(&request_);
        outs << "SETUP " << path_ <<"/"<< streamid << " RTSP/1.0\r\n"
            << "CSeq: " << cseq_++ << "\r\n"
            << "Transport: "<< transport << "\r\n"
            << "Range: 0.00-\r\n"
            << "\r\n";
        }
        boost::asio::async_write(tcpsock_, request_, Action_helper<Setup>{derived()} );
    }

    void play()
    {
        {
        std::ostream outs(&request_);
        outs << "PLAY " << path_ << " RTSP/1.0\r\n"
            << "CSeq: " << cseq_++ << "\r\n"
            << "Session: " << session_ << "\r\n"
            << "\r\n";
        }
        boost::asio::async_write(tcpsock_, request_, Action_helper<Play>{derived()} );
    }

    void teardown()
    {
        {
        std::ostream outs(&request_);
        outs << "TEARDOWN " << path_ << " RTSP/1.0\r\n"
            << "CSeq: " << cseq_++ << "\r\n"
            << "Session: " << session_ << "\r\n"
            << "\r\n";
        }
        boost::asio::write(tcpsock_, request_);//(, Action_helper<Teardown>{derived()} ); // sync-write ?
        boost::system::error_code ec;
        tcpsock_.close(ec);
    }
private:
    Derived* derived() { return static_cast<Derived*>(this); }

    template <typename Action>
    struct Action_helper
    {
        void operator()(boost::system::error_code ec, size_t) const {
            auto* derived = derived_;
            if (ec) {
                derived->on_error(ec, Action{});
            } else {
                derived->response_.consume(derived->response_.size());
                boost::asio::async_read_until(derived->tcpsock_, derived->response_, "\r\n\r\n"
                        , [derived](boost::system::error_code ec, size_t){
                            if (ec) {
                                derived->on_error(ec, Action{});
                            } else {
                                // "RTSP/1.0 200 OK"; // TODO
                                Action fn;
                                fn(derived);
                            }
                    });
            }
        }
        Derived* derived_; // Action_helper(Derived* d) : Action{d} {}
    };
};

struct rtcp_client;
struct rtp_receiver : boost::noncopyable
{
    typedef rtp_receiver This;

    rtp_receiver(boost::asio::io_service& io_s, FILE* dump_fp)
        : nalu_(dump_fp) //(outfile, sps, pps)
        , udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), local_port()))
    {
        LOGD("rtp_receiver");
    }

    void teardown() {
        boost::system::error_code ec;
        udpsock_.close(ec);
    }

    void sprop_parameter_sets(std::string const& sps, std::string const& pps) {
        nalu_.sprop_parameter_sets(sps, pps);
    }
    void start_playing() { handle_receive_from(boost::system::error_code(), 0); }

    static int local_port(int p=0) {
        static int port=0;
        if (!port)
            port = 1000 + (int)time(0) % 3600;
        return port+p;
    }

    int (*rtcp_update_)(rtcp_client*, rtp_header*, uint8_t*, uint8_t*);
    rtcp_client* rtcp_;
private: // rtsp communication
    h264nal nalu_;

    void handle_receive_from(const boost::system::error_code& ec, size_t bytes_recvd)
    {
        if (ec) {
            LOGE("rtp recvd: %d:%s", ec.value(), ec.message().c_str());
        } else {
            if (bytes_recvd > 0) {
                static size_t max_recvd = 0, mrecvd; // test-only
                if ( (mrecvd = std::max(max_recvd, bytes_recvd)) > max_recvd) {
                    max_recvd = mrecvd;
                    LOGD("max recvd bytes: %u", mrecvd);
                }

                rtp_header* h = rtp_header::cast(bufptr(), bufptr()+bytes_recvd);
                if (!h) {
                    ERR_EXIT("rtp_header::cast"); return;
                } else {
                    h->print(bufptr(), bufptr()+bytes_recvd); //bufptr += h->length();
                    rtcp_update_(rtcp_, h, bufptr()+h->length(), bufptr()+bytes_recvd);
                    nalu_.data_incoming(bufptr()+h->length(), bufptr()+bytes_recvd);
                }
            }
            using namespace boost::asio;
            udpsock_.async_receive_from(
                    boost::asio::buffer(bufptr(), BufSiz), peer_endpoint_,
                    boost::bind(&This::handle_receive_from, this, placeholders::error, placeholders::bytes_transferred));
        }
    }

    ip::udp::socket udpsock_;
    ip::udp::endpoint peer_endpoint_;

    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_)+4; }
    enum { BufSiz = (1024*32-4) };
    int buf_[BufSiz/sizeof(int)+1];
};

struct rtcp_client
{
    typedef rtcp_client This;
    //boost::asio::deadline_timer timer_;
    //void check_deadline(deadline_timer* deadline) {
    //    if (!udpsock_.is_open())
    //        return;
    //}
    //void stop() {
    //    boost::system::error_code ec;
    //    timer_.cancel(ec);
    //}

    //rtcp_sr_sender_info last_rtcp_sr_ = {};
    struct receiver_report {
        rtcp_header h;
        uint32_t ssrc;
        rtcp_report_block rb;
    };
    struct composed : receiver_report {
        struct source_description { //sdes
            rtcp_header h;
            uint32_t ssrc;
            struct { // rtcp_sdes_item, source description RTCP packet
                uint8_t pt;
                uint8_t len;
                char data[6];
            } c;
        };
        source_description sdes;
    } rr_;

    int64_t ts_sr_ = 0;
    int64_t ts_rr_ = 0;

    //unsigned char framerate_=30;

    /// https://tools.ietf.org/html/rfc3550#appendix-A.3
    typedef enum {
        RTCP_SR   = 200,
        RTCP_RR   = 201,
        RTCP_SDES = 202,
        RTCP_BYE  = 203,
        RTCP_APP  = 204
    } rtcp_type_t;

    typedef enum {
        RTCP_SDES_END   = 0,
        RTCP_SDES_CNAME = 1,
        RTCP_SDES_NAME  = 2,
        RTCP_SDES_EMAIL = 3,
        RTCP_SDES_PHONE = 4,
        RTCP_SDES_LOC   = 5,
        RTCP_SDES_TOOL  = 6,
        RTCP_SDES_NOTE  = 7,
        RTCP_SDES_PRIV  = 8
    } rtcp_sdes_type_t;
#   define RTP_SEQ_MOD (1<<16)
    enum{ MAX_DROPOUT = 3000 };
    enum{ MAX_MISORDER = 100 };
    enum{ MIN_SEQUENTIAL = 2 };
    typedef uint16_t u_int16;

    struct source {
        uint32_t base_seq;       /* base seq number */
        uint16_t max_seq;        /* highest seq. number seen */
        uint16_t cycles;         /* shifted count of seq. number cycles */
        //uint32_t cycles;         /* shifted count of seq. number cycles */
        uint32_t bad_seq;        /* last 'bad' seq number + 1 */
        uint32_t probation;      /* sequ. packets till source is valid */
        uint32_t received;       /* packets received */

        uint32_t expected_prior; /* packet expected at last interval */
        uint32_t received_prior; /* packet received at last interval */
         int32_t transit;        /* relative trans time for prev pkt */
        uint32_t jitter;         /* estimated jitter */
        /* ... */
    };
    source source_;

    static void init_seq(source *s, u_int16 seq)
    {
        s->base_seq = seq;
        s->max_seq = seq;
        s->bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
        s->cycles = 0;
        s->received = 0;
        s->received_prior = 0;
        s->expected_prior = 0;
        /* other initialization */

        s->probation = 0;
    }
    static int update_seq(source *s, u_int16 seq)
    {
        u_int16 udelta = seq - s->max_seq;
        /*
         * Source is not valid until MIN_SEQUENTIAL packets with
         * sequential sequence numbers have been received.
         */
        if (s->probation) {
            /* packet is in sequence */
            if (seq == s->max_seq + 1) {
                s->probation--;
                s->max_seq = seq;
                if (s->probation == 0) {
                    init_seq(s, seq);
                    s->received++;
                    return 1;
                }
            } else {
                s->probation = MIN_SEQUENTIAL - 1;
                s->max_seq = seq;
            }
            return 0;
        } else if (udelta < MAX_DROPOUT) {
            /* in order, with permissible gap */
            if (seq < s->max_seq) {
                /*
                 * Sequence number wrapped - count another 64K cycle.
                 */
                s->cycles += RTP_SEQ_MOD;
            }
            s->max_seq = seq;
        } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
            /* the sequence number made a very large jump */
            if (seq == s->bad_seq) {
                /*
                 * Two sequential packets -- assume that the other side
                 * restarted without telling us so just re-sync
                 * (i.e., pretend this was the first packet).
                 */
                init_seq(s, seq);
            }
            else {
                s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
                return 0;
            }
        } else {
            /* duplicate or reordered packet */
        }
        s->received++;
        return 1;
    }

    rtcp_client(boost::asio::io_service& io_s, rtp_receiver* rtp/*, ip::udp::endpoint const& remote_ep*/)
        : udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), rtp->local_port(+1)))
    {
        BOOST_STATIC_ASSERT(sizeof(rtcp_header)%4==0);
        BOOST_STATIC_ASSERT(sizeof(rtcp_report_block)%4==0);
        BOOST_STATIC_ASSERT(sizeof(receiver_report)%4==0);

        rtp_ = rtp;
        rtp_->rtcp_update_ = &rtcp_client::rtp_update;
        rtp_->rtcp_ = this;

        memset(&source_, 0, sizeof(source_));

        memset(&rr_, 0, sizeof(rr_));
        rr_.h.rc = 1;
        rr_.h.p = 0;
        rr_.h.v = 2; // version
        rr_.h.pt = RTCP_RR;
        rr_.h.length = htons( (sizeof(receiver_report)-4)/4 );
        rr_.ssrc = ( rand() );

        auto& sdes = rr_.sdes;
        sdes.h = rr_.h;
        sdes.h.pt = RTCP_SDES;
        sdes.h.length = htons(3);
        sdes.ssrc = rr_.ssrc;
        sdes.c.pt = RTCP_SDES_CNAME;
        sdes.c.len = 6;
        snprintf(sdes.c.data,6, "N%04d.", rtp_->local_port()); //strcpy(sdes.c.data, "helo");

        //udpsock_.connect(remote_ep);
        handle_receive_from(boost::system::error_code(), 0);
    }

    void teardown() {
        boost::system::error_code ec;
        udpsock_.close(ec);
    }

    // rtp callback
    static int rtp_update(rtcp_client* self, rtp_header* h, uint8_t*, uint8_t*)
    {
        auto* s = &self->source_;
        if (s->cycles == 0) {
            s->cycles = 1;
            s->base_seq = s->max_seq = h->seq;
            s->received = 1;
        } else {
            if (h->seq < s->max_seq) {
                s->cycles++;
            }
            s->max_seq = h->seq;
            s->received++;
        }

        {
            uint32_t arrival = stimestamp();
            arrival = std::max(arrival, h->timestamp);
            int transit = arrival - h->timestamp;
            int d = transit - s->transit;
            s->transit = transit;
            if (d < 0) d = -d;
            s->jitter += d - ((s->jitter + 8) >> 4); // ... rr->jitter = s->jitter >> 4;
        }
        return 0;
    }

    void handle_receive_from(const boost::system::error_code& ec, size_t bytes_recvd)
    {
        if (ec) {
            LOGE("rtcp recvd: %d:%s", ec.value(), ec.message().c_str());
            return;
        }
        if (bytes_recvd > 0) {
            rtcp_header* h = rtcp_header::cast(bufptr(), bufptr()+bytes_recvd);
            if (!h) {
                ERR_EXIT("rtcp_header::cast"); return;
            } else if (h->pt == RTCP_SR) {
                rtcp_sr_sender_info* si = rtcp_sr_sender_info::cast(bufptr()+sizeof(rtcp_header), bufptr()+bytes_recvd);
                rr_.rb.ssrc = htonl( si->ssrc );
                rr_.rb.lsr = htonl( ((si->ntpmsw&0xFFFF)<<16) | ((si->ntplsw>>16) & 0xFFFF) ); //uint32_t lsr; // last SR
                si->rtpts;
                ts_sr_ = stimestamp( source_.received==0 );
            } else {
                LOGD("pt %d", (int)h->pt);
            }
        }
        using namespace boost::asio;
        udpsock_.async_receive_from(
                boost::asio::buffer(bufptr(), BufSiz), peer_endpoint_,
                boost::bind(&This::handle_receive_from, this, placeholders::error, placeholders::bytes_transferred));
        //LOGD("udpsock %d", bytes_recvd);
    }

    void handle_send_to(const boost::system::error_code& ec, size_t )
    {
        if (ec) {
            LOGE("send %d(%s)", ec.value(), ec.message().c_str());
            return;
        }
    }

    void rreport()
    {
        //if (ts_sr_ == 0) {
        //    if (ts_rr_ == 0 || stimestamp() - ts_rr_ > 1500) {
        //        static uint32_t u4 = 0xfeedface; // ce fa ed fe
        //        udpsock_.async_send_to(
        //                boost::asio::buffer(&u4, 4), peer_endpoint_,
        //                boost::bind(&This::handle_send_to, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        //        ts_rr_ = stimestamp();
        //    }
        //    return;
        //}

        // http://www.ece.rutgers.edu/~marsic/books/CN/projects/wireshark/ws-project-4.html
        //  J(i)  =  J(i–1)  +  ( |D(i–1, i)| – J(i–1) ) / 16 
        //  D(i–1, i)  =  (Ri – Ri–1) – (Si – Si–1)  =  (Ri – Si) – (Ri–1 – Si–1) 
    //  https://tools.ietf.org/html/rfc1889#page-63
    //- extended_max = s->cycles + s->max_seq;
    //- expected = extended_max - s->base_seq + 1;
    //- lost = expected - s->received;
    //
    //  expected_interval = expected - s->expected_prior;
    //  s->expected_prior = expected;
    //  received_interval = s->received - s->received_prior;
    //  s->received_prior = s->received;
    //  lost_interval = expected_interval - received_interval;
    //  if (expected_interval == 0 || lost_interval <= 0) fraction = 0;
    //  else fraction = (lost_interval << 8) / expected_interval;
    //
    //- int transit = arrival - r->ts;
    //- int d = transit - s->transit;
    //- s->transit = transit;
    //- if (d < 0) d = -d;
    //-| s->jitter += d - ((s->jitter + 8) >> 4); ... rr->jitter = s->jitter >> 4;
    //
        if (ts_rr_ +100 < ts_sr_ && source_.received > 0) {
            auto* s = &source_;

            uint8_t fraction;
            int32_t lost;
            uint32_t extended_max = (uint32_t(s->cycles)<<16 | s->max_seq);
            {
                uint32_t expected = extended_max - s->base_seq + 1;
                BOOST_ASSERT(s->received <= expected);
                lost = expected - s->received;

                int expected_interval = expected - s->expected_prior;
                s->expected_prior = expected;
                int received_interval = s->received - s->received_prior;
                s->received_prior = s->received;
                int lost_interval = expected_interval - received_interval;
                if (expected_interval == 0 || lost_interval <= 0)
                    fraction = 0;
                else
                    fraction = (lost_interval << 8) / expected_interval;
            }

            rr_.rb.jitter = htonl( s->jitter >> 4 );
            rr_.rb.fraction = fraction;
            lost = htonl(lost);
            rr_.rb.cumulative = lost>>8;

            auto ts = stimestamp();
            rr_.rb.exthsn = htonl(extended_max); // extended highest sequence number received
            rr_.rb.dlsr = htonl( uint32_t(ts - ts_sr_) );
            udpsock_.async_send_to(
                    boost::asio::buffer(&rr_, sizeof(rr_)), peer_endpoint_,
                    boost::bind(&This::handle_send_to, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            ts_rr_ = ts;
        }
    }

    rtp_receiver* rtp_;

    ip::udp::socket udpsock_;
    ip::udp::endpoint peer_endpoint_;

    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_); }
    enum { BufSiz = (1024*4) };
    int32_t buf_[BufSiz/sizeof(int32_t)];
};

struct rtsp_client : rtsp_connection<rtsp_client>, boost::noncopyable
{
    rtp_receiver* thiz;

    rtsp_client(boost::asio::io_service& io_s, rtp_receiver* ptr, ip::tcp::endpoint remote_endpoint, std::string remote_path)
        : rtsp_connection(io_s, remote_endpoint, remote_path)
    {
        LOGD("rtsp_client");
        thiz = ptr;
    }

    int setup(int, char*[]) {
        connect();
        return 0;
    }

    void teardown() {
        rtsp_connection<rtsp_client>::teardown();
        //boost::asio::io_service& io_s = udpsock_.get_io_service();
        //rtsp_client_.sig_teardown.connect(boost::bind(&boost::asio::io_service::stop, &io_s));
    }

    void on_success(Connect) {
        options();
    }
    void on_success(Options) {
        LOGD("Options");
        describe();
    }
    void on_success(Describe)
    {
        LOGD("success:Describe");
        std::string sps, pps, streamid;
        std::istream ins(&response_);
        std::string line;
        bool m_v = 0;
        while (std::getline(ins, line)) {
            boost::trim_right(line);
            LOGD("| %s", line.c_str());
            if (boost::starts_with(line, "m=")) {
                m_v = boost::starts_with(line, "m=video");
            } else if (m_v) {
                if (boost::starts_with(line, "a=fmtp:")) {
                    re::smatch m; // fmtp:96 profile-level-id=42A01E;packetization-mode=1;sprop-parameter-sets=
                    re::regex re("sprop-parameter-sets=([^=,]+)=*,([^=,;]+)");
                    if (re::regex_search(line, m, re)) {
                        base64dec(m[1].first, m[1].second, std::back_inserter(sps));
                        base64dec(m[2].first, m[2].second, std::back_inserter(pps));
                    }
                } else if (boost::starts_with(line, "a=control:")) {
                    re::smatch m;
                    re::regex re("a=control:([^=]+=[0-9]+)");
                    if (re::regex_search(line, m, re)) {
                        streamid.assign( m[1].first, m[1].second );
                    }
                }
            }
        }

        if (!sps.empty())
            thiz->sprop_parameter_sets(sps, pps);

        char transport[128];
        snprintf(transport,sizeof(transport)
                , "RTP/AVP;unicast;client_port=%d-%d", thiz->local_port(),thiz->local_port(+1));
        rtsp_connection<rtsp_client>::setup(streamid, transport);
    }

    void on_success(Setup)
    {
        LOGD("success:Setup");
        std::istream ins(&response_);
        std::string line;
        while (std::getline(ins, line)) {
            boost::trim_right(line);
            LOGD("%d: %s", __LINE__, line.c_str());
            if (boost::starts_with(line, "Session:")) {
                re::smatch m;
                re::regex re("Session:[[:space:]]*([^[:space:]]+)");
                if (re::regex_search(line, m, re)) {
                    session_.assign(m[1].first, m[1].second);
                }
            }
        }
        thiz->start_playing();
        play();
    }

    void on_success(Play) {
        LOGD("Playing");
    }

    void on_success(Teardown) {
        //sig_teardown(); // kill(getpid(), SIGQUIT);
        LOGD("Teardown");
    }
    template <typename A> void on_success(A) { LOGD("success:A"); }

    template <typename A>
    void on_error(boost::system::error_code ec, A) {
        LOGE("rtsp error: %d:%s", ec.value(), ec.message().c_str());
        // TODO : deadline_timer reconnect
    }

    //boost::signals2::signal<void()> sig_teardown;
}; // rtsp_client rtsp_client_;

//#include <boost/type_erasure/member.hpp>
//BOOST_TYPE_ERASURE_MEMBER((setup_fn), setup, 2)
// BOOST_TYPE_ERASURE_MEMBER((setup_fn), ~, 0)
//boost::type_erasure::any<setup_fn<int(int,char*[])>, boost::type_erasure::_self&> ;

#if 0
struct Objmem {
    int rtp_receiver_[sizeof(rtp_receiver)/sizeof(int)+1];
    int rtcp_client_[sizeof(rtcp_client)/sizeof(int)+1];
    int rtsp_client_[sizeof(rtsp_client)/sizeof(int)+1];
};
static int objmem_[sizeof(rtp_receiver)/sizeof(int)+1];

// int Main::objmem_[sizeof(rtp_receiver)/sizeof(int)+1] = {};
struct Main : boost::asio::io_service, boost::noncopyable
{
    struct Args {
        Args(int ac, char* av[]) {
            if (ac <= 2) {
                if (ac==2 && !(dump_fp = fopen(av[1], "rb"))) {
                    ERR_EXIT("file %s: fail", av[1]); return;
                }
            } else if (ac > 3) {
                endp = ip::tcp::endpoint(ip::address::from_string(av[1]),atoi(av[2]));
                path = av[3];
                if (ac > 4 && !(dump_fp = fopen(av[4], "wb")))
                    ERR_EXIT("file %s: fail", av[2]);
            } else {
                ERR_EXIT("Usage: %s ...", av[0]);
            }
        }
        
        ip::tcp::endpoint endp;
        std::string path;
        FILE* dump_fp = 0;
    };

    Main(int ac, char* av[]) : signals_(*this)
    {
        Args args(ac, av);
        if (ac <= 2) {
            auto* obj = new (&objmem_) h264file_printer(0, args.dump_fp);
            dtor_ = [obj]() { obj->~h264file_printer(); };
            setup_ = [obj](int ac,char*av[]) { obj->setup(ac,av); };
        } else {
            auto* obj = new (&objmem_) rtp_receiver(*this, args.endp, args.path, args.dump_fp);
            dtor_ = [obj]() { obj->~rtp_receiver(); };
            setup_ = [obj](int ac,char*av[]) { obj->setup(ac,av); };

            signals_.add(SIGINT);
            signals_.add(SIGTERM); // (SIGQUIT);
            signals_.async_wait( [obj](boost::system::error_code, int){ obj->teardown(); } );
        }
        // auto* obj = reinterpret_cast<T*>(&objmem_);
    }
    ~Main() { dtor_(); }

    int run(int ac, char* av[])
    {
        setup_(ac, av);
        return boost::asio::io_service::run();
    }

private:
    boost::asio::signal_set signals_;

    std::function<void(int,char*[])> setup_;
    std::function<void()> dtor_;
};

//int main(int argc, char* argv[])
//{
//    //BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END
//    try {
//        Main s(argc, argv);
//        s.run(argc, argv);
//    } catch (std::exception& e) {
//        std::cerr << "Exception: " << e.what() << "\n";
//    }
//
//    return 0;
//}
#endif

// http://stackoverflow.com/questions/6394874/fetching-the-dimensions-of-a-h264video-stream?lq=1
//width = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_left_offset*2 - frame_crop_right_offset*2;
//height= ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);

#if 0
static struct test_h264file {
    int h264file_fd = -1;
    std::pair<uint8_t*,uint8_t*> range_ = {};
    int ncommit_ = 0;
} test_;

void hgs_poll_once()
{
    //static char tmpbuf[1024*64];
    auto* hf = reinterpret_cast<h264file_mmap*>(&objmem_);
    if (test_.range_.first < test_.range_.second) {
        char* p = (char*)test_.range_.first;
        char* end = (char*)test_.range_.second;
        for (; p+64 < end; p += 64) {
            hgs_h264slice_inflate(0, p, 64);
        }
        if (p < end)
            hgs_h264slice_inflate(0, p, end-p);
        hgs_h264slice_commit(test_.ncommit_==0?BUFFER_FLAG_CODEC_CONFIG:0);

        test_.range_ = hf->next(test_.range_);
        test_.ncommit_++;
        if (test_.ncommit_ == 400) {
            test_.ncommit_ = 0;
            test_.range_ = hf->begin();
        }
    }
}

void hgs_exit(int)
{
    LOGD("exit");
    auto* hf = reinterpret_cast<h264file_mmap*>(&objmem_);
    hf->~h264file_mmap();

    close(test_.h264file_fd);
    test_.h264file_fd = -1;
}

void hgs_init()
{
    LOGD("init");
    test_.h264file_fd = open("/sdcard/a.h264", O_RDONLY);
    auto* hf = new (&objmem_) h264file_mmap(test_.h264file_fd);
    test_.range_ = hf->begin();
}
#else

static struct {
    boost::asio::io_service* io_service = 0;
    rtp_receiver* rtp = 0;
    rtcp_client* rtcp = 0;
    rtsp_client* rtsp = 0;
} hgs_;

/// rtsp://127.0.0.1:7654/rtp1
/// rtsp://192.168.2.3/live/ch00_2
void hgs_init() //(int ac, char* const av[]) // 640*480 1280X720 1920X1080
{
    srand( time(0) );

    char const *ip, *port, *path;
    path="rtsp://192.168.2.3/live/ch00_2"; //1920x1080
    path="rtsp://192.168.2.3/live/ch00_0"; //320x240
    path="rtsp://192.168.2.3/live/ch00_1"; //1280x720
    ip="192.168.2.3"; port = "554";
    ip="192.168.2.172"; port = "7654"; path="rtsp://192.168.2.172:7654/rtp1";

    auto endp = ip::tcp::endpoint(ip::address::from_string(ip),atoi(port));

    LOGD("init %s:%s %s", ip, port, path);

    hgs_.io_service = new boost::asio::io_service();
    hgs_.rtp = new rtp_receiver(*hgs_.io_service, 0/*fopen("/tmp/1.rtp.h264","w")*/); // auto* c = new (&objmem_) rtp_receiver(hgs_.io_service, endp, path, 0);
    hgs_.rtcp = new rtcp_client(*hgs_.io_service, hgs_.rtp);
    hgs_.rtsp = new rtsp_client(*hgs_.io_service, hgs_.rtp, endp, path);

    hgs_.rtsp->setup(0,0);

    // auto* hc = reinterpret_cast<rtp_receiver*>(&objmem_);
    LOGD("init ok");
}

void hgs_exit(int preexit)
{
    if (preexit) {
        LOGD("teardown");
        hgs_.rtp->teardown();
        hgs_.rtcp->teardown();
        hgs_.rtsp->teardown();
        hgs_.io_service->poll_one();
    } else {
        LOGD("exit:stop");
        hgs_.io_service->stop();

        delete hgs_.rtsp;       hgs_.rtsp = 0;
        delete hgs_.rtcp;       hgs_.rtcp = 0;
        delete hgs_.rtp;        hgs_.rtp = 0;
        delete hgs_.io_service; hgs_.io_service = 0;
    }
}

void hgs_poll_once()
{
    //LOGD("poll_one");
    //auto* c = reinterpret_cast<rtp_receiver*>(&objmem_);

    hgs_.io_service->poll_one();
    hgs_.rtcp->rreport();
}

#endif
