package com.allcam.allplayer;

import android.content.Context;
import android.content.res.TypedArray;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.SurfaceHolder;

import com.allcam.allplayer.listener.OnPlaybackListener;
import com.allcam.allplayer.listener.OnPlayerstateListener;
import com.allcam.allplayer.playConfig.PlayConfig;
import com.allcam.allplayer.utils.LogUtils;

import org.libsdl.app.R;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class AllVideoView extends GLSurfaceView implements SurfaceHolder.Callback, GLSurfaceView.Renderer {
    final int STREAM_STATUS_SETUP = 20012;        //RTSP SETUP成功，可PLAY

    public void setBussinessId(int bussinessId) {
        this.bussinessId = bussinessId;
    }

    public int getBussinessId() {
        return bussinessId;
    }

    int bussinessId;
    private static final String TAG = "AllVideoView";

    private boolean isPlayerInitReady;
    private boolean isOnCreate;
    private boolean needSurfaceChange;
    AllPlayerJni mWePlayer;
    private OnPlayerstateListener mOnPlayerstateListener;
    private OnPlaybackListener mOnPlaybackListener;
    private Handler mUIHandler = new Handler(Looper.getMainLooper());

    public void setOnPlayerstateListener(OnPlayerstateListener listener) {
        this.mOnPlayerstateListener = listener;
    }

    public void setOnPlaybackListener(OnPlaybackListener mOnPlaybackListener) {
        this.mOnPlaybackListener = mOnPlaybackListener;
    }

    public AllVideoView(Context context) {
        this(context, null);
    }

    public AllVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        if (null != attrs) {
            TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.AllVideoViewType);
            bussinessId = array.getInt(R.styleable.AllVideoViewType_allplayer_type, 1);
            array.recycle();
        }

        //android 8.0 需要设置 此条语句会创建启动 GLThread
        setRenderer(this);
    }

    public boolean isPlayerInitReady() {
        return isPlayerInitReady;
    }

    public void clearSurface() {
        if (isPlayerInitReady)
            mWePlayer.nativeClear(bussinessId);
    }


    private void initPlayer() {

        LogUtils.e(TAG, "initPlayer--------->");
        mWePlayer = new AllPlayerJni();
        isPlayerInitReady = true;

        //录像的回调
        mWePlayer.setOnPlaybackListener(new OnPlaybackListener() {
            @Override
            public void onDataCall(int datacall) {
                if (mOnPlaybackListener != null) {
                    mUIHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mOnPlaybackListener.onDataCall(datacall);
                        }
                    });
                }

            }
        });
        //播放状态的回调
        mWePlayer.setOnOnPlayerstateListener(new OnPlayerstateListener() {
            @Override
            public void onChagnState(int currentState, String msg) {
                LogUtils.e("lhf", "currentState =" + currentState);

                if (mOnPlayerstateListener != null) {
                    mUIHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            mOnPlayerstateListener.onChagnState(currentState, msg);
                            if (currentState == STREAM_STATUS_SETUP && needSurfaceChange) {
                                //这里判断 是否需要重新
                                if (mWePlayer != null && surfaceHolder != null) {
                                    mWePlayer.nativeSurfaceChange(bussinessId, surfaceHolder.getSurface(), mWidth, mHeight, !isOnCreate);
                                }
                            }
                        }
                    });
                }
            }
        });
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        LogUtils.e(TAG, "surfaceCreated--------->");
        isOnCreate = true;
//        bussinessId = System.currentTimeMillis();
        //初始化opengl egl 显示
        InitView(bussinessId, holder.getSurface());

        //只有在绘制数据改变时才绘制view，可以防止GLSurfaceView帧重绘
        //setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        //play("rtsp://172.16.20.252/LiveMedia/ch1/Media1");
        if (!isPlayerInitReady)
            initPlayer();
    }

    private SurfaceHolder surfaceHolder;
    private int mWidth;
    private int mHeight;

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        needSurfaceChange = true;
        LogUtils.e(TAG, "surfaceChanged--------->width, =" + width + "------- h = " + height);
        if (null != mWePlayer) {
            surfaceHolder = holder;
            mWidth = width;
            mHeight = height;
            mWePlayer.nativeSurfaceChange(bussinessId, holder.getSurface(), width, height, !isOnCreate);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder var1) {
        isOnCreate = false;
        LogUtils.e(TAG, "surfaceDestroyed--------->");
        //isPlayerInitReady = false;
    }

    public native void InitView(int m_bizId, Object surface);

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        LogUtils.e(TAG, "onSurfaceCreated--------->");

    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int width, int height) {
        LogUtils.e(TAG, "onSurfaceChanged--------->");
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        LogUtils.e(TAG, "onDrawFrame--------->");
    }

    public void onActivityStart() {
        LogUtils.e(TAG, "onActivityStart--------->");
        super.onResume();
    }

    @Override
    public void onResume() {
        // do nothing 防止外层应用调用 super.onResume();
    }

    public void onActivityStop() {
        LogUtils.e(TAG, "onActivityStop--------->");
        super.onPause();
        // onSurfaceCreated 并不一定与 surfaceCreated 及 surfaceDestroyed 保持一样的调用次数
        // * onSurfaceCreated will be called when the rendering thread
        // * starts and whenever the EGL context is lost. The EGL context will typically
        // * be lost when the Android device awakes after going to sleep.

    }

    //文件名  路径+文件名
    public void snap(String path) {
        if (mWePlayer != null) {
            mWePlayer.capture(bussinessId, path);
        }
    }

    public void setVcrControl(double start, double scale,int platType) {
        if (mWePlayer != null) {
            mWePlayer.nativeVcrControl(bussinessId, start, scale,platType);
        }
    }

    public void sendAudioFrame(byte[] data, int dataSize) {
        if (mWePlayer != null) {
            mWePlayer.sendAudioFrame(data, dataSize);
        }
    }

    /**
     * 如果为true，则是录像回放的暂停播放，false则是本地播放的恢复播放
     *
     * @param isReplay
     */
    public void pause(boolean isReplay) {
        LogUtils.e(TAG, "pause----------------->");
        if (mWePlayer != null) {
            mWePlayer.nativePause(bussinessId, isReplay);
        }
    }

    /**
     * 如果为true,则是录像回放的恢复播放，false则是本地播放的恢复播放
     *
     * @param isReplay
     */
    public void resume(boolean isReplay) {
        LogUtils.e(TAG, "resume----------------->");
        if (mWePlayer != null) {
            mWePlayer.nativeResume(bussinessId, isReplay);
        }
    }

    /**
     * 实时浏览调用
     *
     * @param url         传递的数据Rtsp的地址
     * @param playMode    播放模式 0 是实时性，1 是画质优先，如果为1，则 crashFrames必传
     * @param crashFrames 传递帧数，范围在5-25
     */
    public int play(String url, int playMode, int crashFrames, int devicePlatType) {
        if (mWePlayer != null) {
            needSurfaceChange = false;
            PlayConfig playConfig = new PlayConfig(url, bussinessId, true, playMode, crashFrames, devicePlatType);
            return mWePlayer.nativePlay(playConfig);
        }
        return -1;
    }

    public int startTalk(String url) {
        int code = -1;
        if (mWePlayer != null) {
            code = mWePlayer.startTalk(url, bussinessId);
        }
        return code;
    }

    public int localPlay(String url) {
        if (mWePlayer != null) {
            needSurfaceChange = false;
            return mWePlayer.nativeLocalPlay(url, bussinessId);
        }
        return -1;
    }

    public void localStop() {
        if (mWePlayer != null) {
            mWePlayer.nativeLocalStop(bussinessId);
            mWePlayer.release();
        }
    }

    /**
     * 本地播放百分比跳转
     *
     * @param percentage
     */
    public void localSeek(double percentage) {
        if (mWePlayer != null) {
            mWePlayer.localVideoSeek(bussinessId, percentage);
        }
    }

    public void stop() {
        if (mWePlayer != null) {
            mWePlayer.nativeStop(bussinessId, true);
            mWePlayer.release();
        }
    }

    public void replay(String dataSource, int playMode, int crashFrames, int devicePlatType) {
        LogUtils.e(TAG, "setDataSource----------------->");

        if (mWePlayer != null) {
            needSurfaceChange = false;
            mWePlayer.setDataSource(dataSource, bussinessId, false, playMode, crashFrames, devicePlatType);
        }
    }

    public void reStop() {
        LogUtils.e(TAG, "reStop----------------->");
        if (mWePlayer != null) {
            mWePlayer.reStop(bussinessId);
        }
    }

    public void startRecord(String path) {
        LogUtils.e(TAG, "startRecord----------------->");
        if (mWePlayer != null) {
            mWePlayer.startRecord(bussinessId, path);
        }
    }


    public void endRecord() {
        LogUtils.e(TAG, "endRecord----------------->");
        if (mWePlayer != null) {
            mWePlayer.endRecord(bussinessId);
        }
    }

    public void openVoice(boolean open) {
        LogUtils.e(TAG, "openVoice----------------open ->" + open);
        if (mWePlayer != null) {
            mWePlayer.openVoice(bussinessId, open);
        }
    }

    /**
     * 获取上报的数据
     *
     * @param message
     */
    public void getVideoReportInformation(StringBuffer message) {
        LogUtils.e(TAG, "getVideoInfo----------------->");
        if (null != mWePlayer) {
            mWePlayer.getVideoPlayStatusInfo(bussinessId, message);
        }
    }

    /**
     * 获取播放设备的数据信息
     *
     * @param message
     */
    public void getDeviceReportInformation(StringBuffer message) {
        LogUtils.e(TAG, "getDeviceInfo----------------->");
        if (null != mWePlayer) {
            mWePlayer.getDeviceMediaInfo(bussinessId, message);
        }
    }

    public boolean isOnCreate() {
        return isOnCreate;
    }
}
