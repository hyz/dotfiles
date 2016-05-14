#include <string.h>
#include <jni.h>

#include <android/log.h>
#define  LOG_TAG    "HGSJNI"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// extern "C" {
void hgs_h264slice_inflate(int timestamp, char* p, size_t len);
void hgs_h264slice_commit(int timestamp, int flags);

void hgs_poll_once();
void hgs_exit(int);
void hgs_init();
// }

static JNIEnv * env_= NULL;
static jobject oRtpH264 = NULL;
static jmethodID MID_inflate = 0;
static jmethodID MID_commit = 0;

void hgs_h264slice_inflate(int timestamp, char* p, size_t len)
{
    jobject byteBuffer = (*env_)->NewDirectByteBuffer(env_, p, len);
    (*env_)->CallVoidMethod(env_, oRtpH264, MID_inflate, (int)need_start_bytes, byteBuffer);

    jthrowable ex = (*env_)->ExceptionOccurred(env_);
    if (ex != NULL) {
        (*env_)->ExceptionDescribe(env_);
        (*env_)->ExceptionClear(env_);
    }
}
void hgs_h264slice_commit(int timestamp, int flags)
{
    //hgs_h264slice_inflate(need_start_bytes, p, len);
    //int timestamp=1;
    (*env_)->CallVoidMethod(env_, oRtpH264, MID_commit, timestamp, flags);

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
    hgs_poll_once();
}

JNIEXPORT void JNICALL
Java_com_hg_streaming_RtpH264_initJNI( JNIEnv* env, jobject thiz )
{
    LOGD("initJNI");
    jclass cls =(*env)->GetObjectClass(env, thiz); // = (*env)->FindClass(env, "com/hg/streaming/RtpH264");

    env_ = env;
    oRtpH264 = (*env)->NewGlobalRef(env, thiz);

    MID_inflate = (*env)->GetMethodID(env, cls, "inflate", "(ILjava/nio/ByteBuffer;)V");
    MID_commit  = (*env)->GetMethodID(env, cls, "commit", "(II)V");

    hgs_init();
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
    LOGD("%s", ABI);
    //return (*env)->NewStringUTF(env, "Hello from JNI !  Compiled with ABI " ABI ".");
}

