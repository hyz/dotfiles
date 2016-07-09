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

namespace chrono = boost::chrono;
typedef chrono::process_real_cpu_clock Clock;
inline unsigned milliseconds(Clock::duration const& d) {
    return chrono::duration_cast<chrono::milliseconds>(d).count();
}
inline unsigned seconds(Clock::duration const& d) {
    return chrono::duration_cast<chrono::seconds>(d).count();
}

#if 0 //defined(__ANDROID__)
#   define _TRACE_SIZE_PRINT() ((void)0)
#   define _TRACE_NET_INCOMING(type, size) ((void)0)
//#   define _TRACE_DROP0(n) ((void)0)
//#   define _TRACE_DROP1(n) ((void)0)
#   define _TRACE_DROP_non_IDR(nri, type, n_fwd) ((void)0)
#   define _TRACE_DEC_INC(nri,type,siz) ((void)0)
#   define _TRACE_OUT_INC(siz) ((void)0)
#   define _TRACE_RESET() ((void)0)
#   define _TRACE_PRINT_RATE() ((void)0)
#else
static struct Trace_Info {
    struct TraceSize {
        unsigned len_max=0;
        unsigned len_total=0;
        unsigned count=0;
    };
    std::array<TraceSize,32> sa = {};
    struct { unsigned nfr, dec, out; } ns = {}; // non_IDR_;
    Clock::time_point tp{};
} tc[2] = {};

static void _TRACE_RESET() { tc[1] = tc[0] = Trace_Info{}; }

static void _TRACE_SIZE_PRINT() {
    for (unsigned i=0; i<tc[0].sa.size(); ++i) {
        auto& a = tc[0].sa[i];
        if (a.count > 0) {
            LOGD("NAL-Unit-Type %d: m/a/n: %u %u %u", i, a.len_max, a.len_total/a.count, a.count);
        }
    }
}
static void _TRACE_NET_INCOMING(int type, unsigned size) {
    auto& a = tc[0].sa[type];
    a.len_max = std::max(size, a.len_max);
    a.len_total += size;
    a.count++;

    if (type==1 || type==5) {
        tc[0].ns.nfr++;
    }
}
//static void _TRACE_DROP0(unsigned n) {}
//static void _TRACE_DROP1(unsigned n) { tc[0].n_drop1_ += n; }
// static void _TRACE_DROP_non_IDR(int nri, int type, unsigned nfwd) { }
static void _TRACE_DEC_INC(int nri, int type,unsigned siz) {
    if (type == 5)
        tc[0].ns.dec++;
    else
        LOGD("FWD %d", type);
}
static void _TRACE_OUT_INC(unsigned siz) {
    tc[0].ns.out++;
}
static void _TRACE_PRINT_RATE() {
    tc[0].tp = Clock::now();
    unsigned ms = milliseconds(tc[0].tp - tc[1].tp);
    if (ms > 3000) {
        LOGD("F-rate: %.2f/%.2f, %u%+d%+d"
                , (tc[0].ns.nfr - tc[1].ns.nfr)*1000.0/ms
                , (tc[0].ns.out - tc[1].ns.out)*1000.0/ms
                , tc[0].ns.nfr, -int(tc[0].ns.nfr-tc[0].ns.dec), -int(tc[0].ns.dec-tc[0].ns.out));
        tc[1] = tc[0];
    }
}
#endif

enum { BUFFER_FLAG_CODEC_CONFIG =2 }; //c++: BUFFER_FLAG_CODECCONFIG =2 //OMX: OMX_BUFFERFLAG_CODECCONFIG

//int  javacodec_ibuffer_obtain(int timeout);
//void javacodec_ibuffer_inflate(int idx, char* p, size_t len);
//void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags);
//
//int  javacodec_obuffer_obtain(void** pa, unsigned* len);
//void javacodec_obuffer_release(int idx);

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

struct nalu_data_sink;
static std::unique_ptr<nalu_data_sink> sink_;

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
        _TRACE_OUT_INC(*len);
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
    LOGD("%s %d->%d", fx, y, stage_);
    return y;
}

struct nalu_data_sink
{
    std::mutex mutex_;
    std::deque<mbuffer> bufs_;

    //typedef boost::intrusive::slist<mbuffer,boost::intrusive::cache_last<true>> slist_type;
    //slist_type blis_, blis_ready_;

    nalu_data_sink() {
        //fp_ = fopen("/sdcard/o.sdps", "w");
    }
    ~nalu_data_sink() { _TRACE_SIZE_PRINT(); /*fclose(fp_);*/ }

    enum { Types_SDP78 = ((1<<7)|(1<<8)) };
    enum { Types_SDP67 = ((1<<6)|(1<<7)) };
    unsigned short fwd_types_ = 0;

    bool sdp_ready() const { return ((fwd_types_ & Types_SDP78) == Types_SDP78) || ((fwd_types_ & Types_SDP67) == Types_SDP67); }

    void pushbuf(mbuffer&& bp)
    {
        auto* h = bp.nal_header();
        _TRACE_NET_INCOMING(h->type, bp.end()-bp.begin());

//#define DONOT_DROP 0
        if (h->nri < 3)/*(0)*/ {
            //_TRACE_DROP0(1);
            return;
        }
        if (sdp_ready()) {
            if (h->type > 5) {
                //fwrite((char*)bp.addr(-4), 4+bp.size, 1, fp_);
                return;
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (!bufs_.empty() && sdp_ready())/*(0)*/ {
            //_TRACE_DROP1(bufs_.size());
            bufs_.clear(); //if (bufs_.size() > 4) bufs_.pop_front();
        }
        bufs_.push_back(std::move(bp));
    }
    //void commit(rtp_header const& rh, uint8_t* data, uint8_t* end) { pushbuf(mbuffer(rh, data, end)); }

    void jcodec_inflate()
    {
        _TRACE_PRINT_RATE();
        std::lock_guard<std::mutex> lock(mutex_);
        while (!bufs_.empty() && (tc[0].ns.dec - tc[0].ns.out) < 3 /*&& !(tc[0].ns.nfr&0x400)*/) { // TODO:testing
            int idx = javacodec_ibuffer_obtain(15);
            if (idx < 0) {
                //LOGW("buffer obtain: %d", idx);
                break;
            }

            mbuffer buf = std::move(bufs_.front());
            bufs_.pop_front();
            auto* h = buf.nal_header();

            int flags = 0;
            switch (h->type) {
                case 0: case 7: case 8: flags = BUFFER_FLAG_CODEC_CONFIG; break;
            }
            javacodec_ibuffer_inflate(idx, (char*)buf.begin_s(), buf.end()-buf.begin_s());
            javacodec_ibuffer_release(idx, 1, flags);

            if (!sdp_ready()) {
                fwd_types_ |= (1<<h->type);
                LOGD("sdp %02x", fwd_types_);
            }
            _TRACE_DEC_INC(h->nri, h->type, buf.end()-buf.begin());
        }
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
        nalu_data_sink* sink = new nalu_data_sink();
        sink_.reset(sink);
        hgs_run([sink](mbuffer b){ sink->pushbuf(std::move(b)); });
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
    return JNI_VERSION_1_4;
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
    jobject o = env_->NewObject(CLS_DecoderWrap, MID_DecoderWrap_ctor, w,h, surface);
    oDecoderWrap = env_->NewGlobalRef(o);
    env_->DeleteLocalRef(o);
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
        nalu_data_sink* sink = new nalu_data_sink();
        sink_.reset(sink);
        hgs_run([sink](mbuffer b){ sink->pushbuf(std::move(b)); });
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
            env_->DeleteGlobalRef(oDecoderWrap);
            oDecoderWrap = 0;
        }
        sink_.reset();
        _TRACE_RESET();
    }
}

#ifdef HGS_TEST_BUILD

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_MainActivity_JNIinitDecoder(JNIEnv *env, jclass, jint w, jint h, jobject surface)
{
    env_ = env;
    hgs_init_decoder(w,h, surface);
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_MainActivity_JNIstart(JNIEnv *env, jclass, jstring js_ip, jint port, jstring js_path)
{
    const char *ip = env->GetStringUTFChars(js_ip, 0);
    const char *path = env->GetStringUTFChars(js_path, 0);
    hgs_start(ip, port, path);
    env->ReleaseStringUTFChars(js_ip, ip);
    env->ReleaseStringUTFChars(js_path, path);
}
extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_MainActivity_JNIstop(JNIEnv *env, jclass)
{
    hgs_stop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_MainActivity_JNIpump(JNIEnv *env, jclass)
{
    VideoFrameDecoded d = VideoFrameDecoded::query();
    if (!d.empty())
    {}
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGD("%d:%s", __LINE__,__func__);
#if 0
    static const char* clsName = "com/huazhen/barcode/engine/MainActivity";
    static JNINativeMethod methods[] = {
        { "hello", "(IILandroid.view.Surface;)V", (void*)hello }
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
        return -1;
    }
    LOGD("%s: %d", __func__, retval);
#endif

    return hgs_JNI_OnLoad(vm, reserved);
}
//extern "C" JNIEXPORT jint JNICALL JNI_UnLoad(JavaVM* vm, void* reserved)

#endif

