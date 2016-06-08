#include <string.h>
#include <stdint.h>
#include <jni.h>

#include <android/log.h>
#define  LOG_TAG    "HGSJNI"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// extern "C" {
    int  hgs_queue_input(uint8_t*data, uint8_t*end, int flags, unsigned);

    int  hgs_buffer_obtain(int timeout);
    void hgs_buffer_inflate(int idx, char* p, size_t len);
    void hgs_buffer_release(int idx, unsigned timestamp, int flags);

    void hgs_poll_once(int);
    void hgs_exit(int);
    void hgs_init(char const* ip, int port, char const* path, int w, int h);

    void hgs_run();
    void hgs_pump();
// }

static JNIEnv * env_= NULL;
static jobject oRtpH264 = NULL;
static jmethodID MID_queue_input  = 0;
static jmethodID MID_obtain  = 0;
static jmethodID MID_inflate = 0;
static jmethodID MID_release = 0;

int hgs_queue_input(uint8_t*data, uint8_t*end, int flags, unsigned ts)
{
    jobject buf = (*env_)->NewDirectByteBuffer(env_, data, end-data);
    int idx = (*env_)->CallIntMethod(env_, oRtpH264, MID_queue_input, buf, flags, ts);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
    return idx;
}

int hgs_buffer_obtain(int timeout)
{
    int idx = (*env_)->CallIntMethod(env_, oRtpH264, MID_obtain, timeout);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
    return idx;
}
void hgs_buffer_inflate(int idx, char* p, size_t len)
{
    jobject byteBuffer = (*env_)->NewDirectByteBuffer(env_, p, len);
    (*env_)->CallVoidMethod(env_, oRtpH264, MID_inflate, idx, byteBuffer);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
}
void hgs_buffer_release(int idx, unsigned timestamp, int flags)
{
    (*env_)->CallVoidMethod(env_, oRtpH264, MID_release, idx, timestamp, flags);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_RtpH264_gotFrame( JNIEnv* env, jobject thiz, jobject byteBuf, jlong len )
{
    //jlong len = (*env)->GetDirectBufferCapacity(env, byteBuf);
    void* buf = (*env)->GetDirectBufferAddress(env, byteBuf);  
    len;
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_RtpH264_exitJNI( JNIEnv* env, jobject thiz )
{
    LOGD("exitJNI");
    hgs_exit(1);
    hgs_exit(0);
    (*env)->DeleteGlobalRef(env, oRtpH264);
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_RtpH264_loopJNI( JNIEnv* env, jobject thiz )
{
    hgs_run();//hgs_poll_once(0);
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_RtpH264_pumpJNI( JNIEnv* env, jobject thiz )
{
    hgs_pump();//hgs_poll_once(1);
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_RtpH264_initJNI( JNIEnv* env, jobject thiz )
{
    LOGD("initJNI");
    jclass cls =(*env)->GetObjectClass(env, thiz); // = (*env)->FindClass(env, "com/hg/streaming/RtpH264");

    env_ = env;
    oRtpH264 = (*env)->NewGlobalRef(env, thiz);

    MID_queue_input  = (*env)->GetMethodID(env, cls, "queueInput", "(Ljava/nio/ByteBuffer;II)I");
    MID_obtain  = (*env)->GetMethodID(env, cls, "obtain" , "(I)I");
    MID_inflate = (*env)->GetMethodID(env, cls, "inflate", "(ILjava/nio/ByteBuffer;)V");
    MID_release = (*env)->GetMethodID(env, cls, "release", "(III)V");

    // TEST
    char const *path, *ip;
    int port;
    ip="192.168.2.3"; port = 554; path="rtsp://192.168.2.3/live/ch00_1"; //1280x720
    ip="192.168.2.172"; port = 554; path="rtsp://192.168.2.172/b.mov";
    ip="192.168.0.1"; port = 554; path="rtsp://192.168.0.1/live/ch00_1";
    ip="192.168.9.177"; port = 554; path="rtsp://192.168.9.177/live/ch00_1";
    hgs_init(ip, port, path, 480, 320);

#if defined(__arm__)
  #if defined(__ARM_ARCH_7A__)
    #if defined(__ARM_NEON__)
      #if defined(__ARM_PCS_VFP)
        #define ABI "armeabi-v7a/NEON (hard-float)"
      #else
        #define ABI "armeabi-v7a/NEON"
      #endif
    #else
      #if defined(__ARM_PCS_VFP)
        #define ABI "armeabi-v7a (hard-float)"
      #else
        #define ABI "armeabi-v7a"
      #endif
    #endif
  #else
   #define ABI "armeabi"
  #endif
#elif defined(__i386__)
   #define ABI "x86"
#elif defined(__x86_64__)
   #define ABI "x86_64"
#elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
   #define ABI "mips64"
#elif defined(__mips__)
   #define ABI "mips"
#elif defined(__aarch64__)
   #define ABI "arm64-v8a"
#else
   #define ABI "unknown"
#endif
    LOGD("ABI %s", ABI);
    //return (*env)->NewStringUTF(env, "Hello from JNI !  Compiled with ABI " ABI ".");
}

