// #include <sys/types.h> #include <signal.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <regex> //#<boost/regex.hpp>
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

#include <android/log.h>
#define  LOG_TAG    "HGSX"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
enum { BUFFER_FLAG_CODEC_CONFIG=2 };

extern "C" {
    void hgs_h264slice_inflate(int need_start_bytes, char* p, size_t len);
    void hgs_h264slice_commit(int flags);

    void hgs_poll_once();
    void hgs_exit();
    void hgs_init();
}

template <typename... As> void err_exit_(int lin_, char const* fmt, As... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... As> void err_msg_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
}
//#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
//#define ERR_MSG(...) err_msg_(__LINE__, "%d: " __VA_ARGS__)
#define ERR_EXIT(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ERR_MSG(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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
        h->timestamp = ntohl(h->timestamp);
        h->ssrc = ntohl(h->ssrc);
        for (unsigned i=0; i < h->cc; ++i)
            h->csrc[i] = ntohl(h->csrc[i]);
        return h;
    }
    unsigned length() const { return sizeof(rtp_header)-4 + 4*this->cc; }

    void print(uint8_t* data, uint8_t* end) {
#if !defined(__android__)
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
#if !defined(__android__)
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
#if !defined(__android__)
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

struct nal_unit_sink : boost::noncopyable
{
    //struct Fclose { void operator()(FILE* fp) const { if(fp)fclose(fp); } };
    // std::string sps_, pps_;
    FILE* dump_fp_; //std::unique_ptr<FILE,decltype(&fclose)> fp_;

    explicit nal_unit_sink(FILE* fp=0) : dump_fp_(fp) {}

    ~nal_unit_sink() {
        if (dump_fp_)
            fclose(dump_fp_);
    }

    void sprop_parameter_sets(std::string const& sps, std::string const& pps)
    {
        //sps_ = std::move(sps); pps_ = std::move(pps);
        std::vector<int> buf( (sps.size()+pps.size())/sizeof(int)+1 );
        uint8_t* beg = reinterpret_cast<uint8_t*>( &buf[0] );

        memcpy(beg, sps.data(), sps.length());
        int len = sps.length();
        // write(beg, beg+sps.length());

        if (!pps.empty()) {
            memcpy(beg+len, pps.data(), pps.length());
            len += pps.length();
        }
        //write(beg, beg+pps.length());

        output_helper out(dump_fp_);
        out.put(0, beg,beg+len);
        out.commit(BUFFER_FLAG_CODEC_CONFIG);
    }

    size_t totsiz_ = 0, sizprint_=0;
    void data_coming(uint8_t* data, uint8_t* end) //const
    {
        output_helper out(dump_fp_);
        nal_unit_header* h1 = nal_unit_header::cast(data, end);
        if (!h1)
            return;

        if (totsiz_-sizprint_ > 1024*10) {
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
            hgs_h264slice_inflate(0, (char*)data, end-data);

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
            hgs_h264slice_commit(flags);
            if (flags) {
                LOGD("BUFFER_FLAG_CODEC_CONFIG %d", flags);
            }
        }
        // void commit(uint8_t const* data, uint8_t const* end, int flags) { return commit(1, data, end, flags); }
    };
};

#include <sys/mman.h>
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
    nal_unit_sink nalu_;
    h264file_mmap h264f_;

    h264file_printer(int fd, FILE* dumpfd) : nalu_(dumpfd), h264f_(fd)
    {}
    int setup(int, char*[])
    {
        for (auto p = h264f_.begin(); p.first < p.second; p = h264f_.next(p)) {
            nalu_.data_coming(p.first+4, p.second);
        }
        return 0;
    }
};

template <typename Derived>
struct rtsp_connection
{
    typedef rtsp_connection<Derived> This;
    //BOOST_STATIC_ASSERT(std::is_base_of<This, Derived>::value);

    ip::tcp::socket tcpsock_;
    ip::tcp::endpoint endpoint_;
    std::string path_;
    std::string session_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    int cseq_ = 1;

    rtsp_connection(boost::asio::io_service& io_s, ip::tcp::endpoint ep, std::string path)
        : tcpsock_(io_s), endpoint_(ep), path_(path)
    { LOGD("rtsp_connection"); }

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

    void setup(std::string streamid, std::string transport)
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
        boost::asio::async_write(tcpsock_, request_, Action_helper<Teardown>{derived()} );
        // sync-write ?
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

struct rtph264_client : boost::noncopyable
{
    typedef rtph264_client This;

    rtph264_client(boost::asio::io_service& io_s
            , ip::tcp::endpoint remote_endpoint , std::string remote_path
            , FILE* dump_fp)
        : rtsp_client_(this, io_s, remote_endpoint, remote_path)
        , nalu_(dump_fp) //(outfile, sps, pps)
        , udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), local_port()))
    {
        LOGD("rtph264_client");
    }

    int setup(int, char*[]) {
        rtsp_client_.connect();
        return 0;
    }

    void teardown() {
        rtsp_client_.teardown();

        boost::asio::io_service& io_s = udpsock_.get_io_service();
        //rtsp_client_.sig_teardown.connect(boost::bind(&boost::asio::io_service::stop, &io_s));
    }

private: // rtsp communication
    struct rtsp_client : rtsp_connection<rtsp_client>, boost::noncopyable
    {
        rtph264_client* thiz;

        rtsp_client(rtph264_client* ptr, boost::asio::io_service& io_s, ip::tcp::endpoint remote_endpoint, std::string remote_path)
            : rtsp_connection(io_s, remote_endpoint, remote_path)
        {
            LOGD("rtsp_client");
            thiz = ptr;
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
            LOGD("Describe");
            std::string sps, pps, streamid;
            std::istream ins(&response_);
            std::string line;
            bool v = 0;
            while (std::getline(ins, line)) {
                boost::trim_right(line);
                LOGD("%s", line.c_str());
                if (boost::starts_with(line, "m=")) {
                    v = boost::starts_with(line, "m=video");
                } else if (v) {
                    if (boost::starts_with(line, "a=fmtp:")) {
                        std::smatch m;
                        std::regex re("sprop-parameter-sets=([^=,]+)=*,([^=,;]+)");
                        if (std::regex_search(line, m, re)) {
                            base64dec(m[1].first, m[1].second, std::back_inserter(sps));
                            base64dec(m[2].first, m[2].second, std::back_inserter(pps));
                        }
                    } else if (boost::starts_with(line, "a=control:")) {
                        std::smatch m;
                        std::regex re("a=control:([^=]+=[0-9]+)");
                        if (std::regex_search(line, m, re)) {
                            streamid.assign( m[1].first, m[1].second );
                        }
                    }
                }
            }

            if (!sps.empty())
                thiz->nalu_.sprop_parameter_sets(sps, pps);

            char transport[128];
            snprintf(transport,sizeof(transport)
                    , "RTP/AVP;unicast;client_port=%d-%d", thiz->local_port(),thiz->local_port(+1));
            setup(streamid, transport);
        }

        void on_success(Setup)
        {
            LOGD("Setup");
            std::istream ins(&response_);
            std::string line;
            while (std::getline(ins, line)) {
                boost::trim_right(line);
                LOGD("%s", line.c_str());
                if (boost::starts_with(line, "Session:")) {
                    std::smatch m;
                    std::regex re("Session:[[:space:]]*([^[:space:]]+)");
                    if (std::regex_search(line, m, re)) {
                        session_.assign(m[1].first, m[1].second);
                    }
                }
            }
            thiz->handle_receive_from(boost::system::error_code(), 0);
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
            LOGE(">Error: %d(%s)", ec.value(), ec.message().c_str());
            // TODO : deadline_timer reconnect
        }

        //boost::signals2::signal<void()> sig_teardown;
    };

    rtsp_client rtsp_client_;
    nal_unit_sink nalu_;

private:
    static int local_port(int p=0) { return 3395+p; } // TODO

    void handle_receive_from(const boost::system::error_code& ec, size_t bytes_recvd)
    {
        if (!ec && bytes_recvd > 0) {
            static size_t max_recvd = 0, mrecvd;
            if ( (mrecvd = std::max(max_recvd, bytes_recvd)) > max_recvd) {
                max_recvd = mrecvd;
                LOGD("max recvd bytes: %u", mrecvd);
            }
            rtp_header* h = rtp_header::cast(bufptr(), bufptr()+bytes_recvd);
            if (!h) {
                ERR_EXIT("rtp_header::cast"); return;
            }

            h->print(bufptr(), bufptr()+bytes_recvd); //bufptr += h->length();
            nalu_.data_coming(bufptr()+h->length(), bufptr()+bytes_recvd);
        }
        using namespace boost::asio;
        udpsock_.async_receive_from(
                boost::asio::buffer(bufptr(), BufSiz), peer_endpoint_,
                boost::bind(&This::handle_receive_from, this, placeholders::error, placeholders::bytes_transferred));
    }

    ip::udp::socket udpsock_;
    ip::udp::endpoint peer_endpoint_;

    //boost::filesystem::path dir_; int dg_idx_ = 0;
    //std::aligned_storage<1024*64,alignof(int)>::type data_;
    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_)+4; }
    enum { BufSiz = (1024*128-4) };
    int buf_[BufSiz/sizeof(int)+1];
};

//#include <boost/type_erasure/member.hpp>
//BOOST_TYPE_ERASURE_MEMBER((setup_fn), setup, 2)
// BOOST_TYPE_ERASURE_MEMBER((setup_fn), ~, 0)
//boost::type_erasure::any<setup_fn<int(int,char*[])>, boost::type_erasure::_self&> ;

static int objmem_[sizeof(rtph264_client)/sizeof(int)+1];
// int Main::objmem_[sizeof(rtph264_client)/sizeof(int)+1] = {};
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
            auto* obj = new (&objmem_) rtph264_client(*this, args.endp, args.path, args.dump_fp);
            dtor_ = [obj]() { obj->~rtph264_client(); };
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

void hgs_exit()
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
    rtph264_client* rhc = 0;
} hgs_;

    /// rtsp://192.168.2.3/live/ch00_2
void hgs_init() //(int ac, char* const av[]) // 640*480 1280X720 1920X1080
{
    char const *path;
    path="rtsp://192.168.2.3/live/ch00_2";
    path="rtsp://192.168.2.3/live/ch00_1"; //1280x720
    path="rtsp://192.168.2.3/live/ch00_0"; //320x240
    auto *ip="192.168.2.3";
    auto *port = "554";
    auto endp = ip::tcp::endpoint(ip::address::from_string(ip),atoi(port));

    LOGD("init %s:%s %s", ip, port, path);

    hgs_.io_service = new boost::asio::io_service();
    LOGD("new2");
    hgs_.rhc = new rtph264_client(*hgs_.io_service, endp, path, 0); // auto* c = new (&objmem_) rtph264_client(hgs_.io_service, endp, path, 0);
    LOGD("setup");
    hgs_.rhc->setup(0,0);
    // auto* hc = reinterpret_cast<rtph264_client*>(&objmem_);
    LOGD("return");
}

void hgs_exit()
{
    LOGD("exit");
    delete hgs_.rhc;
    delete hgs_.io_service;
    hgs_.rhc = 0;
    hgs_.io_service = 0;
    // auto* c = reinterpret_cast<rtph264_client*>(&objmem_);
    // c->~rtph264_client();
}

void hgs_poll_once()
{
    //LOGD("poll_one");
    //auto* c = reinterpret_cast<rtph264_client*>(&objmem_);
    hgs_.io_service->poll_one();
}

#endif
