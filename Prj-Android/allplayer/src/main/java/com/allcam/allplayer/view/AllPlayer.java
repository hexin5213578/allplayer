package com.allcam.allplayer.view;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.res.TypedArray;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;

import com.allcam.allplayer.AllPlayerJni;
import com.allcam.allplayer.AllVideoView;
import com.allcam.allplayer.MediaStatus;
//import com.allcam.allplayer.R;
import com.allcam.allplayer.listener.OnPlaybackListener;
import com.allcam.allplayer.utils.AudioCollector;
import com.allcam.allplayer.utils.IMediaRecorder;
import com.allcam.http.AllcamApi;
import com.allcam.http.protocol.base.BaseBean;
import com.allcam.http.protocol.live.LiveBean;
import com.allcam.http.protocol.record.RecordListBean;
import com.allcam.http.protocol.record.RecordUrlBean;
import com.hjq.http.listener.OnHttpListener;
import com.hjq.http.request.PostRequest;

import org.libsdl.app.R;
import org.libsdl.app.SDL;

/**
 * Created on 2021/11/22.
 *
 * @author Des：
 */
public class AllPlayer extends FrameLayout {
    private String TAG = "AllPlayer";
    protected MediaStatus status;

    final int STREAM_STATUS_CONNECTED = 20001;            //媒体连接已建立
    final int STREAM_STATUS_KEYFRAME = 20002;            //视频渲染第一帧
    final int STREAM_STATUS_PAUSE = 20003;            //暂停
    final int STREAM_STATUS_RESUME = 20004;            //恢复
    final int STREAM_STATUS_TEARDOWN = 20005;            //rtsp流结束
    final int STREAM_STATUS_TIMEOUT = 20006;            //数据流接收超时
    final int STREAM_STATUS_STOP = 20007;            //资源回收完毕
    final int STREAM_STATUS_RTCP_TIMEOUT = 20008;        //RTCP信令超时

    final int STREAM_STATUS_CONN_ERROR = 20009;        //连接关闭，服务器返回错误等异常情况
    final int STREAM_STATUS_SRV_ERROR = 20010;        //服务端错误
    final int STREAM_STATUS_EOS = 20011;        //end of stream
    final int STREAM_STATUS_SETUP = 20012;        //RTSP SETUP成功，可PLAY
    final int STREAM_STATUS_IO_FINISH = 20013;        //o结束(写mp4)
    final int STREAM_CONNECT_FAILED = 20014;
    final int URL_EOF = 10015;// 本地视频播放结束的回调
    final static int STREAM_AUDIO_COLLECT_SUCCESS = 20019;    //对讲开启成功
    final static int STREAM_AUDIO_COLLECT_FAILED = 20018;    //语音对讲开启失败

    final static int START_AUDIO_FAILE_HTTP = 30001;    //语音对讲 http错误
    final static int START_AUDIO_FAILE_RECORD = 30002;    //语音对讲  录制错误
    final static int START_AUDIO_SUCCESS = 30003;    //语音对讲 开启成功，外部
    final static int START_AUDIO_FAILE = 30004;    //语音对讲 开启是吧，外部
    final static int STOP_AUDIO = 30005;    //关闭语音对讲
    /**
     * 录像分段
     */
    final static int RECORD_SUBSECTION = 10008; // 录像分段
    final static int STOP_RECORD = 10004;// 结束录像
    final static int RECORD_ERROR = 10017;// 本地录像失败

    private Context mContext;

    public AllVideoView getVideoView() {
        return videoView;
    }

    private AllVideoView videoView;
    /**
     * 主码流
     */
    public static final int STREAM_MAIN = 1;
    /**
     * 子码流
     */
    public static final int STREAM_SUB_1 = 2;
    /**
     * 多个镜头时
     */
    private int playBussinessId;

    private int codeStream = STREAM_MAIN;

    private boolean isVideoRecord;// 是否是录像回放，这样的逻辑会不同
    private boolean haveVoice;// 是否有声音

    //单个
    public AllPlayer(Context context) {
        this(context, null);
    }

    //多屏幕
    public AllPlayer(Context context, int type) {
        super(context);

        mContext = context;
        playBussinessId = type;
        View.inflate(context, R.layout.view_player, this);
        videoView = findViewById(R.id.video_view);
        videoView.setBussinessId(type);

    }

    public AllPlayer(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }


    public AllPlayer(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        if (null != attrs) {
            TypedArray array = context.obtainStyledAttributes(attrs, R.styleable.AllVideoViewType);
            playBussinessId = array.getInt(R.styleable.AllVideoViewType_allplayer_type, 0);
            array.recycle();
        }
        mContext = context;
        View.inflate(context, R.layout.view_player, this);
        videoView = findViewById(R.id.video_view);
        if (playBussinessId != 0) {
            videoView.setBussinessId(playBussinessId);
        }
        videoView.setOnPlayerstateListener((state, msg) -> {

            switch (state) {
                case STREAM_AUDIO_COLLECT_FAILED:
                    AllPlayerJni.stopTalk(videoView.getBussinessId());
                    if (allPlayerListener != null) {
                        allPlayerListener.startTalkState(START_AUDIO_FAILE);
                    }
                    break;
                case STREAM_AUDIO_COLLECT_SUCCESS:
                    if (allPlayerListener != null) {
                        allPlayerListener.startTalkState(START_AUDIO_SUCCESS);
                        switchVoice(true);
                    }
                    collectVoice();
                    break;
                case RECORD_SUBSECTION:
                    if (allPlayerListener != null) {
                        allPlayerListener.recordPath(msg, playBussinessId);
                    }
                    break;
                case RECORD_ERROR:// 本地录像失败，将错误抛出，需要结束录像，
                    if (allPlayerListener != null) {
                        allPlayerListener.recordError();
                    }
                    break;
                case STOP_RECORD:// 结束录像
                    if (allPlayerListener != null) {
                        allPlayerListener.recordEnd();
                    }
                    break;
                case STREAM_STATUS_CONNECTED:
                    break;
                case STREAM_STATUS_KEYFRAME:
                    status = MediaStatus.PLAYING;
                    break;
                case STREAM_STATUS_PAUSE:
                    break;
                case STREAM_STATUS_RESUME:
                    break;
                case STREAM_STATUS_STOP:
                case URL_EOF:
                    status = MediaStatus.STOPPED;
                    break;
                case STREAM_STATUS_TEARDOWN:
                case STREAM_STATUS_TIMEOUT:
                case STREAM_STATUS_RTCP_TIMEOUT:
                case STREAM_STATUS_CONN_ERROR:
                case STREAM_STATUS_SRV_ERROR:
                case STREAM_CONNECT_FAILED:
                    status = MediaStatus.ERROR;
                    if (videoView != null) {
                        videoView.stop();
                    }
                    break;
                case STREAM_STATUS_EOS:
                    if (videoView != null) {
                        videoView.stop();
                    }
                    break;
                case STREAM_STATUS_SETUP:
                    break;
            }
            Log.i(TAG, "onChagnState  state=" + state + "------msg=" + msg);
            if (allPlayerListener != null) {
                allPlayerListener.onStateChanged(state, msg, playBussinessId);
            }
        });

    }

    AudioCollector mAudioCollector;
    private boolean isAudioTalk = false;

    /**
     * 采集声音
     */
    private void collectVoice() {
        mAudioCollector = new AudioCollector(new IMediaRecorder() {
            @Override
            public void startMux() {
            }

            @Override
            public void onAudioError(int what, String message) {
                if (allPlayerListener != null) {
                    allPlayerListener.startTalkState(START_AUDIO_FAILE_RECORD);
                }
            }

            @Override
            public void receiveAudioData(byte[] sampleBuffer, int dataSize) {
                AllPlayerJni.sendAudio(sampleBuffer, dataSize);
            }
        });
        mAudioCollector.start();
    }

    /**
     * 设置主子码流
     *
     * @param code
     */
    public void setCodeStream(int code) {
        if (code != STREAM_MAIN) {
            codeStream = STREAM_SUB_1;
        } else {
            codeStream = STREAM_MAIN;
        }
    }

    /**
     * 根据镜头id 进行播放
     *
     * @param cameraId
     */
    public void playCamera(String cameraId) {
        stop();
        AllcamApi.getInstance().getLiveUrl((LifecycleOwner) mContext, cameraId, codeStream, new OnHttpListener<LiveBean>() {
            @Override
            public void onSucceed(LiveBean result) {

                Log.d("hmy", "当前镜头为" + cameraId + "-----当前rtsp流为" + result.getUrl());
                if (null != allPlayerListener) {
                    allPlayerListener.cameraHttpListener(cameraId, result.getUrl(), playBussinessId);
                }
                status = MediaStatus.NONE;
                // 0是实时性优先，1是画质优先
                // 50是帧数 范围在5-25
                videoView.play(result.getUrl(), 0, 25, 0);
            }

            @Override
            public void onFail(Exception e) {
                if (null != allPlayerListener) {
                    allPlayerListener.cameraHttpListener(cameraId, null, playBussinessId);
                }
            }
        });
    }

    /**
     * 开始语音对讲
     *
     * @param cameraId
     */
    public void startTalk(String cameraId) {
        if (!isPalying()) return;
        AllcamApi.getInstance().getAudioTalkUrl((LifecycleOwner) mContext, cameraId, new OnHttpListener<LiveBean>() {
            @Override
            public void onSucceed(LiveBean liveBean) {
                if (liveBean != null && !TextUtils.isEmpty(liveBean.getUrl())) {
                    videoView.startTalk(liveBean.getUrl());
                } else {
                    if (allPlayerListener != null) {
                        allPlayerListener.startTalkState(START_AUDIO_FAILE_HTTP);
                    }
                }
            }

            @Override
            public void onFail(Exception e) {
                if (allPlayerListener != null) {
                    allPlayerListener.startTalkState(START_AUDIO_FAILE_HTTP);
                }
            }
        });
    }

    /**
     * 结束语音对讲
     */
    public void stopTalk() {
        switchVoice(false);
        if (mAudioCollector != null) {
            AllPlayerJni.stopTalk(videoView.getBussinessId());
            mAudioCollector.interrupt();
            mAudioCollector = null;
        }
        if (allPlayerListener != null) {
            allPlayerListener.startTalkState(STOP_AUDIO);
        }
    }

    /**
     * 播放本地的url
     *
     * @param localUrl 本地的视频地址
     */
    public int playLocalUrl(String localUrl) {
        localStop();
        status = MediaStatus.NONE;
        return videoView.localPlay(localUrl);
    }

    /**
     * 本地播放停止
     */
    public void localStop() {
        if (videoView != null) {
            videoView.localStop();
        }
    }

    /**
     * @param bean 录像回放时，调用播放的接口
     */
    public void replayCamera(RecordListBean bean, int devicePlatType) {
        stop();
        AllcamApi.getInstance().getRecordUrl((LifecycleOwner) mContext, bean, new OnHttpListener<RecordUrlBean>() {
            @Override
            public void onSucceed(RecordUrlBean recordUrlBean) {
                if (null != allPlayerListener) {
                    allPlayerListener.cameraHttpListener(bean.getCameraId(), recordUrlBean.getUrl(), playBussinessId);
                }
                status = MediaStatus.NONE;
                videoView.replay(recordUrlBean.getUrl(), 0, 25, devicePlatType);
            }

            @Override
            public void onFail(Exception e) {
                if (null != allPlayerListener) {
                    allPlayerListener.cameraHttpListener(bean.getCameraId(), null, playBussinessId);
                }
            }
        });
    }

    /**
     * 停止播放
     */
    public void stop() {
        if (videoView != null) {
            if (isVideoRecord) {
                videoView.reStop();
            } else {
                videoView.stop();
            }
        }
    }

    /**
     * 暂停
     *
     * @param isReplay true 是录像回放的暂停，false是本地播放的暂停
     */
    public void pause(boolean isReplay) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.pause(isReplay);
            status = MediaStatus.PAUSE;
        }
    }

    /**
     * 播放
     *
     * @param isReplay true 是录像回放的恢复播放，false是本地播放的恢复播放
     */
    public void resume(Boolean isReplay) {
        if (videoView != null && status == MediaStatus.PAUSE) {
            status = MediaStatus.PLAYING;
            videoView.resume(isReplay);
        }
    }

    /**
     * 本地抓拍
     *
     * @param path 路径
     */
    public void localSnap(String path) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.snap(path);
        }
    }

    /**
     * 本地播放器的进度跳转
     *
     * @param percentage 这个是百分比
     */
    public void localSeek(double percentage) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.localSeek(percentage);
        }
    }

    String postRequest;

    /**
     * 平台抓拍
     *
     * @param cameraId
     */
    public void platformSnap(String cameraId) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            postRequest = AllcamApi.getInstance().devSnap((LifecycleOwner) mContext, cameraId, new OnHttpListener<BaseBean>() {
                @Override
                public void onSucceed(BaseBean result) {
                    if (null != allPlayerListener) {
                        allPlayerListener.platformSnap(true, playBussinessId);
                    }
                }

                @Override
                public void onFail(Exception e) {
                    if (null != allPlayerListener) {
                        allPlayerListener.platformSnap(false, playBussinessId);
                    }
                }
            });
        }

    }

    /**
     * 声音开关
     *
     * @param isOpen 开/关
     */
    public void switchVoice(boolean isOpen) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.openVoice(isOpen);
            haveVoice = isOpen;
        }
    }

    /**
     * 开始 录像
     *
     * @param path
     */
    public void startRecord(String path) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.startRecord(path);
        }
    }

    /**
     * 直接播放rtspUrl
     *
     * @param rtspUrl
     */
    public void playRtsp(String rtspUrl, int devicePlatType) {
        if (videoView != null) {
            stop();
            status = MediaStatus.NONE;
            videoView.play(rtspUrl, 0, 25, devicePlatType);
        }
    }

    public void setVcrControl(double start, double scale, int platType) {
        if (videoView != null && isVideoRecord) {
            videoView.setVcrControl(start, scale, platType);
        }
    }

    public void sendAudioFrame(byte[] data, int dataSize) {
        if (videoView != null) {
            videoView.sendAudioFrame(data, dataSize);
        }
    }

    /**
     * 停止录像
     */
    public void endRecord() {
        if (videoView != null) {
            videoView.endRecord();
        }
    }

    /**
     * 获取上报的数据
     *
     * @param message
     */
    public void getVideoReportInformation(StringBuffer message) {
        if (null != videoView) {
            videoView.getVideoReportInformation(message);
        }
    }

    /**
     * 获取播放设备的数据信息
     *
     * @param message
     */
    public void getDeviceReportInformation(StringBuffer message) {
        if (null != videoView) {
            videoView.getDeviceReportInformation(message);
        }
    }

    private AllPlayerListener allPlayerListener;

    public interface AllPlayerListener {
        //实时视频接口请求的成功或者失败
        void cameraHttpListener(String camera, String url, int playBussinessId);

        //播放器 状态回掉
        void onStateChanged(int state, String msg, int playBussinessId);

        //平台抓拍成功/失败的回掉
        void platformSnap(boolean success, int playBussinessId);

        //录像成功后地址的回掉
        void recordPath(String path, int playBussinessId);

        void startTalkState(int state);

        // 录像失败的回调
        void recordError();

        // 录像结束回调
        void recordEnd();
    }


    public void setAllPlayerListener(AllPlayerListener allPlayerListener) {
        this.allPlayerListener = allPlayerListener;
    }

    /**
     * 释放
     */
    public void release() {
        if (!TextUtils.isEmpty(postRequest)) {
            AllcamApi.getInstance().cancel(postRequest);
            postRequest = null;
        }
    }

    /**
     * 是否在播放
     *
     * @return
     */
    public boolean isPalying() {
        return status == MediaStatus.PLAYING;
    }

    /**
     * 播放的状态
     *
     * @return
     */
    public MediaStatus getPlayStatus() {
        if (status == null) {
            status = MediaStatus.NONE;
        }
        return status;
    }

    /**
     * 动态设置是否是录像回放。
     *
     * @param videoRecord
     */
    public void setVideoRecord(boolean videoRecord) {
        isVideoRecord = videoRecord;
    }


    public int getPlayBussinessId() {
        return playBussinessId;
    }

    public boolean isVideoRecord() {
        return isVideoRecord;
    }

    public boolean isHaveVoice() {
        return haveVoice;
    }

    public void setHaveVoice(boolean haveVoice) {
        this.haveVoice = haveVoice;
    }
}