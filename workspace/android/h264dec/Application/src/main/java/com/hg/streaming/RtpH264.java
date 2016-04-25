package com.hg.streaming;

import android.animation.TimeAnimator;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by wood on 4/22/2016. Java_com_hg_streaming_RtpH264_stringFromJNI
 */
public class RtpH264 implements TimeAnimator.TimeListener {
    static {
        System.loadLibrary("hello-jni");
    }

    static final String H264FILE = "/sdcard/a.h264";
    private static final String TAG = "H264S";
    final int FRAME_RATE = 30;
    MediaCodec mMediaDecoder;

    byte[] startBytes = new byte[4];
    byte[] h264Data = null;
    int h264DataOffset = 0;

    int mFrameIndex = 0;
    int mOutFrameIndex = 0;

    public native String  stringFromJNI();

    RtpH264(Surface surface, int width, int height) throws IOException {
        Log.d(TAG, "Build.VERSION.SDK_INT:" + Build.VERSION.SDK_INT + " JNI " );

        MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", width, height);
        mMediaDecoder = MediaCodec.createDecoderByType("video/avc");
        mMediaDecoder.configure(mediaFormat, surface, null, 0);
        mMediaDecoder.start();

        startBytes[0] = startBytes[1] = startBytes[2] = 0;
        startBytes[3] = 1;
        long timep = 0;

        File file = new File(H264FILE);
        if (!file.exists() || !file.canRead()) {
            Log.e(TAG, H264FILE);
            return;
        }
        h264Data = new byte[(int) file.length()];//byte[] h264Data = new byte[1024]
        FileInputStream fis = new FileInputStream(file);
        DataInputStream dis = new DataInputStream(fis);
        dis.readFully(h264Data);
        dis.close();
        // jni_init();

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(999999);
                     //jni_loop();
                }catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }

    @Override
    public void onTimeUpdate(TimeAnimator timeAnimator, long l, long l1) {
        if /*while*/ (true) {
            decode();
        }

        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
        int outputBufferIndex = mMediaDecoder.dequeueOutputBuffer(bufferInfo, 0);
        if (outputBufferIndex >= 0) {
            ByteBuffer outputBuffer;
            if (Build.VERSION.SDK_INT >= 21)
                outputBuffer = mMediaDecoder.getOutputBuffer(outputBufferIndex);
            else
                outputBuffer = mMediaDecoder.getOutputBuffers()[outputBufferIndex];
            if (mOutFrameIndex++ % 100 == 0)
                Log.d(TAG, "dequeueOutputBuffer .size:" + bufferInfo.size +" .isDirect: " + outputBuffer.isDirect());

            mMediaDecoder.releaseOutputBuffer(outputBufferIndex, true);
            //outputBufferIndex = mMediaDecoder.dequeueOutputBuffer(bufferInfo, 0);
        } else if (outputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
            Log.d(TAG, "dequeueOutputBuffer: " + outputBufferIndex);
        }
    }

    ByteBuffer inputBuffer = null;
    int inputBufferIndex = -1;

    private void decode() {

        int iBeg = h264DataOffset;
        int iEnd = iBeg;
        int flags = 0;
        if (h264DataOffset == 0) {
            if ((iEnd = indexOf(h264Data, startBytes, iEnd + 4)) < 0
                    || (iEnd = indexOf(h264Data, startBytes, iEnd + 4)) < 0) {
                Log.e(TAG, "indexOf 0");
                //Thread.sleep(10);
                return;//continue;
            }
            flags = MediaCodec.BUFFER_FLAG_CODEC_CONFIG;
        } else {
            if ((iEnd = indexOf(h264Data, startBytes, iEnd + 4)) < 0) {
                Log.e(TAG, "indexOf");
                //Thread.sleep(10);
                return;//continue;
            }
        }

        if (inputBufferIndex < 0) {
            inputBufferIndex = mMediaDecoder.dequeueInputBuffer(5000);
            if (inputBufferIndex < 0) {
                //if (inputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {}
                Log.d(TAG, "dequeueInputBuffer: " + inputBufferIndex);
                return;//continue;
            } else {
                ByteBuffer[] inputBuffers = mMediaDecoder.getInputBuffers();
                inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
            }
        }
        inputBuffer.put(h264Data, iBeg, iEnd - iBeg);

        long timestamp = mFrameIndex++ * 1000000 / FRAME_RATE + 1000;
        mMediaDecoder.queueInputBuffer(inputBufferIndex, 0, inputBuffer.position(), timestamp, flags);
        if (mFrameIndex % 100 == 0)
            Log.d(TAG, "dequeueInputBuffer timestamp: " + timestamp + " inputSize: " + inputBuffer.position() + " .isDirect: " + inputBuffer.isDirect());

        h264DataOffset = iEnd;
        inputBufferIndex = -1;
    }

    public void inflate(ByteBuffer buf) {
        if (inputBufferIndex < 0) {
            inputBufferIndex = mMediaDecoder.dequeueInputBuffer(5000);
            if (inputBufferIndex < 0) {
                //if (inputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {}
                Log.d(TAG, "dequeueInputBuffer: " + inputBufferIndex);
                return;//continue;
            } else {
                ByteBuffer[] inputBuffers = mMediaDecoder.getInputBuffers();
                inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
            }
        }
        inputBuffer.put(buf);
    }

    public void commit(int flags) {
        long timestamp = mFrameIndex++ * 1000000 / FRAME_RATE + 1000;
        mMediaDecoder.queueInputBuffer(inputBufferIndex, 0, inputBuffer.position(), timestamp, flags);
        inputBufferIndex = -1;
        if (mFrameIndex % 100 == 0)
            Log.d(TAG, "dequeueInputBuffer timestamp: " + timestamp + " inputSize: " + inputBuffer.position() + " .isDirect: " + inputBuffer.isDirect());
    }

    public static int indexOf(byte[] largeArray, byte[] subArray, int index) {
        int iLast = largeArray.length - subArray.length;
        forL:
        for (; index <= iLast; index++) {
            for (int j = 0; j < subArray.length; j++) {
                if (subArray[j] != largeArray[index + j]) {
                    continue forL;
                }
            }
            return index;
        }
        return -1;
    }

}
