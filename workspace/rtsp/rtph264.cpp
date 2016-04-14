#include <cstdlib>
#include <fstream>
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

template <typename... Args> void err_exit_(int lin_, char const* fmt, Args... a)
{
    fprintf(stderr, fmt, lin_, a...);
    exit(127);
}
#define ERR_EXIT(...) err_exit_(__LINE__, "%d: " __VA_ARGS__)

template <typename Int,int> struct Ntoh_;
template <typename Int> struct Ntoh_<Int,4> { static Int cast(Int val) { return ntohl(val); } };
template <typename Int> struct Ntoh_<Int,2> { static Int cast(Int val) { return ntohs(val); } };
//template <typename Int> struct Ntoh_<Int,1> { static Int cast(Int val) { return (val); } };
template <typename Int> Int Ntoh(uint8_t* data,uint8_t* end)
{
    if (end-data < sizeof(Int))
        ERR_EXIT("Ntoh");
    Int val;
    memcpy(&val, data, sizeof(Int));
    return Ntoh_<Int,sizeof(Int)>::cast(val);
}

template <typename I2>
void base64dec(char const* beg, char const* end, I2 out_it)
{
    while (end > beg && *(end-1) == '=')
        --end;
    using namespace boost::archive::iterators;
    using Iter = transform_width<binary_from_base64<char const*>,8,6>;
    std::copy(Iter(beg), Iter(end), out_it);
    //return boost::algorithm::trim_right_copy_if( std::string(Iter(beg), Iter(end)), [](char c) { return c == '\0'; });
}
//std::string base64enc(const std::string &val) {
//    using namespace boost::archive::iterators;
//    using Iter = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
//    auto tmp = std::string(Iter(std::begin(val)), Iter(std::end(val)));
//    return tmp.append((3 - val.size() % 3) % 3, '=');
//}

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
        printf("%4u:%u: version %d p %d x %d cc %d pt %d seq %d: %s\n"
                , int(end-data), this->length()
                , this->version, this->p, this->x, this->cc, this->pt, this->seq
                , xsfmt(xs,sizeof(xs), data,end));
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
    }
};
#define FU_A_HEADER_LENGTH	2

struct nal_unit
{
    std::unique_ptr<FILE,decltype(&fclose)> fp_;
    std::string sps_, pps_;

    explicit nal_unit(FILE* fp, std::string sps, std::string pps)
        : fp_(fp, fclose) , sps_(sps) , pps_(pps)
    {
        std::vector<int> buf( std::max(sps_.size(),pps_.size())/sizeof(int)+1 );
        uint8_t* begin = reinterpret_cast<uint8_t*>( &buf[0] );
        //uint8_t* end = begin + buf.size()*sizeof(int);

        //nal_unit_header* h = nal_unit_header::cast(begin,end);
        //h->nri = 3; // 0x67, 0x68
        //h->type = 7;

        memcpy(begin, sps_.data(), sps_.length());
        dump(begin, begin+sps_.length());

        //h->type = 8;
        memcpy(begin, pps_.data(), pps_.length());
        dump(begin, begin+pps_.length());
    }
    explicit nal_unit(FILE* fp=0) : fp_(fp, fclose) {}

    void dump(uint8_t* data, uint8_t* end) const
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
                        nalu_write(data, data+len);

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
                    nalu_write(fuh.s, data + !fuh.s, end);
                }
                break;
            default:
                if (h1->type < 24)
                    nalu_write(data, end);
                break;
        }
    }

    void nalu_write(bool sbytes, uint8_t const* data, uint8_t const* end) const
    {
        if (fp_) {
            if (sbytes) {
                uint8_t zd[4] = {0,0,0,1};
                fwrite(&zd, sizeof(zd), 1, fp_.get());
            }
            fwrite(data, end-data, 1, fp_.get());
        }
    }
    void nalu_write(uint8_t const* data, uint8_t const* end) const { nalu_write(1, data, end); }
};

struct h264_reader : nal_unit, boost::noncopyable
{
    std::unique_ptr<FILE,decltype(&fclose)> fp_;

    h264_reader(FILE* fp) : fp_(fp,fclose)
    {}
    int setup(int, char*[])
    {
        FILE* fp = fp_.get();
        struct stat st;
        if (fstat(fileno(fp), &st) <0)
            ERR_EXIT("fstat");

        std::vector<int> buf( st.st_size/sizeof(int)+1 );
        uint8_t* begin = reinterpret_cast<uint8_t*>( &buf[0] );
        uint8_t* end = begin + buf.size()*sizeof(int);

        if ((int)fread(begin, 1,st.st_size, fp) != st.st_size)
            ERR_EXIT("fread");
        uint8_t dx[] = {0, 0, 0, 1};

        begin = std::search(begin, end, &dx[0], &dx[4]) + 4;
        while (begin < end) {
            uint8_t* p = std::search(begin, end, &dx[0], &dx[4]);
            if (p > begin)
                this->dump(begin, p);
            begin = p + 4;
        }
        return 0;
    }
};

struct rtph264_client : boost::noncopyable, nal_unit
{
    typedef rtph264_client This;

    rtph264_client(boost::asio::io_service& io_s
            , int port
            , std::string sps, std::string pps, FILE* outfile)
        : nal_unit(outfile, sps, pps)
        , tcpsock_(io_s)
        , udpsock_(io_s, ip::udp::endpoint(ip::udp::v4(), port))
    {}

    int setup(int, char*[])
    {
        udpsock_.async_receive_from(
                boost::asio::buffer(bufptr(), BufSiz), peer_endpoint_,
                boost::bind(&This::handle_receive_from, this,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        return 0;
    }

private:
    void handle_receive_from(const boost::system::error_code& ec, size_t bytes_recvd)
    {
        if (!ec && bytes_recvd > 0) {
            rtp_header* h = rtp_header::cast(bufptr(), bufptr()+bytes_recvd);
            if (!h)
                ERR_EXIT("rtp_header::cast");

            h->print(bufptr(), bufptr()+bytes_recvd); //bufptr += h->length();
            this->dump(bufptr()+h->length(), bufptr()+bytes_recvd);
            //boost::filesystem::ofstream ofs(dir_/std::to_string(dg_idx_++));
        }
        using namespace boost::asio;
        udpsock_.async_receive_from(
                boost::asio::buffer(bufptr(), BufSiz), peer_endpoint_,
                boost::bind(&This::handle_receive_from, this, placeholders::error, placeholders::bytes_transferred));
    }

    ip::tcp::socket tcpsock_;
    ip::udp::socket udpsock_;
    ip::udp::endpoint peer_endpoint_;

    //boost::filesystem::path dir_; int dg_idx_ = 0;
    //std::aligned_storage<1024*64,alignof(int)>::type data_;
    uint8_t* bufptr() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->buf_); }
    enum { BufSiz = 1024*64 };
    int buf_[BufSiz/sizeof(int)+1];
};

//#include <boost/type_erasure/member.hpp>
//BOOST_TYPE_ERASURE_MEMBER((setup_fn), setup, 2)
// BOOST_TYPE_ERASURE_MEMBER((setup_fn), ~, 0)
//boost::type_erasure::any<setup_fn<int(int,char*[])>, boost::type_erasure::_self&> ;

struct Main : boost::asio::io_service, boost::noncopyable
{
    struct Args {
        Args(int ac, char* av[]) : port(0) {
            if (ac == 2) {
                if ( !(fp = fopen(av[1], "rb")))
                    ERR_EXIT("file %s: fail", av[1]);
            } else if (ac == 5) {
                port = atoi(av[1]);
                if ( !(fp = fopen(av[2], "wb")))
                    ERR_EXIT("file %s: fail", av[2]);
                b64dec(av[3], sps);
                b64dec(av[4], pps);
            } else {
                ERR_EXIT("Usage: %s <port>", av[0]);
            }
        }
        static void b64dec(char const* cs, std::string& out) {
            ::base64dec(cs, cs+strlen(cs), std::back_inserter(out));
        }
        int port = 0;
        FILE* fp;
        std::string sps, pps;
    };

    Main(int ac, char* av[]) : signals_(*this)
    {
        Args args(ac, av);
        if (args.port > 0) {
            auto* obj = new (&objmem_) rtph264_client(*this, args.port, args.sps, args.pps, args.fp);
            dtor_ = [obj]() { obj->rtph264_client::~rtph264_client(); };
            setup_ = [obj](int ac,char*av[]) { obj->setup(ac,av); };

            signals_.add(SIGINT);
            signals_.add(SIGTERM);
            //signals_.add(SIGQUIT);
            signals_.async_wait(boost::bind(&boost::asio::io_service::stop, this));
        } else {
            auto* obj = new (&objmem_) h264_reader(args.fp);
            dtor_ = [obj]() { obj->h264_reader::~h264_reader(); };
            setup_ = [obj](int ac,char*av[]) { obj->setup(ac,av); };
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
    static int objmem_[sizeof(rtph264_client)/sizeof(int)+1];
    BOOST_STATIC_ASSERT(__BYTE_ORDER == __LITTLE_ENDIAN);
};
int Main::objmem_[sizeof(rtph264_client)/sizeof(int)+1] = {};

int main(int argc, char* argv[])
{
    //BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END
    try {
        Main s(argc, argv);
        s.run(argc, argv);
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

// http://stackoverflow.com/questions/6394874/fetching-the-dimensions-of-a-h264video-stream?lq=1
//width = ((pic_width_in_mbs_minus1 +1)*16) - frame_crop_left_offset*2 - frame_crop_right_offset*2;
//height= ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 +1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);


