#include <media/AudioTrack.h>

#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/ColorConverter.h>
#include <media/stagefright/AudioPlayer.h>

#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/ACodec.h>
#include <media/ICrypto.h>
#include <media/IMediaPlayerService.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/DisplayInfo.h>
#include <android_runtime/android_view_Surface.h>

#include <system/graphics.h>
#include <system/audio.h>

#include <new>
#include <map>

#include <vpu_global.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include <OMX_Component.h>
#include <media/IOMX.h>

extern "C" {
#include "libstagefright.h"
#include "libdvbpsi.h"
#include "receiver.h"

#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>

extern para_process_t para;
extern media_handle_t programe_media;
}

using namespace android;

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

struct Frame {
    status_t status;
    uint32_t size;
    int64_t time;
    uint8_t *buffer;
};

struct TimeStamp {
    int64_t pts;
    int64_t reordered_opaque;
};

struct StagefrightContext {
    int width, height;

    pthread_mutex_t in_mutex, out_mutex;
    pthread_cond_t condition;
    pthread_t decode_thread_id;

    std::map<int64_t, TimeStamp> *ts_map;

    /* MediaCodec ACodec */
    sp<MediaCodec> mACodecDecoder;
    android::sp<android::Surface> surface;
    sp<Surface> mSurfaceTextureClient;        //for ACodec(HARDWARERENDER)
    Vector<sp<ABuffer> > mInBuffers;
    Vector<sp<ABuffer> > mOutBuffers;

    sp<MediaCodec> mACodecAudioDecoder;
    Vector<sp<ABuffer> > mAudioInBuffers;
    Vector<sp<ABuffer> > mAudioOutBuffers;

    sp<ANativeWindow> mVideoNativeWindow; // for OMXCodec(HARDWARERENDER AND SOFTEWARERENDER), ACodec(SOFTWARERENDER)

    /* MediaPlayer OMXCodec */
    OMXClient mClient;
    sp<MediaSource> *decoder;
    sp<MediaSource> *source;

    int fd;
    sp<MediaSource> *mAudioDecoder;
    sp<MediaSource> *mAudioSource;

    int mSampleRate;
    sp<AudioTrack> mAudioTrack;
};

/*function ....*/
static int av_read_frame(struct Frame *frame);
static int audio_read_frame(struct Frame *frame);
static int video_decoder_init(struct StagefrightContext *s, int video_type);
static int audio_decoder_init(StagefrightContext *s, struct audio_config audio_cfg);
static void audio_pcm_play(StagefrightContext *s);
/*function ....*/

/*param ....*/
struct StagefrightContext sCtx_g;
/*param ....*/

class CStageFrightAudioSource;

class CStageFrightAudioSource : public MediaSource {
public:
        CStageFrightAudioSource(StagefrightContext *sCtx, sp<MetaData> meta) {
                s = sCtx;
                source_meta = meta;
                frame_size  = 8192;
                buf_group.add_buffer(new MediaBuffer(frame_size));
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

        virtual status_t read(MediaBuffer **mbuffer, const MediaSource::ReadOptions *options) {
                status_t ret;
                struct Frame *frame = NULL;
static int first_flag = true;
static unsigned char data[2] = {0x11, 0x90};
                if((frame = (struct Frame *)malloc(sizeof(struct Frame))) == NULL) {
                        printf("%s:: Failed to malloc a frame\n", __FUNCTION__);
                        return 1;
                }
if(first_flag)
{
                frame->buffer = data;
                frame->size = 2;
                frame->status = OK;
                frame->time = 3000;
                first_flag = false;
}
else
{
                audio_read_frame(frame);
}
                printf("audio_frame_size is %d,[0x%02x],[0x%02x],[0x%02x],[0x%02x],[0x%02x]\n", frame->size, frame->buffer[0], frame->buffer[1], frame->buffer[2], frame->buffer[3], frame->buffer[4]);
                ret = frame->status;
               
                if (ret == OK) {
                        ret = buf_group.acquire_buffer(mbuffer);
                        if (ret == OK) {
                                memcpy((*mbuffer)->data(), frame->buffer, frame->size);
                                (*mbuffer)->set_range(0, frame->size);
                                (*mbuffer)->meta_data()->clear();
                                (*mbuffer)->meta_data()->setInt64(kKeyTime, frame->time);
                        } else {
                                printf("%s@Failed to acquire MediaBuffer\n", __FUNCTION__);
                        }
                }
                else
                        printf("%s@This frame is error\n", __FUNCTION__);
               
                free(frame);
                frame = NULL;
                return ret;
        }
       
private:
        MediaBufferGroup buf_group;
        sp<MetaData> source_meta;
        StagefrightContext *s;
        int frame_size;
};

class CStageFrightMediaSource;

class CStageFrightMediaSource : public MediaSource {
public:
        CStageFrightMediaSource(StagefrightContext *sCtx, sp<MetaData> meta) {
                s = sCtx;
                source_meta = meta;
                frame_size  = (sCtx->width * sCtx->height * 3) / 2;
                buf_group.add_buffer(new MediaBuffer(frame_size));
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

        virtual status_t read(MediaBuffer **mbuffer, const MediaSource::ReadOptions *options) {
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
                av_read_frame(&frame);
                //printf("%s: status[%d], size[%d], buffer[0x%x][0x%x][0x%x][0x%x][0x%x][0x%x]\n", __FUNCTION__, frame->status, frame->size, \
                        //frame->buffer[0], frame->buffer[1], frame->buffer[2], frame->buffer[3], frame->buffer[4], frame->buffer[5]);

                ret = frame.status;
               
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
                }
                else
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
        StagefrightContext *s;
        int frame_size;
};


//将x规整为y的倍数,也就是将x按y对齐  
static int ALIGN(int x, int y) {  
    // y must be a power of 2.  
    return (x + y - 1) & ~(y - 1);  
}

/** 渲染 **/
static void my_render(const void *data, size_t size, const sp<ANativeWindow> &nativeWindow ,int width,int height, uint64_t timeUs)
{  
        sp<ANativeWindow> mNativeWindow = nativeWindow;  
        int err;

        if(height%16)
                height += (height%16);// DEBUG_HEIGHT --> 1088

        int mCropWidth = width;  
        int mCropHeight = height;
        int halFormat = HAL_PIXEL_FORMAT_YCrCb_NV12;//颜色空间  
        int bufWidth = (mCropWidth + 1) & ~1;//按2对齐  
        int bufHeight = (mCropHeight + 1) & ~1;  

        native_window_set_usage(mNativeWindow.get(), GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN  | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP);  

        native_window_set_scaling_mode(mNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

        // Width must be multiple of 32???  YES!!!
        //很重要,配置宽高和和指定颜色空间yuv420  
        //如果这里不配置好，下面deque_buffer只能去申请一个默认宽高的图形缓冲区  
        native_window_set_buffers_geometry(mNativeWindow.get(), bufWidth, bufHeight, halFormat);  
/*
** runtime start
*/
#if 0
struct timeval tpstart,tpend;
float timeuse;
gettimeofday(&tpstart,NULL);        
#endif

        ANativeWindowBuffer *buf;//描述buffer  
        //申请一块空闲的图形缓冲区  
        if ((err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(), &buf)) != 0) {  
            printf("Surface::dequeueBuffer returned error %d", err);
            return;  
        }
/*
** runtime end
*/
#if 0
gettimeofday(&tpend,NULL);
timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+ tpend.tv_usec-tpstart.tv_usec;
timeuse/=1000000;
printf("%s[%d]Used Time:%f\n", __FUNCTION__, __LINE__, timeuse);
#endif
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();

        Rect bounds(mCropWidth, mCropHeight);  

        void *dst;  
       
        mapper.lock(buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst);

        if (true){  
            size_t dst_y_size = buf->stride * buf->height;  
            size_t dst_c_stride = ALIGN(buf->stride / 2, 16);//1行v/u的大小  
            size_t dst_c_size = dst_c_stride * buf->height / 2;//u/v的大小  
            //printf("[%d][%d][%d]xxxxxxxx[%d][%s]\n", dst_y_size, dst_c_stride, dst_c_size, __LINE__, __FUNCTION__);
            memcpy(dst, data, dst_y_size + dst_c_size*2);//将yuv数据copy到图形缓冲区  
        }

        mapper.unlock(buf->handle);

        native_window_set_buffers_timestamp(mNativeWindow.get(), timeUs * 1000);
        if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf, -1)) != 0) {  
            printf("Surface::queueBuffer returned error %d", err);  
        }

        buf = NULL;  

}  

uint8_t rgb16[DEBUG_WIDTH*DEBUG_HEIGHT*2] = {0};
uint8_t yuv420[DEBUG_WIDTH*DEBUG_HEIGHT*3/2] = {0};
extern sem_t decode_sem;
#include <sys/time.h>
#include <math.h>

int64_t PTS_now = 0;
extern int64_t PCR_base;
extern sem_t display_sem;
static void* video_decode_display_thread(void *arg)
{
        StagefrightContext *s = (StagefrightContext*)arg;
        static bool debug = false;
        printf("[%s]Thread id:%d/n", __FUNCTION__, gettid());
        s->width = para.video.width;
        s->height = para.video.height;
        video_decoder_init(s, para.video.stream_type);
#if 1       
#if (defined OMXCODEC)
        MediaBuffer *buffer = NULL;
        int32_t mWidth, mHeight, mFormat;
        int32_t mCropLeft, mCropTop, mCropRight, mCropBottom;
        int32_t mCropWidth, mCropHeight;
        /*
        for(int i =0; i < sizeof(rgb16); i++)
                rgb16 = 0x80;//red
        */
        //sem_init(&decode_sem, 0, 0);
        //sem_wait(&decode_sem);
        static uint32_t tmp = 0;
        while(1)
        {       
                status_t status = (*s->decoder)->read(&buffer, NULL);
                if (status == OK) {       
                         printf("DECODER OK\n");
                        if (buffer->range_length() == 0)
                        {
                                printf("%s:ERROR_BUFFER_TOO_SMALL\n", __FUNCTION__);
                                status = ERROR_BUFFER_TOO_SMALL;
                                buffer->release();
                                buffer = NULL;
                                continue;
                        }
                        //printf("BUFFER RANGE LENGTH[%d]\n", buffer->range_length());
                 }
                 else
                         printf("%s@NOT OK\n", __FUNCTION__);

                if(status == OK) {
                        sp<MetaData> outFormat = (*s->decoder)->getFormat();
                        outFormat->findInt32(kKeyWidth , &mWidth);
                        outFormat->findInt32(kKeyHeight, &mHeight);
                        outFormat->findInt32(kKeyColorFormat, &mFormat);
                        printf("W[%d], H[%d], COLORFORMAT[0x%08x]\n", mWidth, mHeight, mFormat);
                }

                //VPU_FRAME *frame = (VPU_FRAME *)buffer->data();
                //printf("frame->FrameType[%d], FrameWidth[%d], FrameHeight[%d]\n", frame->FrameType, frame->FrameWidth, frame->FrameHeight);

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
                }
                else {
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
        }
#if 0
        buffer->release();
        buffer = NULL;
        free(frame);
        frame = NULL;
#endif       
#elif (defined ACODEC)
        status_t err;
        static int64_t kTimeout = 10000;
        size_t inIndex;
        size_t outIndex;
        size_t offset;
        size_t size;
        int64_t presentationTimeUs, tmp;
        double now_pts_time, now_pcr_time;
        uint32_t flags;
        while(1)
        {
                err = s->mACodecDecoder->dequeueInputBuffer(&inIndex, kTimeout);
                if (err == OK) {
                        //printf("filling input buffer %d\n", inIndex);
                       
                        const sp<ABuffer> &buffer = s->mInBuffers.itemAt(inIndex);
                       
#if defined(QUEUE_FRAME_DATA)
                        int64_t pts;
                        size_t sampleSize = av_read_one_frame((uint8_t *)buffer->data(), &pts);

                        #if 0
                        //（pts_now -pts_00）/90000获得了当前播放的帧的时间戳，单位是秒
                        presentationTimeUs = (pts-para.pts_timebase)*100/9;// *100/9转换成微妙
                        #else
                        //使用PCR来同步，比较PCR_base和PTS，若相同则显示
                        presentationTimeUs = pts/100*100;//*100/100 忽略低两位，以让PCR_base可以和PTS相等
                        #endif
                        //printf("presentationTimeUs is %lld\n", presentationTimeUs);

#elif defined(CIRCULAR_SAMPLE_DATA)
                        size_t sampleSize = read_sample_data((uint8_t *)buffer->data());
#else
#endif

                        if(sampleSize <= 0)
                                break;

                        if (buffer->capacity() < sampleSize) {
                                printf("buffer capacity overflow\n");
                                break;
                        }
                       
                        buffer->setRange(0, sampleSize);
                        //printf("buffer size is %d\n", buffer->size());
                        //usleep(18000);
                        err = s->mACodecDecoder->queueInputBuffer(
                                inIndex,
                                0 /* offset */,
                                buffer->size(),
                                presentationTimeUs,
                                0 /* flag*/);
                        //printf("queueInputBuffer err is %d\n", err);
                }
                else {
                        //printf("dequeueInputBuffer err is %d\n", err);
                        //continue;
                }

                err = s->mACodecDecoder->dequeueOutputBuffer(&outIndex, &offset, &size, &presentationTimeUs, &flags, kTimeout);
                //printf("dequeueOutputBuffer err is %d\n", err);
                if (err == OK) {
                        PTS_now = presentationTimeUs;
                        //printf("554@#@#@@@PCR_base is %lld, PTS_now is %lld\n", PCR_base, PTS_now);
                        if(PCR_base < PTS_now)
                        {
                                //printf("CR_base[%lld] < PTS_now[%lld]\n", PCR_base, PTS_now);
                                 sem_wait(&display_sem);               
                        }

                        if(USE_SURFACE_ALLOC)
                        {
                                err = s->mACodecDecoder->renderOutputBufferAndRelease(outIndex);
                        } else {
                                s->mACodecDecoder->getOutputBuffers(&s->mOutBuffers);
                                //printf("got %d output buffers", s->mAudioOutBuffers.size());
                                const sp<ABuffer> &buffer = s->mOutBuffers.itemAt(outIndex);
                                //printf("output buffers[%d] size[%d]\n",outIndex, buffer->size());
                                if(debug)
                                {
                                        int fd = open("/mnt/sdcard/Movies/test.nv12", O_WRONLY | O_CREAT);
                                        if(fd < 0) {
                                                printf("store the nv12 data failed\n");
                                                return NULL;
                                        }
                                        int cnts = write(fd, buffer->data(), buffer->size());
                                        printf("write nv12 cnts is %d\n", cnts);
                                        debug = false;
                                }
                                my_render(buffer->data(), buffer->size(), s->mVideoNativeWindow, para.video.width, para.video.height, 0);
                                s->mACodecDecoder->releaseOutputBuffer(outIndex);
                        }
                               
                } else if (err == INFO_OUTPUT_BUFFERS_CHANGED) {
                        //printf("INFO_OUTPUT_BUFFERS_CHANGED");
                        //s->mACodecDecoder->getOutputBuffers(&s->mOutBuffers);
                        //printf("got %d output buffers", s->mOutBuffers.size());
                } else if (err == INFO_FORMAT_CHANGED) {
                        //sp<AMessage> format;
                        //s->mACodecDecoder->getOutputFormat(&format);
                        //printf("INFO_FORMAT_CHANGED: %s", format->debugString().c_str());
                } else {
                        ;//
                }
        }

#endif
#endif
        return NULL;
}

extern thread_pool_t thread_desc;
void video_decoder_display_start(void)
{
        pthread_t handle_video_decoder_display;
        if (pthread_create(&handle_video_decoder_display, NULL, video_decode_display_thread, (void *)&sCtx_g) < 0)
        {
            printf("failed creating thread video_decode_display_thread\n");
            exit(EXIT_FAILURE);
        }
        pthread_detach(handle_video_decoder_display);
        thread_desc.handle_video_decoder_display = handle_video_decoder_display;
        printf("create video decoder display thread over!\n");
}

#define ENCODING_PCM_16BIT 2
static void* audio_decode_sound_thread(void *arg)
{
        status_t err;
        StagefrightContext *s = (StagefrightContext*)arg;
        MediaBuffer *buffer = NULL;
        printf("[%s]Thread id:%d/n", __FUNCTION__, gettid());

        if(audio_decoder_init(s, para.audio) == -1)
                return NULL;

        size_t frameCount = 0;
        if (AudioTrack::getMinFrameCount(&frameCount, AUDIO_STREAM_DEFAULT, s->mSampleRate) != NO_ERROR) {
            return NULL;
        }
       
        int nbChannels = 2;
        int audioFormat = ENCODING_PCM_16BIT;
        size_t size =  frameCount * nbChannels * (audioFormat == ENCODING_PCM_16BIT ? 2 : 1);
        printf("size is %d, s->mSampleRate is %d\n", size, s->mSampleRate);

        s->mAudioTrack = new AudioTrack(AUDIO_STREAM_MUSIC,
                                                                        s->mSampleRate,
                                                                        AUDIO_FORMAT_PCM_16_BIT,
                                                                        AUDIO_CHANNEL_OUT_STEREO,
                                                                        0,
                                                                        AUDIO_OUTPUT_FLAG_NONE,
                                                                        NULL,
                                                                        NULL,
                                                                        0);
        if ((err = s->mAudioTrack->initCheck()) != OK) {
                printf("AudioTrack initCheck failed\n");
                s->mAudioTrack.clear();
        }
        s->mAudioTrack->setVolume(1.0f);
        s->mAudioTrack->start();
#if 1
#if (defined OMXCODEC)
        while(1)
        {
                status_t status = (*s->mAudioDecoder)->read(&buffer, NULL);

                if (status == OK) {       
                        printf("%s@AUDIO DECODER OK\n", __FUNCTION__);
                        if (buffer->range_length() == 0)
                        {
                                printf("%s:ERROR_BUFFER_TOO_SMALL\n", __FUNCTION__);
                                status = ERROR_BUFFER_TOO_SMALL;
                                buffer->release();
                                buffer = NULL;
                                continue;
                        }
                        //printf("BUFFER RANGE LENGTH[%d]\n", buffer->range_length());
                }
                else
                        ;//printf("%s@AUDIO DECODER NOT OK\n", __FUNCTION__);

                if(status == OK) {
                        sp<MetaData> outFormat = (*s->mAudioDecoder)->getFormat();
                        outFormat->findInt32(kKeySampleRate, &s->mSampleRate);
                        printf("SAMPLERATE[%d]\n", s->mSampleRate);
                }

                if (status == OK) {       
                        s->mAudioTrack->write(buffer->data(), buffer->range_length());
                        buffer->release();
                        buffer = NULL;
                }

        }
#elif (defined ACODEC)
        static int first_flag = true;
        int sampleSize;
        static int64_t kTimeout_audio = 10000;
        size_t inIndex;
        size_t outIndex;
        size_t offset;
        size_t len;
        int64_t presentationTimeUs;
        uint32_t flags;
       
        while(1)
        {
                err = s->mACodecAudioDecoder->dequeueInputBuffer(&inIndex, kTimeout_audio);
                if (err == OK) {
                        //printf("filling input buffer %d\n", inIndex);
                       
                        const sp<ABuffer> &buffer = s->mAudioInBuffers.itemAt(inIndex);

if((para.audio.stream_type == AUDIO_AAC_ADTS) && first_flag)
{
                memcpy((uint8_t *)buffer->data(), para.audio.csd, 2);
                sampleSize = 2;
                first_flag = false;
}
else
{
                        sampleSize = audio_read_one_frame((uint8_t *)buffer->data());
}
                        presentationTimeUs = 0;
                        if(sampleSize <= 0)
                                break;

                        if (buffer->capacity() < sampleSize) {
                                printf("buffer capacity overflow\n");
                                break;
                        }
                       
                        buffer->setRange(0, sampleSize);
                       
                        err = s->mACodecAudioDecoder->queueInputBuffer(
                                inIndex,
                                0 /* offset */,
                                buffer->size(),
                                presentationTimeUs,
                                0 /* flag*/);
                        //printf("queueInputBuffer err is %d\n", err);
                }

                err = s->mACodecAudioDecoder->dequeueOutputBuffer(&outIndex, &offset, &len, &presentationTimeUs, &flags, kTimeout_audio);
                //printf("dequeueOutputBuffer err is %d\n", err);
                if (err == OK) {
                                s->mACodecAudioDecoder->getOutputBuffers(&s->mAudioOutBuffers);
                                //printf("got %d output buffers", s->mAudioOutBuffers.size());
                                const sp<ABuffer> &buffer = s->mAudioOutBuffers.itemAt(outIndex);
                                //printf("output buffers[%d] size[%d]\n",outIndex, buffer->size());
                                s->mAudioTrack->write(buffer->data(), buffer->size());
                                s->mACodecAudioDecoder->releaseOutputBuffer(outIndex);
                }
        }
#endif
#endif
}

void audio_decode_sound_start(void)
{
        pthread_t handle_audio_decode_sound;
        if (pthread_create(&handle_audio_decode_sound, NULL, audio_decode_sound_thread, (void *)&sCtx_g) < 0)
        {
            printf("failed creating thread handle_audio_decode_sound\n");
            exit(EXIT_FAILURE);
        }
        pthread_detach(handle_audio_decode_sound);
        thread_desc.handle_audio_decode_sound = handle_audio_decode_sound;
        printf("create audio decoder sound thread over!\n");
}

static int renderer_init(StagefrightContext *sCtx, JNIEnv* env, jobject surface)
{
        sCtx->mVideoNativeWindow = ANativeWindow_fromSurface(env, surface);
        if(sCtx->mVideoNativeWindow == NULL) {
                printf("Failed to get a ANativeWindow from Surface\n");
                return -1;
        }
        else
                printf("sCtx->mVideoNativeWindow[OK]\n");
       
        if(USE_SURFACE_ALLOC) {
                //ANativeWindow_setBuffersGeometry(sCtx->mVideoNativeWindow.get(), DEBUG_WIDTH, DEBUG_HEIGHT, WINDOW_FORMAT_RGB_565);
                // it is necessary
                //native_window_set_scaling_mode(sCtx->mVideoNativeWindow.get(), NATIVE_WINDOW_SCALING_MODE_SCALE_CROP);
        }
        return 0;
}

static int OMXCodec_video_decoder_init(StagefrightContext *s, int video_type)
{
        int ret = 0;
        sp<MetaData> meta;
        const char* mimetype;

        meta = new MetaData;
        if (meta == NULL) {
                printf("cannot allocate MetaData\n");
                return -1;
        }
        if(video_type == VIDEO_AVC)
        {
                mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
        }
        else if(video_type == VIDEO_HEVC)
        {
                mimetype = MEDIA_MIMETYPE_VIDEO_HEVC;
        }

        meta->setCString(kKeyMIMEType, mimetype);
        meta->setInt32(kKeyWidth, s->width);
        meta->setInt32(kKeyHeight, s->height);
       
        s->source    = new sp<MediaSource>();

        *s->source   = new CStageFrightMediaSource(s, meta);
       
        if (s->source == NULL) {
                s->source = NULL;
                printf("Cannot obtain source / mClient");
                return -1;
        }

        if (s->mClient.connect() !=  OK) {
                printf("Cannot connect OMX mClient\n");
                ret = -1;
                goto fail;
            }
        printf("Connect OMX mClient success\n");
       
        s->decoder  = new sp<MediaSource>();
        printf("[%s]@OMXCodec::Create____________________________START###\n", __FUNCTION__);
        *s->decoder = OMXCodec::Create(s->mClient.interface(),
                                    meta,
                                    false,
                                    *s->source,
                                    NULL,
                                    OMXCodec::kHardwareCodecsOnly | OMXCodec::kClientNeedsFramebuffer,
                                    USE_SURFACE_ALLOC ? s->mVideoNativeWindow : NULL);
        if (!(s->decoder != NULL && (*s->decoder)->start() ==  OK)) {
                printf("Cannot start decoder\n");
                ret = -1;
                s->mClient.disconnect();
                s->source = NULL;
                s->decoder = NULL;
                goto fail;
        }
        printf("[%s]@OMXCodec::Create____________________________END\n", __FUNCTION__);
fail:
        return ret;
}


static android::Surface* getNativeSurface(JNIEnv* env, jobject jsurface)
{
    jclass clazz = env->FindClass("android/view/Surface");
    jfieldID field_surface;

    field_surface = env->GetFieldID(clazz, "mNativeObject"/*ANDROID_VIEW_SURFACE_JNI_ID*/, "I");

    if (field_surface == NULL)
    {
        printf("failed to get field_surface\n");
        return NULL;
    }
    return (android::Surface *) env->GetIntField(jsurface, field_surface);
}

static int ACodec_video_decoder_init(StagefrightContext *s, /*JNIEnv* env, jobject jsurface, */int video_type)
{
        int ret = 0;
        status_t err;
        const char* mimetype;
#if 0       
        if(USE_SURFACE_ALLOC)
        {
                #ifndef USE_JAVA_SURFACE
                        //成功!
                        #if 0
                        //失败!(ANDROID_VIEW_SURFACE_JNI_ID宏找不到头文件，mNativeObject获取到的不是SURFACE)参照网上获取JAVA层SURFACE方法
                        s->surface = getNativeSurface(env, jsurface);
                        if(s->surface == NULL)
                        {
                                printf("the surface is null\n");       
                                return -1;
                        }
                        #else
                        //成功!参考MediaCodec.java+android_media_MediaCodec.cpp
                        sp<IGraphicBufferProducer> bufferProducer;
                        if (jsurface != NULL) {
                                sp<Surface> surface = android_view_Surface_getSurface(env, jsurface);
                                if (surface != NULL) {
                                        bufferProducer = surface->getIGraphicBufferProducer();
                                } else {
                                        printf("the surface is null\n");
                                        return -1;
                                }
                        }

                        if (bufferProducer != NULL) {
                                s->mSurfaceTextureClient = new Surface(bufferProducer, true /* controlledByApp */);
                        } else {
                                printf("the bufferProducer is null\n");
                                s->mSurfaceTextureClient.clear();
                        }

                        if(s->mSurfaceTextureClient == NULL)
                        {
                                printf("Failed to create mSurfaceTextureClient\n");
                                return -1;
                        }
                        #endif
                #else
                        //失败!(在C++层创建SURFACE)参照frameworks/av/cmds/stagefright/Codec.cpp
                        sp<SurfaceComposerClient> composerClient;
                        sp<SurfaceControl> control;

                        composerClient = new SurfaceComposerClient;
                        err = composerClient->initCheck();
                        if(err != OK)
                        {
                                printf("Failed to initCheck composerClient\n");
                                return -1;
                        }
                       
                        sp<IBinder> display(SurfaceComposerClient::getBuiltInDisplay(
                        ISurfaceComposer::eDisplayIdMain));
                        DisplayInfo info;
                        SurfaceComposerClient::getDisplayInfo(display, &info);
                        ssize_t displayWidth = info.w;
                        ssize_t displayHeight = info.h;

                        printf("display is %d x %d\n", displayWidth, displayHeight);

                        control = composerClient->createSurface(
                                String8("A Surface"),
                                displayWidth,
                                displayHeight,
                                PIXEL_FORMAT_RGB_565,
                                0);
                        if(control == NULL)
                        {
                                printf("Failed to createSurface composerClient\n");
                                return -1;
                        }
                       
                        if(!control->isValid())
                        {
                                printf("SurfaceControl is not valid\n");
                                return -1;
                        }                       

                        SurfaceComposerClient:penGlobalTransaction();
                        err = control->setLayer(INT_MAX);
                        if(err != OK)
                        {
                                printf("Failed to setLayer SurfaceControl\n");
                                return -1;
                        }
                       
                        err = control->show();
                        if(err != OK)
                        {
                                printf("Failed to show SurfaceControl\n");
                                return -1;
                        }
                        SurfaceComposerClient::closeGlobalTransaction();

                        s->surface = control->getSurface();
                        if(s->surface == NULL)
                        {
                                printf("the surface is null\n");       
                                return -1;
                        }
                #endif
        }
#endif       
        if(video_type == VIDEO_AVC)
        {
                mimetype = MEDIA_MIMETYPE_VIDEO_AVC;
        }
        else if(video_type == VIDEO_HEVC)
        {
                mimetype = MEDIA_MIMETYPE_VIDEO_HEVC;
        }
       
        bool isAudio = !strncasecmp(mimetype, "audio/", 6);
        bool isVideo = !strncasecmp(mimetype, "video/", 6);

        sp<AMessage> format;
        format = new AMessage;
        if(format == NULL) {
                printf("cannot allocate format\n");
                return -1;
        }

        format->setString("mime", mimetype);
        format->setInt32("width", s->width);
        format->setInt32("height", s->height);

        printf("[%s]@ACodec::Create____________________________START\n", __FUNCTION__);

        sp<ALooper> mLooper = new ALooper;
        mLooper->setName("MediaCodec_looper");
        mLooper->start(
                false,      // runOnCallingThread
                false,       // canCallJava
                PRIORITY_FOREGROUND);

        s->mACodecDecoder = MediaCodec::CreateByType(
                mLooper, mimetype, false /* encoder */);
        if(s->mACodecDecoder == NULL)
        {
                printf("Failed to create mACodecDecoder\n");
                return -1;
        }

        err = s->mACodecDecoder->configure(
                format, USE_SURFACE_ALLOC ? s->mSurfaceTextureClient : NULL,
                NULL /* crypto */,
                0 /* flags */);
        if(err != OK)
        {
                printf("Failed to configure mACodecDecoder\n");
                return -1;
        }

        printf("[%s]@ACodec::Create____________________________END\n", __FUNCTION__);
       
        err  = s->mACodecDecoder->start();
        if(err != OK)
        {
                printf("Failed to start mACodecDecoder\n");
                return -1;
        }
       
        err = s->mACodecDecoder->getInputBuffers(&s->mInBuffers);
        if(err != OK)
        {
                printf("Failed to getInputBuffers mACodecDecoder\n");
                return -1;
        }

        err = s->mACodecDecoder->getOutputBuffers(&s->mOutBuffers);
        if(err != OK)
        {
                printf("Failed to getOutputBuffers mACodecDecoder\n");
                return -1;
        }

        printf("got %d input and %d output buffers", s->mInBuffers.size(), s->mOutBuffers.size());
fail:
        return ret;
}

static int audio_decoder_init(StagefrightContext *s, struct audio_config audio_cfg)
{
#ifdef AUDIO_DECODER
//#define PCM_FILE_PLAY_DEBUG
#ifdef PCM_FILE_PLAY_DEBUG
        audio_pcm_play(s);
        return 0;
#endif

        int ret = 0;
        sp<MetaData> meta;
        const char* mimetype;
        int32_t channel_configuration;
       
        printf("%s_%d\n", __FUNCTION__,  __LINE__);
#if (defined OMXCODEC)
        meta = new MetaData;
        if (meta == NULL) {
                printf("cannot allocate MetaData");
                return -1;
        }

        if(audio_cfg.stream_type == AUDIO_MP3)
        {
                mimetype = MEDIA_MIMETYPE_AUDIO_MPEG;
                meta->setCString(kKeyMIMEType, mimetype);
        }
        else if(audio_cfg.stream_type == AUDIO_AAC_ADTS)
        {
                mimetype = MEDIA_MIMETYPE_AUDIO_AAC;
                meta->setCString(kKeyMIMEType, mimetype);
                meta->setInt32(kKeyIsADTS, 1);
                meta->setInt32(kKeyAACProfile, 0x0002);
        }
       
        s->mSampleRate = audio_cfg.sampling_frequency;
        channel_configuration = audio_cfg.channel_configuration;

        meta->setInt32(kKeySampleRate, s->mSampleRate);
        meta->setInt32(kKeyChannelCount, channel_configuration);

        s->mAudioSource = new sp<MediaSource>();
        *s->mAudioSource  = new CStageFrightAudioSource(s, meta);

        if (s->mAudioSource == NULL) {
                s->mAudioSource = NULL;
                printf("Cannot obtain source / mClient");
                return -1;
        }

        if (s->mClient.connect() !=  OK) {
                printf("Cannot connect OMX mClient\n");
                ret = -1;
                goto fail;
            }

        s->mAudioDecoder= new sp<MediaSource>();
        printf("[%s]@OMXCodec::Create____________________________START\n", __FUNCTION__);
        *s->mAudioDecoder = OMXCodec::Create(s->mClient.interface(),
                meta,
                false,
                *s->mAudioSource,
                NULL,
                OMXCodec::kSoftwareCodecsOnly,
                NULL);
        if (!(s->mAudioDecoder != NULL && (*s->mAudioDecoder)->start() ==  OK)) {
                printf("[%s]@Cannot start decoder\n", __FUNCTION__);
                ret = -1;
                s->mClient.disconnect();
                s->mAudioSource = NULL;
                s->mAudioDecoder = NULL;
                goto fail;
        }
        printf("[%s]@OMXCodec::Create____________________________END\n", __FUNCTION__);
fail:
        return ret;
       
#elif (defined ACODEC)
        sp<AMessage> format;
        format = new AMessage;
        if(format == NULL) {
                printf("cannot allocate format\n");
                return -1;
        }
       
        if(audio_cfg.stream_type == AUDIO_MP3)
        {
                mimetype = MEDIA_MIMETYPE_AUDIO_MPEG;
                format->setString("mime", mimetype);
                s->mSampleRate = audio_cfg.sampling_frequency;
        }
        else if(audio_cfg.stream_type == AUDIO_AAC_ADTS)
        {
                mimetype = MEDIA_MIMETYPE_AUDIO_AAC;
                format->setString("mime", mimetype);
                s->mSampleRate = audio_cfg.sampling_frequency/2;
                format->setInt32("is-adts", 1);
                format->setInt32("aac-profile", 0x0002);
        }
       
       
        channel_configuration = audio_cfg.channel_configuration;

        format->setInt32("sample-rate", s->mSampleRate);
        format->setInt32("channel-count", channel_configuration);

        printf("[%s]@ACodec::Create____________________________START\n", __FUNCTION__);
       
        sp<ALooper> mLooper = new ALooper;
        mLooper->setName("MediaCodec_Adio_looper");
        mLooper->start(
                false,      // runOnCallingThread
                false,       // canCallJava
                PRIORITY_FOREGROUND);
        s->mACodecAudioDecoder = MediaCodec::CreateByType(
                mLooper, mimetype, false /* encoder */);
        if(s->mACodecAudioDecoder == NULL)
        {
                printf("Failed to create mACodecAudioDecoder\n");
                return -1;
        }

        ret = s->mACodecAudioDecoder->configure(
                format, NULL /* surface */,
                NULL /* crypto */,
                0 /* flags */);
        if(ret != OK)
        {
                printf("Failed to configure mACodecAudioDecoder\n");
                return -1;
        }

        printf("[%s]@ACodec::Create____________________________END\n", __FUNCTION__);
       
        ret  = s->mACodecAudioDecoder->start();
        if(ret != OK)
        {
                printf("Failed to start mACodecAudioDecoder\n");
                return -1;
        }
       
        ret = s->mACodecAudioDecoder->getInputBuffers(&s->mAudioInBuffers);
        if(ret != OK)
        {
                printf("Failed to getInputBuffers mACodecAudioDecoder\n");
                return -1;
        }

        ret = s->mACodecAudioDecoder->getOutputBuffers(&s->mAudioOutBuffers);
        if(ret != OK)
        {
                printf("Failed to getOutputBuffers mACodecAudioDecoder\n");
                return -1;
        }

        printf("got %d input and %d output buffers", s->mAudioInBuffers.size(), s->mAudioOutBuffers.size());
fail:
        return ret;

#endif

#endif       
}

uint8_t pcm16[4096] = {0};
static void audio_pcm_play(StagefrightContext *s)
{
        status_t err;
        s->mSampleRate = 44100;//44100;
       
        size_t frameCount = 0;
        if (AudioTrack::getMinFrameCount(&frameCount, AUDIO_STREAM_DEFAULT, s->mSampleRate) != NO_ERROR) {
            return ;
        }
       
        int nbChannels = 2;
        int audioFormat = ENCODING_PCM_16BIT;
        size_t size =  frameCount * nbChannels * (audioFormat == ENCODING_PCM_16BIT ? 2 : 1);
        printf("size is %d, s->mSampleRate is %d\n", size, s->mSampleRate);

        s->mAudioTrack = new AudioTrack(AUDIO_STREAM_MUSIC,
                                                                        s->mSampleRate,
                                                                        AUDIO_FORMAT_PCM_16_BIT,
                                                                        AUDIO_CHANNEL_OUT_STEREO,
                                                                        0,
                                                                        AUDIO_OUTPUT_FLAG_NONE,
                                                                        NULL,
                                                                        NULL,
                                                                        0);
        if ((err = s->mAudioTrack->initCheck()) != OK) {
                printf("AudioTrack initCheck failed\n");
                s->mAudioTrack.clear();
        }
        s->mAudioTrack->setVolume(1.0f);
        s->mAudioTrack->start();

        int fd = open("/mnt/sdcard/Music/zxy.pcm", O_RDWR);
        if(fd < 0) {
                printf("store the pcm data failed\n");
                s->mAudioTrack->stop();
                return ;
        }
        while(1)
        {
                int cnts = read(fd, pcm16, sizeof(pcm16));
                printf("read test.pcm size is %d\n", cnts);
                if(cnts <= 0)
                {
                        printf("pcm file is end\n");
                        close(fd);
                        s->mAudioTrack->stop();
                        break;
                }
                s->mAudioTrack->write(pcm16, cnts);
        }
}

static int video_decoder_init(struct StagefrightContext *s, int video_type)
{
        int ret = 0;
       
#ifdef VIDEO_DECODER
#if (defined OMXCODEC)
//#define NV12_FILE_DISPLAY_DEBUG       
#ifdef NV12_FILE_DISPLAY_DEBUG
        int fd = open("/mnt/sdcard/Movies/test.nv12", O_RDWR);
        if(fd < 0) {
                printf("store the nv12 data failed\n");
                return -1;
        }
        int cnts = read(fd, yuv420, sizeof(yuv420));
        printf("read test.nv12 size is %d\n", cnts);
        close(fd);
        debug = false;
        my_render(yuv420, sizeof(yuv420), s->mVideoNativeWindow, DEBUG_WIDTH, DEBUG_HEIGHT, 0);
        return 0;
#endif

        ret = OMXCodec_video_decoder_init(s, video_type);// 阻塞式调用OMX IL组件
        if(ret < 0)
                return -1;
#elif (defined ACODEC)
        ret = ACodec_video_decoder_init(s, /*env, surface,*/ video_type);//非阻塞调用OMX IL组件
        if(ret < 0)
                return -1;
#endif
#endif
        return ret;
}

int stagefright_init(JNIEnv* env, jobject surface)
{
        int ret = 0;
        sp<MetaData> meta;
        uint8_t *extradata;
        int extradata_size;
        const char* mimetype;
       
        /*
        StagefrightContext *s = (struct StagefrightContext *)malloc(sizeof(struct StagefrightContext));
        if(s == NULL) {
                printf("Cannot allocate StagefrightContext\n");
                return -1;
        }
        */
        printf("%s:_______________start\n", __FUNCTION__);
       
        struct StagefrightContext *s = &sCtx_g;

        /*** INIT NATIVE WINDOW ***/
        ret = renderer_init(s, env, surface);
        if(ret < 0)
                return -1;

        return ret;
}


/** this function is taken from the h264bitstream library written by Alex Izvorski and Alex Giladi
Find the beginning and end of a NAL (Network Abstraction Layer) unit in a byte buffer containing H264 bitstream data.
@param[in]   buf        the buffer
@param[in]   size       the size of the buffer
@param[out]  nal_start  the beginning offset of the nal
@param[out]  nal_end    the end offset of the nal
@return                 the length of the nal, or 0 if did not find start of nal, or -1 if did not find end of nal
*/
static int find_nal_unit(uint8_t* buf, int size, int* nal_start, int* nal_end)
{
    int i;
    // find start
    *nal_start = 0;
    *nal_end = 0;
   
    i = 0;
    while (   //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
           (buf != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) &&
           (buf != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01)
           )
    {
        i++; // skip leading zero
        if (i+4 >= size) { return 0; } // did not find nal start
    }
   
    if  (buf != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) // ( next_bits( 24 ) != 0x000001 )
    {
        i++;
    }
   
    if  (buf != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) { /* error, should never happen */ return 0; }
    i+= 3;
    *nal_start = i;
   
    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
           (buf != 0 || buf[i+1] != 0 || buf[i+2] != 0) &&
           (buf != 0 || buf[i+1] != 0 || buf[i+2] != 0x01)
           )
    {
        i++;
        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
        if (i+3 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first
    }
   
    *nal_end = i;
    return (*nal_end - *nal_start);
}

#if defined(CIRCULAR_SAMPLE_DATA)
extern sample_data_t *sample_data;
extern pthread_mutex_t nalu_read_lock;
extern pthread_cond_t  nalu_fifo_empty_cond;
#define SAMPLE_DATA_SIZE (3*1024*1024)
static uint8_t fbuf_S[SAMPLE_DATA_SIZE] = {0};
static uint32_t fstart_S = 0, fend_S = 0;
static int av_read_frame(struct Frame *frame)
{
    int ustart, uend;
    int cstart, cend;
    int found;
    int rsz;
    int frame_len;
    uint8_t nal_unit_type;
    static bool flag = true;
    static unsigned int frame_num = 0;
    static bool debug = false;
    //printf("%s:________, start read!\n", __FUNCTION__);
   
start:
    if(fstart_S > SAMPLE_DATA_SIZE/12 || flag == true)
    {
        //printf("FILL THE SAMPLE CIRCULAR,  in[%d], out[%d]\n", sample_data->fifo[0].in, sample_data->fifo[0].out);
        memcpy(fbuf_S, fbuf_S+fstart_S, fend_S-fstart_S);
        fend_S -= fstart_S;
        fstart_S = 0;

        //printf("%s: fend_S is %d\n", __FUNCTION__, fend_S);

        pthread_mutex_lock(&nalu_read_lock);

        while(!can_circular_buf_read(&sample_data->fifo[0]))
            pthread_cond_wait(&nalu_fifo_empty_cond, &nalu_read_lock);
        //printf("in[%d],out[%d]\n", sample_data->fifo[0].in, sample_data->fifo[0].out);

        rsz = read_circular_buf(&sample_data->fifo[0], fbuf_S+fend_S, SAMPLE_DATA_SIZE/6-fend_S+fstart_S);

        //if(rsz > 0)
            //pthread_cond_signal(&nalu_fifo_empty_cond);
        
        pthread_mutex_unlock(&nalu_read_lock);
        //printf("%s:________read circular buf size is %d\n", __FUNCTION__, rsz);
        if(rsz > 0)
        {
            fend_S += rsz;
        }
        flag = false;
    }
   
    cstart = cend = -1;
    found = 0;
    int uend_tmp = 0;
    while(find_nal_unit(fbuf_S+fstart_S, fend_S-fstart_S, &ustart, &uend) >0)
    {
        nal_unit_type = fbuf_S[fstart_S+ustart] & 0x1f;
        if(nal_unit_type == (uint8_t)6 || nal_unit_type == (uint8_t)7 || nal_unit_type == (uint8_t)8)
        {
            //printf("%s: FIND VIDEO SPS PPS DATA\n", __FUNCTION__);
            //SEI , SPS , PPS
            if(!found)
            {
                found = 1;
                cstart = fstart_S+ustart-3;
                if(cstart >0 && !fbuf_S[cstart-1])
                    cstart--;
            }
        }
        else if(nal_unit_type == (uint8_t)1 || nal_unit_type == (uint8_t)5)
        {
            //printf("%s: FIND VIDEO ES DATA\n", __FUNCTION__);
            if(!found)
            {
                cstart = fstart_S+ustart-3;
                if(cstart > 0 && !fbuf_S[cstart-1])
                    cstart--;
            }
            cend = fstart_S+uend;
            break;
        }
        fstart_S += uend;
        uend_tmp += uend;
    }
   
   if(cend == -1)
    {
        //printf("the zone data is not enough![%d][%d][%d][%d]\n", uend_tmp, fend_S, fstart_S, fend_S-fstart_S);
        fstart_S-=uend_tmp;
        flag = true;
        goto start;
   }
           
        int offset = fstart_S;
        frame_len = cend-cstart; // CEND一帧的结束位置，CSTART一帧SPS/PPS/SEI起始地址，OFFSET一帧的开始位置不包含SPS/PPS/SEI

        if(frame_len > 0)
        {
                //printf("FRAME NUM[%d], Len[%d], fstart_S[%d], fend_S[%d], cstart[%d], cend[%d]\n", frame_num++, frame_len, fstart_S, fend_S, cstart, cend);
                if(debug) {
                        for(int i = 0; i < 6; i++)
                        printf("[0x%x]-", fbuf_S[offset+i]);
                        debug = false;
                }
               
                //memcpy(frame->buffer, fbuf_S+cstart, frame_len);
                frame->buffer = fbuf_S+cstart;
                frame->time = (++frame_num)*3000;
                frame->status = OK;
                frame->size = frame_len;
        }
        else
        {
                frame->status = 1;
                frame->size = 0;
        }

        fstart_S = cend;
        return frame->size;
}
#elif defined(QUEUE_FRAME_DATA)
static int LIBSTAGEFRIGH_get_bottom_field_pos(uint8_t *p_data, size_t len, int *start)
{
    int ustart, uend;
    int cstart, cend;
    int found;
    int fstart;
    uint8_t nal_unit_type;
   
    fstart = 0;
    cstart = cend = -1;
    found = 0;
    bool aud_flag = false;

    while(find_nal_unit(p_data+fstart, len-fstart, &ustart, &uend) >0)
    {
        //printf("ustart[%d], uend[%d]\n", ustart, uend);
        nal_unit_type = p_data[fstart+ustart] & 0x1f;
            
        if(nal_unit_type == (uint8_t)6 || nal_unit_type == (uint8_t)7 || nal_unit_type == (uint8_t)8 || nal_unit_type == (uint8_t)1 || nal_unit_type == (uint8_t)5)
        {
            //SEI , SPS , PPS, ES
            if(!found)
            {
                found = 1;
                cstart = fstart+ustart-3;
                if(cstart >0 && !p_data[cstart-1])
                    cstart--;
            }
            aud_flag = true;    //真正的AUD
        }
        else if(nal_unit_type == (uint8_t)9)
        {
            // AUD
           if(aud_flag)         //如果数据起始就是AUD，则跳过这个AUD
            {
                if(!found)
                {
                    cstart = fstart+ustart-3;
                    if(cstart > 0 && !p_data[cstart-1])
                        cstart--;
                }
                cend = fstart+uend;
                break;
            }
        }
        //printf("CSTART[%d] CEND [%d]\n", cstart, cend);
        fstart += uend;
        //printf("fstart[0x%x]\n", fstart);
    }
    *start = cstart;
    return cend-cstart;
}
uint8_t frame_buff_cpp[128*1024] = {0};
uint32_t frame_offset_cpp = 0;
static int av_read_frame(struct Frame *frame)
{
        int data_fifo_count = 0;
        int offset;
        static bool field_flag = false;
        char field_count = 0;
        int frame_len = 0, cstart = 0;
        buffer_t *buffer = NULL;
        struct frame_queue *p_frame_queue = &programe_media.video.frame_queue;
        static bool debug = false;
        static unsigned int frame_num = 0;               
start:
/*       
    data_fifo_count = fifo_count(p_frame_queue->fifo);

        if(data_fifo_count >= 30)
        {
            printf("discard %d video frame\n",data_fifo_count);
            while((buffer = fifo_pop_nowait(p_frame_queue->fifo)) != NULL)
            {
                buffer_free(buffer);
                buffer = NULL;               
            }               

            if(p_frame_queue->b_fifo_full == true)
            {
                pthread_mutex_lock(&p_frame_queue->lock);
                p_frame_queue->b_fifo_full = false;
                pthread_cond_signal(&p_frame_queue->fifo_full);
                pthread_mutex_unlock(&p_frame_queue->lock);                               
            }
        }
*/
        if(!field_flag)
        {
                /* Wait for data to arrive */
                buffer = fifo_pop(p_frame_queue->fifo);
                if (buffer == NULL)
                {
                    printf("fifo_pop data null\n");
                    goto start;
                }

                if(para.video.frame_field_mode == INTERLACED_MODE)
                {      
                        offset = LIBSTAGEFRIGH_get_bottom_field_pos(buffer->p_data,buffer->i_data_size, &cstart);
                        if(offset == -1)
                        {
                                printf("I can't find bottom field data len :%d\n",buffer->i_data_size);
                                goto ret_point;
                        }

                        frame_len = offset;

                        frame->buffer = buffer->p_data+cstart;
                        frame->time = (++frame_num)*1;
                        frame->status = OK;
                        frame->size = frame_len;       
                        //printf("frame_len_top[%d]\n", frame_len);

                        /*---bottom data---*/
                        frame_offset_cpp = buffer->i_data_size - offset  - cstart;
                       
                        memcpy(frame_buff_cpp, buffer->p_data + cstart + offset, frame_offset_cpp);

                        field_flag = true;
                }
                else
                {
                        frame_len = buffer->i_data_size;
                        frame->buffer = buffer->p_data;
                        frame->time = (++frame_num)*3000;
                        frame->status = OK;
                        frame->size = frame_len;
                        //printf("frame_len[%d]\n", frame_len);
                }
               
                if(debug) {
                    printf("frame_len[%d]\n", frame_len);
                    for(int i = 0; i < 20; i++)  {
                        if(i%16 == 0)
                            printf("\n");
                        printf("[0x%x]-", frame->buffer);
                    }
                    debug = false;
                }

/* reuse buffer */
ret_point:               
                fifo_push(p_frame_queue->empty, buffer);
                buffer = NULL;

                /* check fifo size */
                if(p_frame_queue->b_fifo_full == true)
                {
                    if (fifo_size(p_frame_queue->fifo) < FIFO_VIDEO_THRESHOLD_SIZE_HALF)
                    {
                        pthread_mutex_lock(&p_frame_queue->lock);
                        p_frame_queue->b_fifo_full = false;
                        pthread_cond_signal(&p_frame_queue->fifo_full);
                        pthread_mutex_unlock(&p_frame_queue->lock);
                    }
                }

                return frame_len;
        }
        else
        {
                frame->buffer = frame_buff_cpp;
                frame->time = (++frame_num)*3000;
                frame->status = OK;
                frame->size = frame_offset_cpp;       
                field_flag = false;
                //printf("frame_len_bottom[%d]\n", frame_offset_cpp);
                return frame_offset_cpp;
        }
}
#endif

/*
LyaerI使用公式：
帧长度（字节） = (( 每帧采样数/ 8 * 比特率 ) / 采样频率 ) + 填充 * 4

LyerII和LyaerIII使用公式：
帧长度（字节）= (( 每帧采样数/ 8 * 比特率 ) / 采样频率 ) + 填充

例：LayerIII 比特率 128000，采样频率 44100，填充0  =〉帧大小 417字节
比特率为128K，采样率为44.1K，填充0，则其帧长度为：
(1152 / 8 * 128K)/44.1K + 0 = 417 (字节)
*/

#define BITRATE                                        320000
#define SAMPLE_RATE                                44100
#define FILL                                                1
#define MP3_AUDIO_FRAME_SIZE        (((1152/8*BITRATE)/SAMPLE_RATE)+FILL)

#define AAC_AUDIO_FRAME_SIZE        1045
uint8_t audio[AAC_AUDIO_FRAME_SIZE] = {0};
extern sample_data_t *sample_data;
extern pthread_mutex_t audio_read_lock;
extern pthread_cond_t  audio_fifo_empty_cond;
static int audio_read_frame(struct Frame *frame)
{
        static int frame_num = 0;
        #if 0
        int cnts = read(sCtx_g.fd, audio, sizeof(audio));
        //printf("read audio size is %d\n", cnts);
        if(cnts <= 0)
        {
                frame->status = -1;
                printf("audio file is end\n");
                close(sCtx_g.fd);
                return -1;
        }
        frame->buffer = audio;
        frame->time = (++frame_num)*3000;
        frame->status = OK;
        frame->size = sizeof(audio);
        #endif
#if defined(CIRCULAR_SAMPLE_DATA)
        pthread_mutex_lock(&audio_read_lock);
        //while((sample_data->fifo[1].in - sample_data->fifo[1].out) < 768)
        while(!can_circular_buf_read(&sample_data->fifo[1]))
                pthread_cond_wait(&audio_fifo_empty_cond, &audio_read_lock);
               
        int size = read_circular_buf(&sample_data->fifo[1], audio, sizeof(audio));
       
        pthread_mutex_unlock(&audio_read_lock);
       
        if(size > 0)
        {
                frame->buffer = audio;
                frame->time = (++frame_num)*3000;
                frame->status = OK;
                frame->size = size;
        }
        else
        {
                frame->status = -1;
        }

#elif defined(QUEUE_FRAME_DATA)
start:
        buffer_t *buffer_f = NULL;
        struct frame_queue *p_frame_queue = &programe_media.audio.frame_queue;
        /*
        int data_fifo_count = fifo_count(p_frame_queue->fifo);
        if(data_fifo_count >= 50)
        {
                printf("discard %d audio frame\n",data_fifo_count);
                while((buffer_f = fifo_pop_nowait(p_frame_queue->fifo)) != NULL)
                {
                        buffer_free(buffer_f);
                        buffer_f = NULL;               
                }               

                if(p_frame_queue->b_fifo_full == true)
                {
                    pthread_mutex_lock(&p_frame_queue->lock);
                    p_frame_queue->b_fifo_full = false;
                    pthread_cond_signal(&p_frame_queue->fifo_full);
                    pthread_mutex_unlock(&p_frame_queue->lock);                               
                }
                return -1;
        }
        while(data_fifo_count > 0)
        */
       
        {
                buffer_f = fifo_pop(p_frame_queue->fifo);
                if (buffer_f == NULL)
                {
                        printf("fifo_pop data null\n");
                        goto start;
                }

                frame->buffer = buffer_f->p_data;
                frame->size = buffer_f->i_data_size;
                frame->time = (++frame_num)*3000;
                frame->status = OK;

                //printf("audio frame size is %d\n", frame->size);
               
                /* reuse buffer */
                fifo_push(p_frame_queue->empty, buffer_f);
                buffer_f = NULL;

                /* check fifo size */
                if(p_frame_queue->b_fifo_full == true)
                {
                    if (fifo_size(p_frame_queue->fifo) < FIFO_AUDIO_THRESHOLD_SIZE_HALF)
                    {
                        pthread_mutex_lock(&p_frame_queue->lock);
                        p_frame_queue->b_fifo_full = false;
                        pthread_cond_signal(&p_frame_queue->fifo_full);
                        pthread_mutex_unlock(&p_frame_queue->lock);
                    }
                }
        }
#endif
       
        return 0;
}

