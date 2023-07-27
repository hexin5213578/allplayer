package com.allcam.allplayer.view;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;

import com.allcam.allplayer.AllVideoView;
import com.allcam.allplayer.MediaStatus;

import com.allcam.allplayer.listener.OnPlaybackListener;
import com.allcam.allplayer.listener.OnPlayerstateListener;
import com.allcam.http.AllcamApi;
import com.allcam.http.protocol.base.BaseBean;
import com.allcam.http.protocol.live.LiveBean;
import com.allcam.http.protocol.record.RecordListBean;
import com.allcam.http.protocol.record.RecordUrlBean;
import com.hjq.http.listener.OnHttpListener;
import com.hjq.http.request.PostRequest;

import org.libsdl.app.R;

/**
 * Created on 2021/11/22.
 *
 * @author Des：录像回放的播放器
 */
public class AllRecordPlayer extends FrameLayout {
    private String TAG = "AllPlayer";
    protected MediaStatus status;

    final int RECORD_SUBSECTION = 10008;            //录像下载回掉
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
    final static int STOP_RECORD = 10004;// 结束录像
    final static int RECORD_ERROR = 10017;// 本地录像失败

    private Context mContext;
    String postRequest;


    public AllVideoView getVideoView() {
        return videoView;
    }

    private AllVideoView videoView;
    private double currentScale = 1;
    /**
     * 多个镜头时
     */
    private int playType;


    //单个
    public AllRecordPlayer(Context context) {
        this(context, null);
    }

    //多屏幕
    public AllRecordPlayer(Context context, int type) {
        super(context);
        mContext = context;
        View.inflate(context, R.layout.view_player, this);
        videoView = findViewById(R.id.video_view);
        videoView.setBussinessId(type);
    }

    public AllRecordPlayer(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public AllRecordPlayer(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mContext = context;
        View.inflate(context, R.layout.view_player, this);
        videoView = findViewById(R.id.video_view);

        videoView.setOnPlayerstateListener(new OnPlayerstateListener() {
            @Override
            public void onChagnState(int state, String msg) {
                Log.e(TAG, "onChagnState  state=" + state);
                if (allPlayerListener != null) {
                    allPlayerListener.onStateChanged(state, msg);
                }
                switch (state) {
//                    case VIDEO_END:
//                        allPlayerListener.recordPath(msg);
                    case RECORD_SUBSECTION:
                        if (allPlayerListener != null) {
                            allPlayerListener.recordPath(msg);
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
                        status = MediaStatus.STOPPED;
                        break;
                    case STREAM_STATUS_TEARDOWN:
                    case STREAM_STATUS_RTCP_TIMEOUT:

                    case STREAM_STATUS_CONN_ERROR:
                    case STREAM_STATUS_SRV_ERROR:
                        status = MediaStatus.ERROR;
                        if (videoView != null) {
                            videoView.reStop();
                        }
                        break;
                    case STREAM_STATUS_TIMEOUT:
                        if (status != MediaStatus.PAUSE) {
                            if (videoView != null) {
                                videoView.reStop();
                            }

                        }
                        break;
                    case STREAM_STATUS_EOS:
                        if (videoView != null) {
                            videoView.reStop();
                        }
                        break;
                    case STREAM_STATUS_SETUP:
                        break;
                    default:

                        break;
                }
            }
        });
    }

    /**
     * @param bean
     */
    public void replayCamera(RecordListBean bean, int devicePlatType) {
        stop();
        AllcamApi.getInstance().getRecordUrl((LifecycleOwner) mContext, bean, new OnHttpListener<RecordUrlBean>() {
            @Override
            public void onSucceed(RecordUrlBean recordUrlBean) {
                if (null != allPlayerListener) {
                    allPlayerListener.cameraHttpListener(bean.getCameraId(), recordUrlBean.getUrl());
                }
                status = MediaStatus.NONE;
                videoView.replay(recordUrlBean.getUrl(), 0, 25, devicePlatType);
            }

            @Override
            public void onFail(Exception e) {
                if (null != allPlayerListener) {
                    allPlayerListener.cameraHttpListener(bean.getCameraId(), null);
                }
            }
        });
    }


    /**
     * 停止播放
     */
    public void stop() {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.reStop();
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
                        allPlayerListener.platformSnap(true);
                    }
                }

                @Override
                public void onFail(Exception e) {
                    if (null != allPlayerListener) {
                        allPlayerListener.platformSnap(false);
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
     * 停止录像
     */
    public void endRecord() {
        if (videoView != null) {
            videoView.endRecord();
        }
    }

    /**
     * 暂停
     */
    public void pause() {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.pause(true);
            status = MediaStatus.PAUSE;
        }
    }

    /**
     * 播放
     */
    public void resume() {
        if (videoView != null && status == MediaStatus.PAUSE) {
            status = MediaStatus.PLAYING;
            videoView.resume(true);
        }
    }


    /**
     * 设置倍速
     *
     * @param scale -4 -2 -1 1 2 4
     */
    public void setSpeed(double scale) {
        this.currentScale = scale;
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.setVcrControl(-1, scale,0);
        }
    }

    /**
     * 拖拽间隔
     *
     * @param interval
     */
    public void dragInterval(double interval) {
        if (videoView != null && status == MediaStatus.PLAYING) {
            videoView.setVcrControl(interval, currentScale,0);
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

    public void setAllPlayerListener(AllPlayerListener allPlayerListener) {
        this.allPlayerListener = allPlayerListener;
    }

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


    private AllPlayerListener allPlayerListener;

    public interface AllPlayerListener {
        //实时视频接口请求的成功或者失败
        void cameraHttpListener(String camera, String url);

        //播放器 状态回掉
        void onStateChanged(int state, String msg);

        //平台抓拍成功/失败的回掉
        void platformSnap(boolean success);

        //录像成功后地址的回掉
        void recordPath(String path);

        //录像回放时 每秒 数据回调
        void onDataCall(int datacall);

        // 录像失败的回调
        void recordError();

        // 录像结束回调
        void recordEnd();
    }
}