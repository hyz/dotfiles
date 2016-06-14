#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <jni.h>

#include <android/log.h>
#define  LOG_TAG    "HGSJNI"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// extern "C" {
    int  javacodec_ibuffer_obtain(int timeout);
    void javacodec_ibuffer_inflate(int idx, char* p, size_t len);
    void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags);

    int  javacodec_obuffer_obtain(void** pa, unsigned* len);
    void javacodec_obuffer_release(int idx);

    void hgs_exit(int);
    void hgs_run();
    void hgs_init(char const* ip, int port, char const* path, int w, int h);

    void jni_pump();
// }

static JNIEnv * env_= NULL;
static jobject oDecoderWrap = NULL;
//static jmethodID MID_queue_input  = 0;
static jmethodID MID_IBufferobtain  = 0;
static jmethodID MID_IBufferinflate = 0;
static jmethodID MID_IBufferrelease = 0;
static jmethodID MID_OBufferobtain  = 0;
static jmethodID MID_OBufferrelease = 0;
static jfieldID  FID_outputBuffer   = 0;

//int hgs_queue_input(uint8_t*data, uint8_t*end, int flags, unsigned ts)
//{
//    jobject buf = (*env_)->NewDirectByteBuffer(env_, data, end-data);
//    int idx = (*env_)->CallIntMethod(env_, oDecoderWrap, MID_queue_input, buf, flags, ts);
//
//    jthrowable ex = (*env_)->ExceptionOccurred(env_);
//    if (ex != NULL) {
//        (*env_)->ExceptionDescribe(env_);
//        (*env_)->ExceptionClear(env_);
//    }
//    return idx;
//}

int javacodec_ibuffer_obtain(int timeout)
{
    int idx = (*env_)->CallIntMethod(env_, oDecoderWrap, MID_IBufferobtain, timeout);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
    return idx;
}
void javacodec_ibuffer_inflate(int idx, char* p, size_t len)
{
    jobject byteBuffer = (*env_)->NewDirectByteBuffer(env_, p, len);
    (*env_)->CallVoidMethod(env_, oDecoderWrap, MID_IBufferinflate, idx, byteBuffer);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
}
void javacodec_ibuffer_release(int idx, unsigned timestamp, int flags)
{
    (*env_)->CallVoidMethod(env_, oDecoderWrap, MID_IBufferrelease, idx, timestamp, flags);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
}

int javacodec_obuffer_obtain(void** pv, unsigned* len) {
    int obidx = (*env_)->CallIntMethod(env_, oDecoderWrap, MID_OBufferobtain, 50);
    if (obidx >= 0) {
        jobject bytebuf = (*env_)->GetObjectField(env_, oDecoderWrap, FID_outputBuffer);
        *pv = (*env_)->GetDirectBufferAddress(env_, bytebuf);
        *len = (*env_)->GetDirectBufferCapacity(env_, bytebuf);
    }
    return obidx;
    //void*       (*GetDirectBufferAddress)(JNIEnv*, jobject);
    //jlong       (*GetDirectBufferCapacity)(JNIEnv*, jobject);
    //jfieldID    (*GetFieldID)(JNIEnv*, jclass, const char*, const char*);
    //jobject     (*GetObjectField)(JNIEnv*, jobject, jfieldID);
}

void javacodec_obuffer_release(int idx) {
    (*env_)->CallVoidMethod(env_, oDecoderWrap, MID_OBufferrelease, idx, 1);
    //jthrowable ex = (*env_)->ExceptionOccurred(env_);
    //if (ex != NULL) {
    //    (*env_)->ExceptionDescribe(env_);
    //    (*env_)->ExceptionClear(env_);
    //}
}

//JNIEXPORT void JNICALL Java_com_hg_streaming_DecoderWrap_onFrame( JNIEnv* env, jobject thiz, jobject byteBuf )
//{
//    jlong len = (*env)->GetDirectBufferCapacity(env, byteBuf);
//    void* buf = (*env)->GetDirectBufferAddress(env, byteBuf);  
//}

JNIEXPORT void JNICALL
Java_com_hg_streaming_DecoderWrap_exitHGS( JNIEnv* env, jobject thiz )
{
    LOGD("exitHGS");
    hgs_exit(1);
    hgs_exit(0);
    (*env)->DeleteGlobalRef(env, oDecoderWrap);
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_DecoderWrap_runHGS( JNIEnv* env, jobject thiz )
{
    hgs_run();
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_DecoderWrap_initHGS( JNIEnv* env, jobject thiz, jstring js_ip, jint port, jstring js_path )
{
    env_ = env;
    oDecoderWrap = (*env)->NewGlobalRef(env, thiz);

    jclass cls =(*env)->GetObjectClass(env, thiz); // = (*env)->FindClass(env, "com/hg/streaming/DecoderWrap");
    //MID_queue_input  = (*env)->GetMethodID(env, cls, "queueInput", "(Ljava/nio/ByteBuffer;II)I");
    MID_IBufferobtain  = (*env)->GetMethodID(env, cls, "cIBufferObtain" , "(I)I");
    MID_IBufferinflate = (*env)->GetMethodID(env, cls, "cIBufferInflate", "(ILjava/nio/ByteBuffer;)V");
    MID_IBufferrelease = (*env)->GetMethodID(env, cls, "cIBufferRelease", "(III)V");

    MID_OBufferobtain  = (*env)->GetMethodID(env, cls, "cOBufferObtain" , "(I)I");
    MID_OBufferrelease = (*env)->GetMethodID(env, cls, "cOBufferRelease", "(II)V");
    FID_outputBuffer   = (*env)->GetFieldID (env, cls, "outputBuffer"   , "Ljava/nio/ByteBuffer;");

    const char *ip = (*env)->GetStringUTFChars(env, js_ip, 0);
    const char *path = (*env)->GetStringUTFChars(env, js_path, 0);

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

    (*env)->ReleaseStringUTFChars(env, js_ip, ip);
    (*env)->ReleaseStringUTFChars(env, js_path, path);

    //return (*env)->NewStringUTF(env, "Hello from JNI !  Compiled with ABI " ABI ".");
    extern void abi_print_(); abi_print_();
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_DecoderWrap_pumpJNI( JNIEnv* env, jobject thiz )
{
    jni_pump();
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

