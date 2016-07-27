#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <chrono> //#include <boost/chrono.hpp>
#include <thread> //#include <boost/chrono.hpp>
#include "hgs.hpp"

static unsigned runc_ = 0x7fffffff;
void sig_stop(int) {
    runc_=32;
    hgs_exit(1);
    signal(SIGINT, SIG_DFL);
}

#if defined(__ANDROID__)
#include <jni.h>
#  define MAIN JNIEXPORT int JNICALL main_and
#else
#  define MAIN int main
#endif
int main(int argc, char* const argv[])
{
    signal(SIGINT, &sig_stop);

    char const *path, *ip;
    int port;
    ip="192.168.9.172"; port=7654; path="/rtp0";
    ip="192.168.0.1"  ; port= 554; path="/live/ch00_2";
    switch (argc-1) {
        case 3: port = atoi(argv[3]);
        case 2: ip = argv[2];
        case 1: path = argv[1];
    }

    FILE* fp = fopen("o.h264", "w");
    hgs_init(ip, port, path, 0,0);
    hgs_run([fp](mbuffer b){
                fwrite(b.begin_s(), b.end()-b.begin_s(), 1, fp);
            });

    while (--runc_ > 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    hgs_exit(0);
    return 0;
}

