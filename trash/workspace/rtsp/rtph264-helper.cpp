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

template <typename I2>
void base64dec(char const* beg, char const* end, I2 out_it)
{
    while (end > beg && *(end-1) == '=')
        --end;
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<char const*>,8,6>;
    std::copy(It(beg), It(end), out_it);
    //return boost::algorithm::trim_right_copy_if( std::string(It(beg), It(end)), [](char c) { return c == '\0'; });
}
//inline std::string base64dec(char const* cstr) { return base64dec(cstr, cstr+strlen(cstr)); }

std::string base64enc(const std::string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

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

struct rtph264
{
    std::unique_ptr<FILE,decltype(&fclose)> fp_;
    std::string sps_, pps_;

    explicit rtph264(FILE* fp, std::string sps, std::string pps)
        : fp_(fp, fclose) , sps_(sps) , pps_(pps)
    {
        union { int ints[32]; uint8_t bytes[sizeof(ints)]; } u = {};

        nal_unit_header* h = nal_unit_header::cast(u.bytes,u.bytes+12);
        h->nri = 3; // 0x67, 0x68

        h->type = 7;
        memcpy(h+1, sps_.data(), sps_.length());
        dump(u.bytes, u.bytes+1+sps_.length());

        h->type = 8;
        memcpy(h+1, pps_.data(), pps_.length());
        dump(u.bytes, u.bytes+1+pps_.length());

        //nalu_write(fp_.get(), &h, (uint8_t*)sps_.data(), (uint8_t*)sps_.data()+sps_.length());
        //nalu_write(fp_.get(), &h, (uint8_t*)pps_.data(), (uint8_t*)pps_.data()+sps_.length());
    }
    explicit rtph264(FILE* fp=0) : fp_(fp, fclose) {}

    void dump(uint8_t* data, uint8_t* end) const
    {
        nal_unit_header* h1 = nal_unit_header::cast(data, end);
        if (!h1)
            return;

        if (h1->type == 1) // Ignore
            return;
        BOOST_SCOPE_EXIT(void){ printf("\n"); }BOOST_SCOPE_EXIT_END

        h1->print(data,end);
        data += 1; //h1->length();

        switch (h1->type) {
            case 7: // SPS
                nalu_write(data, end);
                break;
            case 8: // PPS
                nalu_write(data, end);
                break;
            case 24: // STAP-A
                while (end-data >= 2) {
                    uint16_t len = Ntoh<uint16_t>(data,end);
                    data += 2;
                    if (nal_unit_header* h2 = nal_unit_header::cast(data,data+len)) {
                        h2->print(data,data+len);
                        nalu_write(data, data+len);

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
                    nalu_write(data, end);

                    if (cpy.s)
                        printf("FU-A START\n");
                    else if (cpy.e)
                        printf("FU-A END\n");
                    else if (cpy.s && cpy.e)
                        printf("FU-A START&END\n");
                    //data += h.length();
                }
                break;
        }
    }

    void nalu_write( uint8_t const* data, uint8_t const* end) const
    {
        if (fp_) {
            uint8_t zd[4] = {0,0,0,1};
            fwrite(&zd, sizeof(zd), 1, fp_.get());
            fwrite(data, end-data, 1, fp_.get());
        }
    }
    void nalu_write(nal_unit_header const* h, uint8_t const* data, uint8_t const* end) const
    { //writev
        if (fp_) {
            uint8_t zd[4] = {0,0,0,1};
            fwrite(&zd, sizeof(zd), 1, fp_.get());
            fwrite(h, sizeof(*h), 1, fp_.get());
            fwrite(data, end-data, 1, fp_.get());
        }
    }
};

struct rtph264_client : boost::noncopyable, rtph264
{
    typedef rtph264_client This;
    rtph264_client(boost::asio::io_service& io_s
            , int port
            , std::string sps, std::string pps, FILE* outfile)
        : rtph264(outfile, sps, pps)
        , socket_(io_s, udp::endpoint(udp::v4(), port))
        //, dir_(dir)
    {
        socket_.async_receive_from(
                boost::asio::buffer(data(), max_length), peer_endpoint_,
                boost::bind(&This::handle_receive_from, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
    {
        if (!error && bytes_recvd > 0) {
            rtp_header* h = rtp_header::cast(data(), data()+bytes_recvd);
            if (!h)
                ERR_EXIT("rtp_header::cast");

            h->print(data(), data()+bytes_recvd); //data += h->length();

            this->dump(data()+h->length(), data()+bytes_recvd);
            //boost::filesystem::ofstream ofs(dir_/std::to_string(dg_idx_++));
            //ofs.write(data(), bytes_recvd);
        }
        using namespace boost::asio::placeholders;
        socket_.async_receive_from(
                boost::asio::buffer(data(), max_length), peer_endpoint_,
                boost::bind(&This::handle_receive_from, this, error, bytes_transferred));
    }

    udp::socket socket_;
    udp::endpoint peer_endpoint_;

    enum { max_length = 4096 };
    uint32_t data_[max_length/sizeof(uint32_t)+1];
    uint8_t* data() const { return reinterpret_cast<uint8_t*>(&const_cast<This*>(this)->data_); }

    //boost::filesystem::path dir_;
    int dg_idx_ = 0;
};

struct h264_reader : boost::noncopyable, rtph264
{
    h264_reader(FILE* infile) : rtph264()
    {
        struct stat st;
        if (fstat(fileno(infile), &st) <0)
            ERR_EXIT("fstat");

        std::vector<uint32_t> buf( st.st_size/sizeof(int32_t)+1 );
        uint8_t* begin = reinterpret_cast<uint8_t*>( &buf[0] );
        uint8_t* end = begin + st.st_size;

        if ((int)fread(begin, 1,st.st_size, stdin) != st.st_size)
            ERR_EXIT("fread");
        uint8_t dx[] = {0, 0, 0, 1};

        begin = std::search(begin, end, &dx[0], &dx[4]) + 4;
        while (begin < end) {
            uint8_t* p = std::search(begin, end, &dx[0], &dx[4]);
            if (p == begin)
                break;
            dump(begin, p);

            begin = p + 4;
        }
    }
};

struct Main : boost::asio::io_service, boost::noncopyable
{
    struct Args {
        Args(int ac, char* av[]) {
            if (ac == 2) {
                if ( !(fp = fopen(av[1], "rb")))
                    ERR_EXIT("file %s: fail", av[1]);
            } else if (ac == 5) {
                port = atoi(av[1]);
                if ( !(fp = fopen(av[1], "wb")))
                    ERR_EXIT("file %s: fail", av[2]);
                base64dec(av[3], sps);
                base64dec(av[4], pps);
            } else {
                ERR_EXIT("Usage: %s <port>", av[0]);
            }
        }
        static void base64dec(char const* cstr, std::string& out) {
            ::base64dec(cstr, cstr+strlen(cstr), std::back_inserter(out));
        }
        FILE* fp;
        int port = 0;
        std::string sps, pps;
    };

    Main(int ac, char* av[])
    {
        Args args(ac, av);
        if (args.port == 0) {
            new (&objmem_) rtph264_client(*this, args.port, args.sps, args.pps, args.fp);
            dtor_ = [this]() {
                auto* obj = reinterpret_cast<rtph264_client*>(&objmem_);
                obj->rtph264_client::~rtph264_client();
            };
        } else {
            new (&objmem_) h264_reader(args.fp);
            dtor_ = [this]() {
                auto* obj = reinterpret_cast<h264_reader*>(&objmem_);
                obj->h264_reader::~h264_reader();
            };
        }
    }
    ~Main() { dtor_(); }

    int run(int ac, char* av[])
    {
        //= struct stat st;
        //= if (fstat(fileno(stdin), &st) <0)
        //=     return 1;

        //= std::vector<uint32_t> buf( st.st_size/sizeof(int32_t)+1 );
        //= uint8_t* begin = reinterpret_cast<uint8_t*>( &buf[0] );
        //= uint8_t* end = begin + st.st_size;

        //= if ((int)fread(begin, 1,st.st_size, stdin) != st.st_size)
        //=     return 2;
        //= dump(begin, end);

        //= return 0;
        return boost::asio::io_service::run();
    }

private:
    std::function<void()> dtor_;
    static int objmem_[sizeof(rtph264_client)/sizeof(int)+1];
    BOOST_STATIC_ASSERT(__BYTE_ORDER == __LITTLE_ENDIAN);
};
int Main::objmem_[sizeof(rtph264_client)/sizeof(int)+1] = {};

int main(int argc, char* argv[])
{
    try {
        Main s(argc, argv);
        s.run(argc, argv);
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

