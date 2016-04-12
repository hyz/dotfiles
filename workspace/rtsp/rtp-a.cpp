#include <cstdlib>
#include <fstream>
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

using boost::asio::ip::udp;

static char* xsfmt(char xs[],unsigned siz, uint8_t*data,uint8_t*end)
{
    int j=0;
    for (int i=0, n=std::min(16,int(end-data)); i < n; ++i)
        j += snprintf(&xs[j],siz-j, ((i>1&&i%2==0)?" %02x":"%02x"), (int)data[i]);
    return xs;
}

struct rtp_header
{
    // 12: version 2 p 0 x 0 cc 0 pt 96: 80e0 2f04 4691 97f6
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
        char xs[128] = {};
        printf("%4u:%u: version %d p %d x %d cc %d pt %d: %s\n"
                , int(end-data), this->length()
                , this->version, this->p, this->x, this->cc, this->pt
                , xsfmt(xs,sizeof(xs), data,end));
    }
};

struct nal_unit_header
{
    // char *data_, *end_;
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
        char xs[128] = {};
        printf("%4u:%u: f %d nri %d type %d: %s\n"
                , int(end-data), this->length()
                , this->f, this->nri, this->type
                , xsfmt(xs,sizeof(xs), data,end));
    }
};
#define MAX_NAL_FRAME_LENGTH	200000

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
        char xs[128] = {};
        printf("%4u:%u: s %d e %d type %d: %s\n"
                , int(end-data), this->length()
                , this->s, this->e, this->type
                , xsfmt(xs,sizeof(xs), data,end));
    }
};
#define FU_A_HEADER_LENGTH	2

template <typename Int,unsigned> struct Ntoh_;
template <typename Int> struct Ntoh_<Int,4> { static Int cast(Int val) { return ntohl(val); } };
template <typename Int> struct Ntoh_<Int,2> { static Int cast(Int val) { return ntohs(val); } };
//template <typename Int> struct Ntoh_<Int,1> { static Int cast(Int val) { return (val); } };
template <typename Int> Int Ntoh(uint8_t* data,uint8_t* end)
{
    if (end-data < sizeof(Int))
        return (Int)0; //ERR_EXIT("")//TODO
    Int val;
    memcpy(&val, data, sizeof(Int));
    return Ntoh_<Int,sizeof(Int)>::cast(val);
}

struct Main : boost::asio::io_service
{
    BOOST_STATIC_ASSERT(__BYTE_ORDER == __LITTLE_ENDIAN);

    Main(short port/*, boost::filesystem::path dir*/)
        : socket_(*this, udp::endpoint(udp::v4(), port))
        //, dir_(dir)
    {
        socket_.async_receive_from(
                boost::asio::buffer(data(), max_length), peer_endpoint_,
                boost::bind(&Main::handle_receive_from, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

    //void run() { boost::asio::io_service::run(); }
private:
    static void rtp_a(uint8_t* data, uint8_t* end)
    {
        rtp_header* h0 = rtp_header::cast(data, end);
        if (!h0)
            return;
        nal_unit_header* h1 = nal_unit_header::cast(data + h0->length(), end);
        if (!h1)
            return;

        if (h1->type == 1) // Ignore
            return;
        BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END

        h0->print(data,end);
        data += h0->length();

        h1->print(data,end);
        data += 1; //h1->length();

        switch (h1->type) {
            case 24: // STAP-A
                while (end-data >= 2) {
                    uint16_t len = Ntoh<uint16_t>(data,end);
                    data += 2;
                    if (nal_unit_header* h2 = nal_unit_header::cast(data,data+len)) {
                        h2->print(data,data+len);

                        data += len; //
                    }
                }
                break;
            case 28: // FU-A
                if (fu_header* h2 = fu_header::cast(data,end)) {
                    h2->print(data,end);
                    fu_header cpy = *h2;

                    nal_unit_header* h = nal_unit_header::cast(data,end);
                    h->nri = h1->nri;
                    h->f = h1->f;
                    h->print(data,end);
                    if (cpy.s)
                        printf("FU-A START\n");
                    else if (cpy.e)
                        printf("FU-A END\n");
                    else if (cpy.s && cpy.e)
                        printf("FU-A START&END\n");
//1472:12: version 2 p 0 x 0 cc 0 pt 96: 8060 ab0e 04a0 f0b1
//1460:1: f 0 nri 2 type 28: 5c81 88c0 94bf fff0
//1459:1: s 1 e 0 type 1: 8188 c094 bfff f044
//1459:1: f 0 nri 2 type 1: 4188 c094 bfff f044

                    //data += h.length();
                }
                writev;
                break;
        }
    }

    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
    {
        if (!error && bytes_recvd > 0) {
            rtp_a(data(), data()+bytes_recvd);
            //boost::filesystem::ofstream ofs(dir_/std::to_string(dg_idx_++));
            //ofs.write(data(), bytes_recvd);
        }
        using namespace boost::asio::placeholders;
        socket_.async_receive_from(
                boost::asio::buffer(data(), max_length), peer_endpoint_,
                boost::bind(&Main::handle_receive_from, this, error, bytes_transferred));
    }

    udp::socket socket_;
    udp::endpoint peer_endpoint_;
    enum { max_length = 4096 };
    uint32_t data_[max_length/sizeof(uint32_t)+1];

    uint8_t* data() const { return reinterpret_cast<uint8_t*>(&const_cast<Main*>(this)->data_); }

    //boost::filesystem::path dir_;
    int dg_idx_ = 0;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            fprintf(stderr,"Usage: %s <port>\n", argv[0]);
            return 1;
        }

        using namespace std; // For atoi.
        Main s(atoi(argv[1]));
        s.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
