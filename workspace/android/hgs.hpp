#ifndef HGS_HPP__
#define HGS_HPP__
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#if defined(__ANDROID__)
#  include <jni.h> //<boost/regex.hpp>
#  define NOT_PRINT_PROTO 1
#endif

int hgs_register_natives(JNIEnv* env);
int hgs_save_JNIEnv(JNIEnv* env);

struct QueryDecoded
{
    void* data;
    unsigned size;

    bool empty() const { return oidx_ < 0; }

    QueryDecoded(JNIEnv* env=0) {
        oidx_ = query_s(env, &data, &size);
    }
    ~QueryDecoded() { _release(); }

    QueryDecoded(QueryDecoded && rhs) {
        data = rhs.data;
        size = rhs.size;
        oidx_ = rhs.oidx_;
        rhs.oidx_ = -1;
    }
    QueryDecoded& operator=(QueryDecoded && rhs) {
        if (this != &rhs) {
            this->_release();
            data = rhs.data;
            size = rhs.size;
            oidx_ = rhs.oidx_;
            rhs.oidx_ = -1;
        }
        return *this;
    }
private:
    int oidx_;
    void _release() {
        if (oidx_ >= 0) {
            release_s(oidx_);
        }
    }
    static int query_s(JNIEnv* env, void**data, unsigned* size);
    static void release_s(int idx);
    QueryDecoded(QueryDecoded const&);
    QueryDecoded& operator=(QueryDecoded const&);
};


////////////////////////////////////// //////////////////////////////////////
//
//
#if !defined(NOT_PRINT_PROTO)
inline char* xsfmt(char xs[],unsigned siz, uint8_t*data,uint8_t*end)
{
    int len=std::min(16,int(end-data));
    for (int j=0, i=0; j < (int)siz && i < len; ++i)
        j += snprintf(&xs[j],siz-j, ((i>1&&i%2==0)?" %02x":"%02x"), (int)data[i]);
    return xs;
}
#endif

struct rtp_header
{
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

struct mbuffer /*: boost::intrusive::slist_base_hook<>*/
{
    rtp_header rtp_h;
    struct Nal {
        uint32_t _4bytes;
        nal_unit_header nal_h; // size 0 pos
        uint8_t date_p_[1];
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

    void put(uint8_t const* p, uint8_t const* end) {
        assert( base_ptr );
        unsigned siz = end - p;
        if (capacity - this->size < siz) {
            capacity += (siz+2047)/1024*1024;
            base_ptr = (Nal*)realloc(base_ptr, capacity);
        }
        memcpy(addr(this->size), p, siz);
        this->size += siz;
    }
    uint8_t* addr(int offs) const { return base_ptr->date_p_ +(-1 + offs); }
    uint8_t* nal_header() const { return addr(0); }
    uint8_t* frame_data() const { return addr(1); }

    bool _using() const { return bool(base_ptr); }
private:
    mbuffer(mbuffer const&);// = delete;
    mbuffer& operator=(mbuffer const&);// = delete;
};

#include <functional>

void hgs_exit(int);
void hgs_run(std::function<void(mbuffer)> sink);
void hgs_init(char const* ip, int port, char const* path, int w, int h);

#endif // HGS_HPP__

