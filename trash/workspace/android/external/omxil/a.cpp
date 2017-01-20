#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/ACodec.h>

#include <media/ICrypto.h>
#include <media/IMediaPlayerService.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/DisplayInfo.h>
#include <android_runtime/android_view_Surface.h>

#include <new>
#include <map>
#include <vector>
#include <string>
#include <new>
#include <stdio.h>

#include <system/graphics.h>
//#include <vpu_global.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
//#include <OMX_Component.h>
//#include <media/IOMX.h>

//extern "C" {
////#include "libstagefright.h"
////#include "libdvbpsi.h"
////#include "receiver.h"
//
//#include <unistd.h>
//#include <fcntl.h>
//#include <semaphore.h>
//
//extern para_process_t para;
//extern media_handle_t programe_media;
//}
#define ERR_EXIT(...) exit(__LINE__)

using namespace android;

struct TimeStamp {
    int64_t pts;
    int64_t reordered_opaque;
};
struct Frame {
    uint32_t size;
    uint8_t *buffer;
    int64_t time;
};

class CStageFrightMediaSource : public MediaSource
{
    status_t av_read_frame(struct Frame *frame) {
        uint32_t siz;
        int64_t timestamp;
        uint8_t *data;

        frame->buffer = data;
        frame->time = timestamp;//(++frame_num)*3000;
        frame->size = siz;
        //frame->status = OK;
        return OK;
    }

public:
    CStageFrightMediaSource(char const* mimetype, int w, int h/*sp<MetaData> meta*/) {
        sp<MetaData> meta = new MetaData;
        if (meta == NULL) {
            ERR_EXIT("cannot allocate MetaData");
        }
        //if(video_type == VIDEO_AVC) {
        //        mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
        //} else if(video_type == VIDEO_HEVC) {
        //        mimetype = MEDIA_MIMETYPE_VIDEO_HEVC;
        //}
        meta->setCString(kKeyMIMEType, mimetype);
        meta->setInt32(kKeyWidth, w);
        meta->setInt32(kKeyHeight, h);
        source_meta = meta;

        buf_group.add_buffer(new MediaBuffer((w * h * 3) / 2));
        buf_group.add_buffer(new MediaBuffer((w * h * 3) / 2));
    }

    virtual sp<MetaData> getFormat() {
        return source_meta;
    }

    virtual status_t start(MetaData *params) {
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

    virtual status_t read(MediaBuffer **mbuffer, const MediaSource::ReadOptions *options)
    {
        MediaBuffer *mbuf = *mbuffer;
        printf("[%d] %p\n", __LINE__, mbuf);
        //printf("@@@@@@@@@@@@@@@@@@@@[%d]\n", __LINE__);
        status_t ret;
        struct Frame frame;

        /*
           struct Frame *frame = NULL;
           if((frame = (struct Frame *)malloc(sizeof(struct Frame))) == NULL) {
           printf("%s:: Failed to malloc a frame\n", __FUNCTION__);
           return 1;
           }
           */

        //frame->buffer = (uint8_t *)malloc(256*1024);
        ret = buf_group.acquire_buffer(mbuffer);
        ret = av_read_frame(&frame);
        //printf("%s: status[%d], size[%d], buffer[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x]\n", __FUNCTION__, frame->status, frame->size, \
        //frame->buffer[0], frame->buffer[1], frame->buffer[2], frame->buffer[3], frame->buffer[4], frame->buffer[5]);

        if (ret == OK) {
            //printf("READ A FRAME/FIELD OK\n")
            ret = buf_group.acquire_buffer(mbuffer);
            if (ret == OK) {
                //printf("ACUIRE GROUP BUFFER OK, frame->size[%d]\n", frame->size);
                memcpy((*mbuffer)->data(), frame.buffer, frame.size);
                (*mbuffer)->set_range(0, frame.size);
                (*mbuffer)->meta_data()->clear();
                (*mbuffer)->meta_data()->setInt64(kKeyTime, frame.time);
                //(*mbuffer)->meta_data()->setInt32(kKeyIsDeInterlace, 1);
            } else {
                printf("%s@Failed to acquire MediaBuffer\n", __FUNCTION__);
            }
        } else
            printf("%s@This frame is error\n", __FUNCTION__);

        //free(frame->buffer);
        //frame->buffer = NULL;
        /*       
                 free(frame);
                 frame = NULL;
                 */
        //printf("##############################[%d]\n", __LINE__);
        return ret;
    }

private:
    MediaBufferGroup buf_group;
    sp<MetaData> source_meta;
    //StagefrightContext *s;
    //int frame_size;
};

struct StagefrightContext
{
    StagefrightContext(); //(const char* mimetype, int w, int h);
    ~StagefrightContext();

    OMXClient mClient;
    sp<MediaSource> decoder;
    sp<MediaSource> source;

    pthread_mutex_t in_mutex, out_mutex;
    pthread_cond_t condition;
    //int width, height;
    //pthread_t decode_thread_id;

    //std::map<int64_t, TimeStamp> *ts_map;

    /* MediaCodec ACodec */
    //sp<MediaCodec> mACodecDecoder;
    //android::sp<android::Surface> surface;
    //sp<Surface> mSurfaceTextureClient;        //for ACodec(HARDWARERENDER)
    //Vector<sp<ABuffer> > mInBuffers;
    //Vector<sp<ABuffer> > mOutBuffers;

    //sp<MediaCodec> mACodecAudioDecoder;
    //Vector<sp<ABuffer> > mAudioInBuffers;
    //Vector<sp<ABuffer> > mAudioOutBuffers;

    //sp<ANativeWindow> mVideoNativeWindow; // for OMXCodec(HARDWARERENDER AND SOFTEWARERENDER), ACodec(SOFTWARERENDER)

    /* MediaPlayer OMXCodec */

    //int fd;
    //sp<MediaSource> *mAudioDecoder;
    //sp<MediaSource> *mAudioSource;

    //int mSampleRate;
    //sp<AudioTrack> mAudioTrack;
};
//static StagefrightContext sCtx_g = {};

StagefrightContext::~StagefrightContext()
{
    this->mClient.disconnect();
}

StagefrightContext::StagefrightContext() //static int OMXCodec_video_decoder_init(StagefrightContext *s, char const* mimetype) //(int video_type)
{
    const char* mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
    int w = 1920, h = 1080;
    //s->source    = new sp<MediaSource>();
    this->source   = new CStageFrightMediaSource(mimetype, w, h);//(meta);
    if (this->source == NULL) {
        ERR_EXIT("Cannot obtain source / mClient");
    }

    if (this->mClient.connect() !=  OK) {
        ERR_EXIT("Cannot connect OMX mClient");
    }
    printf("Connect OMX mClient success\n");

    //this->decoder  = new sp<MediaSource>();
    printf("[%s]@OMXCodec::Create____________________________START###\n", __FUNCTION__);
    this->decoder = OMXCodec::Create(this->mClient.interface(),
            this->source->getFormat(),
            false,
            this->source,
            NULL,
            OMXCodec::kHardwareCodecsOnly | OMXCodec::kClientNeedsFramebuffer,
            /*USE_SURFACE_ALLOC ? this->mVideoNativeWindow :*/ NULL);
    if (this->decoder == NULL || this->decoder->start() !=  OK) {
        this->mClient.disconnect();
        this->source = NULL;
        this->decoder = NULL;
        ERR_EXIT("Cannot start decoder");
    }
    printf("[%s]@OMXCodec::Create____________________________END\n", __FUNCTION__);
}

static void* video_decode_display_thread(void *arg)
{
    printf("[%s]Thread id:%d/n", __FUNCTION__, gettid());
    //StagefrightContext *s = (StagefrightContext*)arg;
    //static bool debug = false;
    //s->width = 1920;//para.video.width;
    //s->height = 1080; //para.video.height;
    StagefrightContext ctx; //OMXCodec_video_decoder_init// 阻塞式调用OMX IL组件

    /*
   for(int i =0; i < sizeof(rgb16); i++)
   rgb16 = 0x80;//red
   */
    //sem_init(&decode_sem, 0, 0);
    //sem_wait(&decode_sem);
    //static uint32_t tmp = 0;
    while(1) {       
        MediaBuffer *buffer = NULL;
        status_t status = ctx.decoder->read(&buffer, NULL);
        if (status == OK) {       
            printf("DECODER OK\n");
            if (buffer->range_length() == 0) {
                printf("%s:ERROR_BUFFER_TOO_SMALL\n", __FUNCTION__);
                status = ERROR_BUFFER_TOO_SMALL;
                buffer->release();
                buffer = NULL;
                continue;
            }
            //printf("BUFFER RANGE LENGTH[%d]\n", buffer->range_length());
        } else
            printf("%s@NOT OK\n", __FUNCTION__);

        if(status == OK) {
            sp<MetaData> outFormat = ctx.decoder->getFormat();
            int32_t width, height, format;
            outFormat->findInt32(kKeyWidth , &width);
            outFormat->findInt32(kKeyHeight, &height);
            outFormat->findInt32(kKeyColorFormat, &format);
            printf("W[%d], H[%d], COLORFORMAT[0x%08x]\n", width, height, format);
        }

        //VPU_FRAME *frame = (VPU_FRAME *)buffer->data();
        //printf("frame->FrameType[%d], FrameWidth[%d], FrameHeight[%d]\n", frame->FrameType, frame->FrameWidth, frame->FrameHeight);

#if 0
        if(USE_SURFACE_ALLOC) {
            //HARDWARE RENDERER
            if (status == OK) {
                // ONLY ADPAPTED FOR AVC/HEVC PROGRESSIVE MODE
                //采用OMX::CREAT默认的RENDER
                int64_t timeUs = 0;
                buffer->meta_data()->findInt64(kKeyTime, &timeUs);
                //printf("T[%lld]\n", timeUs);
                native_window_set_buffers_timestamp(s->mVideoNativeWindow.get(), timeUs * 1000);
                status_t err = s->mVideoNativeWindow->queueBuffer(s->mVideoNativeWindow.get(), buffer->graphicBuffer().get(), -1);
                if (err != 0) {
                    printf("queueBuffer failed with error %s (%d)", strerror(-err), -err);
                }
                else {
                    sp<MetaData> metaData = buffer->meta_data();
                    metaData->setInt32(kKeyRendered, 1);
                }
                buffer->release();
                buffer = NULL;
            }
        } else {
            //SOFTEWARE RENDERER
            if (status == OK) {
                if(DIRECT_RENDER) {       
                    //DIRECT RENDER
                    // ONLY FOR AVC PROGRESSIVE/INTERLACED MODE
                    int64_t timeUs = 0;
                    //buffer->meta_data()->findInt64(kKeyTime, &timeUs);
                    //printf("T[%lld]\n", timeUs);
                    my_render(buffer->data(), buffer->range_length(), s->mVideoNativeWindow, mWidth, mHeight, timeUs);
                } else {
                    // COLORCONVERTER YUV->RGB
                    ANativeWindow_Buffer Abuffer;
                    memset((void*)&Abuffer,0,sizeof(Abuffer));
                    int lockResult = -22;
                    lockResult = ANativeWindow_lock(s->mVideoNativeWindow.get(), &Abuffer, NULL);
                    if (lockResult == 0) {
                        printf("ANativeWindow_locked");

                        ColorConverter *mConverter;
                        OMX_COLOR_FORMATTYPE mColorFormat = (OMX_COLOR_FORMATTYPE)mFormat;
                        mConverter = new ColorConverter(mColorFormat, OMX_COLOR_Format16bitRGB565);

    //int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
    //int32_t mCropWidth, mCropHeight;
                        mCropLeft = mCropTop = 0;
                        mCropRight = mWidth - 1;
                        mCropBottom = mHeight - 1;
                        mCropWidth = mCropRight - mCropLeft + 1;
                        mCropHeight = mCropBottom - mCropTop + 1;

                        if(debug)
                        {
                            int fd = open("/mnt/sdcard/Movies/test.nv12", O_WRONLY | O_CREAT);
                            if(fd < 0) {
                                printf("store the nv12 data failed\n");
                                return NULL;
                            }
                            write(fd, buffer->data(), buffer->range_length());
                            debug = false;
                        }

                        if (mConverter) {
                            mConverter->convert(
                                    buffer->data(),
                                    mWidth, mHeight,
                                    mCropLeft, mCropTop, mCropRight, mCropBottom,
                                    rgb16,
                                    Abuffer.stride, Abuffer.height,
                                    0, 0, mCropWidth - 1, mCropHeight - 1);
                        }
                        memcpy(Abuffer.bits, rgb16,  sizeof(rgb16));//demo dislay rgb16 color red
                        delete mConverter;
                        mConverter = NULL;
                        ANativeWindow_unlockAndPost(s->mVideoNativeWindow.get());
                    }
                    else
                    {
                        printf("ANativeWindow_lock failed error %d",lockResult);
                    }

                    //printf("Releasing window");
                    //ANativeWindow_release(s->mVideoNativeWindow.get());
                }
                buffer->release();
                buffer = NULL;
            }
        }
#endif
    }
    return 0;
}

//extern thread_pool_t thread_desc;
pthread_t video_decoder_display_start(void)
{
    pthread_t handle_video_decoder_display;
    if (pthread_create(&handle_video_decoder_display, NULL, video_decode_display_thread, NULL) < 0) {
        printf("failed creating thread video_decode_display_thread\n");
        exit(EXIT_FAILURE);
    }
    //pthread_detach(handle_video_decoder_display);
    //thread_desc.handle_video_decoder_display = handle_video_decoder_display;
    printf("create video decoder display thread over!\n");
    return handle_video_decoder_display;
}

int main() {
    printf("hello world\n");
    pthread_t tid = video_decoder_display_start();
    void* ret;
    pthread_join(tid, &ret);
}

#if 0
#include <jni.h>
#include <android_runtime/AndroidRuntime.h>
#include <android_runtime/android_view_Surface.h>
#include <gui/Surface.h>
#include <assert.h>
#include <utils/Log.h>
#include <nativehelper/JNIHelp.h>
#include <media/stagefright/foundation/ADebug.h>
#include <ui/GraphicBufferMapper.h>
#include <cutils/properties.h>
using namespace android;

static sp<Surface> surface;

inline int ALIGN(int x, int y) {
    // y must be a power of 2.
    return (x + y - 1) & ~(y - 1);
}

static void render(const void *data, size_t size, const sp<ANativeWindow> &nativeWindow,int width,int height)
{
	ALOGE("[%s]%d",__FILE__,__LINE__);
    sp<ANativeWindow> mNativeWindow = nativeWindow;
    int err;
	int mCropWidth = width;
	int mCropHeight = height;
	
	int halFormat = HAL_PIXEL_FORMAT_YV12;//颜色空间
    int bufWidth = (mCropWidth + 1) & ~1;//按2对齐
    int bufHeight = (mCropHeight + 1) & ~1;
	
	CHECK_EQ(0,
            native_window_set_usage(
            mNativeWindow.get(),
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

    CHECK_EQ(0,
            native_window_set_scaling_mode(
            mNativeWindow.get(),
            NATIVE_WINDOW_SCALING_MODE_SCALE_CROP));

    // Width must be multiple of 32???
	//很重要,配置宽高和和指定颜色空间yuv420
	//如果这里不配置好，下面deque_buffer只能去申请一个默认宽高的图形缓冲区
    CHECK_EQ(0, native_window_set_buffers_geometry(
                mNativeWindow.get(),
                bufWidth,
                bufHeight,
                halFormat));
	
	
	ANativeWindowBuffer *buf;//描述buffer
	//申请一块空闲的图形缓冲区
    if ((err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(),
            &buf)) != 0) {
        ALOGW("Surface::dequeueBuffer returned error %d", err);
        return;
    }

    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    Rect bounds(mCropWidth, mCropHeight);

    void *dst;
    CHECK_EQ(0, mapper.lock(//用来锁定一个图形缓冲区并将缓冲区映射到用户进程
                buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));//dst就指向图形缓冲区首地址

    if (true){
        size_t dst_y_size = buf->stride * buf->height;
        size_t dst_c_stride = ALIGN(buf->stride / 2, 16);//1行v/u的大小
        size_t dst_c_size = dst_c_stride * buf->height / 2;//u/v的大小
        
        memcpy(dst, data, dst_y_size + dst_c_size*2);//将yuv数据copy到图形缓冲区
    }

    CHECK_EQ(0, mapper.unlock(buf->handle));

    if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf,
            -1)) != 0) {
        ALOGW("Surface::queueBuffer returned error %d", err);
    }
    buf = NULL;
}

static void nativeTest(){
	ALOGE("[%s]%d",__FILE__,__LINE__);
}

static jboolean
nativeSetVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface){
	ALOGE("[%s]%d",__FILE__,__LINE__);
	surface = android_view_Surface_getSurface(env, jsurface);
	if(android::Surface::isValid(surface)){
		ALOGE("surface is valid ");
	}else {
		ALOGE("surface is invalid ");
		return false;
	}
	ALOGE("[%s][%d]\n",__FILE__,__LINE__);
	return true;
}
static void
nativeShowYUV(JNIEnv *env, jobject thiz,jbyteArray yuvData,jint width,jint height){
	ALOGE("width = %d,height = %d",width,height);
	jint len = env->GetArrayLength(yuvData);
	ALOGE("len = %d",len);
	jbyte *byteBuf = env->GetByteArrayElements(yuvData, 0);
	render(byteBuf,len,surface,width,height);
}
static JNINativeMethod gMethods[] = {
    {"nativeTest",       			"()V",    							(void *)nativeTest},
	{"nativeSetVideoSurface",		"(Landroid/view/Surface;)Z", 		(void *)nativeSetVideoSurface},
	{"nativeShowYUV",				"([BII)V",							(void *)nativeShowYUV},
};

static const char* const kClassPathName = "com/example/myyuvviewer/MainActivity";

// This function only registers the native methods
static int register_com_example_myyuvviewer(JNIEnv *env)
{
	ALOGE("[%s]%d",__FILE__,__LINE__);
    return AndroidRuntime::registerNativeMethods(env,
                kClassPathName, gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	ALOGE("[%s]%d",__FILE__,__LINE__);
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);
	ALOGE("[%s]%d",__FILE__,__LINE__);
   if (register_com_example_myyuvviewer(env) < 0) {
        ALOGE("ERROR: MediaPlayer native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
#endif

