package com.allcam.allplayer;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import com.allcam.allplayer.listener.OnPlaybackListener;
import com.allcam.allplayer.listener.OnPlayerstateListener;
import com.allcam.allplayer.listener.OnPreparedListener;
import com.allcam.allplayer.listener.OnReleasedListener;
import com.allcam.allplayer.listener.OnResetListener;
import com.allcam.allplayer.listener.OnStoppedListener;
import com.allcam.allplayer.listener.OnYUVDataListener;
import com.allcam.allplayer.playConfig.PlayConfig;
import com.allcam.allplayer.utils.LogUtils;
import com.allcam.allplayer.utils.VideoUtils;
import com.allcam.allplayer.waterMark.VideoWaterMark;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.libsdl.app.R;
import org.libsdl.app.SDL;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import javax.sql.CommonDataSource;

public class AllPlayerJni {
    private static final String TAG = "AllPlayerJava";

    private static volatile boolean mIsLibLoaded = false;
    private OnPreparedListener mOnPreparedListener;
    private OnYUVDataListener mOnYUVDataListener;
    private OnStoppedListener mOnStoppedListener;
    private OnResetListener mOnResetListener;
    private OnReleasedListener mOnReleasedListener;
    private OnPlayerstateListener mPlayerStateListener;

    private OnPlaybackListener mOnPlaybackListener;

    private static String businessType = "BusinessType";
    private static String rtspUrl = "RtspUrl";
    private static String bussinessIdArr = "BussinessIdArr";
    private static String pos = "Pos";

    //1，加载 so 初始化allplayer（）
    public static void loadLibrariesOnce(AllPlayerLibLoader libLoader, Context context) {
        Log.e(TAG, "执行AllPlayerJni loadLibrariesOnce()");

        synchronized (AllPlayerLibLoader.class) {
            if (!mIsLibLoaded) {
                if (libLoader == null) libLoader = sLocalLibLoader;

                libLoader.loadLibrary("native-lib");
                libLoader.loadLibrary("avcodec");
                libLoader.loadLibrary("avdevice");
                libLoader.loadLibrary("avfilter");
                libLoader.loadLibrary("avformat");
                libLoader.loadLibrary("avutil");
                libLoader.loadLibrary("swresample");
                libLoader.loadLibrary("swscale");
                libLoader.loadLibrary("hidapi");
                libLoader.loadLibrary("SDL2");
                libLoader.loadLibrary("ssl");
                libLoader.loadLibrary("crypto");
                libLoader.loadLibrary("main");
                libLoader.loadLibrary("fontconfig");
                libLoader.loadLibrary("postproc");
                mIsLibLoaded = true;
                SDL.setupJNI();

                // Initialize state
                SDL.initialize();

                SDL.setContext(context.getApplicationContext());

                if (libLoader == null) {
                    libLoader = sLocalLibLoader;
                }
                Log.e(TAG, "loadLibrariesOnce()初始化成功");

            }
        }
    }

    private static final AllPlayerLibLoader sLocalLibLoader = new AllPlayerLibLoader() {
        @Override
        public void loadLibrary(String libName) throws UnsatisfiedLinkError, SecurityException {
            System.loadLibrary(libName);
        }
    };

    //2,初始化播放器，只能调用调用一次
    public static void init(String logPath, int logLevel) {
        Log.e(TAG, "执行AllPlayerJni nativieInit()");

        nativieInit(logPath, logLevel);
    }

    public static native void nativieInit(String logPath, int logLevel);

    // 3，本地播放
    public native int nativeLocalPlay(String url, long bussiness);

    public native void nativeLocalStop(long bussiness);

    /**
     * 停止
     *
     * @param bussiness
     * @param bReal
     */
    public int nativeStop(long bussiness, boolean bReal) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", bReal ? String.valueOf(BussinessType.TYPE_REALVIDEO_STOP) : String.valueOf(BussinessType.TYPE_NETRECORD_STOP));
            jsonObject.putOpt("BusinessHwnd", nativeBusinessHwnd(bussiness));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }


    public native void nativeClear(long bussiness);

    public native void nativeSurfaceChange(long bizId, Object surface, int width, int height, boolean surfaceChange);

    //4，释放
    private native void nativeRelease();

    /**
     * 倍速播放
     *
     * @param bussiness
     * @param start
     * @param scale
     * @return
     */
    public int nativeVcrControl(long bussiness, double start, double scale, int platType) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_NETRECORD_CONTROL));
            jsonObject.putOpt("PlayScale",  String.valueOf(scale));
            jsonObject.putOpt("NptPos",  String.valueOf(start));
            if (platType == 9 && scale > 0) {
                jsonObject.putOpt("ScaleOrSpeed", 1);
            } else {
                jsonObject.putOpt("ScaleOrSpeed", 0);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 视频暂停 （TYPE_REALVIDEO_PAUSE：实时浏览，TYPE_NETRECORD_PAUSE：录像回放）
     */
    public int nativePause(long bussiness, boolean isReplay) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", isReplay ? String.valueOf(BussinessType.TYPE_NETRECORD_PAUSE) : String.valueOf(BussinessType.TYPE_REALVIDEO_PAUSE));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 继续播放
     *
     * @param bussiness
     * @param isReplay  TYPE_NETRECORD_RESUME:录像回放继续,TYPE_URL_RESUE:实时浏览
     * @return
     */
    public int nativeResume(long bussiness, boolean isReplay) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", isReplay ? String.valueOf(BussinessType.TYPE_NETRECORD_RESUME) : String.valueOf(BussinessType.TYPE_REALVIDEO_RESUME));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 抓拍
     * @param bussiness
     * @param path
     * @return
     */
    public int capture(long bussiness, String path){
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_CAPTURE));
            jsonObject.putOpt("CapturePath", path);
            jsonObject.putOpt("CaptureFormat", "0");
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }


    /**
     * 开始录像
     *
     * @param bussiness
     * @param path      下载路径
     */
    public int startRecord(long bussiness, String path) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_LOCAL_RECORD_START));
            jsonObject.putOpt("RecordDownloadPath", path);
            jsonObject.putOpt("DownloadCutFormat", "1");
            jsonObject.putOpt("DownloadCutSize", "1024");
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 结束录像
     *
     * @param bussiness
     * @return
     */
    public int endRecord(long bussiness) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_LOCAL_RECORD_STOP));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }


    /**
     * 窗口句柄
     *
     * @param bussiness
     * @return
     */
    public native String nativeBusinessHwnd(long bussiness);

    /**
     *
     * @param bussiness
     * @return
     */
    public native void nativeCallback(long bussiness);


    /**
     * 声音开关
     *
     * @param bussiness
     * @param open      是否开启声音
     * @return
     */
    public static int openVoice(long bussiness, boolean open) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_VOLUME_CONTROL));
            jsonObject.putOpt("VolumeValue", open ? "100" : "0");
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 开始录像回放
     *
     * @param playConfig 需要播放的信息
     * @return
     */
    public int nativeRecordPlay(PlayConfig playConfig) {
        long bussiness = playConfig.getBussinessId();
        int playMode = playConfig.getPlayMode();
        String url = playConfig.getUrl();
        int crashFrames = playConfig.getCrashFrames();

        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_NETRECORD_START));
            jsonObject.putOpt("RtspUrl", url);
            jsonObject.putOpt("RealOrQuality", playMode);
            jsonObject.putOpt("CacheSize", crashFrames / 25);
            jsonObject.putOpt("BusinessHwnd", nativeBusinessHwnd(bussiness));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        nativeCallback(bussiness);
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 开始实时浏览
     *
     * @param playConfig 需要播放的信息
     * @return
     */
    public int nativePlay(PlayConfig playConfig) {
        long bussiness = playConfig.getBussinessId();
        int playMode = playConfig.getPlayMode();
        String url = playConfig.getUrl();
        int crashFrames = playConfig.getCrashFrames();

        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_REALVIDEO_START));
            jsonObject.putOpt("RtspUrl", url);
            jsonObject.putOpt("RealOrQuality", playMode);
            jsonObject.putOpt("CacheSize", crashFrames / 25);
            jsonObject.putOpt("BusinessHwnd", nativeBusinessHwnd(bussiness));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        nativeCallback(bussiness);
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }


    /**
     * 开始语音对讲
     *
     * @param url
     * @param bussiness
     */
    public static int startTalk(String url, long bussiness) {
        if (TextUtils.isEmpty(url)) {
            return -1;
        }
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt(businessType, String.valueOf(BussinessType.TYPE_AUDIO_TALK_START));
            jsonObject.putOpt(rtspUrl, url);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }


    /**
     * 结束语音对讲
     */
    public static int stopTalk(long bussiness) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt(businessType, String.valueOf(BussinessType.TYPE_AUDIO_TALK_STOP));
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        return commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    public static void sendAudio(byte[] data, int dataSize) {
        sendAudioFrame(data, dataSize);
    }

    public static native void sendAudioFrame(byte[] data, int dataSize);

    public static void stopAudio() {
        stopAudioRecord();
    }

    public static native void stopAudioRecord();

    /**
     * 获取 视频播放 的上下行流量
     *
     * @param bussiness
     * @param info
     * @return
     */
    public int getVideoPlayStatusInfo(long bussiness, StringBuffer info) {
        JSONObject jsonObject = new JSONObject();
        try {
            JSONArray jsonArray = new JSONArray();
            jsonArray.put(bussiness);
            jsonObject.putOpt(businessType, String.valueOf(BussinessType.TYPE_VIDEO_PLAY_INFO));
            jsonObject.putOpt(bussinessIdArr, jsonArray);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        if (null == info) {
            info = new StringBuffer();
        }
        return commonSendParams(bussiness, jsonDate, true, info);
    }


    /**
     * 获取摄像机的信息,通过Allplay获取
     *
     * @param bussiness
     * @param info
     * @return
     */
    public int getDeviceMediaInfo(long bussiness, StringBuffer info) {
        JSONObject jsonObject = new JSONObject();
        try {
            JSONArray jsonArray = new JSONArray();
            jsonArray.put(bussiness);
            jsonObject.putOpt(businessType, String.valueOf(BussinessType.TYPE_DEVICE_MEDIA_INFO));
            jsonObject.putOpt(bussinessIdArr, jsonArray);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        if (null == info) {
            info = new StringBuffer();
        }
        return commonSendParams(bussiness, jsonDate, true, info);
    }


    /**
     * 本地回放的跳转seek
     *
     * @param percentage double类型，用于跳转的百分比，假设时长时10s，那么跳转1s就传入0.1
     */
    public void localVideoSeek(long bussiness, double percentage) {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_URL_SEEK));
            jsonObject.putOpt("Pos", percentage);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        Log.d(TAG, "localVideoSeek: jsonDate = " + jsonDate);
        commonSendParams(bussiness, jsonDate, false, new StringBuffer());
    }

    /**
     * 设置全局的视频水印
     *
     * @param bussiness
     * @param videoWaterMark
     */
    public static int setWaterMarkInfo(Context context, long bussiness, VideoWaterMark videoWaterMark) {
        if (null == videoWaterMark) {
            return -1;
        }
        String fontFile = "";
        if (videoWaterMark.getRenderOn() == 1) { // 如果是开启的,那就去初始化.ttf字符集合。
            fontFile = doSaveTTF(context);
            if (TextUtils.isEmpty(fontFile)) {
                return -1;
            }
        }
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.putOpt("BusinessType", String.valueOf(BussinessType.TYPE_SET_WATERMARK));
            jsonObject.putOpt("Text", TextUtils.isEmpty(videoWaterMark.getText()) ? "" : videoWaterMark.getText());
            jsonObject.putOpt("FontColor", TextUtils.isEmpty(videoWaterMark.getFontColor()) ? "" : videoWaterMark.getFontColor());
            jsonObject.putOpt("FontSize", videoWaterMark.getFontSize());
            jsonObject.putOpt("Alpha", videoWaterMark.getAlpha());
            jsonObject.putOpt("FontFile", fontFile);
            jsonObject.putOpt("Position", videoWaterMark.getPosition());
            jsonObject.putOpt("LocalTime", videoWaterMark.getLocalTime());
            jsonObject.putOpt("RenderOn", videoWaterMark.getRenderOn());
            jsonObject.putOpt("Layout", videoWaterMark.getLayout());
        } catch (JSONException e) {
            e.printStackTrace();
        }
        String jsonDate = jsonObject.toString();
        Log.e(TAG, "setWaterMarkInfo: jsonDate = " + jsonDate);
        int code = commonSendParams(bussiness, jsonDate, false, new StringBuffer());
        return code;
    }

    public static native int commonSendParams(long bussiness, String jsonDate, boolean isNeedCallBack, Object stringBuffer);

    private String url;
    //    private boolean isReleased;
    // for video
    private int mVideoWidth;
    private int mVideoHeight;
    private String mFFmpegVideoCodecType = "";
    private boolean beVideoHardCodec;
    private Surface mSurface;
    private MediaCodec mVideoDecoder;
    private MediaFormat mVideoFormat;
    private MediaCodec.BufferInfo mVideoBufferInfo;

    private Handler mUIHandler;// 用以把回调切换到主线程，不占用工作线程资源
    private Handler mWorkHandler;
//    private HandlerThread mWorkThread;

    private static final int HANDLE_SET_DATA_SOURCE = 1;
    private static final int HANDLE_STOP = 2;
    private static final int HANDLE_RESET = 3;
    private static final int HANDLE_RELEASE = 4;
    // 调用 stop 或 reset 时可以直接 remove 的消息，注意不要把 HANDLE_CREATE_PLAYER 删除了
    private static final int[] NEED_REMOVE_WHEN_STOPPING = {HANDLE_SET_DATA_SOURCE};


    /**
     * logPath log 存储地址
     */
    public AllPlayerJni() {
        mUIHandler = new Handler(Looper.getMainLooper());
//        mWorkThread = new HandlerThread("WePlayer-dispatcher");
//        mWorkThread.start();
        mWorkHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                int msgType = msg.what;
                LogUtils.d(TAG, "mWorkHandler handleMessage: " + msgType);
                switch (msgType) {
                    case HANDLE_SET_DATA_SOURCE:
                        handleSetDataSource(msg);
                        break;

                    case HANDLE_STOP:
                        handleStop(msg);
                        break;
                    case HANDLE_RESET:
                        handleReset();
                        break;
                    case HANDLE_RELEASE:
                        handleRelease();
                        break;
                }
            }
        };
    }

    public void release() {
        nativeRelease();
        // 然后停止前驱：工作线程
        mWorkHandler.removeCallbacksAndMessages(null);
        Message msg = mWorkHandler.obtainMessage(HANDLE_RELEASE);
        mWorkHandler.sendMessage(msg);
        // 最后停止回调：工作结果
        mUIHandler.removeCallbacksAndMessages(null);
    }

    private void handleRelease() {
        nativeRelease();

        mWorkHandler.removeCallbacksAndMessages(null);

        mUIHandler.removeCallbacksAndMessages(null);

        if (mOnReleasedListener != null) {
            mUIHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnReleasedListener.onReleased();
                }
            });
        }
    }

    public void setOnPreparedListener(OnPreparedListener listener) {
        this.mOnPreparedListener = listener;
    }


    public void setOnYUVDataListener(OnYUVDataListener listener) {
        this.mOnYUVDataListener = listener;
    }


    public void setOnStoppedListener(OnStoppedListener listener) {
        this.mOnStoppedListener = listener;
    }

    public void setOnResetListener(OnResetListener listener) {
        this.mOnResetListener = listener;
    }

    public void setOnReleasedListener(OnReleasedListener listener) {
        this.mOnReleasedListener = listener;
    }

    public void setOnOnPlayerstateListener(OnPlayerstateListener listener) {
        this.mPlayerStateListener = listener;
    }

    public void setDataSource(String dataSource, int bussinessId, boolean real, int playMode, int crashFrames, int devicePlatType) {
        mWorkHandler.removeMessages(HANDLE_SET_DATA_SOURCE);// 以最新设置的源为准
        Message msg = mWorkHandler.obtainMessage(HANDLE_SET_DATA_SOURCE);
        PlayConfig playConfig = new PlayConfig(dataSource, bussinessId, real, playMode, crashFrames, devicePlatType);
        msg.obj = playConfig;
        msg.arg1 = bussinessId;//传整型
        mWorkHandler.sendMessage(msg);
    }

    public void setOnPlaybackListener(OnPlaybackListener mOnPlaybackListener) {
        this.mOnPlaybackListener = mOnPlaybackListener;
    }

    private void handleSetDataSource(Message msg) {
        PlayConfig dataSource = (PlayConfig) msg.obj;
        int bussinessId = msg.arg1;
        this.url = dataSource.getUrl();
        nativeRecordPlay(dataSource);
    }

    /**
     * called from native
     * ！！！注意：此回调处于 native 的锁中，不可以有其它过多操作，不可以调用 native 方法，以防死锁！！！
     */
    private void onNativePrepared(String dataSource, int videoWidth, int videoHeight) {

        if (!TextUtils.equals(dataSource, url)) {
            LogUtils.w(TAG, "onNativePrepared data source changed! So the preparation is invalid!");
            return;
        }

        mVideoWidth = videoWidth;
        mVideoHeight = videoHeight;


        if (mOnPreparedListener != null) {
            mUIHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnPreparedListener.onPrepared();
                }
            });
        }
    }

    /**
     * 播放的状态
     *
     * @param state
     */
    private void onNativePlayerState(final String msg, final int state) {
        if (mPlayerStateListener != null) {
            mPlayerStateListener.onChagnState(state, msg);
        }
    }

    /**
     * 回放时间
     *
     * @param dataCall
     */
    private void onNativePlayerBackTime(final int dataCall) {
        if (mOnPlaybackListener != null) {
            mOnPlaybackListener.onDataCall(dataCall);
        }
    }

    public void reStop(int bussinessId) {
        beVideoHardCodec = false;

        nativeStop(bussinessId, false);
    }

    private void handleStop(Message msg) {
        int bussinessId = msg.arg1;
        nativeStop(bussinessId, false);
        if (mOnStoppedListener != null) {
            mUIHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnStoppedListener.onStopped();
                }
            });
        }
    }

    /**
     * Resets the MediaPlayer to its uninitialized state. After calling
     * this method, you will have to initialize it again by setting the
     * data source and calling prepare().
     */
    public void reset() {
        // 先清除其它所有未执行消息，再执行具体重置动作
        // 不要使用 removeCallbacksAndMessages(null)，避免清除 HANDLE_CREATE_PLAYER
        for (int msg : NEED_REMOVE_WHEN_STOPPING) {
            mWorkHandler.removeMessages(msg);
        }

        Message msg = mWorkHandler.obtainMessage(HANDLE_RESET);
        mWorkHandler.sendMessage(msg);
    }

    private void handleReset() {
        // 在彻底停止数据回调后调用一次清屏
        if (mOnResetListener != null) {
            mUIHandler.post(new Runnable() {
                @Override
                public void run() {
                    mOnResetListener.onReset();
                }
            });
        }
    }


    /**
     * Gets the current playback position.
     *
     * @return the current position in milliseconds
     */
    public int getCurrentPosition() {

        return 0;
    }


    public void setSurface(Surface surface) {
        if (surface == null) throw new NullPointerException("surface can't be null");
        this.mSurface = surface;
    }

    /**
     * 用于在视频资源 OnPrepared 之后获取视频宽度
     */
    public int getVideoWidthOnPrepared() {

        return mVideoWidth;
    }

    /**
     * 用于在视频资源 OnPrepared 之后获取视频高度
     */
    public int getVideoHeightOnPrepared() {

        return mVideoHeight;
    }

    private void onNativeYUVDataCall(int width, int height, byte[] y, byte[] u, byte[] v) {
        // 直接在本线程中回调，之后 native 内部数据会回收
        Log.e("lhf", "onNativeYUVDataCall time = " + System.currentTimeMillis());
        if (mOnYUVDataListener != null) {
            mOnYUVDataListener.onYUVData(width, height, y, u, v);
        }
    }

    /**
     * 供 Native 层判断是否支持硬解码
     *
     * @param ffmpegCodecType ffmpeg 层的解码器类型
     * @return true 支持硬解
     */
    private boolean onNativeCheckVideoHardCodec(String ffmpegCodecType) {
        this.mFFmpegVideoCodecType = ffmpegCodecType;
        return VideoUtils.isSupportHardCodec(ffmpegCodecType);
    }

    /**
     * Native 通知 Java 层初始化硬解码器
     *
     * @param ffmpegCodecType ffmpeg 层的解码器类型
     * @param width           视频宽度
     * @param height          视频高度
     * @param csd0
     * @param csd1
     * @return true 初始化成功
     */
    private boolean onNativeInitVideoHardCodec(String ffmpegCodecType, int width, int height, byte[] csd0, byte[] csd1) {
        if (mSurface == null) {
            LogUtils.e(TAG, "onNativeInitVideoHardCodec mSurface is null!");
            return false;
        }

        String mime = VideoUtils.findHardCodecType(ffmpegCodecType);
        try {
            mVideoDecoder = MediaCodec.createDecoderByType(mime);
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
        if (mVideoDecoder == null) {
            LogUtils.e(TAG, "MediaCodec create VideoDecoder failed!");
            return false;
        }

        mVideoFormat = MediaFormat.createVideoFormat(mime, width, height);
        mVideoFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);
        mVideoFormat.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
        mVideoFormat.setByteBuffer("csd-1", ByteBuffer.wrap(csd1));

        try {
            mVideoDecoder.configure(mVideoFormat, mSurface, null, 0);
        } catch (Exception e) {
            e.printStackTrace();
            mVideoDecoder.release();
            mVideoDecoder = null;
            mVideoFormat = null;
            return false;
        }

        mVideoBufferInfo = new MediaCodec.BufferInfo();
        mVideoDecoder.start();
        return true;
    }

    /**
     * 最终由 Native 层判定是否可以硬解码
     *
     * @param hardCodec true 可以硬解码
     */
    private void onNativeSetVideoHardCodec(boolean hardCodec) {
        this.beVideoHardCodec = hardCodec;
    }

    public boolean isVideoHardCodec() {
        return beVideoHardCodec;
    }

    public String getVideoCodecType() {
        return mFFmpegVideoCodecType;
    }

    private void onNativeVideoPacketCall(int packetSize, byte[] packet) {
        if (mVideoDecoder == null) {
            LogUtils.e(TAG, "onNativeVideoPacketCall mVideoDecoder == null");
            return;
        }
        if (packetSize <= 0 || packet == null || packet.length < packetSize) {
            LogUtils.e(TAG, "onNativeVideoPacketCall but params is invalid: packetSize=" + packetSize + ", packet=" + packet);
            return;
        }

        // 获取输入 buffer，超时等待
        int inputBufferIndex = mVideoDecoder.dequeueInputBuffer(10);
        if (inputBufferIndex < 0) {
            LogUtils.e(TAG, "mVideoDecoder dequeueInputBuffer failed inputBufferIndex=" + inputBufferIndex);
            return;
        }

        // 成功获取输入 buffer后，填入要处理的数据
        ByteBuffer inputBuffer = mVideoDecoder.getInputBuffers()[inputBufferIndex];
        inputBuffer.clear();
        inputBuffer.put(packet);
        // 填完输入数据后，释放输入 buffer
        mVideoDecoder.queueInputBuffer(inputBufferIndex, 0, packetSize, 0, 0);

        // 获取输出 buffer，超时等待
        int ouputBufferIndex = mVideoDecoder.dequeueOutputBuffer(mVideoBufferInfo, 10);
        while (ouputBufferIndex >= 0) {// 可能一次获取不完，需要多次
//            ByteBuffer outputBuffer = mVideoDecoder.getOutputBuffers()[ouputBufferIndex];
            // do nothing?
            // releaseOutputBuffer
            // * @param render If a valid surface was specified when configuring the codec,
            // *               passing true renders this output buffer to the surface.
            mVideoDecoder.releaseOutputBuffer(ouputBufferIndex, true);
            ouputBufferIndex = mVideoDecoder.dequeueOutputBuffer(mVideoBufferInfo, 10);
        }
    }

    /**
     * 涉及多线程操作，由 Native 层统一调用停止时的资源释放
     */
    private void onNativeStopVideoHardCodec() {
        LogUtils.w(TAG, "onNativeStopVideoHardCodec...");
        if (mVideoDecoder != null) {
            mVideoDecoder.flush();
            mVideoDecoder.stop();
            mVideoDecoder.release();
            mVideoDecoder = null;
        }
        mVideoFormat = null;
        mVideoBufferInfo = null;
    }

    private static String doSaveTTF(Context context) {
        File filesDir = context.getFilesDir();
        File puhuitiMiniPath = new File(filesDir, "allplayer.ttf");
        String filePath = null;
        //判断该文件存不存在
        if (!puhuitiMiniPath.exists()) {
            //如果不存在，开始写入文件
            filePath = copyFilesFromRaw(context, R.font.allplayer, "allplayer.ttf", context.getFilesDir().getAbsolutePath());
        } else {
            return puhuitiMiniPath.getAbsolutePath();
        }
        return filePath;
    }

    private static String copyFilesFromRaw(Context context, int id, String fileName, String storagePath) {
        InputStream inputStream = context.getResources().openRawResource(id);
        storagePath = storagePath + File.separator + fileName;

        File file = new File(storagePath);
        try {
            if (!file.exists()) {
                // 1.建立通道对象
                FileOutputStream fos = new FileOutputStream(file);
                // 2.定义存储空间
                byte[] buffer = new byte[inputStream.available()];
                // 3.开始读文件
                int lenght = 0;
                while ((lenght = inputStream.read(buffer)) != -1) {// 循环从输入流读取buffer字节
                    // 将Buffer中的数据写到outputStream对象中
                    fos.write(buffer, 0, lenght);
                }
                fos.flush();// 刷新缓冲区
                // 4.关闭流
                fos.close();
                inputStream.close();
                return storagePath;
            } else {
                return storagePath;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

}
