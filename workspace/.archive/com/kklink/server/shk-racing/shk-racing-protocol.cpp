#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <boost/algorithm/string/trim.hpp>

union u4 { uint32_t len; uint16_t h[2]; uint8_t u[4]; char s[4]; };

int decode(std::istream& ins, std::ostream& outs)
{
    u4 h[2];
    std::vector<u4> vecbuf;
    for ( ;; ) {
        ins.read(&h->s[3], 5);
        if (!ins) {
            return 1;
        }
        int opx = h->s[3];
        h->len = ntohl(h[1].len);

        vecbuf.resize( h->len/sizeof(u4) + 3 );
        u4* buf = &vecbuf[0];

        ins.read(buf->s, h->len);
        if (!ins) {
            return 1;
        }

        outs << opx;
        if (opx == 53) {
            uint16_t* p = &buf->h[0];
            uint16_t* end = &buf->h[h->len/2];
            while (p < end) {
                u4 tmp[2];
                tmp[0].h[0] = *p++;
                tmp[0].h[1] = *p++;
                tmp[1].h[0] = *p++;
                outs <<"\t"<< ntohl(tmp->len) <<"\t"<< ((unsigned int)ntohs(tmp[1].h[0]));
            }
        } else {
            outs << "\t";
            outs.write(buf->s, h->len);
        }
        outs << "\n";
        outs.flush();
    }
    return 0;
}

int encode(std::istream& ins, std::ostream& outs)
{
    std::vector<u4> vecbuf;
    int opx;
    while (ins >> opx) {
        if (opx > 0xff) {
            return 1;
        }
        std::string line;
        if (!getline(ins, line)) {
            return 1;
        }
        boost::algorithm::trim(line);
        vecbuf.resize( line.size()/sizeof(u4) + 3 );

        u4* h = &vecbuf[0];
        h->u[3] = uint8_t(opx);
        h[1].len = htonl(line.size());
        memcpy(h[2].s, line.data(), line.size());
        outs.write(&h->s[3], 5+line.size());
        outs.flush();
    }
    return 0;
}

int main(int argc, char* const argv[])
{
    if (argc > 1) {
        if (argv[1] == std::string("-e")) {
            return encode(std::cin, std::cout);
        }
    }
    return decode(std::cin, std::cout);
}

