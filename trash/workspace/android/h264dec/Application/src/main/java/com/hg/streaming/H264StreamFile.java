package com.hg.streaming;

import android.animation.TimeAnimator;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * Created by wood on 4/21/2016.
 */
public class H264StreamFile implements TimeAnimator.TimeListener {
    static final String H264FILE = "/sdcard/a.h264";
    private static final String TAG = "H264S";
    final int FRAME_RATE = 30;
    MediaCodec mMediaDecoder;

    byte[] startBytes = new byte[4];
    byte[] h264Data = null;
    int h264DataOffset = 0;

    int mFrameIndex = 0;
    int mOutFrameIndex = 0;

    H264StreamFile(Surface surface, int width, int height) throws IOException {

        MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", width, height);
        mMediaDecoder = MediaCodec.createDecoderByType("video/avc");
        mMediaDecoder.configure(mediaFormat, surface, null, 0);
        mMediaDecoder.start();

        startBytes[0] = startBytes[1] = startBytes[2] = 0;
        startBytes[3] = 1;
    }

    @Override
    public void onTimeUpdate(TimeAnimator timeAnimator, long l, long l1) {
        try {
            if (h264DataOffset == 0) {
                File file = new File(H264FILE);
                if (!file.exists() || !file.canRead()) {
                    Log.e(TAG, "failed to open h264 file.");
                    return;
                }
                h264Data = new byte[(int) file.length()];//byte[] h264Data = new byte[1024]
                FileInputStream fis = new FileInputStream(file);
                DataInputStream dis = new DataInputStream(fis);
                dis.readFully(h264Data);
                dis.close();

                if ((h264DataOffset = indexOf(h264Data, startBytes, +4)) > 0
                        && (h264DataOffset = indexOf(h264Data, startBytes, h264DataOffset + 4)) > 0) {
                    queueBuffers(h264Data, 0, h264DataOffset, MediaCodec.BUFFER_FLAG_CODEC_CONFIG);
                }
            }
            if (h264DataOffset > 0) {
                int iBeg = h264DataOffset;
                if ((h264DataOffset = indexOf(h264Data, startBytes, h264DataOffset + 4)) > 0) {
                    queueBuffers(h264Data, iBeg, h264DataOffset - iBeg, 0);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void queueBuffers(byte[] input, int offs, int length, int flags) {
        try {
            ByteBuffer[] inputBuffers = mMediaDecoder.getInputBuffers();
            int inputBufferIndex = mMediaDecoder.dequeueInputBuffer(5000);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                long timestamp = mFrameIndex++ * 1000000 / FRAME_RATE + 1000;
                if (mFrameIndex % 100 == 0)
                    Log.d(TAG, "queueBuffers timestamp: " + timestamp + " inputSize: " + length);
                inputBuffer.clear();
                inputBuffer.put(input, offs, length);
                mMediaDecoder.queueInputBuffer(inputBufferIndex, 0, length, timestamp, flags);

            } else if (inputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
                Log.d(TAG, "queueBuffers dequeueInputBuffer: " + inputBufferIndex);
            }

            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex;
            while ((outputBufferIndex = mMediaDecoder.dequeueOutputBuffer(bufferInfo, 0)) >= 0) {
                if (mOutFrameIndex++ % 100 == 0)
                    Log.d(TAG, "queueBuffers bufferInfo.size:" + bufferInfo.size);

                mMediaDecoder.releaseOutputBuffer(outputBufferIndex, true);
                //outputBufferIndex = mMediaDecoder.dequeueOutputBuffer(bufferInfo, 0);
            }
            if (outputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
                Log.d(TAG, "queueBuffers dequeueOutputBuffer: " + inputBufferIndex);
            }
        } catch (Throwable t) {
            t.printStackTrace();
        }
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
