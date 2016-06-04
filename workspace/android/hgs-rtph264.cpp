// #include <sys/types.h> #include <signal.h>
#include <sys/mman.h>
#include <string.h>
#include <cstdlib>
#include <deque>
//#include <iostream>
#include <boost/static_assert.hpp>
#define BOOST_SCOPE_EXIT_CONFIG_USE_LAMBDAS
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
#include <boost/intrusive/slist.hpp>
#include <thread>
#include <mutex>
#include <chrono> //#include <boost/chrono.hpp>

#if defined(__ANDROID__)
#  include <regex> //<boost/regex.hpp>
namespace re = std;
#else
#  include <boost/regex.hpp>
namespace re = boost;
#endif
namespace chrono = std::chrono;
namespace ip = boost::asio::ip;

#if defined(__ANDROID__)
#  include <android/log.h>
#  define LOG_TAG    "HGSC"
#  define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#  define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#  define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#  define ERR_EXIT(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#  define NOT_PRINT_PROTO 1

#else // !__ANDROID__

template <typename... As> void err_exit_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
template <typename... As> void err_msg_(int lin_, char const* fmt, As... a) {
    fprintf(stderr, fmt, lin_, a...);
    fprintf(stderr, "\n");
}
#  define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)
#  define LOGD(...) err_msg_(__LINE__, "D:%d: " __VA_ARGS__)
#  define LOGW(...) err_msg_(__LINE__, "W:%d: " __VA_ARGS__)
#  define LOGE(...) err_msg_(__LINE__, "E:%d: " __VA_ARGS__)
#endif

enum { BUFFER_FLAG_CODEC_CONFIG=2 };
extern "C" {
    //int  hgs_queue_input(uint8_t*data, uint8_t*end, int flags, unsigned);

    int  hgs_buffer_obtain(int timeout);
    void hgs_buffer_inflate(int idx, char* p, size_t len);
    void hgs_buffer_release(int idx, unsigned timestamp, int flags);

    void hgs_poll_once(int);
    void hgs_run();
    void hgs_pump();

    void hgs_exit(int);
    void hgs_init(char const* ip, int port, char const* path, int w, int h);
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

struct mbuffer /*: boost::intrusive::slist_base_hook<>*/ {
    rtp_header rtp_h;
    struct Nal {
        uint32_t _4bytes;
        nal_unit_header nal_h; // size 0 pos
        uint8_t data[1];
    } *base_ptr = 0;
    unsigned size, capacity;

    ~mbuffer() {
        if (base_ptr)
            free(base_ptr);
    }
    mbuffer() {
        size = capacity = 0;
        base_ptr = 0; // rtp_h = rtp_header{};
    }
    mbuffer(rtp_header const& rh, uint8_t* data=0, uint8_t* end=0) {
        capacity = 2048;
        size = 0; //+sizeof(nal_unit_header); //sizeof(Nal)-1;
        base_ptr = (Nal*)malloc(capacity);
        rtp_h = rh;
        base_ptr->_4bytes = htonl(0x00000001); // base_ptr->nal_h = nh;

        if (data && data<end)
            put(data, end);
    }

    mbuffer(mbuffer&& rhs) {
        memcpy(this, &rhs, sizeof(*this));
        rhs.size = rhs.capacity = 0;
        rhs.base_ptr = 0;
    }
    mbuffer& operator=(mbuffer&& rhs) {
        if (this != &rhs) {
            if (base_ptr)
                free(base_ptr);
            memcpy(this, &rhs, sizeof(*this));
            rhs.size = rhs.capacity = 0;
            rhs.base_ptr = 0;
        }
        return *this;
    }

    uint8_t* addr(int offs) const { return base_ptr->data +(-1 + offs); }
    bool ok() const { return !!base_ptr; }

    void put(uint8_t const* p, uint8_t const* end) {
        BOOST_ASSERT( base_ptr );
        unsigned siz = end - p;
        if (capacity - size < siz) {
            capacity += (siz+2047)/1024*1024;
            base_ptr = (Nal*)realloc(base_ptr, capacity);
        }
        memcpy(addr(size), p, siz);
        size += siz;
    }

private:
    mbuffer(mbuffer const&);// = delete;
    mbuffer& operator=(mbuffer const&);// = delete;
};

struct data_sink
{
    // uint8_t *begin_, *end_;
    std::deque<mbuffer> bufs_;

    //signed char nri_ = -1;
    //mbuffer bufarray_[6];
    //typedef boost::intrusive::slist<mbuffer,boost::intrusive::cache_last<true>> slist_type;
    //slist_type blis_, blis_ready_;

    virtual void statis(rtp_header* rh, uint8_t const* data, uint8_t const* end) = 0;

    virtual ~data_sink() {}

    void free_buf(mbuffer& b) {
        //hgs_buffer_release(b.bufindex, 0, 0xf7);
        //b.used = 0;
    }

    mbuffer locate_buf(rtp_header const& rh)
    {
        return mbuffer(rh);
#if 0
        auto head_not_used = [this](slist_type& lis) {
            if (!lis.empty()) {
                auto it = lis.before_begin();
                auto prev_it = it++;
                for (; it != lis.end(); prev_it=it++) {
                    if (!it->used) {
                        lis.splice_after(lis.before_begin(), lis, prev_it);
                        return lis.begin();
                    }
                }
            }
            LOGE("locate_buf:head_not_used: None");
            return lis.end();
        };
        //auto discard_byseq = [this](slist_type& lis, rtp_header* rh/*slist_type::iterator& prev_it*/) {
        //    if (lis.empty())
        //        return lis.end();
        //    auto it = lis.before_begin();
        //    auto prev_it = it++;
        //    while (it != lis.end()) {
        //        if (((rh->seq < it->rtp_h.seq ? 0x10000u:0u) + rh->seq - it->rtp_h.seq) >= 6) {
        //            LOGD("seq %d discard: %d", rh->seq, it->rtp_h.seq);
        //            free_buf(*it++);
        //            //lis.splice_after(lis.before_begin(), lis, prev_it);
        //        } else 
        //            prev_it=it++;
        //    }
        //    if (it == lis.end()) {
        //        LOGE("locate_buf:discard_byseq: None");
        //        return it;
        //    }
        //    lis.splice_after(lis.before_begin(), lis, prev_it);
        //    return lis.begin();
        //};
#define NRI(h) (int((h)->nri)&0x3)
        auto discard_bynri = [this](slist_type& lis, int nri) {
            if (lis.empty())
                return 0;
            auto min_it = lis.before_begin();
            auto prev_min_it = min_it++;
            auto it = lis.begin();
            auto prev_it = it++;
            while (it != lis.end()) {
                if (NRI(&it->nal_h) <= NRI(&min_it->nal_h)) {
                    min_it = it;
                    prev_min_it = prev_it;
                }
                prev_it = it++;
            }
            if (NRI(&min_it->nal_h) >= nri)
                return 0;
            hgs_buffer_release(min_it->bufindex, 0, 0xf7);
            blis_.splice_after(blis_.before_begin(), lis, prev_min_it);
            return 1;
        };
        auto found = [this,rh,nh](slist_type::iterator it) { //BOOST_SCOPE_EXIT(this,&it,rh,nh);
            if (nri_>=0) { //(nri_ >= 0 && nri >= 0x3)
                LOGD("reset nri_");
                this->nri_ = -1;
            }
            it->rtp_h = *rh;
            it->nal_h = *nh;
            it->used = 1;
            return it.operator->();
        };

        //if (blis_.empty() && blis_ready_.empty()) { // init
        //    for (int x=0, xe = sizeof(bufarray_)/sizeof(bufarray_[0]); x < xe; ++x) {
        //        int idx;
        //        while ( (idx = hgs_buffer_obtain(1000)) < 0) {
        //            LOGE("buffer obtain x: %d", idx);
        //        }
        //        bufarray_[x].bufindex = idx;
        //        blis_.push_front(bufarray_[x]);
        //    }
        //}
        BOOST_ASSERT(nri_ < 0x3);

        int nri = NRI(nh);
        if (nri <= nri_) {
            LOGD("nri %d discard: %d", nri_, nri);
            return 0;
        }

        slist_type::iterator it = head_not_used(blis_);
        if (it != blis_.end()) {
            return found(it);
        }

        if (!discard_bynri(blis_, nri) && !discard_bynri(blis_ready_, nri)) {
            BOOST_ASSERT(nri < 0x3);
            LOGW("discard_bynri fail: nri %d", nri);
            nri_ = nri;
            return 0;
        }

        BOOST_ASSERT(!blis_.empty() && !blis_.front().used);
        return found(blis_.begin());
#endif
    }

    void put(mbuffer& bp, uint8_t const* data, uint8_t const* end)
    {
        bp.put(data, end);
        this->statis(&bp.rtp_h, data, end);

        //hgs_buffer_inflate(bp->bufindex, (char*)data, end-data);
        //return bp;
    }
    void commit(mbuffer&& bp)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!bufs_.empty() && bufs_.size() % 10 == 0) {
            LOGD("bufs.size %d", (int)bufs_.size());
        }
        bufs_.push_back(std::move(bp));
        //auto it = iterator_before(blis_, *bp);
        //auto p = it++;
        //blis_ready_.splice_after(blis_ready_.last(), blis_, p);
    }

    void commit(rtp_header const& rh, uint8_t* data, uint8_t* end)
    {
        commit(mbuffer(rh, data, end));
        //auto* bp = locate_buf(rh,nh);
        //if (bp) {
        //    put(bp, data, end);
        //    commit(bp, flags);
        //}

        //int flags = 0;
        //switch (nh->type) {
        //    case 0: case 7: case 8: flags = BUFFER_FLAG_CODEC_CONFIG; break;
        //}
        //int idx = hgs_queue_input(data, end, flags, rh->timestamp);
    }

    //void commit0(uint8_t* data, uint8_t* end, unsigned timestamp, int flags)
    //{
    //    int idx = hgs_queue_input(data, end, flags, 0);
    //    //int idx;
    //    //if ((idx = hgs_buffer_obtain(1000)) < 0) {
    //    //    LOGE("buffer obtain: %d", idx);
    //    //    return;
    //    //}
    //    //hgs_buffer_inflate(idx, (char*)data, end-data);
    //    //hgs_buffer_release(idx, timestamp, flags);
    //}

    //int size() const { return bufs_.size(); }

    void input_queue_swap()
    {
        int idx = -1;
        mbuffer buf;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (bufs_.empty())
                return;

            idx = hgs_buffer_obtain(50);
            if (idx < 0) {
                LOGW("buffer obtain: %d", idx);
                return;
            }

            buf = std::move(bufs_.front()); // blis_ready_.pop_front();
            bufs_.pop_front();
        }

        int flags = 0;
        switch (buf.base_ptr->nal_h.type) {
            case 0: case 7: case 8: flags = BUFFER_FLAG_CODEC_CONFIG; break;
        }

        hgs_buffer_inflate(idx, (char*)buf.addr(-4), 4+buf.size);
        hgs_buffer_release(idx, 1, flags);

        //=int idx;
        //=while (!blis_ready_.empty() && (idx = hgs_buffer_obtain(0)) >= 0) {
        //=    mbuffer& b = blis_ready_.front(); // blis_ready_.pop_front();

        //=    hgs_buffer_release(b.bufindex, b.rtp_h.timestamp, b.flags);

        //=    b.bufindex = idx;
        //=    b.used = 0;
        //=    blis_.splice_after(blis_.last(), blis_ready_, blis_ready_.before_begin());
        //=}

        //++ncommit_;
        //unsigned ts = stimestamp();
        //if (ts - ts_ >= 1000) {
        //    LOGD("F-RATE %u/%u, totsiz %u", ncommit_, (ts-ts_), totsiz_); //("BUFFER_FLAG_CODEC_CONFIG");
        //    ts_ = ts;
        //    ncommit_ = 0;
        //}
    }

    //static slist_type::iterator iterator_before(slist_type& blis, mbuffer& b)
    //{
    //    auto it = blis.before_begin();
    //    auto p = it++;
    //    for (; it != blis.end(); p=it++) {
    //        if (it.operator->() == &b)
    //            break;
    //    }
    //    return p;
    //}

    // unsigned ts_ = 0, ncommit_ = 0, totsiz_ = 0; // test-only

    std::mutex mutex_;
};

struct h264nal : private boost::noncopyable
{
    // std::string sps_, pps_;

    void sprop_parameter_sets(std::string sps, std::string pps)
    {
        BOOST_STATIC_ASSERT(sizeof(int)==4);
        if (sps.empty())
            return;
        uint8_t sbytes[4] = {0,0,0,1};

        mbuffer b = sink_->locate_buf( rtp_header{} );
        b.put((uint8_t*)sps.data(), (uint8_t*)sps.data()+sps.length());
        b.put(&sbytes[0], &sbytes[4]);
        b.put((uint8_t*)pps.data(), (uint8_t*)pps.data()+pps.length());
        sink_->commit(std::move(b));
    }

    void nalu_incoming(rtp_header* rtp_h, uint8_t* data, uint8_t* end) //const
    {
        nal_unit_header* h1 = nal_unit_header::cast(data, end);
        if (!h1)
            return;
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
                        sink_->commit(*rtp_h, data, data+len); //sink_->commit(timestamp, 0);
                        data += len; //
                    }
                }
                break;
            case 28: // FU-A
                data += 1; //h1->length();
                if (fu_header* h2 = fu_header::cast(data,end)) {
                    h2->print(data,end);
                    fu_header fuh = *h2;
                    nal_unit_header* h3 = nal_unit_header::cast(data,end);
                    if (fuh.s) {
                        h3->nri = h1->nri;
                        h3->f = h1->f;
                        h3->print(data,end);
                        if (bufp_.ok()) {
                            LOGE("FU-A e loss");
                        }
                        bufp_ = sink_->locate_buf(*rtp_h);
                    } else {
                        BOOST_ASSERT(bufp_.size > 0);
                        if (bufp_.size <= 0) {
                            LOGE("FU-A b.size error");
                            return;
                        }
                        ++data;
                    }
                    sink_->put(bufp_, data, end);
                    if (fuh.e) {
                        sink_->commit(std::move(bufp_));
                    }
                }
                break;
            default:
                if (h1->type < 24) {
                    sink_->commit(*rtp_h, data, end);
                } else {
                    LOGE("nal-unit-type %d", h1->type);
                }
                break;
        }
    }

    void lose(unsigned seq) {
        bufp_ = mbuffer();
    }

    void set_data_sink(data_sink* sink) {
        sink_ = sink;
    }

    data_sink* sink_ = 0;
    mbuffer bufp_;
};

struct rtp_receiver : h264nal //, private boost::noncopyable
{
    typedef rtp_receiver This;

    rtp_receiver(boost::asio::io_service& io_s, FILE* dump_fp)
        : udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), local_port()))
    {
        LOGD("rtp_receiver");
    }

    void teardown() {
        boost::system::error_code ec;
        udpsock_.close(ec);
    }

    void start_playing() { handle_receive_from(boost::system::error_code(), 0); }

    static int local_port(int p=0) {
        static int port=0;
        if (!port)
            port = 1000 + (int)time(0) % 3600;
        return port+p;
    }

private: // rtp communication
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
                    LOGE("rtp_header"); return;
                } else {
                    h->print(bufptr(), bufptr()+bytes_recvd); //bufptr += h->length();
                    if (h->seq == (expseq_&0xffff)) {
                        ++expseq_;
                    } else {
                        LOGE("exp-seq %d:%d", expseq_, h->seq);
                        if (h->seq < (expseq_&0xffff)) {
                            return;
                        }
                        expseq_ = h->seq+1;
                    }
                    rtp_header rtp_h = *h;
                    this->nalu_incoming(&rtp_h, bufptr()+h->length(), bufptr()+bytes_recvd);
                }
            }
            using namespace boost::asio;
            udpsock_.async_receive_from( boost::asio::buffer(bufptr(), BufSiz)
                    , peer_endpoint_
                    , boost::bind(&This::handle_receive_from, this, placeholders::error, placeholders::bytes_transferred));
        }
    }

    ip::udp::socket udpsock_;
    ip::udp::endpoint peer_endpoint_;
    unsigned expseq_ = 0;
    //std::array<uint8_t*,8> bufs_ = {};

    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_)+4; }
    enum { BufSiz = (1024*32-4) };
    int buf_[BufSiz/sizeof(int)+1];
};

struct rtcp_client : boost::noncopyable
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

    struct rtcp_data_sink : data_sink {
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

        virtual void statis(rtp_header* rh, uint8_t const* data, uint8_t const* end) {
            auto* s = this;
            if (s->cycles == 0) {
                s->cycles = 1;
                s->base_seq = s->max_seq = rh->seq;
                s->received = 1;
            } else {
                if (rh->seq < s->max_seq) {
                    s->cycles++;
                }
                s->max_seq = rh->seq;
                s->received++;
            }

            {
                uint32_t arrival = stimestamp();
                arrival = std::max(arrival, rh->timestamp);
                int transit = arrival - rh->timestamp;
                int d = transit - s->transit;
                s->transit = transit;
                if (d < 0) d = -d;
                s->jitter += d - ((s->jitter + 8) >> 4); // ... rr->jitter = s->jitter >> 4;
            }
        }
    };
    rtcp_data_sink sink_;

    rtcp_client(boost::asio::io_service& io_s, rtp_receiver* rtp/*, ip::udp::endpoint const& remote_ep*/)
        : udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), rtp->local_port(+1)))
    {
        BOOST_STATIC_ASSERT(sizeof(rtcp_header)%4==0);
        BOOST_STATIC_ASSERT(sizeof(rtcp_report_block)%4==0);
        BOOST_STATIC_ASSERT(sizeof(receiver_report)%4==0);

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
        snprintf(sdes.c.data,6, "%c%04d.", char(rand()%26+'A'), rtp->local_port()); //strcpy(sdes.c.data, "helo");

        rtp->set_data_sink(&sink_);

        //udpsock_.connect(remote_ep);
        handle_receive_from(boost::system::error_code(), 0);
    }

    void teardown() {
        boost::system::error_code ec;
        udpsock_.close(ec);
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
                ts_sr_ = stimestamp( sink_.received==0 );
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
        if (ts_rr_ +100 < ts_sr_ && sink_.received > 0) {
            auto* s = &sink_;

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

    // rtp_receiver* rtp_;

    ip::udp::socket udpsock_;
    ip::udp::endpoint peer_endpoint_;

    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_); }
    enum { BufSiz = (1024*4) };
    int32_t buf_[BufSiz/sizeof(int32_t)];
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
                                if (unsigned clen = Action_helper<Action>::_parse_head(derived->response_, derived->session_)) {
                                    LOGD("cLength: %d", clen);
                                    boost::asio::async_read(derived->tcpsock_, derived->response_, boost::asio::transfer_exactly(clen)
                                        , [derived](boost::system::error_code ec, size_t) {
                                            if (ec) {
                                                derived->on_error(ec, Action{});
                                            } else {
                                                Action()(derived);
                                            }
                                        });
                                } else {
                                    Action()(derived);
                                }
                            }
                        });
            }
        }
        Derived* derived_; // Action_helper(Derived* d) : Action{d} {}

        static unsigned _parse_head(boost::asio::streambuf& rspbuf, std::string& session)
        {
            size_t clen = size_t(-1);
            auto  bufs = rspbuf.data();
            auto* beg = boost::asio::buffer_cast<const char*>(bufs);
            auto* end = beg + boost::asio::buffer_size(bufs);
            decltype(end) eol, p = beg;
            while ( (eol = std::find(p,end, '\n')) != end) {
                auto* e = eol++;
                while (e != p && isspace(*(e-1)))
                    --e;
                // LOGD("%.*s", int(e-p), p);
                auto linr = boost::make_iterator_range(p,e);
                if (boost::istarts_with(linr, "Content-Length")) {
                    re::regex rexp("^([^:]+):\\s+(.+)$");
                    re::cmatch m;
                    if (re::regex_match(p,e, m, rexp)) {
                        clen = atoi(m[2].first);
                        //std::clog << "Content-Length " << clen << "\n";
                    }
                } else if (boost::starts_with(linr, "Session")) {
                    re::regex rexp("Session:[[:space:]]*([^[:space:]]+)");
                    re::cmatch m;
                    if (re::regex_search(p,e, m, rexp)) {
                        session.assign(m[1].first, m[1].second);
                    }
                }
                if (p == e)
                    break;
                p = eol;
            }
            size_t hlen = eol - beg; // rspbuf.consume(eol - beg);
            if (clen != size_t(-1) && rspbuf.size() < clen+hlen) {
                return int(clen + hlen - rspbuf.size());
            }
            return 0;
        }
    };
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
struct h264file_mmap 
{
    typedef std::pair<uint8_t*,uint8_t*> range;
    uint8_t *begin_, *end_;

    h264file_mmap(int fd) {
        struct stat st; // fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);

        void* p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
            void* m = malloc(st.st_size);
            memcpy(m, p, st.st_size);
            begin_ = (uint8_t*)m;
            end_ = begin_ + st.st_size;
        munmap(p, st.st_size);

        assert(begin_[0]=='\0'&& begin_[1]=='\0'&& begin_[2]=='\0'&& begin_[3]==1);
    }
    ~h264file_mmap() {
        //close(fd);
        if (begin_) free(begin_);
    }

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

struct h264file_printer : boost::noncopyable, data_sink
{
    h264file_mmap h264f_;
    h264file_mmap::range range_ = {};
    h264nal nalu_;
    rtp_header rtp_h = {}; // unsigned ts_ = 0;

    virtual void statis(rtp_header* rh, uint8_t const* data, uint8_t const* end) {}

    h264file_printer(int fd) : h264f_(fd) {
        nalu_.sink_ = this;
        range_ = h264f_.begin();
    }

    int setup(int, char*[]) {
        return 0;
    }

    void pump() {
        if (this->size() > 10) {
            return;
        }

        if (range_.first < range_.second) {
            nalu_.nalu_incoming(&rtp_h, range_.first, range_.second);
            rtp_h.seq = htons(ntohs(rtp_h.seq)+1);
            rtp_h.timestamp = htonl(ntohl(rtp_h.timestamp)+20);
        }
        range_ = h264f_.next(range_);

        //ntohl(rtp_h.timestamp)
        //if (ts_++ == 400) {
        //    ts_ = 0;
        //    range_ = h264f_.begin();
        //}
    }
};

static struct test_h264file {
    int h264file_fd = -1;
    // std::pair<uint8_t*,uint8_t*> range_ = {};
    h264file_printer* hfile_; //h264file_mmap* fmap_;
    // int ncommit_ = 0;
    // int bufindex; int frindex;
} test_;

void hgs_poll_once(int)
{
    test_.hfile_->pump();
    test_.hfile_->input_queue_swap();
}

void hgs_exit(int preexit)
{
    if (preexit==0) {
        LOGD("exit 0");
        close(test_.h264file_fd);
        test_.h264file_fd = -1;
    }
}

void hgs_init(char const* ip, int port, char const* path, int w, int h)
{
#   ifdef __ANDROID__
    path = "/sdcard/a.h264";
#   else
    path = "a.h264";
#   endif
    LOGD("init");
    test_.h264file_fd = open(path, O_RDONLY);
    test_.hfile_ = new h264file_printer(test_.h264file_fd);
    // test_.fmap_ = new h264file_mmap(test_.h264file_fd);
}

#else

// #include <boost/thread/mutex.hpp>

struct App {
    bool running = 0;
    std::thread thread;
    boost::asio::io_service io_service;

    rtp_receiver* rtp = 0;
    rtcp_client* rtcp = 0;
    rtsp_client* rtsp = 0;

    ~App() {
        delete rtsp;       rtsp = 0;
        delete rtcp;       rtcp = 0;
        delete rtp;        rtp = 0;
    }
};
static App hgs_;

/// rtsp://127.0.0.1:7654/rtp1
/// rtsp://192.168.2.3/live/ch00_2
void hgs_init(char const* ip, int port, char const* path, int w, int h) // 640*480 1280X720 1920X1080
{
    LOGD("init %s:%d %s", ip, port, path);

    srand( time(0) );

    auto endp = ip::tcp::endpoint(ip::address::from_string(ip), (port));

    hgs_.rtp = new rtp_receiver(hgs_.io_service, 0/*fopen("/tmp/1.rtp.h264","w")*/); // auto* c = new (&objmem_) rtp_receiver(hgs_.io_service, endp, path, 0);
    hgs_.rtcp = new rtcp_client(hgs_.io_service, hgs_.rtp);
    hgs_.rtsp = new rtsp_client(hgs_.io_service, hgs_.rtp, endp, path);

    // auto* hc = reinterpret_cast<rtp_receiver*>(&objmem_);
    LOGD("init ok");
}

void hgs_exit(int preexit)
{
    hgs_.running = 0;
    hgs_.io_service.post([preexit](){
        if (preexit) {
            LOGD("teardown");
            hgs_.rtp->teardown();
            hgs_.rtcp->teardown();
            hgs_.rtsp->teardown();
            // hgs_.io_service.poll_one();
        } else {
            LOGD("exit:stop");
            hgs_.io_service.stop();
            LOGD("exit:join thread");
        }
    });
    if (preexit == 0)
        hgs_.thread.join();
}

void hgs_poll_once(int)
{
    //LOGD("poll_one");
    //auto* c = reinterpret_cast<rtp_receiver*>(&objmem_);

    hgs_.io_service.poll_one();
    // if (codec) hgs_.rtcp->sink_.input_queue_swap();
}

void hgs_run()
{
    LOGD("run: %d", !!hgs_.rtsp);
    hgs_.running = 1;
    hgs_.rtsp->setup(0,0);

    hgs_.thread = std::thread([](){ hgs_.io_service.run(); });
}

void hgs_pump()
{
    if (hgs_.running) {
        hgs_.io_service.post([](){ hgs_.rtcp->rreport(); });
        hgs_.rtcp->sink_.input_queue_swap();
    }
}

#endif

