#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <fcntl.h>


#include <OpenMAX/IL/OMX_Core.h>
#include <OpenMAX/IL/OMX_Component.h>
#include <OpenMAX/IL/OMX_Types.h>
#include "frameworks/av/media/libstagefright/omx/OMXMaster.h"
#include <utils/Debug.h>
#include <memory>
// #include <initializer_list>

//#define COMPONENT "OMX.Nvidia.h264.decode"
#define COMPONENT "OMX.MTK.VIDEO.DECODER.AVC"

#define DEBUG
//#define DUMP

// android::OMXMaster* omxMaster;
// //OMX_ERRORTYPE OMXMaster::makeComponentInstance(const char *name, const OMX_CALLBACKTYPE *callbacks, OMX_PTR appData, OMX_COMPONENTTYPE **component);
// 
// //static int new_state;
// static sem_t wait_for_state;
// static sem_t wait_buff;
// //static sem_t wait_for_parameters;
// static int image_width=0;
// static int image_height=0;
// 
// static OMX_BUFFERHEADERTYPE **omx_buffers_out;
// static OMX_BUFFERHEADERTYPE **omx_buffers_in;
// 
// static OMX_HANDLETYPE decoderhandle;
// 
// static int buffer_out_mask, buffer_out_nb;
// static int buffer_in_mask, buffer_in_nb, buffer_in_size;
// 
// int dumper, input; // write out fd

template <typename... As> void err_exit_(int ln, char const* fmt, As... a) {
    fprintf(stderr, fmt, ln, a...);
    exit(127);
}
template <typename... As> void err_msg_(int ln, char const* e, char const* fmt, As... a) {
    fprintf(stderr, fmt, ln, e, a...);
    fprintf(stderr, "\n");
}
#define ERR_EXIT(...) err_exit_(__LINE__, "E%d: " __VA_ARGS__)
#define CERR_MSG(e, ...) if(!(e))err_msg_(__LINE__, #e, "E%d:%s: " __VA_ARGS__)

#define bufstate(_d) \
printf(#_d": %d %d %d %x\n", \
		buffer_##_d##_ap, \
		buffer_##_d##_avp, \
		buffer_##_d##_pos%buffer_##_d##_nb, \
		\
		omx_buffers_##_d[ \
			buffer_##_d##_pos % buffer_##_d##_nb \
		] \
	)

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

/// static OMX_ERRORTYPE decoderEmptyBufferDone(
/// 		OMX_OUT OMX_HANDLETYPE hComponent,
/// 		OMX_OUT OMX_PTR pAppData,
/// 		OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) {
/// 
/// 	printf("empty %p\n", pBuffer->pPlatformPrivate);
/// 	//if(pBuffer->pPlatformPrivate < 102400) exit(1);
/// 
/// 	buffer_in_mask |= 1<<*(short*)pBuffer->pPlatformPrivate;
/// 	sem_post(&wait_buff);
/// 
/// 	return OMX_ErrorNone;
/// }
/// 
/// static OMX_ERRORTYPE decoderFillBufferDone(
/// 		OMX_OUT OMX_HANDLETYPE hComponent,
/// 		OMX_OUT OMX_PTR pAppData,
/// 		OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
/// {
/// 
/// 	static int got = 0, times=0;
/// 
/// 	times++;
/// 
/// 	got +=  pBuffer->nFilledLen,
/// 	printf("filled %p %d %d %d\n", pBuffer,  pBuffer->nFilledLen, got, times );
/// 
/// #ifdef DUMP
/// 	write(dumper, pBuffer->pBuffer,  pBuffer->nFilledLen);
/// 	fsync(dumper);
/// 	printf("sync...\n");
/// #endif
/// 
/// 	buffer_out_mask |= 1<<*(short*)pBuffer->pPlatformPrivate;
/// 
/// 	sem_post(&wait_buff);
/// 
/// #if 0
/// 	int i;
/// 	printf("dump: ->");
/// 	for(i=0;i<pBuffer->nFilledLen;i++)
/// 		printf("%02x ", *(pBuffer->pBuffer+i));
/// 
/// 	printf("<-\n");
/// #endif
/// 
/// 	return OMX_ErrorNone;
/// }
/// 
/// static OMX_ERRORTYPE decoderEventHandler(
/// 		OMX_OUT OMX_HANDLETYPE hComponent,
/// 		OMX_OUT OMX_PTR pAppData,
/// 		OMX_OUT OMX_EVENTTYPE eEvent,
/// 		OMX_OUT OMX_U32 Data1,
/// 		OMX_OUT OMX_U32 Data2,
/// 		OMX_OUT OMX_PTR pEventData)
/// {
/// 	printf("VD_OMX:Got event %x %x %x\n", eEvent, (unsigned int)Data1, (unsigned int)Data2);
/// 
/// 	switch(eEvent) {
/// 	case OMX_EventCmdComplete:
/// 		switch((OMX_COMMANDTYPE)Data1) {
/// 		case OMX_CommandStateSet:
/// 			printf("state switch %d\n", Data2);
/// 			//new_state = Data2;
/// 			sem_post(&wait_for_state);
/// 			return OMX_ErrorNone;
/// 		}
/// 	case OMX_EventPortSettingsChanged:
/// 		/*
/// 		if(Data1 != 1) {
/// 			fprintf(stderr, "got a INPUT port changed event !\n");
/// 			return OMX_ErrorNone;
/// 		}
/// 
/// 		OMX_ERRORTYPE err;
/// 		OMX_PARAM_PORTDEFINITIONTYPE paramPort;
/// 		InitOMXParams(&paramPort);
/// 		paramPort.nPortIndex = 1;
/// 
/// 		err=OMX_GetParameter(decoderhandle, OMX_IndexParamPortDefinition, &paramPort);
/// 
/// 		if(err != OMX_ErrorNone) 
/// 			fprintf(stderr, "EE:%s:%d\n", __FUNCTION__, __LINE__);
/// 
/// 
/// 		image_width=paramPort.format.video.nFrameWidth;
/// 		image_height=paramPort.format.video.nFrameHeight;
/// 
/// 		buffer_out_size=paramPort.nBufferSize;
/// 		sem_post(&wait_for_parameters);
/// 
/// 
/// 		fprintf(stderr, "buffer size = %d\n", buffer_out_size);
/// 		fprintf(stderr, "FPS=%d*2**16 + %d\n", paramPort.format.video.xFramerate>>16, paramPort.format.video.xFramerate&0xffff);
/// 		*/
/// 		printf("ou~\n");
/// 		break;
/// 	case OMX_EventBufferFlag:
/// 		printf("thas alll\n");
/// 
/// #ifdef DUMP
/// 		close(dumper);
/// #endif
/// 		exit(0);
/// 	case OMX_EventError:
/// 		printf("fail\n");
/// 		exit(1);
/// 	}
/// 	return OMX_ErrorNone;
/// }
/// 
/// static OMX_CALLBACKTYPE decodercallbacks = {
/// 	.EventHandler = decoderEventHandler,
/// 	.EmptyBufferDone = decoderEmptyBufferDone,
/// 	.FillBufferDone = decoderFillBufferDone,
/// };
/// 
/// static inline void _GoToState(int state) {
/// 	OMX_ERRORTYPE err;
/// 	err = OMX_SendCommand(decoderhandle, OMX_CommandStateSet, state, NULL);
/// 	CERR_MSG(err==OMX_ErrorNone, "OMX_SendCommand");
/// };
/// 
/// static void GoToState(int state) {
/// 	//int sval;
/// 	////Is there a more secure way ?
/// 	// new_state=state;
///     //while (sem_getvalue(&wait_for_state, &sval)==0 && sval > 0) usleep(1000*50);
/// 	_GoToState(state);
/// 	sem_wait(&wait_for_state);
/// }

struct Test
{
    android::OMXMaster omxMaster; // err = OMX_Init();
    OMX_HANDLETYPE component; //OMX_COMPONENTTYPE *component;
    struct {
        OMX_BUFFERHEADERTYPE* bufs[16];
    } iov[2];
    sem_t sem_command_;
    //volatile bool state_changing;

    Test(char const* component_name) {
        static OMX_CALLBACKTYPE callbacks = {
            .EventHandler = &s_eventHandler,
            .EmptyBufferDone = &s_emptyBufferDone,
            .FillBufferDone = &s_fillBufferDone
        };
        sem_init(&sem_command_, 0, 0);
        // state_changing = 0;
        memset(&iov, 0, sizeof(iov));

        OMX_COMPONENTTYPE *handle;
        OMX_ERRORTYPE err = omxMaster.makeComponentInstance(component_name, &callbacks, this, &handle);
        CERR_MSG(err==OMX_ErrorNone, "makeComponentInstance");
        component = /*(OMX_HANDLETYPE)*/handle; // OMX_GetHandle(&component, (char*)component_name, NULL, &callbacks);
    }
    ~Test() {
        omxMaster.destroyComponentInstance((OMX_COMPONENTTYPE*)component);
        sem_destroy(&sem_command_);
        // clearbufs();
        printf("~\n");
    }

    void init() {
        // OMX_IndexParamVideoInit, OMX_PORT_PARAM_TYPE
    }

    void decode() //void * data, int len
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

    void test_disable_ports() {
        disableSomePorts( OMX_IndexParamVideoInit);
        disableSomePorts( OMX_IndexParamImageInit);
        disableSomePorts( OMX_IndexParamAudioInit);
        disableSomePorts( OMX_IndexParamOtherInit);
    }

    void disableSomePorts(OMX_INDEXTYPE indexType)
    {
        OMX_ERRORTYPE err;

        OMX_PORT_PARAM_TYPE param;
        android::InitOMXParams(&param);

        err = OMX_GetParameter(component, indexType, &param);
        CERR_MSG(err==OMX_ErrorNone, "OMX_GetParameter");

        int startPortNumber = param.nStartPortNumber;
        int nPorts = param.nPorts;
        int endPortNumber = startPortNumber + nPorts;
        printf("Port start=%d, count=%d\n", startPortNumber, nPorts);

        for (int n = startPortNumber; n < endPortNumber; n++) {
            OMX_SendCommand(component, OMX_CommandPortDisable, n, NULL);
            sem_wait(&sem_command_);
        }
    }

    //enum { vWidth = 1920, vHeight = 1080 };
    enum { vWidth = 1280, vHeight = 720 };

    void test_port_def(int portIdx)
    {
        // OMX_IndexParamVideoPortFormat OMX_VIDEO_PARAM_PORTFORMATTYPE 
        if (portIdx == 0) {
            OMX_PARAM_PORTDEFINITIONTYPE def;
            OMX_VIDEO_PORTDEFINITIONTYPE *video_def;
            android::InitOMXParams(&def)->nPortIndex = portIdx;
            //
            OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
            CERR_MSG(err==OMX_ErrorNone, "OMX_GetParameter");
            //def.nBufferSize=vWidth*vHeight*3/2;
            //def.format.video.nBitrate = 1024*1024*2;
            def.format.video.nFrameWidth = vWidth;
            def.format.video.nFrameHeight = vHeight;
            def.format.video.nStride = def.format.video.nFrameWidth;
            err=OMX_SetParameter(component, OMX_IndexParamPortDefinition, &def);
            CERR_MSG(err==OMX_ErrorNone, "OMX_SetParameter");
        }
        if (portIdx == 1) {
            OMX_PARAM_PORTDEFINITIONTYPE def;
            OMX_VIDEO_PORTDEFINITIONTYPE *video_def;
            android::InitOMXParams(&def)->nPortIndex = portIdx;
            //
            OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
            CERR_MSG(err==OMX_ErrorNone, "OMX_GetParameter");
            //def.nBufferSize=vWidth*vHeight*3/2;
            def.format.video.nFrameWidth = vWidth;
            def.format.video.nFrameHeight = vHeight;;
            def.format.video.nStride = def.format.video.nFrameWidth;
            err=OMX_SetParameter(component, OMX_IndexParamPortDefinition, &def);
            CERR_MSG(err==OMX_ErrorNone, "OMX_SetParameter");
        }
    }

    void test_allocbufs()
    {
        allocbufs(0);
        allocbufs(1);
    }

    void allocbufs(int portidx)
    {
        OMX_ERRORTYPE err;
        OMX_PARAM_PORTDEFINITIONTYPE def;

        android::InitOMXParams(&def)->nPortIndex = portidx;
        err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
        CERR_MSG(err==OMX_ErrorNone, "OMX_GetParameter");

        if (def.nBufferCountActual > 16)
            fprintf(stderr, "%d nBufferCountActual %d > 16, L%d\n", def.nPortIndex, def.nBufferCountActual, __LINE__);
        int nBufferCountActual = std::min(int(def.nBufferCountActual), 16); //
        //nBufferCountActual = std::max(nBufferCountActual, 5); //

        printf("%d OMX_AllocateBuffer %d size=%d\n", def.nPortIndex, nBufferCountActual, (int)def.nBufferSize);
        for (int i=0; i < nBufferCountActual; ++i) {
            err = OMX_AllocateBuffer(component, &iov[portidx].bufs[i], def.nPortIndex, NULL, def.nBufferSize);
            CERR_MSG(err==OMX_ErrorNone, "OMX_AllocateBuffer");
        }
        //bufs = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * (def[0].nBufferCountMin+def[1].nBufferCountMin));
    }

    void test_change_state(OMX_STATETYPE state1)
    {
        int sv[2] = {};
        sem_getvalue(&sem_command_, &sv[0]);

        static char const* sts[] = { "OMX_StateInvalid", "OMX_StateLoaded", "OMX_StateIdle", "OMX_StateExecuting", "OMX_StatePause", "OMX_StateWaitForResources" };
        OMX_STATETYPE state0;
        OMX_GetState(component, &state0);

        printf("=== %d:x state %d:%s -> %d:%s\n", sv[0], (int)state0, sts[state0], state1, sts[state1]);

        //state_changing = 1;
        printf("OMX_SendCommand:OMX_CommandStateSet %s\n", sts[state1]);
        OMX_ERRORTYPE err = OMX_SendCommand(component, OMX_CommandStateSet, state1, NULL);
        CERR_MSG(err==OMX_ErrorNone, "OMX_SendCommand");

    test_allocbufs();

        printf("sem_wait:sem_command_\n");
        sem_wait(&sem_command_); // OMX_StateExecuting OMX_StateWaitForResources
        //state_changing = 0;

        sem_getvalue(&sem_command_, &sv[1]);
        printf("=== %d:%d state %d:%s -> %d:%s\n", sv[0],sv[1], state0, sts[state0], state1, sts[state1]);
    }

    void test_print_port_params()
    {
        test_print_params(OMX_DirInput);
        test_print_params(OMX_DirOutput);
    }
    void test_print_params(int portIndex)
    {
        OMX_PARAM_PORTDEFINITIONTYPE def;
        android::InitOMXParams(&def)->nPortIndex = portIndex;
        OMX_ERRORTYPE err=OMX_GetParameter(component, OMX_IndexParamPortDefinition, &def);
        CERR_MSG(err==OMX_ErrorNone, "OMX_GetParameter");
        OMX_VIDEO_PORTDEFINITIONTYPE * video_def = &def.format.video;

        printf("%d nBufferCount(Actual/Min) %d %d, %.1fK, Enabled %d, Populated %d"
                    "\n\t%s, %dx%d, f-rate %d, color %d\n"
                , def.nPortIndex, def.nBufferCountActual,def.nBufferCountMin, def.nBufferSize/1024.0, def.bEnabled, def.bPopulated
                , video_def->cMIMEType
                , video_def->nFrameWidth
                , video_def->nFrameHeight
                , video_def->xFramerate
                , video_def->eColorFormat
                ); // OMX_COLOR_FormatYUV420Planar=19
    }

private: // callbacks
    OMX_ERRORTYPE eventHandler(OMX_OUT OMX_EVENTTYPE eEvent
            , OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
    {
        printf("eventHandler %d %d %d\n", (int)eEvent, (int)Data1, (int)Data2);

        switch (eEvent) {
            case OMX_EventCmdComplete:
                printf("OMX_EventCmdComplete %d %d\n", (int)Data1, (int)Data2);
                switch ((OMX_COMMANDTYPE)Data1) {
                    case OMX_CommandStateSet:
                        printf("OMX_CommandStateSet %d %d\n", (int)Data1, (int)Data2);
                        //CERR_MSG(state_changing, "OMX_CommandStateSet");
                        sem_post(&sem_command_);
                        return OMX_ErrorNone;
                    case OMX_CommandPortDisable:
                        printf("OMX_CommandPortDisable %d %d\n", (int)Data1, (int)Data2);
                        sem_post(&sem_command_);
                        return OMX_ErrorNone;
                }
            case OMX_EventPortSettingsChanged:
                printf("OMX_EventPortSettingsChanged %d %d\n", (int)Data1, (int)Data2);
                /*
                   if(Data1 != 1) {
                   fprintf(stderr, "got a INPUT port changed event !\n");
                   return OMX_ErrorNone;
                   }

                   OMX_ERRORTYPE err;
                   OMX_PARAM_PORTDEFINITIONTYPE paramPort;
                   InitOMXParams(&paramPort);
                   paramPort.nPortIndex = 1;

                   err=OMX_GetParameter(decoderhandle, OMX_IndexParamPortDefinition, &paramPort);

                   if(err != OMX_ErrorNone) 
                   fprintf(stderr, "EE:%s:%d\n", __FUNCTION__, __LINE__);


                   image_width=paramPort.format.video.nFrameWidth;
                   image_height=paramPort.format.video.nFrameHeight;

                   buffer_out_size=paramPort.nBufferSize;
                   sem_post(&wait_for_parameters);


                   fprintf(stderr, "buffer size = %d\n", buffer_out_size);
                   fprintf(stderr, "FPS=%d*2**16 + %d\n", paramPort.format.video.xFramerate>>16, paramPort.format.video.xFramerate&0xffff);
                   */
                break;
            case OMX_EventBufferFlag: ///**< component has detected an EOS */
                ERR_EXIT("OMX_EventBufferFlag");
            case OMX_EventError: ///**< component has detected an error condition */
                ERR_EXIT("OMX_EventError");
        }
        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE emptyBufferDone(OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
    {
        printf("emptyBufferDone %p\n", pBuffer->pPlatformPrivate);
        //if(pBuffer->pPlatformPrivate < 102400) exit(1);

        //buffer_in_mask |= 1<<*(short*)pBuffer->pPlatformPrivate;
        //sem_post(&wait_buff);

        return OMX_ErrorNone;
    }
    OMX_ERRORTYPE fillBufferDone(OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
    {
#if 0
        static int got = 0, times=0;

        times++;
        got +=  pBuffer->nFilledLen,
        printf("fillBufferDone %p %d %d %d\n", pBuffer,  pBuffer->nFilledLen, got, times );

#ifdef DUMP
        write(dumper, pBuffer->pBuffer,  pBuffer->nFilledLen);
        fsync(dumper);
        printf("sync...\n");
#endif

        //buffer_out_mask |= 1<<*(short*)pBuffer->pPlatformPrivate;
        //sem_post(&wait_buff);

#if 0
        int i;
        printf("dump: ->");
        for(i=0;i<pBuffer->nFilledLen;i++)
            printf("%02x ", *(pBuffer->pBuffer+i));

        printf("<-\n");
#endif

#endif
        return OMX_ErrorNone;
    }

    static OMX_ERRORTYPE s_eventHandler(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData
            , OMX_OUT OMX_EVENTTYPE eEvent, OMX_OUT OMX_U32 Data1, OMX_OUT OMX_U32 Data2, OMX_OUT OMX_PTR pEventData)
    {
        assert(hComponent == static_cast<Test*>(pAppData)->component);
        return static_cast<Test*>(pAppData)->eventHandler(eEvent, Data1, Data2, pEventData);
    }
    static OMX_ERRORTYPE s_emptyBufferDone(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData
            , OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
    {
        assert(hComponent == static_cast<Test*>(pAppData)->component);
        return static_cast<Test*>(pAppData)->emptyBufferDone(pBuffer);
    }
    static OMX_ERRORTYPE s_fillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_PTR pAppData
            , OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
    {
        assert(hComponent == static_cast<Test*>(pAppData)->component);
        return static_cast<Test*>(pAppData)->fillBufferDone(pBuffer);
    }
};

#if 0
static int init()
{
    OMX_ERRORTYPE err;

    omxMaster = new android::OMXMaster ; // err = OMX_Init();
    //printf("om init %d\n", err);
    //if (err != OMX_ErrorNone) {
    //    return 0;
    //}

    OMX_COMPONENTTYPE *component;
    err = omxMaster->makeComponentInstance(COMPONENT, &decodercallbacks, NULL, &component);
    decoderhandle = (OMX_HANDLETYPE)component;
    //err = OMX_GetHandle(&decoderhandle, (char*)COMPONENT, NULL, &decodercallbacks);
    OMXERR_MSG(err);

    // output buffers
    OMX_PARAM_PORTDEFINITIONTYPE paramPort;
    android::InitOMXParams(&paramPort, 1); //paramPort.nPortIndex = 1;

    err=OMX_GetParameter(decoderhandle, OMX_IndexParamPortDefinition, &paramPort);
    OMXERR_MSG(err);

    /*
     * Codec=7
     * Color=19
     * Size=1920x816
     * */

#ifdef DEBUG
    printf("Requesting %d buffers of %d bytes\n", paramPort.nBufferCountMin, paramPort.nBufferSize);
#endif

    paramPort.nBufferSize=848*480*3/2;
    paramPort.format.video.nFrameWidth = 848;
    paramPort.format.video.nFrameHeight = 480;;

    err=OMX_SetParameter(decoderhandle, OMX_IndexParamPortDefinition, &paramPort);
    OMXERR_MSG(err);

    omx_buffers_out = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * paramPort.nBufferCountMin);

    unsigned i;
    short *count = (short*)malloc(paramPort.nBufferCountMin+100);

    for(i=0;i<paramPort.nBufferCountMin;++i) {
        err = OMX_AllocateBuffer(decoderhandle, &omx_buffers_out[i], 1, NULL, paramPort.nBufferSize);
        OMXERR_MSG(err);

        buffer_out_mask |= 1<<i;

        omx_buffers_out[i]->pPlatformPrivate = count;
        *count = i;
        count++;


#ifdef DEBUG
        printf("buf_out[%d]=%p\n", i, omx_buffers_out[i]);
#endif
    }


    buffer_out_nb = paramPort.nBufferCountMin;

    // input buffers
    android::InitOMXParams(&paramPort, 0); //paramPort.nPortIndex = 0;

    err=OMX_GetParameter(decoderhandle, OMX_IndexParamPortDefinition, &paramPort);
    OMXERR_MSG(err);

#ifdef DEBUG
    printf("Requesting %d buffers of %d bytes\n", paramPort.nBufferCountMin, paramPort.nBufferSize);
#endif

    omx_buffers_in = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * paramPort.nBufferCountMin);

    count = (short*)malloc(paramPort.nBufferCountMin+100);

    printf("buffers: %d x %d\n", paramPort.nBufferSize, paramPort.nBufferCountMin);

    buffer_in_size = paramPort.nBufferSize;

    for(i=0;i<paramPort.nBufferCountMin;++i) {
        err = OMX_AllocateBuffer(decoderhandle, &omx_buffers_in[i], 0, NULL, paramPort.nBufferSize);
        OMXERR_MSG(err);

        buffer_in_mask |= 1<<i;

        omx_buffers_in[i]->pPlatformPrivate = count;
        *count = i;
        count++;

#ifdef DEBUG
        printf("buf_in[%d]=%p\n", i, omx_buffers_in[i]);
#endif
    }

    buffer_in_nb = paramPort.nBufferCountMin;
    return 1;
}

void decode() //void * data, int len)
{
    OMX_ERRORTYPE err;
    OMX_BUFFERHEADERTYPE *buf;

    if(!(buffer_in_mask || buffer_out_mask)) {
        printf("wait damn avp\n");
        sem_wait(&wait_buff);
    }

    for(int i=0;i<buffer_out_nb;i++) {
        if( ! ((1<<i) & buffer_out_mask ) )
            continue;

        err = OMX_FillThisBuffer(decoderhandle, omx_buffers_out[i]);
        OMXERR_MSG(err);

        buffer_out_mask &= (1<<i) ^ 0xFFFFFFFF;
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
}
#endif

int main()
{
    //{ Test test(COMPONENT); test.test_port_def(-1);}//
    //{ Test test(COMPONENT); test.test_port_def(OMX_DirInput); }//
    Test test(COMPONENT);
    test.test_print_port_params();
    //test.test_port_def(-1);
    //test.test_print_port_params();
    test.test_port_def(0); //(OMX_DirInput);
    test.test_port_def(1); //(OMX_DirOutput);
    //test.test_allocbufs();
    test.test_print_port_params();
    //test.test_disable_ports();
    test.test_change_state(OMX_StateIdle);
    test.test_print_port_params();

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
//    init();
//
//    printf("idle\n");
//    GoToState(OMX_StateIdle);
//    printf("go executing\n");
//    GoToState(OMX_StateExecuting);
//
//    while(1)
//        decode();
}

