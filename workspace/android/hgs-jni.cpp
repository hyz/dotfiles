#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <array>
#include <deque>
#include <algorithm>
#include <mutex>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <jni.h>
#include <android/log.h>
#define  LOG_TAG    "HGSJNI"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#include <boost/assert.hpp>
#include "hgs.hpp"

namespace chrono = boost::chrono;
typedef boost::chrono::process_real_cpu_clock Clock;
inline unsigned milliseconds(Clock::duration const& d) {
    return chrono::duration_cast<chrono::milliseconds>(d).count();
}

enum { BUFFER_FLAG_CODEC_CONFIG=2 };

int  javacodec_ibuffer_obtain(int timeout);
void javacodec_ibuffer_inflate(int idx, char* p, size_t len);
void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags);

int  javacodec_obuffer_obtain(void** pa, unsigned* len);
void javacodec_obuffer_release(int idx);

static signed char stage_ = 0;
static JavaVM *  jvm_ = NULL;
static JNIEnv *  env_= NULL;
static jobject   oDecoderWrap = NULL;
static jclass    CLS_DecoderWrap = 0;
static jmethodID MID_DecoderWrap_close = 0;
static jmethodID MID_DecoderWrap_ctor = 0;
static jmethodID MID_IBufferobtain  = 0;
static jmethodID MID_IBufferinflate = 0;
static jmethodID MID_IBufferrelease = 0;
static jmethodID MID_OBufferobtain  = 0;
static jmethodID MID_OBufferrelease = 0;
static jfieldID  FID_outputBuffer   = 0;

struct data_sink;
static std::unique_ptr<data_sink> sink_;

inline int javacodec_ibuffer_obtain(int timeout)
{
    int idx = env_->CallIntMethod(oDecoderWrap, MID_IBufferobtain, timeout);
    //jthrowable ex = env_->ExceptionOccurred();
    //if (ex != NULL) {
    //    env_->ExceptionDescribe();
    //    env_->ExceptionClear();
    //}
    return idx;
}
inline void javacodec_ibuffer_inflate(int idx, char* p, size_t len)
{
    jobject byteBuffer = env_->NewDirectByteBuffer(p, len);
    env_->CallVoidMethod(oDecoderWrap, MID_IBufferinflate, idx, byteBuffer);
    env_->DeleteLocalRef(byteBuffer);
    //jthrowable ex = env_->ExceptionOccurred();
    //if (ex != NULL) {
    //    env_->ExceptionDescribe();
    //    env_->ExceptionClear();
    //}
}
inline void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags)
{
    env_->CallVoidMethod(oDecoderWrap, MID_IBufferrelease, idx, timestamp, flags);
    //jthrowable ex = env_->ExceptionOccurred();
    //if (ex != NULL) {
    //    env_->ExceptionDescribe();
    //    env_->ExceptionClear();
    //}
}

int javacodec_obuffer_obtain(void** pv, unsigned* len)
{
    int idx = env_->CallIntMethod(oDecoderWrap, MID_OBufferobtain, 50);
    if (idx >= 0) {
        jobject bytebuf = env_->GetObjectField(oDecoderWrap, FID_outputBuffer);
        *pv = env_->GetDirectBufferAddress(bytebuf);
        *len = env_->GetDirectBufferCapacity(bytebuf);
        env_->DeleteLocalRef(bytebuf);
    }
    return idx;
    //void*       (*GetDirectBufferAddress)(JNIEnv*, jobject);
    //jlong       (*GetDirectBufferCapacity)(JNIEnv*, jobject);
    //jfieldID    (*GetFieldID)(JNIEnv*, jclass, const char*, const char*);
    //jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
}

void javacodec_obuffer_release(int idx) {
    env_->CallVoidMethod(oDecoderWrap, MID_OBufferrelease, idx, 0);
    //jthrowable ex = env_->ExceptionOccurred();
    //if (ex != NULL) {
    //    env_->ExceptionDescribe();
    //    env_->ExceptionClear();
    //}
}

inline int stage(signed char y, char const* fx) {
    std::swap(stage_, y);
    LOGD("HGS:%s %d->%d", fx, y, stage_);
    return y;
}

static char const* abi_str()
{
#if defined(__arm__)
  #if defined(__ARM_ARCH_7A__)
    #if defined(__ARM_NEON__)
      #if defined(__ARM_PCS_VFP)
#  define ABI "armeabi-v7a/NEON (hard-float)"
      #else
#  define ABI "armeabi-v7a/NEON"
      #endif
    #else
      #if defined(__ARM_PCS_VFP)
#  define ABI "armeabi-v7a (hard-float)"
      #else
#  define ABI "armeabi-v7a"
      #endif
    #endif
  #else
#  define ABI "armeabi"
  #endif
#elif defined(__i386__)
#  define ABI "x86"
#elif defined(__x86_64__)
#  define ABI "x86_64"
#elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
#  define ABI "mips64"
#elif defined(__mips__)
#  define ABI "mips"
#elif defined(__aarch64__)
#  define ABI "arm64-v8a"
#else
#  define ABI "unknown"
#endif
    return ABI;
}

struct data_sink
{
#if 0 //defined(__ANDROID__)
#   define _TRACE_SIZE_PRINT() ((void)0)
#   define _TRACE_SIZE(type, size) ((void)0)
#   define _TRACE_DROP0(n) ((void)0)
#   define _TRACE_DROP1(n) ((void)0)
#   define _TRACE_DROP_non_IDR(nri, type, n_fwd) ((void)0)
#   define _TRACE_FWD_INCR(nri,type,siz) ((void)0)
#else
    struct TraceSize {
        unsigned len_max=0;
        unsigned len_total=0;
        unsigned count=0;
    };
    std::array<TraceSize,32> tsa_;
    Clock::time_point tp_{}; //struct timeval tp_;
    unsigned n_fr_ = 0;
    unsigned n_drop1_ = 0;
    unsigned n_fwd_ = 0; // signed char non_IDR_ = 4;
    //unsigned size_fwd_ = 0;

    void _TRACE_SIZE_PRINT() {
        for (unsigned i=0; i<tsa_.size(); ++i) {
            auto& a = tsa_[i];
            if (a.count > 0) {
                LOGD("NAL-Unit-Type %d: m/a/n: %u %u %u", i, a.len_max, a.len_total/a.count, a.count);
            }
        }
    }
    void _TRACE_SIZE(int type, unsigned size) {
        auto& a = tsa_[type];
        a.len_max = std::max(size, a.len_max);
        a.len_total += size;
        a.count++;

        if (type==1 || type==5) {
            ++n_fr_;
            if (n_fr_ == 1) {
                tp_ = Clock::now(); // gettimeofday(&tp_, NULL);
            } else if ((n_fr_ & 0x7f) == 0) {
                LOGD("Net F-rate: %.2f, %u %u %u", 0x7f*1000.0/milliseconds(Clock::now()-tp_), n_fr_, n_fwd_, n_drop1_);
                tp_ = Clock::now();
            }
        }
    }
    void _TRACE_DROP0(unsigned n) {
    }
    void _TRACE_DROP1(unsigned n) {
        n_drop1_ += n;
    }
    void _TRACE_DROP_non_IDR(int nri, int type, unsigned nfwd) {
        ;
    }
    void _TRACE_FWD_INCR(int nri, int type,unsigned siz) {
        if (type > 5)
            LOGD("FWD %d", type);
        ++n_fwd_;
    }
#endif

    std::mutex mutex_;
    std::deque<mbuffer> bufs_;

    //typedef boost::intrusive::slist<mbuffer,boost::intrusive::cache_last<true>> slist_type;
    //slist_type blis_, blis_ready_;

    data_sink() {
        fp_ = fopen("/sdcard/o.sdps", "w");
    }
    ~data_sink() { _TRACE_SIZE_PRINT(); fclose(fp_); }
    FILE* fp_;

    enum { Types_SDP78 = ((1<<7)|(1<<8)) };
    enum { Types_SDP67 = ((1<<6)|(1<<7)) };
    unsigned short fwd_types_ = 0;

    bool sdp_ready() const { return ((fwd_types_ & Types_SDP78) == Types_SDP78) || ((fwd_types_ & Types_SDP67) == Types_SDP67); }

    void commit(mbuffer&& bp)
    {
        auto& h = bp.base_ptr->nal_h;
        _TRACE_SIZE(h.type, bp.size);

//#define DONOT_DROP 0
        if (h.nri < 3)/*(0)*/ {
            _TRACE_DROP0(1);
            return;
        }
        if (sdp_ready()) {
            if (h.type > 5) {
                fwrite((char*)bp.addr(-4), 4+bp.size, 1, fp_);
                return;
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!bufs_.empty() && sdp_ready())/*(0)*/ {
            _TRACE_DROP1(bufs_.size());
            bufs_.clear(); //if (bufs_.size() > 4) bufs_.pop_front();
        }
        bufs_.push_back(std::move(bp));
    }
    //void commit(rtp_header const& rh, uint8_t* data, uint8_t* end) { commit(mbuffer(rh, data, end)); }

    void jcodec_inflate()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!bufs_.empty()) {
            int idx = javacodec_ibuffer_obtain(15);
            if (idx < 0) {
                //LOGW("buffer obtain: %d", idx);
                break;
            }

            mbuffer buf = std::move(bufs_.front());
            bufs_.pop_front();
            auto& h = buf.base_ptr->nal_h;

            int flags = 0;
            switch (h.type) {
                case 0: case 7: case 8: flags = BUFFER_FLAG_CODEC_CONFIG; break;
            }
            javacodec_ibuffer_inflate(idx, (char*)buf.addr(-4), 4+buf.size);
            javacodec_ibuffer_release(idx, 1, flags);

            if (!sdp_ready()) {
                fwd_types_ |= (1<<h.type);
                LOGD("sdp %02x", fwd_types_);
            }
            _TRACE_FWD_INCR(h.nri, h.type, buf.size);
        }
        // LOGD("%d:%s", __LINE__,__func__);
    }

    //static slist_type::iterator iterator_before(slist_type& blis, mbuffer& b)
    //{
    //    auto it = blis.before_begin();
    //    auto p = it++;
    //    for (; it != blis.end(); p=it++) {
    //        if (it.operator->() == &b)
    //            break;
    //    }
    //    return p;
    //}
};

JNIEXPORT int VideoFrameDecoded::query_s(void**data, unsigned* size) {
    if (sink_) {
        sink_->jcodec_inflate();
        if (sink_->sdp_ready()) {
            return javacodec_obuffer_obtain(data, size);
        }
    }
    return -1;
}
JNIEXPORT void VideoFrameDecoded::release_s(int idx) {
    javacodec_obuffer_release(idx);
}

#if 0
extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_pumpJNI(JNIEnv* env, jobject thiz, jint w, jint h, jobject surface)
{
    if (!env_) {
        env_ = env;
        hgs_init_decoder(w,h, surface);
    }
    VideoFrameDecoded d = VideoFrameDecoded::query(env);
    if (!d.empty())
    {}
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_stopHGS(JNIEnv* env, jobject thiz)
{
    stage(-1, __func__);
    hgs_exit(1);
    hgs_exit(0);
    sink_.reset();
    env_->DeleteLocalRef(oDecoderWrap); // env->DeleteGlobalRef(oDecoderWrap);
    oDecoderWrap = 0;
}

//extern "C" JNIEXPORT void JNICALL Java_com_huazhen_barcode_engine_DecoderWrap_runHGS(JNIEnv* env, jobject thiz) {}
extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_startHGS(JNIEnv* env, jobject thiz, jstring js_ip, jint port, jstring js_path)
{
    stage(2, __func__);
    oDecoderWrap = env->NewGlobalRef(thiz);

    jclass cls =env->GetObjectClass(thiz); // = env->FindClass("com/hg/streaming/DecoderWrap");

    MID_IBufferobtain  = env->GetMethodID(cls, "cIBufferObtain" , "(I)I");
    MID_IBufferinflate = env->GetMethodID(cls, "cIBufferInflate", "(ILjava/nio/ByteBuffer;)V");
    MID_IBufferrelease = env->GetMethodID(cls, "cIBufferRelease", "(III)V");

    MID_OBufferobtain  = env->GetMethodID(cls, "cOBufferObtain" , "(I)I");
    MID_OBufferrelease = env->GetMethodID(cls, "cOBufferRelease", "(II)V");
    FID_outputBuffer   = env->GetFieldID (cls, "outputBuffer"   , "Ljava/nio/ByteBuffer;");

    const char *ip = env->GetStringUTFChars(js_ip, 0);
    const char *path = env->GetStringUTFChars(js_path, 0);

    char uri[256];
    if (port == 554) {
        snprintf(uri,sizeof(uri), "rtsp://%s%s", ip, path);
    } else {
        snprintf(uri,sizeof(uri), "rtsp://%s:%d%s", ip, (int)port, path);
    }
    LOGD("startHGS %s", uri);
    hgs_init(ip, port, uri, 0, 0);

    env->ReleaseStringUTFChars(js_ip, ip);
    env->ReleaseStringUTFChars(js_path, path);

    {}/*=*/{
        data_sink* sink = new data_sink();
        sink_.reset(sink);
        hgs_run([sink](mbuffer b){ sink->commit(std::move(b)); });
    }
    LOGD("%s", abi_str());
}

#endif

JNIEXPORT int hgs_JNI_OnLoad(JavaVM* vm, void*)
{
    jvm_ = vm;
    JNIEnv* env;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
        return -1;
    char const* clsName = "com/huazhen/barcode/engine/DecoderWrap";
    jclass cls = env->FindClass(clsName);//CHECK(cls); //=env->GetObjectClass(oDecoderWrap);
    CLS_DecoderWrap = (jclass)env->NewGlobalRef(cls);

    MID_IBufferobtain  = env->GetMethodID(cls, "cIBufferObtain" , "(I)I");
    MID_IBufferinflate = env->GetMethodID(cls, "cIBufferInflate", "(ILjava/nio/ByteBuffer;)V");
    MID_IBufferrelease = env->GetMethodID(cls, "cIBufferRelease", "(III)V");

    MID_OBufferobtain  = env->GetMethodID(cls, "cOBufferObtain" , "(I)I");
    MID_OBufferrelease = env->GetMethodID(cls, "cOBufferRelease", "(II)V");
    FID_outputBuffer   = env->GetFieldID (cls, "outputBuffer"   , "Ljava/nio/ByteBuffer;");

    MID_DecoderWrap_close = env->GetMethodID(cls, "close" , "()V");
    // java/lang/String android/view/Surface
    MID_DecoderWrap_ctor = env->GetMethodID(cls, "<init>","(IILandroid/view/Surface;)V");

    LOGD("%d:%s %p %s: %p", __LINE__,__func__, env, clsName, cls);
    return 0;
}

JNIEXPORT JNIEnv* hgs_AttachCurrentThread()
{
    env_ = 0;
    jvm_->AttachCurrentThread(&env_, 0);
    LOGD("%d:%s %p %p", __LINE__,__func__, jvm_, env_);
    if (!env_) {
        LOGE("AttachCurrentThread");
    }
    return env_;
}
JNIEXPORT void hgs_DetachCurrentThread()
{
    if (jvm_->DetachCurrentThread() != JNI_OK) {
        LOGE("DetachCurrentThread");
    }
    LOGD("%d:%s %p %p", __LINE__,__func__, jvm_, env_);
}

JNIEXPORT void* hgs_init_decoder(int w, int h, jobject surface)
{
    oDecoderWrap = env_->NewObject(CLS_DecoderWrap, MID_DecoderWrap_ctor, w,h, surface);
    LOGD("%d:%s %p", __LINE__,__func__, oDecoderWrap);
    return (void*)oDecoderWrap;
}

JNIEXPORT int hgs_start(char const* ip, int port, char const* path)
{
    BOOST_ASSERT(oDecoderWrap);
    stage(1, __func__);

    char uri[256];
    if (port == 554) {
        snprintf(uri,sizeof(uri), "rtsp://%s%s", ip, path);
    } else {
        snprintf(uri,sizeof(uri), "rtsp://%s:%d%s", ip, (int)port, path);
    }
    int retval = 0;
    hgs_init(ip, port, uri, 0, 0);

    //const char *ip = env->GetStringUTFChars(js_ip, 0);
    //env->ReleaseStringUTFChars(js_ip, ip);

    {}/*=*/{
        data_sink* sink = new data_sink();
        sink_.reset(sink);
        hgs_run([sink](mbuffer b){ sink->commit(std::move(b)); });
    }
    LOGD("%d:%s %d %s", __LINE__,__func__, stage_, abi_str());
    return retval;
}

JNIEXPORT void hgs_stop()
{
    LOGD("%d:%s %d", __LINE__,__func__, stage_);
    if (stage_ > 0) {
        stage(0, __func__);
        hgs_exit(1);
        hgs_exit(0);

        if (oDecoderWrap) {
            env_->CallVoidMethod(oDecoderWrap, MID_DecoderWrap_close);
            env_->DeleteLocalRef(oDecoderWrap); // ->DeleteGlobalRef(oDecoderWrap);
            oDecoderWrap = 0;
        }
        sink_.reset();
    }
}

#if 0

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_initDecoder(JNIEnv* env, jobject thiz, jint w, jint h, jobject surface)
{
    env_ = env;
    hgs_init_decoder(w,h, surface);
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_startRTSP(JNIEnv* env, jobject thiz, jobject ip, jint port, jobject path)
{
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_pumpJNI(JNIEnv* env, jobject thiz)
{
    VideoFrameDecoded d = VideoFrameDecoded::query(env);
    if (!d.empty())
    {}
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    static const char* clsName = "com/huazhen/barcode/engine/DecoderWrap";
    static JNINativeMethod methods[] = {
        { "pumpJNI", "(IILandroid.view.Surface;)V", (void*)Java_com_huazhen_barcode_engine_DecoderWrap_pumpJNI }
    };
    enum { N_methods = sizeof(methods)/sizeof(methods[0]) };

    jclass clz = env->FindClass(clsName);
    if (!clz) {
        LOGE("FindClass %s: fail", clsName);
        return -1;
    }
    int retval = env->RegisterNatives(clz, methods, N_methods);
    if (retval < 0) {
        LOGE("RegisterNatives %s: fail", clsName);
    } else {
        hgs_JNI_OnLoad(vm, 0);
    }
    LOGD("%s: %d", __func__, retval);
    return retval;
}
//JNIEXPORT jint JNI_UnLoad(JavaVM* vm, void* reserved) {}

#endif

