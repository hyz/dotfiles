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

#include <OpenMAX/IL/OMX_Core.h>
#include <OpenMAX/IL/OMX_Component.h>
#include <OpenMAX/IL/OMX_Types.h>
#include "frameworks/av/media/libstagefright/omx/OMXMaster.h"
#include <utils/Debug.h>
#include <memory>
#include <vector>
#include <type_traits>
#include <algorithm>
// #include <initializer_list>

//#define COMPONENT "OMX.Nvidia.h264.decode"
#define COMPONENT "OMX.MTK.VIDEO.DECODER.AVC"

#define DEBUG

//OMX_ERRORTYPE OMXMaster::makeComponentInstance(const char *name, const OMX_CALLBACKTYPE *callbacks, OMX_PTR appData, OMX_COMPONENTTYPE **component);
// EMPTY_BUFFER // OMX_EmptyThisBuffer(handle, buf);

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
inline int naltype(OMX_BUFFERHEADERTYPE*) {
    return 0;
}

struct h264nalu_reader
{
    typedef std::pair<uint8_t*,uint8_t*> range;
    uint8_t *begin_ = 0, *end_;

    range init(int fd, int naltype) {
        struct stat st; // fd = open(fn, O_RDONLY);
        fstat(fd, &st); // printf("Size: %d\n", (int)st.st_size);

        void* p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
            void* m = malloc(st.st_size);
            memcpy(m, p, st.st_size);
            begin_ = (uint8_t*)m;
            end_ = begin_ + st.st_size;
        munmap(p, st.st_size);
        assert(begin_[0]=='\0'&& begin_[1]=='\0'&& begin_[2]=='\0'&& begin_[3]==1);

        return next(std::make_pair(begin_,begin_), naltype);
    }
    //range next(range const& prev) const { return find_(prev.second); }

    range next(range rng, int type) const {
        for (rng = find_(rng.second); rng.first != rng.second; rng = find_(rng.second)) {
            nal_unit_header* h = reinterpret_cast<nal_unit_header*>(rng.first);
            if (h->nri < 3) {
                continue;
            }
            if (h->type != type) {
                printf("nal nri %d type %d\n", int(h->nri), int(h->type));
                continue;
            }
            break;
        }
        return rng; //find_(begin_);
    }

    ~h264nalu_reader() {
        if (begin_) free(begin_);
    }
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

struct OMXControl
{
    typedef OMXControl This;
    android::OMXMaster omxMaster; // err = OMX_Init();
    OMX_HANDLETYPE component; //OMX_COMPONENTTYPE *component;
    OMX_STATETYPE state;

    //struct BufferWrap { OMX_BUFFERHEADERTYPE* pbuf = 0; };
    struct IOPort : OMX_PARAM_PORTDEFINITIONTYPE, std::vector<OMX_BUFFERHEADERTYPE*> {
        unsigned nCallback;
        unsigned bufflag_send;
        unsigned bufflag_allocated;

        IOPort(unsigned char portIdx=0) /*: std::vector<BufferWrap>(24)*/ {
            nPortIndex = portIdx;
            nCallback=0; //nBusy=nTotal=0;
            bufflag_send = bufflag_allocated = 0;
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
                    , vdef.eColorFormat, vdef.eCompressionFormat
                    ); // OMX_COLOR_FormatYUV420Planar=19
        }
    }; // ioports[2] = {0,1};
    IOPort input_port{0};
    IOPort output_port{1};

    sem_t sem_command_;
    //volatile bool state_changing;

    h264nalu_reader h264f_; //(fd_ = open("/sdcard/a.h264", O_RDONLY));
    h264nalu_reader::range rng_;

    OMXControl(char const* component_name)
    {
        //static_assert(std::is_same<OMX_HANDLETYPE,OMX_COMPONENTTYPE*>::value);
        static OMX_CALLBACKTYPE callbacks = {
            .EventHandler = &cbEventHandler0,
            .EmptyBufferDone = &cbEmptyThisBuffer0,
            .FillBufferDone = &cbFillThisBuffer0
        };
        sem_init(&sem_command_, 0, 0);

        OMX_COMPONENTTYPE *handle;
        OMX_ERRORTYPE err = omxMaster.makeComponentInstance(component_name, &callbacks, this, &handle);
        ERR_MSG_IF(err!=OMX_ErrorNone, "makeComponentInstance");
        component = /*(OMX_HANDLETYPE)*/handle; // OMX_GetHandle(&component, (char*)component_name, NULL, &callbacks);

        //sync_port_definition(input_port);
        //sync_port_definition(output_port);
        //sync_state();
    }
    ~OMXControl() {
        omxbufs_deinit(input_port);
        omxbufs_deinit(output_port);
        omxMaster.destroyComponentInstance((OMX_COMPONENTTYPE*)component);
        sem_destroy(&sem_command_);
        printf("~\n");
    }

    //inline IOPort& output_port() const { return const_cast<This*>(this)->ioports[1]; }
    //inline IOPort& input_port() const { return const_cast<This*>(this)->ioports[0]; }

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
    ///OMX_STATETYPE sync_state(OMX_STATETYPE state1) {
    ///    OMX_STATETYPE state0 = state;
    ///    return state;
    ///}

    void sync_port_definition(IOPort& iop) {
        int portIndex = iop.nPortIndex;

        OMX_PARAM_PORTDEFINITIONTYPE* def = &iop;
        android::InitOMXParams(def)->nPortIndex = portIndex;
        OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, def);
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_GetParameter");

        iop.reserve(iop.nBufferCountActual);

        iop.print();
    }

    void sync_port_definition(IOPort& iop, OMX_PARAM_PORTDEFINITIONTYPE const& def) {
        OMX_ERRORTYPE err=OMX_SetParameter(component
                , OMX_IndexParamPortDefinition, const_cast<OMX_PARAM_PORTDEFINITIONTYPE*>(&def));
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SetParameter");
        sync_port_definition(iop);
    }

    void omxbufs_deinit(IOPort& iop) {
        printf("%s: Port %d: Actual %u, %d\n",__FUNCTION__, iop.nPortIndex, iop.nBufferCountActual, (int)iop.size());
        for (unsigned i=0; i < iop.size(); ++i) {
            OMX_FreeBuffer(component, iop.nPortIndex, iop[i]);
        }
        iop.clear();
        iop.bufflag_send = iop.bufflag_allocated = 0;
    }
    void omxbufs_reinit(IOPort& iop)/*(OMX_HANDLETYPE component)*/ {
        if (!iop.empty())
            omxbufs_deinit(iop);
        printf("%s: Port %d: Actual %u, %d\n",__FUNCTION__, iop.nPortIndex, iop.nBufferCountActual, (int)iop.size());

        auto& vec = iop;
        vec.resize(iop.nBufferCountActual);
        for (unsigned i=0; i < iop.nBufferCountActual; ++i) {
            vec[i] = omx_alloc_buf(iop, i);
            ERR_EXIT_IF(!vec[i], "%s",__FUNCTION__);
        }
        sync_port_definition(iop);
    }

    struct CommandHelper
    {
        This* self;
        OMX_COMMANDTYPE Cmd; OMX_U32 nParam1; OMX_PTR pCmdData;

        CommandHelper const& send() const {
            print(__FUNCTION__);
            int sv;
            if (sem_getvalue(&self->sem_command_, &sv)==0 && sv > 0) {
                ERR_MSG("sem-value %d", sv);
                while (sv-- > 0 && sem_wait(&self->sem_command_)==0)
                    ;
            }
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

    void nextFillBuffer(IOPort& iop, OMX_BUFFERHEADERTYPE* buf)
    {
        // buf->nPortIndex;
        iop.bufflag_send &= ~(1<<index(buf));
        iop.bufflag_allocated |= (1<<index(buf));
        OMX_ERRORTYPE err = OMX_FillThisBuffer(component, buf);
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_FillThisBuffer %d", int(err));
    }
    void nextFillBuffer()
    {
        IOPort& iop = output_port;
        for (unsigned i=0; i<iop.size(); ++i) {
            nextFillBuffer(iop, iop[i]);
        }
        printf("%s: nBufferCountActual %u %d #%d\n",__FUNCTION__, iop.nBufferCountActual, (int)iop.size(), __LINE__);
    }

    void decode_start(char const* h264_filename) {
        {
            int fd = open(h264_filename, O_RDONLY);
            rng_ = h264f_.init(fd, 7);
            close(fd);
            ERR_EXIT_IF(rng_.first==rng_.second, "%s: nal-type:7 not-found", h264_filename);
        }
        //// OMX_IndexParamVideoInit, OMX_PORT_PARAM_TYPE
        // struct timespec tv; tv.tv_sec = 0; tv.tv_nsec = 1000*1000*100;
        // sem_timedwait;

        sync_state();
        sync_port_definition(input_port);
        sync_port_definition(output_port);
        /*{
            OMX_PARAM_PORTDEFINITIONTYPE def = input_port;
            def.format.video.nFrameWidth = vWidth;
            def.format.video.nFrameHeight = vHeight;
            def.format.video.nStride = def.format.video.nFrameWidth;
            sync_port_definition(input_port);
        }{
            OMX_PARAM_PORTDEFINITIONTYPE def = output_port;
            def.format.video.nFrameWidth = vWidth;
            def.format.video.nFrameHeight = vHeight;
            def.format.video.nStride = def.format.video.nFrameWidth;
            sync_port_definition(output_port);
        }*/// init_port_def(output_port); //(OMX_DirOutput);

        command(OMX_CommandStateSet, OMX_StateIdle, NULL)([this](){
                    omxbufs_reinit(input_port);
                    omxbufs_reinit(output_port);
                }).wait();
        command(OMX_CommandStateSet, OMX_StateExecuting, NULL).wait();

        nextFillBuffer();

pos_again__:
        if (rng_.first != rng_.second) {
            OMX_BUFFERHEADERTYPE* buf = alloc_buf(input_port);
            buf->nFilledLen = rng_.second - rng_.first;
            memcpy( buf->pBuffer, rng_.first, buf->nFilledLen );
            send_buf(input_port, buf);
        }
        CommandHelper{this,OMX_CommandMax,OMX_EventPortSettingsChanged,0}.wait();
        sync_port_definition(output_port);
        sync_port_definition(input_port);
        printf("===%d sleep\n",__LINE__);sleep(3);

        command(OMX_CommandStateSet, OMX_StateIdle, NULL).wait();
        command(OMX_CommandStateSet, OMX_StateLoaded, NULL)([this](){
                    omxbufs_deinit(input_port);
                    omxbufs_deinit(output_port);
                }).wait();

        sync_port_definition(input_port);
        OMX_PARAM_PORTDEFINITIONTYPE def = input_port;
        def.format.video.nFrameWidth = output_port.format.video.nFrameWidth;
        def.format.video.nFrameHeight = output_port.format.video.nFrameHeight;
        def.format.video.nStride = output_port.format.video.nStride;
        def.format.video.nSliceHeight = output_port.format.video.nSliceHeight;
        sync_port_definition(input_port, def);
        // OMX_IndexParamVideoPortFormat OMX_VIDEO_PARAM_PORTFORMATTYPE OMX_VIDEO_PORTDEFINITIONTYPE video;

        command(OMX_CommandStateSet, OMX_StateIdle, NULL)([this](){
                    omxbufs_reinit(input_port);
                    omxbufs_reinit(output_port);
                }).wait();

        command(OMX_CommandStateSet, OMX_StateExecuting, NULL).wait();
        sync_port_definition(input_port);
        sync_port_definition(output_port);

        nextFillBuffer();

        printf("===%d sleep - again\n",__LINE__);sleep(3);
        goto pos_next__; //pos_again__;

pos_next__:
        rng_ = h264f_.next(rng_, 8);

        if (rng_.first != rng_.second) {
            OMX_BUFFERHEADERTYPE* buf = alloc_buf(input_port);
            buf->nFilledLen = rng_.second - rng_.first;
            memcpy( buf->pBuffer, rng_.first, buf->nFilledLen );
            send_buf(input_port, buf);
        }

        printf("===%d event-waited\n",__LINE__);sleep(6);

        //rng_ = h264f_.next(rng_, 5);
        //while (rng_.first != rng_.second) {
        //    send_data(input_port, rng_.first, rng_.second);
        //}

        //nextEmptyBuffer();
    }

    OMX_BUFFERHEADERTYPE* alloc_buf(IOPort& iop) {
        OMX_BUFFERHEADERTYPE* buf = 0;
        for (unsigned i=0; i<iop.size(); ++i) {
            if ((iop.bufflag_allocated & (1<<i))==0) {
                buf = iop[i];
                iop.bufflag_allocated |= (1<<i); // index(buf)
                break;
            }
        }
        return buf;
    }

    void send_buf(IOPort& iop, OMX_BUFFERHEADERTYPE* buf) {
        iop.bufflag_send |= (1<<index(buf));
        OMX_ERRORTYPE err = OMX_EmptyThisBuffer(component, buf);
        ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_EmptyThisBuffer %d", int(err));
    }

    static inline unsigned index(OMX_BUFFERHEADERTYPE* buf) {
        return (unsigned)buf->pAppPrivate;
    }

    //void change_state(OMX_STATETYPE state1)
    //{
    //    int sv[2] = {};
    //    sem_getvalue(&sem_command_, &sv[0]);

    //    OMX_STATETYPE state0 = state;

    //    printf("=== %d:x state %d:%s -> %d:%s\n", sv[0], (int)state0, state_str(state0), state1, state_str(state1));

    //    //state_changing = 1;
    //    printf("OMX_SendCommand:OMX_CommandStateSet %s\n", state_str(state1));
    //    OMX_ERRORTYPE err = OMX_SendCommand(component, OMX_CommandStateSet, state1, NULL);
    //    ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SendCommand");

    //    if (state1 == OMX_StateIdle)
    //        test_allocbufs();

    //    printf("sem_wait:sem_command_\n");
    //    sem_wait(&sem_command_); // OMX_StateExecuting OMX_StateWaitForResources
    //    //state_changing = 0;
    //    OMX_GetState(component, &state);
    //    ERR_EXIT_IF(state!=state1, "%s",__FUNCTION__);

    //    sem_getvalue(&sem_command_, &sv[1]);
    //    printf("=== %d:%d state %d:%s -> %d:%s\n", sv[0],sv[1], state0, state_str(state0), state1, state_str(state1));
    //}

    void decode_next() //void * data, int len
    {
#if 0
        OMX_ERRORTYPE err;
        OMX_BUFFERHEADERTYPE *buf;

        if(!(buffer_in_mask || buffer_out_mask)) {
            printf("wait damn avp\n");
            sem_wait(&wait_buff);
        }

        for (int i=0;i<buffer_out_nb;i++) {
            if (! ((1<<i) & buffer_out_mask ) )
                continue;

            err = OMX_FillThisBuffer(decoderhandle, omx_buffers_out[i]);
            OMXERR_MSG(err);

            buffer_out_mask &= (1<<i) ^ 0xFFFFFFFF;
            break;
        }

        int read_len;

        for (int i=0;i<buffer_in_nb;i++) {
            buf = omx_buffers_in[i];

            if( ! ((1<<i) & buffer_in_mask ) )
                continue;

            read_len = read(input, buf->pBuffer, buffer_in_size/4);
            printf("read: %d\n", read_len);
            buf->nFilledLen = read_len;

            err = OMX_EmptyThisBuffer(decoderhandle, buf);
            OMXERR_MSG(err);

            buffer_in_mask &= (1<<i) ^ 0xFFFFFFFF;
        }

        /*
           printf("wait filled cb\n");
           sem_wait(&buffer_out_filled);
           printf("ok...\n");
           */
#endif
    }

    //void init_port_def(IOPort& iop)
    //{
    //    // OMX_IndexParamVideoPortFormat OMX_VIDEO_PARAM_PORTFORMATTYPE 
    //    if (iop.nPortIndex == 0) {
    //        //OMX_PARAM_PORTDEFINITIONTYPE def;
    //        //OMX_VIDEO_PORTDEFINITIONTYPE *video_def;
    //        //android::InitOMXParams(&def)->nPortIndex = portIdx;
    //        ////
    //        //OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
    //        //ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_GetParameter");
    //        ////def.nBufferSize=vWidth*vHeight*3/2;
    //        ////def.format.video.nBitrate = 1024*1024*2;
    //        //def.format.video.nFrameWidth = vWidth;
    //        //def.format.video.nFrameHeight = vHeight;
    //        //def.format.video.nStride = def.format.video.nFrameWidth;
    //        //err=OMX_SetParameter(component, OMX_IndexParamPortDefinition, &def);
    //        //ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SetParameter");
    //    }
    //    if (iop.nPortIndex == 1) {
    //        OMX_PARAM_PORTDEFINITIONTYPE def = iop;
    //        def.format.video.nFrameWidth = vWidth;
    //        def.format.video.nFrameHeight = vHeight;
    //        def.format.video.nStride = def.format.video.nFrameWidth;
    //        iop.sync_port_definition(def);
    //        //OMX_PARAM_PORTDEFINITIONTYPE def;
    //        //OMX_VIDEO_PORTDEFINITIONTYPE *video_def;
    //        //android::InitOMXParams(&def)->nPortIndex = portIdx;
    //        ////
    //        //OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
    //        //ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_GetParameter");
    //        ////def.nBufferSize=vWidth*vHeight*3/2;
    //        //def.format.video.nFrameWidth = vWidth;
    //        //def.format.video.nFrameHeight = vHeight;;
    //        //def.format.video.nStride = def.format.video.nFrameWidth;
    //        //err=OMX_SetParameter(component, OMX_IndexParamPortDefinition, &def);
    //        //ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_SetParameter");
    //    }
    //}

    // OMX_FreeBuffer
    //void test_allocbufs()
    //{
    //    auto& ipbs = input_port; //ports[0].sync_port_definition();
    //    auto& opbs = output_port; //ports[1].sync_port_definition();
    //    ipbs.resize(ipbs.nBufferCountActual);
    //    opbs.resize(opbs.nBufferCountActual);
    //    unsigned i=0;
    //    while ( (ipbs[i]=alloc_buf(ipbs,i)) || (opbs[i]=alloc_buf(opbs,i)) )
    //        ++i;
    //}
    //void allocbufs(OMX_PARAM_PORTDEFINITIONTYPE const& def, int nBufferCount)
    //{
    //    printf("Port%d OMX_AllocateBuffer %d size=%d\n", def.nPortIndex, nBufferCount, (int)def.nBufferSize);
    //    for (int i=0; i < nBufferCount; ++i) {
    //        printf("OMX_AllocateBuffer [%d:%d] %u\n", def.nPortIndex, i, def.nBufferSize);
    //        BufferWrap& pb = bufs[i]; // pAppPrivate
    //        err = OMX_AllocateBuffer(component, &pb.pbuf, def.nPortIndex, &pb, def.nBufferSize);
    //        sleep(1);
    //        ERR_MSG_IF(err!=OMX_ErrorNone, "OMX_AllocateBuffer [%d:%d] %u\n", def.nPortIndex, i, def.nBufferSize);
    //        printf("OMX_AllocateBuffer [%d:%d] %u OK\n", def.nPortIndex, i, def.nBufferSize);
    //    }
    //}

    //void test_print_port_params() {
    //    test_print_params(OMX_DirInput);
    //    test_print_params(OMX_DirOutput);

    //    static char const* sts[] = { "OMX_StateInvalid", "OMX_StateLoaded", "OMX_StateIdle", "OMX_StateExecuting", "OMX_StatePause", "OMX_StateWaitForResources" };
    //    OMX_STATETYPE state0;
    //    OMX_GetState(component, &state0);
    //    printf("=== state %d:%s\n", state0, sts[state0]);
    //}
    //void test_print_params(int portIndex)
    //{
    //    OMX_PARAM_PORTDEFINITIONTYPE def;
    //    android::InitOMXParams(&def)->nPortIndex = portIndex;
    //    OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
    //    ERR_MSG_IF(err!=OMX_ErrorNone, "OMX_GetParameter");
    //    OMX_VIDEO_PORTDEFINITIONTYPE * video_def = &def.format.video;

    //    printf("%d nBufferCount(Actual/Min) %d %d, %.1fK, Enabled %d, Populated %d"
    //                "\n\t%s, %dx%d, f-rate %d, color %d\n"
    //            , def.nPortIndex, def.nBufferCountActual,def.nBufferCountMin, def.nBufferSize/1024.0, def.bEnabled, def.bPopulated
    //            , video_def->cMIMEType
    //            , video_def->nFrameWidth
    //            , video_def->nFrameHeight
    //            , video_def->xFramerate
    //            , video_def->eColorFormat
    //            ); // OMX_COLOR_FormatYUV420Planar=19
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
    OMX_BUFFERHEADERTYPE* omx_alloc_buf(IOPort& iop, unsigned bidx) {
        OMX_BUFFERHEADERTYPE* buf = 0;
        OMX_PARAM_PORTDEFINITIONTYPE* def = &iop; //(*this)[bidx];
        if (bidx < def->nBufferCountActual && bidx < iop.size()) {
            //printf("OMX_AllocateBuffer [%d:%d] %u\n", def->nPortIndex, bidx, def->nBufferSize);
            OMX_ERRORTYPE err = OMX_AllocateBuffer(component, &buf, def->nPortIndex, (OMX_PTR)bidx, def->nBufferSize); // pAppPrivate
            ERR_EXIT_IF(err!=OMX_ErrorNone, "OMX_AllocateBuffer %d [%d] %u: %s\n", def->nPortIndex, bidx, def->nBufferSize, error_str(err));
            //sleep(1); // TODO: remove sleep
            //printf("OMX_AllocateBuffer [%d:%d] %u OK\n", def->nPortIndex, bidx, def->nBufferSize);
        }
        return buf;
    }

private: // callbacks
    OMX_ERRORTYPE cbEmptyThisBuffer(OMX_BUFFERHEADERTYPE* buf)
    {
        IOPort& iop = input_port;
        iop.bufflag_send &= ~(1<<index(buf));
        iop.bufflag_allocated &= ~(1<<index(buf));

        printf("%s: bidx %d, %u %u %u\n",__FUNCTION__, index(buf), buf->nAllocLen, buf->nFilledLen, buf->nOffset);
        iop.nCallback++;
        // nextEmptyBuffer();

        //if(pBuffer->pPlatformPrivate < 102400) exit(1);
        //buffer_in_mask |= 1<<*(short*)pBuffer->pPlatformPrivate;
        //sem_post(&wait_buff);

        return OMX_ErrorNone;
    }
    OMX_ERRORTYPE cbFillThisBuffer(OMX_BUFFERHEADERTYPE* buf)
    {
        if (buf->nFilledLen <= 0)/*(command_ == OMX_CommandPortDisable)*/ {
            output_port.bufflag_allocated &= ~(1<<index(buf));
            if (output_port.bufflag_allocated == 0) {
                printf("%s\n",__FUNCTION__);
                //sync_port_definition(output_port);
                //? sem_post(&sem_command_);
            }
        } else {
        printf("%s: bidx %d, %u %u %u\n",__FUNCTION__, index(buf), buf->nAllocLen, buf->nFilledLen, buf->nOffset);
            IOPort& iop = output_port;
            iop.bufflag_send |= (1<<index(buf));
            //iop.bufflag_send &= ~(1<<index(buf));
            //iop.bufflag_allocated &= ~(1<<index(buf));

            iop.nCallback++;
            printf("%s: bidx %d, nCB %d\n",__FUNCTION__, index(buf), iop.nCallback);
            nextFillBuffer(iop, buf);
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
        sync_state();
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

#define CASE_THEN_RETURN_STR(y) case OMX_ ## y: return #y

    static char const* cmd_str(unsigned x) {
        switch (x) {
            CASE_THEN_RETURN_STR(CommandStateSet);
            CASE_THEN_RETURN_STR(CommandFlush);
            CASE_THEN_RETURN_STR(CommandPortDisable);
            CASE_THEN_RETURN_STR(CommandPortEnable);
            CASE_THEN_RETURN_STR(CommandMarkBuffer);
            CASE_THEN_RETURN_STR(CommandMax);
        }
        return "Unknown-Command";
    }

    static char const* state_str(int x) {
        switch (x) {
            CASE_THEN_RETURN_STR(StateInvalid);
            CASE_THEN_RETURN_STR(StateLoaded);
            CASE_THEN_RETURN_STR(StateIdle);
            CASE_THEN_RETURN_STR(StateExecuting);
            CASE_THEN_RETURN_STR(StatePause);
            CASE_THEN_RETURN_STR(StateWaitForResources);
        }
        return "Unknown-State";
    }
    static char const* event_str(int x) {
        switch (x) {
            CASE_THEN_RETURN_STR(EventCmdComplete);
            CASE_THEN_RETURN_STR(EventError);
            CASE_THEN_RETURN_STR(EventMark);
            CASE_THEN_RETURN_STR(EventPortSettingsChanged);
            CASE_THEN_RETURN_STR(EventBufferFlag);
            CASE_THEN_RETURN_STR(EventResourcesAcquired);
            CASE_THEN_RETURN_STR(EventComponentResumed);
            CASE_THEN_RETURN_STR(EventDynamicResourcesAvailable);
            CASE_THEN_RETURN_STR(EventPortFormatDetected);
        }
        return "Unknown-Event";
    }
    static char const* error_str(/*OMX_ERRORTYPE*/int x) {
        switch (x) {
            case OMX_ErrorNone: return "OK";
            CASE_THEN_RETURN_STR(ErrorInsufficientResources);
            CASE_THEN_RETURN_STR(ErrorUndefined);
            CASE_THEN_RETURN_STR(ErrorInvalidComponentName);
            CASE_THEN_RETURN_STR(ErrorComponentNotFound);
            CASE_THEN_RETURN_STR(ErrorInvalidComponent);
            CASE_THEN_RETURN_STR(ErrorBadParameter);
            CASE_THEN_RETURN_STR(ErrorNotImplemented);
            CASE_THEN_RETURN_STR(ErrorUnderflow);
            CASE_THEN_RETURN_STR(ErrorOverflow);
            CASE_THEN_RETURN_STR(ErrorHardware);
            CASE_THEN_RETURN_STR(ErrorInvalidState);
            CASE_THEN_RETURN_STR(ErrorStreamCorrupt);
            CASE_THEN_RETURN_STR(ErrorPortsNotCompatible);
            CASE_THEN_RETURN_STR(ErrorResourcesLost);
            CASE_THEN_RETURN_STR(ErrorNoMore);
            CASE_THEN_RETURN_STR(ErrorVersionMismatch);
            CASE_THEN_RETURN_STR(ErrorNotReady);
            CASE_THEN_RETURN_STR(ErrorTimeout);
            CASE_THEN_RETURN_STR(ErrorSameState);
            CASE_THEN_RETURN_STR(ErrorResourcesPreempted);
            CASE_THEN_RETURN_STR(ErrorPortUnresponsiveDuringAllocation);
            CASE_THEN_RETURN_STR(ErrorPortUnresponsiveDuringDeallocation);
            CASE_THEN_RETURN_STR(ErrorPortUnresponsiveDuringStop);
            CASE_THEN_RETURN_STR(ErrorIncorrectStateTransition);
            CASE_THEN_RETURN_STR(ErrorIncorrectStateOperation);
            CASE_THEN_RETURN_STR(ErrorUnsupportedSetting);
            CASE_THEN_RETURN_STR(ErrorUnsupportedIndex);
            CASE_THEN_RETURN_STR(ErrorBadPortIndex);
            CASE_THEN_RETURN_STR(ErrorPortUnpopulated);
            CASE_THEN_RETURN_STR(ErrorComponentSuspended);
            CASE_THEN_RETURN_STR(ErrorDynamicResourcesUnavailable);
            CASE_THEN_RETURN_STR(ErrorMbErrorsInFrame);
            CASE_THEN_RETURN_STR(ErrorFormatNotDetected);
            CASE_THEN_RETURN_STR(ErrorContentPipeOpenFailed);
            CASE_THEN_RETURN_STR(ErrorContentPipeCreationFailed);
            CASE_THEN_RETURN_STR(ErrorSeperateTablesUsed);
            CASE_THEN_RETURN_STR(ErrorTunnelingUnsupported);
        }
        return "Unknown-Error";
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
    //enum { vWidth = 1920, vHeight = 1080 };
    //enum { vWidth = 1280, vHeight = 720 };
    enum { vWidth = 480, vHeight = 272 };

    OMXControl ox(COMPONENT); // ox.test_print_port_params();
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

