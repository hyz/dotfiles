package com.qjie.study;

import android.animation.TimeAnimator;
import android.annotation.SuppressLint;
import android.graphics.ImageFormat;
import android.graphics.Paint;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

@SuppressLint("NewApi")
public class CameraActivity extends ActionBarActivity implements SurfaceHolder.Callback, Camera.PreviewCallback, OnClickListener {

    private static final String TAG = "StudyCamera";
    private static final int FRAME_RATE = 15;
    private static final String REMOTE_HOST = "192.168.1.105";
    private static final short REMOTE_HOST_PORT = 5000;
    private static final String H264FILE = "/sdcard/a.h264";

    private boolean bOpening = false;
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;

    private Surface mSurface;
    private Camera mCamera;
    private int mCameraWidth, mCameraHeight;
    private int mSurfaceWidth = 480, mSurfaceHeight = 320;
    private MediaCodec mMediaEncoder;
    private MediaCodec mMediaDecoder;
    private Paint paint;

    private InetAddress address;
    private DatagramSocket socket;
    private UdpSendTask netSendTask;
    private H264FileTask h264FileTask;

    private int mFrameIndex = 0;
    private int mOutFrameIndex = 0;
    private byte[] mEncoderH264Buf;
    private byte[] mMediaHead = null;

    private byte[] mYuvBuffer = new byte[1280 * 720 * 3 / 2];

    TimeAnimator mTimeAnimator = new TimeAnimator();

    /**
     * Returns the first codec capable of encoding the specified MIME type, or null if no
     * match was found.
     */
    private static MediaCodecInfo selectCodec(String mimeType) {
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);

            if (!codecInfo.isEncoder()) {
                continue;
            }

            String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                    return codecInfo;
                }
            }
        }
        return null;
    }

    /**
     * Returns a color format that is supported by the codec and by this test code.  If no
     * match is found, this throws a test failure -- the set of formats known to the test
     * should be expanded for new platforms.
     */
    private static int selectColorFormat(MediaCodecInfo codecInfo, String mimeType) {
        MediaCodecInfo.CodecCapabilities capabilities = codecInfo.getCapabilitiesForType(mimeType);
        for (int i = 0; i < capabilities.colorFormats.length; i++) {
            int colorFormat = capabilities.colorFormats[i];
            if (isRecognizedFormat(colorFormat)) {
                return colorFormat;
            }
        }
        Log.e(TAG, "couldn't find a good color format for " + codecInfo.getName() + " / " + mimeType);
        return 0;   // not reached
    }

    /**
     * Returns true if this is a color format that this test code understands (i.e. we know how
     * to read and generate frames in this format).
     */
    private static boolean isRecognizedFormat(int colorFormat) {
        switch (colorFormat) {
            // these are the formats we know how to handle for this test
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
                return true;
            default:
                return false;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.activity_camera);
        findViewById(R.id.btnOpen).setOnClickListener(this);
        findViewById(R.id.btnClose).setOnClickListener(this);

        mCameraWidth = 640;
        mCameraHeight = 480;
        if (!setupView()) {
            Log.e(TAG, "failed to setupView");
            return;
        }
        mMediaEncoder = null;
        mMediaDecoder = null;
        mSurface = null;
    }

    @Override
    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.btnOpen: {
//                paint = new Paint();
//                mEncoderH264Buf = new byte[10240];
//                netSendTask = new UdpSendTask();
//                netSendTask.init();
//                netSendTask.start();

//				 if(!setupCamera()){
//					Log.e(TAG, "failed to setupCamera");
//					return;
//				}
//
//				if(!startCamera()){
//					Log.e(TAG, "failed to openCamera");
//					return;
//				}

                //play h264 file test
                startPlayH264File();
            }
            break;
            case R.id.btnClose: {
                if (!stopCamera()) {
                    Log.e(TAG, "failed to stopCamera");
                    return;
                }
            }
            break;
        }
        view.setClickable(false);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, "surfaceCreated.");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
        Log.i(TAG, "surfaceChanged w:" + width + " h:" + height);
        mSurface = surfaceHolder.getSurface();

        //mSurfaceWidth = width;
        //mSurfaceHeight = height;

        /*
         * get canvas from surface holder with draw something
         * */
//        Surface surface = holder.getSurface();
//        if(!setupDecoder(surface,"video/avc",mCameraWidth,mCameraHeight)){
//			releaseCamera();
//			Log.e(TAG, "failed to setupEncoder");
//			return;
//		}

//      Canvas c = holder.lockCanvas();
//		paint.setColor(Color.WHITE);
//		paint.setStrokeWidth(4);
//		c.drawRect(0, 0, width, height, paint);
//		paint.setColor(Color.BLACK);
//		paint.setTextSize(30);
//		paint.setStrokeWidth(1);
//		c.drawText("http://blog.csdn.net/", 100, 100, paint);


//        Bitmap bitmap = null;
//        bitmap = BitmapFactory.decodeResource(getResources(), R.drawable.waiting);
//        if (bitmap != null) {
//            float left=(c.getWidth()-bitmap.getWidth())/2;
//            float top=(c.getHeight()-bitmap.getHeight())/2;
//            c.drawBitmap(bitmap, left, top, paint);
//        }
//
//        holder.unlockCanvasAndPost(c);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "surfaceDestroyed");
        mSurface = null;
        releaseCamera();

        if (mTimeAnimator != null && mTimeAnimator.isRunning()) {
            mTimeAnimator.end();
        }
    }

    @Override
    public void onPreviewFrame(byte[] rawData, Camera camera) {
        int w = camera.getParameters().getPreviewSize().width;
        int h = camera.getParameters().getPreviewSize().height;
        int format = camera.getParameters().getPreviewFormat();
        Log.d(TAG, "preview frame format:" + format + " size:" + rawData.length + " w:" + w + " h:" + h);

        if (mMediaEncoder == null) {
            if (!setupEncoder("video/avc", w, h)) {
                releaseCamera();
                Log.e(TAG, "failed to setupEncoder");
                return;
            }
        }

        assert (mSurface != null);
        if (mMediaDecoder == null) {
            if (!setupDecoder(mSurface, "video/avc", w, h)) {
                releaseCamera();
                Log.e(TAG, "failed to setupDecoder");
                return;
            }
        }

        //convert yv12 to i420
        swapYV12toI420(rawData, mYuvBuffer, w, h);
        System.arraycopy(mYuvBuffer, 0, rawData, 0, rawData.length);

        //set h264 buffer to zero.
        for (int i = 0; i < mEncoderH264Buf.length; i++)
            mEncoderH264Buf[i] = 0;
        int encoderRet = offerEncoder(rawData, mEncoderH264Buf);
        if (encoderRet > 0) {
            Log.d(TAG, "encoder output h264 buffer len:" + encoderRet);
            /**
             * send to VLC client by udp://@port;
             */
            netSendTask.pushBuf(mEncoderH264Buf, encoderRet);

            offerDecoder(mEncoderH264Buf, 0, encoderRet, 0);
        }

        //reset buff to camera.
        camera.addCallbackBuffer(rawData);
    }

    private boolean setupView() {
        Log.d(TAG, "fall in setupView");

        if (null != surfaceHolder) {
            surfaceHolder.removeCallback(this);
            surfaceView = null;
        }
        if (null != surfaceView) {
            surfaceView = null;
        }

        surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        surfaceHolder.addCallback(this);
        return true;
    }

    private boolean setupCamera() {
        Log.d(TAG, "fall in setupCamera");
        if (null != mCamera) {
            mCamera.release();
            mCamera = null;
        }

        mCamera = Camera.open(); // Turn on the camera

        Camera.Parameters parameters = mCamera.getParameters(); // Camera parameters to obtain
        List<Size> listSize = parameters.getSupportedVideoSizes();
        for (int i = 0; listSize != null && i < listSize.size(); i++) {
            Size size = listSize.get(i);
            Log.d(TAG, "supportedSize:" + size.width + "-" + size.height);
        }

        int width = mCameraWidth, height = mCameraHeight;

        List<Integer> listFormats = parameters.getSupportedPreviewFormats();
        for (int i = 0; i < listFormats.size(); i++) {
            Integer format = listFormats.get(i);
            Log.d(TAG, "supportedFormat:" + format);
        }

        parameters.setFlashMode("off"); // �������
        parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
        parameters.setSceneMode(Camera.Parameters.SCENE_MODE_AUTO);
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
        //parameters.setPreviewFormat(ImageFormat.NV21);
        parameters.setPreviewFormat(ImageFormat.YV12);
        parameters.setPreviewSize(width, height);
        parameters.setPictureSize(width, height);

        Log.d(TAG, "setup Camera w:" + width + " h:" + height);
        mCamera.setParameters(parameters); // Setting camera parameters


        //alloc buffer for camera callback data with once.
        byte[] rawBuf = new byte[mCameraWidth * mCameraHeight * 3 / 2];
        mCamera.addCallbackBuffer(rawBuf);

		/*set the buffer must be use setPreviewCallbackWithBuffer*/
        //mCamera.setPreviewCallback(this);
        mCamera.setPreviewCallbackWithBuffer(this);

        //unused preview display to surface view by first;
		/*
		try {
            mCamera.setPreviewDisplay(surfaceHolder); // Set Preview
            mCamera.setDisplayOrientation(90);
        } catch (IOException e) {
        	Log.e(TAG,"failed to setPreviewDisplay");
            mCamera.release();// release camera
            mCamera = null;
            return false;
        }
		*/

        return true;
    }

    private boolean startCamera() {
        Log.d(TAG, "fall in startCamera");
        if (bOpening) return false;
        mCamera.startPreview(); // Start Preview
        bOpening = true;
        return true;
    }

    private boolean stopCamera() {
        Log.d(TAG, "fall in stop Camera");
        if (!bOpening) return false;

        mCamera.stopPreview();// stop preview
        bOpening = false;
        return true;
    }

    private boolean releaseCamera() {
        if (mCamera == null)
            return true;
        Log.d(TAG, "fall in release Camera");
        mCamera.stopPreview();
        mCamera.release(); // Release camera resources
        mCamera = null;
        return true;
    }

    private boolean setupEncoder(String mime, int width, int height) {
        int colorFormat = selectColorFormat(selectCodec(mime), mime);
        Log.d(TAG, "setupEncoder " + mime + " colorFormat:" + colorFormat + " w:" + width + " h:" + height);

        mMediaEncoder = MediaCodec.createEncoderByType(mime);
        MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, 125000);
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
        //mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar);
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 5);
        mMediaEncoder.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mMediaEncoder.start();

        return true;
    }

    private boolean setupDecoder(Surface surface, String mime, int width, int height) {
        Log.d(TAG, "setupDecoder surface:" + surface + " mime:" + mime + " w:" + width + " h:" + height);

        MediaFormat mediaFormat = MediaFormat.createVideoFormat(mime, width, height);
        mMediaDecoder = MediaCodec.createDecoderByType(mime);
        if (mMediaDecoder == null) {
            Log.e("DecodeActivity", "createDecoderByType fail!");
            return false;
        }

		/*
		int codecCount = MediaCodecList.getCodecCount();
		for(int i=0;i<codecCount;i++){
			MediaCodecInfo info =MediaCodecList.getCodecInfoAt(i);
			Log.d(TAG,"codec:"+info.getName());
		}*/

        mMediaDecoder.configure(mediaFormat, surface, null, 0);
        mMediaDecoder.start();

        return true;
    }

    private int offerEncoder(byte[] input, byte[] output) {

        int pos = 0;
        try {
            ByteBuffer[] inputBuffers = mMediaEncoder.getInputBuffers();
            ByteBuffer[] outputBuffers = mMediaEncoder.getOutputBuffers();
            int inputBufferIndex = mMediaEncoder.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                Log.d(TAG, "offerEncoder InputBufSize: " + inputBuffer.capacity() + " inputSize: " + input.length + " bytes");
                inputBuffer.clear();
                inputBuffer.put(input);
                mMediaEncoder.queueInputBuffer(inputBufferIndex, 0, input.length, 0, 0);
            }

            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex = mMediaEncoder.dequeueOutputBuffer(bufferInfo, 0);
            while (outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];

                byte[] data = new byte[bufferInfo.size];
                outputBuffer.get(data);

                Log.d(TAG, "offerEncoder InputBufSize:" + outputBuffer.capacity() + " outputSize:" + data.length + " bytes written");

                if (mMediaHead != null) {
                    System.arraycopy(data, 0, output, pos, data.length);
                    pos += data.length;
                } else // ����pps sps ֻ�п�ʼʱ ��һ��֡���У� ��������������
                {
                    Log.d(TAG, "offer Encoder save sps head,len:" + data.length);
                    ByteBuffer spsPpsBuffer = ByteBuffer.wrap(data);
                    if (spsPpsBuffer.getInt() == 0x00000001) {
                        mMediaHead = new byte[data.length];
                        System.arraycopy(data, 0, mMediaHead, 0, data.length);
                    } else {
                        Log.e(TAG, "not found media head.");
                        return -1;
                    }
                }

                mMediaEncoder.releaseOutputBuffer(outputBufferIndex, false);
                outputBufferIndex = mMediaEncoder.dequeueOutputBuffer(bufferInfo, 0);
            }

            if (output[4] == 0x65) //key frame   ���������ɹؼ�֡ʱֻ�� 00 00 00 01 65 û��pps sps�� Ҫ����
            {
                System.arraycopy(output, 0, input, 0, pos);
                System.arraycopy(mMediaHead, 0, output, 0, mMediaHead.length);
                System.arraycopy(input, 0, output, mMediaHead.length, pos);
                pos += mMediaHead.length;
            }

        } catch (Throwable t) {
            t.printStackTrace();
        }
        return pos;
    }

    private void offerDecoder(byte[] input, int offs, int length, int flags) {
        try {
            ByteBuffer[] inputBuffers = mMediaDecoder.getInputBuffers();
            int inputBufferIndex = mMediaDecoder.dequeueInputBuffer(100000);
            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                long timestamp = mFrameIndex++ * 1000000 / FRAME_RATE;
                if (mFrameIndex % 100 == 0)
                    Log.d(TAG, "offerDecoder timestamp: " + timestamp + " inputSize: " + length + " bytes");
                inputBuffer.clear();
                inputBuffer.put(input, offs, length);
                mMediaDecoder.queueInputBuffer(inputBufferIndex, 0, length, timestamp, flags);

            } else if (inputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
                Log.d(TAG, "offerDecoder dequeueInputBuffer: " + inputBufferIndex);
            }

            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex;
            while ((outputBufferIndex = mMediaDecoder.dequeueOutputBuffer(bufferInfo, 0)) >= 0) {
                if (mOutFrameIndex++ % 100 == 0)
                    Log.d(TAG, "offerDecoder bufferInfo.size:" + bufferInfo.size);

                //If a valid surface was specified when configuring the codec,
                //passing true renders this output buffer to the surface.
                mMediaDecoder.releaseOutputBuffer(outputBufferIndex, true);
                //outputBufferIndex = mMediaDecoder.dequeueOutputBuffer(bufferInfo, 0);
            }
            if (outputBufferIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
                Log.d(TAG, "offerDecoder dequeueOutputBuffer: " + inputBufferIndex);
            }
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    private void startPlayH264File() {
        assert (mSurface != null);
        if (mMediaDecoder == null) {
            if (!setupDecoder(mSurface, "video/avc", mSurfaceWidth, mSurfaceHeight)) {
                Log.e(TAG, "failed to setupDecoder");
                return;
            }
        }

        //h264FileTask = new H264FileTask();
        //h264FileTask.start();
        mTimeAnimator.setTimeListener(new H264FileTask2());
        mTimeAnimator.start();
    }

    private void swapYV12toI420(byte[] yv12bytes, byte[] i420bytes, int width, int height) {
        System.arraycopy(yv12bytes, 0, i420bytes, 0, width * height);
        System.arraycopy(yv12bytes, width * height + width * height / 4, i420bytes, width * height, width * height / 4);
        System.arraycopy(yv12bytes, width * height, i420bytes, width * height + width * height / 4, width * height / 4);
    }

    public static int indexOf(byte[] largeArray, byte[] subArray, int ix) {
        int ilast = largeArray.length - subArray.length;
        for_:
        for (; ix <= ilast; ix++) {
            for (int j = 0; j < subArray.length; j++) {
                if (subArray[j] != largeArray[ix + j]) {
                    continue for_;
                }
            }
            return ix;
        }
        return -1;
    }

    private void readH264FromFile() {
        try {
            File file = new File(H264FILE);
            if (!file.exists() || !file.canRead()) {
                Log.e(TAG, "failed to open h264 file.");
                return;
            }

            byte[] buf = new byte[(int) file.length()];//byte[] fileBuf = new byte[1024];
            FileInputStream fis = new FileInputStream(file);
            DataInputStream dis = new DataInputStream(fis);
            dis.readFully(buf);
            dis.close();
            dis = null;

            byte[] sc = new byte[4];
            sc[0] = sc[1] = sc[2] = 0;
            sc[3] = 1;

            int ix;
            if ( (ix = indexOf(buf, sc, +4)) > 0
                    && (ix = indexOf(buf, sc, ix+4)) > 0) {
                offerDecoder(buf, 0, ix, MediaCodec.BUFFER_FLAG_CODEC_CONFIG);

                for (int ibeg = ix; (ix = indexOf(buf, sc, ix + 4)) > 0; ibeg = ix) {
                    offerDecoder(buf, ibeg, ix - ibeg, 0);
                }
            }

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    class H264FileTask extends Thread {
        @Override
        public void run() {
            Log.d(TAG, "fall in H264File Read thread");
            readH264FromFile();
        }
    }
    class H264FileTask2 implements TimeAnimator.TimeListener {
        byte[] fileBuf = null;
        int index = 0;
        @Override
        public void onTimeUpdate(TimeAnimator timeAnimator, long l, long l1) {
            byte[] startBytes = new byte[4];
            startBytes[0] = startBytes[1] = startBytes[2] = 0;
            startBytes[3] = 1;
            try {
                if (index == 0) {
                    File file = new File(H264FILE);
                    if (!file.exists() || !file.canRead()) {
                        Log.e(TAG, "failed to open h264 file.");
                        return;
                    }
                    fileBuf = new byte[(int) file.length()];//byte[] fileBuf = new byte[1024];
                    FileInputStream fis = new FileInputStream(file);
                    DataInputStream dis = new DataInputStream(fis);
                    dis.readFully(fileBuf);
                    dis.close();
                    dis = null;

                    if ((index = indexOf(fileBuf, startBytes, +4)) > 0
                            && (index = indexOf(fileBuf, startBytes, index + 4)) > 0) {
                        offerDecoder(fileBuf, 0, index, MediaCodec.BUFFER_FLAG_CODEC_CONFIG);
                    }
                }
                if (index > 0) {
                    int ibeg = index;
                    if ((index = indexOf(fileBuf, startBytes, index + 4)) > 0) {
                        offerDecoder(fileBuf, ibeg, index - ibeg, 0);
                    }
                }
            }catch (IOException e) {
                e.printStackTrace();
            }
        }
    };

    class UdpSendTask extends Thread {
        private ArrayList<ByteBuffer> mList;

        public void init() {
            try {
                socket = new DatagramSocket();
                address = InetAddress.getByName(REMOTE_HOST);
            } catch (SocketException e) {
                e.printStackTrace();
            } catch (UnknownHostException e) {
                e.printStackTrace();
            }

            mList = new ArrayList<ByteBuffer>();

        }

        public void pushBuf(byte[] buf, int len) {
            ByteBuffer buffer = ByteBuffer.allocate(len);
            buffer.put(buf, 0, len);
            mList.add(buffer);
        }

        @Override
        public void run() {
            Log.d(TAG, "fall in udp send thread");
            while (true) {
                if (mList.size() <= 0) {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                while (mList.size() > 0) {
                    ByteBuffer sendBuf = mList.get(0);
                    try {
                        Log.d(TAG, "send udp packet len:" + sendBuf.capacity());
                        DatagramPacket packet = new DatagramPacket(sendBuf.array(), sendBuf.capacity(), address, REMOTE_HOST_PORT);
                        socket.send(packet);
                    } catch (Throwable t) {
                        t.printStackTrace();
                    }
                    mList.remove(0);
                }
            }
        }
    }
}
