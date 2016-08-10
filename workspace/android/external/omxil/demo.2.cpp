#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openmax/OMX_Core.h>
#include <openmax/OMX_Component.h>
#include <openmax/OMX_Types.h>
#include "frameworks/av/media/libstagefright/omx/OMXMaster.h"
#include <utils/Debug.h>
#include <set>
#include <memory>
#include <vector>
#include <type_traits>
#include <algorithm>
// #include <initializer_list>

//#define COMPONENT_NAME "OMX.Nvidia.h264.decode"
//#define COMPONENT_NAME "OMX.google.h264.decoder"
#define COMPONENT_NAME "OMX.MTK.VIDEO.DECODER.AVC"

/// OMXMaster::makeComponentInstance
/// EMPTY_BUFFER // OMX_EmptyThisBuffer(handle, buf);

template <typename... As> void err_exit_(int ln, char const* e, char const* fmt, As... a) {
    fprintf(stderr, fmt, ln, e, a...);
    exit(127);
}
template <typename... As> void err_msg_(int ln, char const* e, char const* fmt, As... a) {
    fprintf(stderr, fmt, ln, e, a...);
    fprintf(stderr, "\n");
}
#define ERR_EXIT(...) err_exit_(__LINE__, "", "E%d: " __VA_ARGS__)
#define ERR_MSG(...) err_msg_(__LINE__, "", "E%d:%s: " __VA_ARGS__)
#define ERR_MSG_IF(e, ...) if(e)err_msg_(__LINE__, #e, "E%d:%s: " __VA_ARGS__)
#define ERR_EXIT_IF(e, ...) if(e)err_exit_(__LINE__, #e, "E%d:%s: " __VA_ARGS__)

namespace android {
template<class T> static T* InitOMXParams(T *params)
{
    COMPILE_TIME_ASSERT_FUNCTION_SCOPE(sizeof(OMX_PTR) == 4); // check OMX_PTR is 4 bytes.
    memset(params, 0, sizeof(T));
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;

    return params; //params->nPortIndex = portIndex;
}
} // namespace android

struct nal_unit_header
{
    uint8_t type:5;
    uint8_t nri:2;
    uint8_t f:1;
};

struct h264nalu_reader
{
    typedef std::pair<uint8_t*,uint8_t*> range;
    uint8_t *begin_ = 0, *end_;

    range open(char const* h264_filename)
    {
        int fd = ::open(h264_filename, O_RDONLY);
        struct stat st; // fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);

        void* p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
            void* m = malloc(st.st_size);
            memcpy(m, p, st.st_size);
            begin_ = (uint8_t*)m;
            end_ = begin_ + st.st_size;
        munmap(p, st.st_size);
        assert(begin_[0]=='\0'&& begin_[1]=='\0'&& begin_[2]=='\0'&& begin_[3]==1);

        close(fd);
        //ERR_EXIT_IF(rng_.first==rng_.second, "%s: nal-type:7 not-found", h264_filename);
        return std::make_pair(begin_,begin_);
    }

    range next(range r, int type=0) const {
        for (r = find_(r.second); r.first != r.second; r = find_(r.second)) {
            nal_unit_header* h = reinterpret_cast<nal_unit_header*>(r.first+4);
            if (h->nri < 3) {
                continue;
            }
            if (type && h->type != type) {
                printf("nal nri %d type %d\n", int(h->nri), int(h->type));
                continue;
            }
            printf("nal nri %d type %d, len %u\n", int(h->nri), int(h->type), int(r.second-r.first));
            break;
        }
        return r; //find_(begin_);
    }
    range begin(int naltype=0) const {
        range r = next(std::make_pair(begin_,begin_), naltype);
        range y = next(r, naltype+1);
        ERR_EXIT_IF(r.second!=y.first, "%s",__FUNCTION__);
        r.second = y.second;
        return r;
    }

    ~h264nalu_reader() {
        if (begin_) free(begin_);
    }
    range find_(uint8_t* e) const {
        uint8_t* b = e;
        if (e+4 < end_) {
            static uint8_t startbytes[] = {0, 0, 0, 1};
            //b = e + 4;
            e = std::search(e+4, end_, &startbytes[0], &startbytes[4]);
        }
        return std::make_pair(b,e);
    }
};

struct OMXH264Decoder
{
    typedef OMXH264Decoder This;
    android::OMXMaster omxMaster; // err = OMX_Init();
    OMX_HANDLETYPE component; //OMX_COMPONENTTYPE *component;
    OMX_STATETYPE state;

    //struct BufferWrap { OMX_BUFFERHEADERTYPE* pbuf = 0; };
    struct IOPort : OMX_PARAM_PORTDEFINITIONTYPE, std::vector<OMX_BUFFERHEADERTYPE*> {
        unsigned nCallback;
        unsigned bufflag_filled;
        unsigned bufflag_allocated;

        IOPort(unsigned char portIdx=0) /*: std::vector<BufferWrap>(24)*/ {
            nPortIndex = portIdx;
            nCallback=0; //nBusy=nTotal=0;
            bufflag_filled = bufflag_allocated = 0;
        }
        //~IOPort() { omxbufs_deinit(component); }

        void print() const {
            OMX_PARAM_PORTDEFINITIONTYPE const& def = *this;
            OMX_VIDEO_PORTDEFINITIONTYPE const& vdef = def.format.video;
            printf("Port %d nBufferCount(Actual/Min) %d %d, %.1fK, Enabled %d, Populated %d"
                        "\n\t%s, %ux%u %u %u, xFrate %u, nBrate %u, color %u, compress %u\n"
                    , def.nPortIndex, def.nBufferCountActual,def.nBufferCountMin, def.nBufferSize/1024.0, def.bEnabled, def.bPopulated
                    , vdef.cMIMEType
                    , vdef.nFrameWidth, vdef.nFrameHeight, vdef.nStride, vdef.nSliceHeight
                    , vdef.xFramerate, vdef.nBitrate
                    , vdef.eColorFormat, vdef.eCompressionFormat //OMX_COLOR_FormatYUV420Planar=19,OMX_VIDEO_CodingAVC=7
                    );
        }
    }; // ioports[2] = {0,1};
    IOPort input_port{0};
    IOPort output_port{1};

    sem_t sem_command_;
    sem_t sem_bufempty_;
    sem_t sem_buffilled_;

    h264nalu_reader h264f_; //(fd_ = open("/sdcard/a.h264", O_RDONLY));
    h264nalu_reader::range rng_;

    OMXH264Decoder(char const* component_name)
    {
        //static_assert(std::is_same<OMX_HANDLETYPE,OMX_COMPONENTTYPE*>::value);
        static OMX_CALLBACKTYPE callbacks = {
            .EventHandler = &cbEventHandler0,
            .EmptyBufferDone = &cbEmptyThisBuffer0,
            .FillBufferDone = &cbFillThisBuffer0
        };
        sem_init(&sem_command_, 0, 0);
        sem_init(&sem_bufempty_, 0, 0);
        sem_init(&sem_buffilled_, 0, 0);

        OMX_COMPONENTTYPE *handle;
        OMX_ERRORTYPE err = omxMaster.makeComponentInstance(component_name, &callbacks, this, &handle);
        ERR_MSG_IF(err!=OMX_ErrorNone, "makeComponentInstance");
        component = /*(OMX_HANDLETYPE)*/handle; // OMX_GetHandle(&component, (char*)component_name, NULL, &callbacks);

        //sync_port_definition(input_port);
        //sync_port_definition(output_port);
        //sync_state();
    }
    ~OMXH264Decoder() {
        omxbufs_deinit(input_port);
        omxbufs_deinit(output_port);
        omxMaster.destroyComponentInstance((OMX_COMPONENTTYPE*)component);
        sem_destroy(&sem_command_);
        sem_destroy(&sem_bufempty_);
        sem_destroy(&sem_buffilled_);
        printf("~\n");
    }

    //void disable(IOPort& iop) {
    //    printf("%s: Port %d\n",__FUNCTION__, iop.nPortIndex);
    //    command_nowait(OMX_CommandPortDisable, iop.nPortIndex, NULL); //OMX_SendCommand(component, OMX_CommandPortDisable, iop.nPortIndex, NULL);
    //    // wait_command(); //sem_wait(&sem_command_);
    //}
    //void enable(IOPort& iop) {
    //    printf("%s: Port %d\n",__FUNCTION__, iop.nPortIndex);
    //    command(OMX_CommandPortEnable, iop.nPortIndex, NULL); //OMX_SendCommand(component, OMX_CommandPortEnable, iop.nPortIndex, NULL);
    //    // wait_command(); //sem_wait(&sem_command_);
    //}

    OMX_STATETYPE sync_state() {
        OMX_STATETYPE state0 = state;
        OMX_GetState(component, &state);
        printf("state %d:%s => %d:%s\n", state0, state_str(state0), state, state_str(state));
        return state;
    }

void _set_port_definition(IOPort& xp) {
    // OMX_VIDEO_PARAM_AVCTYPE h264type;
    // InitOMXParams(&h264type);
    // h264type.nPortIndex = kPortIndexOutput;
    static unsigned char OMX_PARAM_PORTDEFINITIONTYPE_0[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x2a, 0x15, 0x03, 0xaf, 0x00, 0x00, 0x00, 0x00,
        0x80, 0x07, 0x00, 0x00, 0x38, 0x04, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00,
        0x90, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static unsigned char OMX_PARAM_PORTDEFINITIONTYPE_1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
        0x40, 0xdb, 0x2f, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x5c, 0x14, 0x03, 0xaf, 0x00, 0x00, 0x00, 0x00,
        0x80, 0x07, 0x00, 0x00, 0x38, 0x04, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00,
        0x40, 0x04, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x56, 0x31, 0x32,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    int portIndex = xp.nPortIndex;
    sync_port_definition(xp);
    printf("=== === %d\n",__LINE__);

    OMX_PARAM_PORTDEFINITIONTYPE def; // = &xp;
    if (portIndex == 0) {
        memcpy(&def, OMX_PARAM_PORTDEFINITIONTYPE_0, sizeof(def));
    } else {
        memcpy(&def, OMX_PARAM_PORTDEFINITIONTYPE_1, sizeof(def));
    }
    ERR_EXIT_IF(def.nPortIndex!=xp.nPortIndex, "");
    ERR_EXIT_IF(def.eDir!=xp.eDir, "");
            xp.nBufferCountActual = def.nBufferCountActual;
            xp.nBufferCountMin = def.nBufferCountMin;
            xp.nBufferSize = def.nBufferSize;
            xp.eDomain = def.eDomain;
    //xp.format.video.cMIMEType;
            xp.format.video.nFrameWidth        = def.format.video.nFrameWidth;
            xp.format.video.nFrameHeight       = def.format.video.nFrameHeight;
            xp.format.video.nStride            = def.format.video.nStride;//.nFrameWidth;
            xp.format.video.nSliceHeight       = def.format.video.nSliceHeight;//.nFrameHeight;
            xp.format.video.nBitrate           = def.format.video.nBitrate;
            xp.format.video.xFramerate         = def.format.video.xFramerate;
            xp.format.video.eCompressionFormat = def.format.video.eCompressionFormat;
            xp.format.video.eColorFormat       = def.format.video.eColorFormat;
    printf("=== === %d\n",__LINE__);
    //printf("%dsizeof: %u %u\n",__LINE__, sizeof(def), sizeof(OMX_PARAM_PORTDEFINITIONTYPE_0));
    xp.print();
    //exit(portIndex); // XXX

    printf("=== === %d\n",__LINE__);

    OMX_ERRORTYPE err=OMX_SetParameter(component , OMX_IndexParamPortDefinition, &xp);
    ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SetParameter");
    sync_port_definition(xp);

    printf("=== === %d\n",__LINE__);

    //android::InitOMXParams(def)->nPortIndex = portIndex;
    //OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, def);
    //ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_GetParameter");
    xp.reserve(xp.nBufferCountActual);
    printf("=== === %d\n",__LINE__);
}

    void sync_port_definition(IOPort& iop) {
        int portIndex = iop.nPortIndex;

        OMX_PARAM_PORTDEFINITIONTYPE* def = &iop;
        android::InitOMXParams(def)->nPortIndex = portIndex;
        OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, def);
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_GetParameter");
        iop.print();

        iop.reserve(iop.nBufferCountActual);
    }

    //void sync_port_definition(IOPort& iop, OMX_PARAM_PORTDEFINITIONTYPE const& def) {
    //    OMX_ERRORTYPE err=OMX_SetParameter(component
    //            , OMX_IndexParamPortDefinition, const_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(&def));
    //    ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SetParameter");
    //    sync_port_definition(iop);
    //}

    void omxbufs_deinit(IOPort& iop) {
        printf("%s: Port %d: Actual %u, %d\n",__FUNCTION__, iop.nPortIndex, iop.nBufferCountActual, (int)iop.size());
        for (unsigned i=0; i < iop.size(); ++i) {
            OMX_FreeBuffer(component, iop.nPortIndex, iop[i]);
        }
        iop.clear();
        iop.bufflag_filled = iop.bufflag_allocated = 0;
    }
    void omxbufs_reinit(IOPort& iop)/*(OMX_HANDLETYPE component)*/ {
        if (!iop.empty())
            omxbufs_deinit(iop);
        printf("%s: Port %d: Actual %u\n",__FUNCTION__, iop.nPortIndex, iop.nBufferCountActual);

        auto& vec = iop;
        vec.resize(iop.nBufferCountActual);
        for (unsigned i=0; i < iop.nBufferCountActual; ++i) {
            vec[i] = omxbuf_alloc(iop, i);
            ERR_EXIT_IF(!vec[i], "%s",__FUNCTION__);
        }
    }

    struct CommandHelper
    {
        This* self;
        OMX_COMMANDTYPE Cmd; OMX_U32 nParam1; OMX_PTR pCmdData;

        CommandHelper const& send() const {
            print(__FUNCTION__);
            if (Cmd < OMX_CommandMax) {
                OMX_ERRORTYPE err = OMX_SendCommand(self->component, Cmd, nParam1, pCmdData);
                ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SendCommand");
            }
            return *this;
        }
        template <typename F> CommandHelper const& operator()(F&& fn) const {
            fn();
            return *this;
        }
        void wait() const {
            print(__FUNCTION__);
            int ret = sem_wait(&self->sem_command_); // OMX_StateExecuting OMX_StateWaitForResources
            ERR_EXIT_IF(ret<0, "sem_wait");
            if (OMX_CommandStateSet == nParam1)
                self->sync_state();
            print(__FUNCTION__, " [OK]\n");
        }
        void print(char const* pfx, char const* end="\n") const {
            if (Cmd == OMX_CommandStateSet)
                printf("%s:%s: %s%s", pfx, self->cmd_str(Cmd), self->state_str(nParam1), end);
            else if (Cmd == OMX_CommandMax)
                printf("%s:%s: %u%s", pfx, self->event_str(nParam1), nParam1, end);
            else
                printf("%s:%s: %u%s", pfx, self->cmd_str(Cmd), nParam1, end);
        }
    };
    CommandHelper command(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData) {
        return CommandHelper{this,Cmd,nParam1,pCmdData}.send();
    }

    void decode_start(char const* h264_filename)
    {
        rng_ = h264f_.open(h264_filename); //(fd, 7);
        //// OMX_IndexParamVideoInit, OMX_PORT_PARAM_TYPE
        // struct timespec tv; tv.tv_sec = 0; tv.tv_nsec = 1000*1000*100;
        // sem_timedwait;

        enum { vWidth = 1920, vHeight = 1080 };
        //enum { vWidth = 1280, vHeight = 720 };
        //enum { vWidth = 480, vHeight = 272 };
        // OMX_VIDEO_PARAM_PROFILELEVELTYPE

        sync_state();
        _set_port_definition(input_port); //sync_port_definition(input_port);
        _set_port_definition(output_port); //sync_port_definition(output_port);
        //{
        //    OMX_PARAM_PORTDEFINITIONTYPE def = input_port;
        //    def.nBufferSize = vWidth*vHeight*3/2;
        //    def.format.video.nFrameWidth = vWidth;
        //    def.format.video.nFrameHeight = vHeight;
        //    def.format.video.nStride = def.format.video.nFrameWidth;
        //    sync_port_definition(input_port, def);
        //}
        //{
        //    OMX_PARAM_PORTDEFINITIONTYPE def = output_port;
        //    def.format.video.nFrameWidth = vWidth;
        //    def.format.video.nFrameHeight = vHeight;
        //    def.format.video.nStride = def.format.video.nFrameWidth;
        //    sync_port_definition(output_port, def);
        //}// init_port_def(output_port); //(OMX_DirOutput);

        command(OMX_CommandStateSet, OMX_StateIdle, NULL)([this](){
                    omxbufs_reinit(input_port);
                    omxbufs_reinit(output_port);
                }).wait();
        sync_state();
        command(OMX_CommandStateSet, OMX_StateExecuting, NULL).wait();
        sync_state();
        //sync_port_definition(input_port);
        //sync_port_definition(output_port);

#if 0
        /*while */(obuf_alloc())
            ;

pos_test_again_:
        OMX_BUFFERHEADERTYPE* buf = ibuf_alloc();
        auto sps = rng_ = h264f_.next(rng_, 7);
        auto pps = rng_ = h264f_.next(rng_, 8);
        if (sps.first != sps.second) {
            /// static unsigned char sps_[] = { 0,0,0,1,
            ///     //0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbc, 0xa8
            ///     //0x67, 0x64, 0x00, 0x28, 0xac, 0x34, 0xe8, 0x07, 0x80, 0x22, 0x5e, 0x58, 0x05, 0xf5
            ///     0x67, 0x64, 0x00, 0x28, 0xac, 0x34, 0xe8, 0x07, 0x80, 0x22, 0x5e, 0x54
            /// };
            /// unsigned sps_len = sizeof(sps_);
            /// memcpy( buf->pBuffer+buf->nFilledLen, sps_, sps_len );
            /// buf->nFilledLen += sps_len;
            unsigned sps_len = sps.second - sps.first;
            memcpy( buf->pBuffer+buf->nFilledLen, sps.first, sps_len );
            buf->nFilledLen += sps_len;
        }
        if (pps.first != pps.second) {
            unsigned pps_len = pps.second - pps.first;
            memcpy( buf->pBuffer+buf->nFilledLen, pps.first, pps_len );
            buf->nFilledLen += pps_len;
        }
        ibuf_filled(buf);

        CommandHelper{this,OMX_CommandMax,OMX_EventPortSettingsChanged,0}.wait();
        printf("===%d OMX_EventPortSettingsChanged ...\n",__LINE__);

        sync_port_definition(output_port);
        command(OMX_CommandPortDisable, output_port.nPortIndex, NULL)([this](){
                    if (output_port.bufflag_allocated)
                        CommandHelper{this,OMX_CommandMax,OMX_EventBufferFlag,0}.wait(); //XXX
        printf("===%d OMX_EventBufferFlag %x ...\n",__LINE__, output_port.bufflag_allocated);
                    omxbufs_deinit(output_port);
                }).wait();
        printf("===%d OMX_CommandPortDisable OK\n",__LINE__);

        //command(OMX_CommandStateSet, OMX_StateIdle, NULL).wait();
        //command(OMX_CommandStateSet, OMX_StateExecuting, NULL).wait();

        sync_port_definition(output_port);
        command(OMX_CommandPortEnable, output_port.nPortIndex, NULL)([this](){
                    omxbufs_reinit(output_port);
                }).wait();
        printf("===%d OMX_CommandPortEnable OK\n",__LINE__);//sleep(1);
#endif
#if 0
        command(OMX_CommandStateSet, OMX_StateIdle, NULL).wait();
        command(OMX_CommandStateSet, OMX_StateLoaded, NULL)([this](){
                    omxbufs_deinit(input_port);
                    omxbufs_deinit(output_port);
                }).wait();

        sync_port_definition(input_port);
        OMX_PARAM_PORTDEFINITIONTYPE def = input_port;
        def.format.video.nFrameWidth  = output_port.format.video.nFrameWidth;
        def.format.video.nFrameHeight = output_port.format.video.nFrameHeight;
        def.format.video.nStride      = output_port.format.video.nStride;
        def.format.video.nSliceHeight = output_port.format.video.nSliceHeight;
        sync_port_definition(input_port, def);
        // OMX_IndexParamVideoPortFormat OMX_VIDEO_PARAM_PORTFORMATTYPE OMX_VIDEO_PORTDEFINITIONTYPE video;

        command(OMX_CommandStateSet, OMX_StateIdle, NULL)([this](){
                    omxbufs_reinit(input_port);
                    omxbufs_reinit(output_port);
                }).wait();

        command(OMX_CommandStateSet, OMX_StateExecuting, NULL).wait();
#endif
        sync_port_definition(input_port);
        sync_port_definition(output_port);
        sync_state();

        //obuf_alloc();
        for (int i=0; ; ++i)
            if (!obuf_alloc()) {
                printf("obuf_alloc : %d\n", i);
                break;
            }

        printf("===%d dec loop\n",__LINE__); //sleep(1);
//goto pos_test_again_;

        while (sem_trywait(&sem_buffilled_) == 0)
            printf("sem_trywait(&sem_buffilled_) == 0\n");
        assert(errno == EAGAIN);
        while (sem_trywait(&sem_bufempty_) == 0)
            printf("sem_trywait(&sem_bufempty_) == 0\n");
        assert(errno == EAGAIN);

        rng_ = h264f_.next(rng_, 5);
        rng_ = h264f_.next(rng_, 5);
        for (int n=40; rng_.first != rng_.second; rng_ = h264f_.next(rng_, 5)) {
            //if (int(rng_.second-rng_.first) < 1024) {
            //    continue;
            //}
            //OMX_BUFFERHEADERTYPE* obuf = obuf_alloc();

            OMX_BUFFERHEADERTYPE* ibuf = ibuf_alloc();
            ibuf->nFilledLen = rng_.second - rng_.first;
            memcpy( ibuf->pBuffer, rng_.first, ibuf->nFilledLen );
            ibuf_filled(ibuf);
            sem_wait(&sem_bufempty_);

            // TODO
            // ...
            bufflag_off(input_port, ibuf);
            if (n-- == 1) break;

            //printf("wait cbFillThisBuffer ...\n");
            //sem_wait(&sem_buffilled_);
        
        //printf("%s: Port 1 idx %d len %u\n",__FUNCTION__, index(obuf), buf->nFilledLen);

            // TODO
            //bufflag_off(output_port, obuf);
        }
    } // decode_start

    void bufflag_off(IOPort& p, OMX_BUFFERHEADERTYPE* buf) {
        p.bufflag_filled &= ~(1<<index(buf));
        p.bufflag_allocated &= ~(1<<index(buf));
    }

    OMX_BUFFERHEADERTYPE* obuf_alloc() {
        OMX_BUFFERHEADERTYPE* buf = 0;
        for (unsigned i=0; i<output_port.size(); ++i) {
            if ((output_port.bufflag_allocated & (1<<i))==0) {
                buf = output_port[i];
        OMX_ERRORTYPE err = OMX_FillThisBuffer(component, buf);
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_FillThisBuffer %d", int(err));
        printf("OMX_FillThisBuffer %d\n", i);
                output_port.bufflag_allocated |= (1<<i); // index(buf)
                break;
            }
        }
        return buf;
    }
    OMX_BUFFERHEADERTYPE* ibuf_alloc() {
        OMX_BUFFERHEADERTYPE* buf = 0;
        for (unsigned i=0; i<input_port.size(); ++i) {
            if ((input_port.bufflag_allocated & (1<<i))==0) {
                buf = input_port[i];
                input_port.bufflag_allocated |= (1<<i); // index(buf)
                break;
            }
        }
        return buf;
    }

    void ibuf_filled(OMX_BUFFERHEADERTYPE* buf) {
        input_port.bufflag_filled |= (1<<index(buf));
        OMX_ERRORTYPE err = OMX_EmptyThisBuffer(component, buf);
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_EmptyThisBuffer %d", int(err));
        printf("%s: Port 0 idx %d len %u\n",__FUNCTION__, index(buf), buf->nFilledLen);
    }

    void obuf_filled(OMX_BUFFERHEADERTYPE* buf) {

        //TODO
    }

    static inline unsigned index(OMX_BUFFERHEADERTYPE* buf) { return (unsigned)buf->pAppPrivate; }

    //void nextEmptyBuffer()
    //{
    //    IOPort& iop = input_port;
    //    int n = 0, bidx=0;
    //    for (; rng_.first != rng_.second; rng_ = h264f_.next(rng_)) {
    //        OMX_BUFFERHEADERTYPE* buf = iop[bidx]; // 

    //        nal_unit_header* h = reinterpret_cast<nal_unit_header*>(rng_.first);
    //        if (h->nri < 3) {
    //            continue;
    //        }
    //        if (1 ||h->type < 5) {
    //            printf("nal nri %d type %d, %d\n", int(h->nri), int(h->type), bidx);
    //        }

    //        unsigned len = (rng_.second - rng_.first);
    //        if (len > iop.nBufferSize) {
    //            ERR_MSG("len %u > %u", len, iop.nBufferSize);
    //            continue;
    //        }

    //        //if (h->type != 5) { continue; }
    //        if (h->type == 5) {
    //            //printf("Press 'Enter' key ... %d\n", bidx);
    //            //char tmp[32];
    //            //while (fgets(tmp, sizeof(tmp), stdin))
    //            //    if (tmp[strlen(tmp)-1] == '\n')
    //            //        break;
    //    //printf("%d:nextEmptyBuffer %d, %d\n",__LINE__, n, bidx);
    //            //rng_ = h264f_.next(rng_); break;
    //        }

    //        memcpy( buf->pBuffer, rng_.first, len );
    //        buf->nFilledLen = len;

    //        OMX_ERRORTYPE err = OMX_EmptyThisBuffer(component, buf);
    //        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_EmptyThisBuffer %d", int(err));
    //        ++n;
    //        ++bidx;
    //    printf("%d:nextEmptyBuffer %d, %d\n",__LINE__, n, bidx);
    //        sleep(5);
    //    }
    //    printf("%d:nextEmptyBuffer %d, %d\n",__LINE__, n, bidx);
    //}

    //void nextFillBuffer(OMX_BUFFERHEADERTYPE* buf)
    //{
    //    // buf->nPortIndex;
    //    output_port.bufflag_filled &= ~(1<<index(buf));
    //    output_port.bufflag_allocated |= (1<<index(buf));
    //    OMX_ERRORTYPE err = OMX_FillThisBuffer(component, buf);
    //    ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_FillThisBuffer %d", int(err));
    //}
    //void nextFillBuffer()
    //{
    //    IOPort& iop = output_port;
    //    for (unsigned i=0; i<iop.size(); ++i) {
    //        nextFillBuffer(iop[i]);
    //    }
    //    printf("%s: nBufferCountActual %u %d #%d\n",__FUNCTION__, iop.nBufferCountActual, (int)iop.size(), __LINE__);
    //}

    //void test_disable_ports() {
    //    disableSomePorts( OMX_IndexParamVideoInit);
    //    disableSomePorts( OMX_IndexParamImageInit);
    //    disableSomePorts( OMX_IndexParamAudioInit);
    //    disableSomePorts( OMX_IndexParamOtherInit);
    //}

    //void disableSomePorts(OMX_INDEXTYPE indexType)
    //{
    //    OMX_ERRORTYPE err;
    //    OMX_PORT_PARAM_TYPE param;
    //    android::InitOMXParams(&param);
    //    err = OMX_GetParameter(component, indexType, &param);
    //    ERR_MSG_IF(err!=OMX_ErrorNone, "OMX_GetParameter");
    //    int startPortNumber = param.nStartPortNumber;
    //    int nPorts = param.nPorts;
    //    int endPortNumber = startPortNumber + nPorts;
    //    printf("Port start=%d, count=%d\n", startPortNumber, nPorts);
    //    for (int n = startPortNumber; n < endPortNumber; n++) {
    //        OMX_SendCommand(component, OMX_CommandPortDisable, n, NULL);
    //        sem_wait(&sem_command_);
    //    }
    //}

private:
    OMX_BUFFERHEADERTYPE* omxbuf_alloc(IOPort& iop, unsigned bidx) {
        OMX_BUFFERHEADERTYPE* buf = 0;
        OMX_PARAM_PORTDEFINITIONTYPE* def = &iop; //(*this)[bidx];
        if (bidx < def->nBufferCountActual && bidx < iop.size()) {
            OMX_ERRORTYPE err = OMX_AllocateBuffer(component, &buf, def->nPortIndex, (OMX_PTR)bidx, def->nBufferSize); // pAppPrivate
            ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_AllocateBuffer %d [%d] %u: %s\n", def->nPortIndex, bidx, def->nBufferSize, error_str(err));
        }
        return buf;
    }

private: // callbacks
    OMX_ERRORTYPE cbEmptyThisBuffer(OMX_BUFFERHEADERTYPE* buf)
    {
        printf("%s: bidx %d, %u %u %u\n",__FUNCTION__, index(buf), buf->nFilledLen, buf->nOffset, buf->nAllocLen);
        input_port.nCallback++;
        sem_post(&sem_bufempty_);

        // nextEmptyBuffer();

        //if(pBuffer->pPlatformPrivate < 102400) exit(1);
        //buffer_in_mask |= 1<<*(short*)pBuffer->pPlatformPrivate;
        //sem_post(&wait_buff);

        return OMX_ErrorNone;
    }
    OMX_ERRORTYPE cbFillThisBuffer(OMX_BUFFERHEADERTYPE* buf)
    {
        printf("%s: bidx %d, %u %u %u\n",__FUNCTION__, index(buf), buf->nFilledLen, buf->nOffset, buf->nAllocLen);
        if (buf->nFilledLen <= 0)/*(command_ == OMX_CommandPortDisable)*/ {
            output_port.bufflag_allocated &= ~(1<<index(buf));
            if (output_port.bufflag_allocated == 0) {
                printf("%s OMX_CommandPortDisable\n",__FUNCTION__);
                sem_post(&sem_command_); // OMX_EventBufferFlag
            }
        } else {
            output_port.nCallback++;
            printf("%s: bidx %d, nCB %d\n",__FUNCTION__, index(buf), output_port.nCallback);
            output_port.bufflag_filled |= (1<<index(buf));
        sem_post(&sem_buffilled_);
            printf("%s: bidx %d === === ===\n",__FUNCTION__, index(buf));
            // obuf_alloc(); //nextFillBuffer(iop, buf);
        }
        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE cbEventHandler(OMX_OUT OMX_EVENTTYPE eEvent
            , OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
    {
        printf("%s: %d:%s %d %d %p\n",__FUNCTION__, int(eEvent),event_str(eEvent), (int)Data1, (int)Data2, pEventData);

        switch (eEvent) {
            case OMX_EventCmdComplete:
                switch ((OMX_COMMANDTYPE)Data1) {
                    case OMX_CommandStateSet:
                        printf("%s: OMX_EventCmdComplete:OMX_CommandStateSet %d:%s\n",__FUNCTION__, (int)Data2,state_str(Data2));
        //sync_state();
                        sem_post(&sem_command_);
                        break;
                    case OMX_CommandPortDisable:
                        printf("%s: OMX_EventCmdComplete:OMX_CommandPortDisable %d %d\n",__FUNCTION__, (int)Data1, (int)Data2);
                        sem_post(&sem_command_);
                        break;
                    case OMX_CommandPortEnable:
                        printf("%s: OMX_EventCmdComplete:OMX_CommandPortEnable %d %d\n",__FUNCTION__, (int)Data1, (int)Data2);
                        sem_post(&sem_command_);
                        break;
                }
                break;
            case OMX_EventPortSettingsChanged: // TODO
                printf("%s: OMX_EventPortSettingsChanged %d %d\n",__FUNCTION__, (int)Data1, (int)Data2);
                sem_post(&sem_command_);
                break;
            case OMX_EventBufferFlag: ///**< component has detected an EOS */
                ERR_EXIT("%s: OMX_EventBufferFlag",__FUNCTION__);
            case OMX_EventError: ///**< component has detected an error condition */
                ERR_EXIT("%s: OMX_EventError",__FUNCTION__);
        }
        return OMX_ErrorNone;
    }

#define CASE_THEN_RETURN_STR(y) case y: return idstrcat(y,#y)
    static char const* idstrcat(int x, char const* s) {
        static std::set<char const*> ss;
        if (ss.insert(s).second) {
            printf("%s=%d\n",s,x);
        }
        return s;
        //static char idstr_tmpbuf_[64];
        //snprintf(idstr_tmpbuf_,sizeof(idstr_tmpbuf_), "%d:%s", x,s);
        //return idstr_tmpbuf_;
    }

    static char const* cmd_str(unsigned x) {
        switch (x) {
            CASE_THEN_RETURN_STR(OMX_CommandStateSet);
            CASE_THEN_RETURN_STR(OMX_CommandFlush);
            CASE_THEN_RETURN_STR(OMX_CommandPortDisable);
            CASE_THEN_RETURN_STR(OMX_CommandPortEnable);
            CASE_THEN_RETURN_STR(OMX_CommandMarkBuffer);
            CASE_THEN_RETURN_STR(OMX_CommandMax);
        }
        return idstrcat(x,"OMX_Command-Unknown");
    }

    static char const* state_str(int x) {
        switch (x) {
            CASE_THEN_RETURN_STR(OMX_StateInvalid);
            CASE_THEN_RETURN_STR(OMX_StateLoaded);
            CASE_THEN_RETURN_STR(OMX_StateIdle);
            CASE_THEN_RETURN_STR(OMX_StateExecuting);
            CASE_THEN_RETURN_STR(OMX_StatePause);
            CASE_THEN_RETURN_STR(OMX_StateWaitForResources);
        }
        return idstrcat(x,"OMX_State-Unknown");
    }
    static char const* event_str(int x) {
        switch (x) {
            CASE_THEN_RETURN_STR(OMX_EventCmdComplete);
            CASE_THEN_RETURN_STR(OMX_EventError);
            CASE_THEN_RETURN_STR(OMX_EventMark);
            CASE_THEN_RETURN_STR(OMX_EventPortSettingsChanged);
            CASE_THEN_RETURN_STR(OMX_EventBufferFlag);
            CASE_THEN_RETURN_STR(OMX_EventResourcesAcquired);
            CASE_THEN_RETURN_STR(OMX_EventComponentResumed);
            CASE_THEN_RETURN_STR(OMX_EventDynamicResourcesAvailable);
            CASE_THEN_RETURN_STR(OMX_EventPortFormatDetected);
        }
        return idstrcat(x,"OMX_Event-Unknown");
    }
    static char const* error_str(/*OMX_ERRORTYPE*/int x) {
        switch (x) {
            CASE_THEN_RETURN_STR(OMX_ErrorNone);
            CASE_THEN_RETURN_STR(OMX_ErrorInsufficientResources);
            CASE_THEN_RETURN_STR(OMX_ErrorUndefined);
            CASE_THEN_RETURN_STR(OMX_ErrorInvalidComponentName);
            CASE_THEN_RETURN_STR(OMX_ErrorComponentNotFound);
            CASE_THEN_RETURN_STR(OMX_ErrorInvalidComponent);
            CASE_THEN_RETURN_STR(OMX_ErrorBadParameter);
            CASE_THEN_RETURN_STR(OMX_ErrorNotImplemented);
            CASE_THEN_RETURN_STR(OMX_ErrorUnderflow);
            CASE_THEN_RETURN_STR(OMX_ErrorOverflow);
            CASE_THEN_RETURN_STR(OMX_ErrorHardware);
            CASE_THEN_RETURN_STR(OMX_ErrorInvalidState);
            CASE_THEN_RETURN_STR(OMX_ErrorStreamCorrupt);
            CASE_THEN_RETURN_STR(OMX_ErrorPortsNotCompatible);
            CASE_THEN_RETURN_STR(OMX_ErrorResourcesLost);
            CASE_THEN_RETURN_STR(OMX_ErrorNoMore);
            CASE_THEN_RETURN_STR(OMX_ErrorVersionMismatch);
            CASE_THEN_RETURN_STR(OMX_ErrorNotReady);
            CASE_THEN_RETURN_STR(OMX_ErrorTimeout);
            CASE_THEN_RETURN_STR(OMX_ErrorSameState);
            CASE_THEN_RETURN_STR(OMX_ErrorResourcesPreempted);
            CASE_THEN_RETURN_STR(OMX_ErrorPortUnresponsiveDuringAllocation);
            CASE_THEN_RETURN_STR(OMX_ErrorPortUnresponsiveDuringDeallocation);
            CASE_THEN_RETURN_STR(OMX_ErrorPortUnresponsiveDuringStop);
            CASE_THEN_RETURN_STR(OMX_ErrorIncorrectStateTransition);
            CASE_THEN_RETURN_STR(OMX_ErrorIncorrectStateOperation);
            CASE_THEN_RETURN_STR(OMX_ErrorUnsupportedSetting);
            CASE_THEN_RETURN_STR(OMX_ErrorUnsupportedIndex);
            CASE_THEN_RETURN_STR(OMX_ErrorBadPortIndex);
            CASE_THEN_RETURN_STR(OMX_ErrorPortUnpopulated);
            CASE_THEN_RETURN_STR(OMX_ErrorComponentSuspended);
            CASE_THEN_RETURN_STR(OMX_ErrorDynamicResourcesUnavailable);
            CASE_THEN_RETURN_STR(OMX_ErrorMbErrorsInFrame);
            CASE_THEN_RETURN_STR(OMX_ErrorFormatNotDetected);
            CASE_THEN_RETURN_STR(OMX_ErrorContentPipeOpenFailed);
            CASE_THEN_RETURN_STR(OMX_ErrorContentPipeCreationFailed);
            CASE_THEN_RETURN_STR(OMX_ErrorSeperateTablesUsed);
            CASE_THEN_RETURN_STR(OMX_ErrorTunnelingUnsupported);
        }
        return idstrcat(x,"OMX_Error-Unknown");
    }

private:
    static OMX_ERRORTYPE cbEventHandler0(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData
            , OMX_OUT OMX_EVENTTYPE eEvent, OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
    {
        ERR_MSG_IF(hComponent!=static_cast<This*>(pAppData)->component, "%p %p", hComponent,pAppData);
        return static_cast<This*>(pAppData)->cbEventHandler(eEvent, Data1, Data2, pEventData);
    }
    static OMX_ERRORTYPE cbEmptyThisBuffer0(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData
            , OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
    {
        ERR_MSG_IF(hComponent!=static_cast<This*>(pAppData)->component, "%p %p", hComponent,pAppData);
        assert(pBuffer);
        return static_cast<This*>(pAppData)->cbEmptyThisBuffer(pBuffer);//((unsigned)pBuffer->pAppPrivate);
    }
    static OMX_ERRORTYPE cbFillThisBuffer0(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData
            , OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
    {
        ERR_MSG_IF(hComponent!=static_cast<This*>(pAppData)->component, "%p %p", hComponent,pAppData);
        assert(pBuffer);
        return static_cast<This*>(pAppData)->cbFillThisBuffer(pBuffer);//((unsigned)pBuffer->pAppPrivate);
    }
};

int main()
{
    OMXH264Decoder ox(COMPONENT_NAME); // ox.test_print_port_params();
    ox.decode_start("/sdcard/a.h264");

    printf("Ctrl-C ...\n");
    while (sleep(5) == 0)
        ;
    return 0;
//
//    input = open("test.h264", O_RDONLY);
//#ifdef DUMP
//    dumper=open("dump.out", O_WRONLY | O_CREAT);
//#endif
//
//    sem_init(&wait_for_state, 0, 0);
//    //sem_init(&wait_for_parameters, 0, 0);
//    sem_init(&wait_buff, 0, 0);
//
//    decode_init();
//
//    printf("idle\n");
//    GoToState(OMX_StateIdle);
//    printf("go executing\n");
//    GoToState(OMX_StateExecuting);
//
//    while(1)
//        decode();
}

