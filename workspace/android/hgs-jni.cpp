#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <jni.h>

#include <android/log.h>
#define  LOG_TAG    "HGSJNI"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void abi_print_();

int  javacodec_ibuffer_obtain(int timeout);
void javacodec_ibuffer_inflate(int idx, char* p, size_t len);
void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags);

JNIEXPORT int  codecio_query(void** data, unsigned* size);
JNIEXPORT int  javacodec_obuffer_obtain(void** pa, unsigned* len);
JNIEXPORT void javacodec_obuffer_release(int idx);

void hgs_exit(int);
void hgs_run();
void hgs_init(char const* ip, int port, char const* path, int w, int h);

static JNIEnv * env_= NULL;
static jobject oDecoderWrap = NULL;
//static jmethodID MID_queue_input  = 0;
static jmethodID MID_IBufferobtain  = 0;
static jmethodID MID_IBufferinflate = 0;
static jmethodID MID_IBufferrelease = 0;
static jmethodID MID_OBufferobtain  = 0;
static jmethodID MID_OBufferrelease = 0;
static jfieldID  FID_outputBuffer   = 0;

int javacodec_ibuffer_obtain(int timeout)
{
    int idx = env_->CallIntMethod(oDecoderWrap, MID_IBufferobtain, timeout);

    jthrowable ex = env_->ExceptionOccurred();
    if (ex != NULL) {
        env_->ExceptionDescribe();
        env_->ExceptionClear();
    }
    return idx;
}
void javacodec_ibuffer_inflate(int idx, char* p, size_t len)
{
    jobject byteBuffer = env_->NewDirectByteBuffer(p, len);
    env_->CallVoidMethod(oDecoderWrap, MID_IBufferinflate, idx, byteBuffer);

    jthrowable ex = env_->ExceptionOccurred();
    if (ex != NULL) {
        env_->ExceptionDescribe();
        env_->ExceptionClear();
    }
}
void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags)
{
    env_->CallVoidMethod(oDecoderWrap, MID_IBufferrelease, idx, timestamp, flags);

    jthrowable ex = env_->ExceptionOccurred();
    if (ex != NULL) {
        env_->ExceptionDescribe();
        env_->ExceptionClear();
    }
}

int javacodec_obuffer_obtain(void** pv, unsigned* len) {
    int obidx = env_->CallIntMethod(oDecoderWrap, MID_OBufferobtain, 50);
    if (obidx >= 0) {
        jobject bytebuf = env_->GetObjectField(oDecoderWrap, FID_outputBuffer);
        *pv = env_->GetDirectBufferAddress(bytebuf);
        *len = env_->GetDirectBufferCapacity(bytebuf);
    }
    return obidx;
    //void*       (*GetDirectBufferAddress)(JNIEnv*, jobject);
    //jlong       (*GetDirectBufferCapacity)(JNIEnv*, jobject);
    //jfieldID    (*GetFieldID)(JNIEnv*, jclass, const char*, const char*);
    //jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
}

void javacodec_obuffer_release(int idx) {
    env_->CallVoidMethod(oDecoderWrap, MID_OBufferrelease, idx, 1);
    //jthrowable ex = env_->ExceptionOccurred();
    //if (ex != NULL) {
    //    env_->ExceptionDescribe();
    //    env_->ExceptionClear();
    //}
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_exitHGS(JNIEnv* env, jobject thiz)
{
    LOGD("exitHGS");
    hgs_exit(1);
    hgs_exit(0);
    env_->DeleteGlobalRef(oDecoderWrap);
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_runHGS(JNIEnv* env, jobject thiz)
{
    hgs_run();
}

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_initHGS(JNIEnv* env, jobject thiz, jstring js_ip, jint port, jstring js_path)
{
    env_ = env;
    oDecoderWrap = env_->NewGlobalRef(thiz);

    jclass cls =env_->GetObjectClass(thiz); // = env_->FindClass("com/hg/streaming/DecoderWrap");
    //MID_queue_input  = env_->GetMethodID(cls, "queueInput", "(Ljava/nio/ByteBuffer;II)I");
    MID_IBufferobtain  = env_->GetMethodID(cls, "cIBufferObtain" , "(I)I");
    MID_IBufferinflate = env_->GetMethodID(cls, "cIBufferInflate", "(ILjava/nio/ByteBuffer;)V");
    MID_IBufferrelease = env_->GetMethodID(cls, "cIBufferRelease", "(III)V");

    MID_OBufferobtain  = env_->GetMethodID(cls, "cOBufferObtain" , "(I)I");
    MID_OBufferrelease = env_->GetMethodID(cls, "cOBufferRelease", "(II)V");
    FID_outputBuffer   = env_->GetFieldID (cls, "outputBuffer"   , "Ljava/nio/ByteBuffer;");

    const char *ip = env_->GetStringUTFChars(js_ip, 0);
    const char *path = env_->GetStringUTFChars(js_path, 0);

    //// TEST
    // ip="192.168.2.3"  ; port = 554; path="/live/ch00_1"; //1280x720
    // ip="192.168.2.172"; port = 554; path="/b.mov";
    // ip="192.168.0.1"  ; port = 554; path="/live/ch00_1";
    // ip="192.168.9.177"; port = 554; path="/live/ch00_1";
    // ip="192.168.9.177"; port = 554; path="/live/ch00_2";

    char uri[256];
    if (port == 554) {
        snprintf(uri,sizeof(uri), "rtsp://%s%s", ip, path);
    } else {
        snprintf(uri,sizeof(uri), "rtsp://%s:%d%s", ip, (int)port, path);
    }
    LOGD("initHGS %s", uri);
    hgs_init(ip, port, uri, 0, 0);

    env_->ReleaseStringUTFChars(js_ip, ip);
    env_->ReleaseStringUTFChars(js_path, path);

    //return env_->NewStringUTF("Hello from JNI !  Compiled with ABI " ABI ".");
    abi_print_();
}

struct QueryDecoded
{
    void* data;
    unsigned size;
    bool is_ready() const { return oidx_ >= 0; }

    QueryDecoded() { oidx_ = codecio_query(&data, &size); }
    ~QueryDecoded() { _release(); }

    QueryDecoded(QueryDecoded && rhs) {
        data = rhs.data;
        size = rhs.size;
        oidx_ = rhs.oidx_;
        rhs.oidx_ = -1;
    }
    QueryDecoded& operator=(QueryDecoded && rhs) {
        if (this != &rhs) {
            this->_release();
            data = rhs.data;
            size = rhs.size;
            oidx_ = rhs.oidx_;
            rhs.oidx_ = -1;
        }
        return *this;
    }
private:
    int oidx_;
    void _release() {
        if (oidx_ >= 0) {
            javacodec_obuffer_release(oidx_);
            LOGD("Decoded %d: %u:%p", oidx_, size, data);
        }
    }

    QueryDecoded(QueryDecoded const&);
    QueryDecoded& operator=(QueryDecoded const&);
};

extern "C" JNIEXPORT void JNICALL
Java_com_huazhen_barcode_engine_DecoderWrap_pumpJNI(JNIEnv* env, jobject thiz)
{
    QueryDecoded qdec;
    if (qdec.is_ready()) {
    }
}

int hgs_env_setup(JNIEnv* env)
{
    static const char* ClassName = "com/huazhen/barcode/engine/DecoderWrap";
    static JNINativeMethod methods[] = {
        { "initHGS", "(Ljava/lang/String;ILjava/lang/String;)V", (void*)Java_com_huazhen_barcode_engine_DecoderWrap_initHGS }
      , { "runHGS" , "()V", (void*)Java_com_huazhen_barcode_engine_DecoderWrap_runHGS }
      , { "exitHGS", "()V", (void*)Java_com_huazhen_barcode_engine_DecoderWrap_exitHGS }
    };

    jclass clz = env_->FindClass(ClassName);
    if (!clz) {
        LOGE("FindClass %s: fail", ClassName);
        return -1;
    }
    int retval = env_->RegisterNatives(clz, methods, sizeof(methods)/sizeof(*methods));
    if (retval < 0) {
        LOGE("RegisterNatives %s: fail", ClassName);
    }
    LOGD("%s: %d", __func__, retval);
    return retval;
}

void abi_print_()
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
    LOGD("ABI %s", ABI);
}

#if 0
extern "C" {
    int codecio_query(void** data, unsigned* size);
    void javacodec_obuffer_release(int idx);
}

#endif

