package com.allcam.player;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.os.Environment;
import android.text.InputType;
import android.text.TextUtils;
import android.text.method.DigitsKeyListener;
import android.util.Base64;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Chronometer;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.constraintlayout.widget.Group;
import androidx.lifecycle.LifecycleOwner;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.allcam.allplayer.AllPlayerJni;
import com.allcam.allplayer.MediaStatus;
import com.allcam.allplayer.listener.OnPlaybackListener;
import com.allcam.allplayer.utils.AudioCollector;
import com.allcam.allplayer.utils.DisplayUtil;
import com.allcam.allplayer.utils.IMediaRecorder;
import com.allcam.allplayer.view.AllPlayer;
import com.allcam.allplayer.waterMark.VideoWaterMark;
import com.allcam.basemodule.base.BaseActivity;
import com.allcam.basemodule.base.BasePopupWindow;
import com.allcam.basemodule.base.FragmentBaseActivity;
import com.allcam.basemodule.dialog.MessageDialog;
import com.allcam.basemodule.permission.Permission;
import com.allcam.basemodule.permission.XXPermissions;
import com.allcam.basemodule.utils.AppExecutors;
import com.allcam.basemodule.utils.MMKVUtils;
import com.allcam.http.AllcamApi;
import com.allcam.http.protocol.AcProtocol;
import com.allcam.http.protocol.Imageverification.VerificationBean;
import com.allcam.http.protocol.Login.UserLogin;
import com.allcam.http.protocol.base.BaseBean;
import com.allcam.http.protocol.device.PayloadBean;
import com.allcam.player.adapter.ControlAdapter;
import com.allcam.player.bean.ControlBean;
import com.allcam.player.bean.ControlType;
import com.allcam.player.constant.PlayConstant;
import com.allcam.player.dialog.LocalResourceDialog;
import com.allcam.player.fragment.RecordFragment;
import com.allcam.player.mvp.control.MainPresenterControl;
import com.allcam.player.mvp.presenter.MainPresenter;
import com.allcam.player.util.Utils;
import com.allcam.view.DeviceConfig;
import com.allcam.view.activity.SearchActivity;
import com.allcam.view.recyclerView.DeviceListView;
import com.bumptech.glide.Glide;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends BaseActivity<MainPresenter> implements MainPresenterControl.MainView, AllPlayer.AllPlayerListener {
    // 搜索页面的回调
    private static final int REQUEST_CAMERA_CODE = 0x66;
    public static final Integer RECORD_CODE = 1211;
    /***************播放器相关的view**********************/
    private ConstraintLayout constraintLayout;
    private List<AllPlayer> allPlayers = new ArrayList<>();
    private int[] bussinessIds = new int[]{};
    /***************设备列表相关的view**********************/
    // 展示设备列表list的view
    private DeviceListView deviceListView;
    private TextView tvSearch;
    /***************登录相关的view**********************/
    private ImageView ivVerification;// 验证码
    private EditText edInputAddress;// 输入ip地址
    private EditText edInputUserName;// 输入用户名
    private EditText edInputUserPassword;// 输入密码
    private EditText edInputVerification;// 输入验证码
    private PayloadBean selectPlayLoadBean;// 当前选择的设备
    private MessageDialog.Builder messageDialog;
    private Group loginGroup;
    private TextView tvSelect;
    /****************控制器按钮相关的**********************/
    private RecyclerView ryControl;// 控制器相关的
    private ControlAdapter controlAdapter;// 适配器
    private boolean isSingle = true; // 是否是单屏
    private Integer selectPlayNum = 0; // 当前选中的播放视频，默认是0
    private boolean isSelectPlatform = true; // 默认使用平台抓拍
    private BasePopupWindow basePopupWindow;
    private boolean isRecord;// 是否正在录像
    private Chronometer chronometer; //计时器
    private LocalResourceDialog localResourceDialog;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_main;
    }

    @Override
    protected void initView() {
        // 用来控制屏幕常亮-
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        constraintLayout = findViewById(R.id.group_play_view);// 放置PlayView的布局

        deviceListView = findViewById(R.id.device_list);

        ivVerification = findViewById(R.id.iv_verification);
        edInputAddress = findViewById(R.id.input_ip_address);
        edInputUserName = findViewById(R.id.ed_input_userName);
        edInputUserPassword = findViewById(R.id.ed_input_user_password);
        edInputVerification = findViewById(R.id.ed_input_verification);
        loginGroup = findViewById(R.id.group_login);
        tvSelect = findViewById(R.id.iv_show_close);

        ryControl = findViewById(R.id.ry_control);
        chronometer = findViewById(R.id.chronometer);
        tvSearch = findViewById(R.id.tv_device_search);
        tvSearch.setOnClickListener(this::searchDevice);

        initPlayView(constraintLayout);
        initEditInputAddress();// 初始化ip输入框，限制只输入ip
        initDeviceList();// 初始化设备列表的view
        initMessageDialog();// 初始化跳转对话框
        initControlView();// 初始化功能按钮框
        initPopWindow();
        String[] premission = new String[]{Permission.RECORD_AUDIO, Permission.MANAGE_EXTERNAL_STORAGE};

        XXPermissions.with(this).permission(premission).request((list, b) -> {

        });
    }

    @Override
    protected void initData() {
        AllcamApi.getInstance().setClientNonce(Utils.getClientNonce("prjAndroid"));
        readMMKV();
        mPresenter.getControlItem();// 获取控制器的数据
        mPresenter.getVerificationImage();
    }


    @Override
    protected MainPresenter onCreatePresenter() {
        return new MainPresenter(this);
    }

    @Override
    protected boolean needEventsBus() {
        return false;
    }

    /**
     * 声音录制
     */
    protected AudioCollector mAudioCollector;

    private void initControlView() {
        controlAdapter = new ControlAdapter(this);
        controlAdapter.setOnItemClickListener((recyclerView, view, i) -> {
            ControlBean controlBean = controlAdapter.getData().get(i);
            if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_SPLIT)) {// 点击了单屏和四分屏按钮
                controlBean.setContent(isSingle ? getString(R.string.common_single_screen) : getString(R.string.common_mulit_screen));
                changeSplit(!isSingle);
                controlAdapter.notifyDataSetChanged();
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_PLAY)) {// 点击了播放按钮
                changPlayViewStatus();
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_STOP)) {// 点击了停止按钮
                allPlayers.get(selectPlayNum).stop();
                if (isRecord) {
                    record();
                }
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_SNAP)) {// 点击了抓拍按钮
                if (!allPlayers.get(selectPlayNum).isPalying()) {
                    toast(R.string.please_play_video_first);
                    return;
                }
                if (checkIsRecord()) return;
                showPopWindow();
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_VIDEO)) {// 点击了录像
                if (!allPlayers.get(selectPlayNum).isPalying()) {
                    toast(R.string.please_play_video_first);
                    return;
                }
                checkPermission(false);
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_VOICE)) {// 点击了声音
                changControlPlayStatus(selectPlayNum, ControlType.CONTROL_TYPE_VOICE);
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_DEFAULT)) {// 默认播放rtsp
                if (checkIsRecord()) return;
                String defaultRtsp = "rtsp://admin:Allcam@2020@172.16.20.127";
                allPlayers.get(selectPlayNum).playRtsp(defaultRtsp, 0);
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_LISTENER)) {// 监听
                allPlayers.get(selectPlayNum).getVideoView().setOnPlaybackListener(new OnPlaybackListener() {
                    @Override
                    public void onDataCall(int datacall) {
                        Log.e("lhf", "onDataCall = " + datacall);
                    }
                });
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_TYPE_DRAG_FORWARD)) {// 拖拽
                allPlayers.get(selectPlayNum).setVcrControl(-1, -2f,0);
//                allPlayers.get(selectPlayNum).localSeek(0.5);
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_START_RECORD)) {// 开始录制音频
                allPlayers.get(selectPlayNum).startTalk(selectPlayLoadBean.getDeviceId());
//                AllPlayerJni.startTalk("rtsp://admin:Allcam@172.16.21.31:1554/audio/01353337173501650101?audioformat=1&domaincode=97f2dd92e4984b1ab6fb961d135d8148", allPlayers.get(selectPlayNum).getPlayBussinessId());
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_STOP_RECORD)) {// 结束
                AllPlayerJni.stopTalk(allPlayers.get(selectPlayNum).getPlayBussinessId());
                if (mAudioCollector != null) {
                    mAudioCollector.interrupt();
                    try {
                        mAudioCollector.join();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    mAudioCollector = null;
                }
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_LOCAL_PLAY)) {// 本地播放开始
                allPlayers.get(selectPlayNum).localStop();
                StringBuffer buffer = new StringBuffer();
                buffer.append(Environment.getExternalStorageDirectory())
                        .append("/DCIM/")
                        .append("/com.allcam.platcommon.jsqly/")
                        .append("/admin/")
                        .append("/video/")
                        .append("temp_1653912501_121_1.mp4");
                String urlLocal = buffer.toString();
                String httpUrl = "http://vfx.mtime.cn/Video/2019/03/12/mp4/190312083533415853.mp4";
                String httpsUrl = "https://media.w3.org/2010/05/sintel/trailer.mp4";
                allPlayers.get(selectPlayNum).setVideoRecord(true);
                allPlayers.get(selectPlayNum).playLocalUrl(urlLocal);
                showLocalResourceSelect();
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_LOCAL_STOP)) {
                allPlayers.get(selectPlayNum).localStop();
                if (isRecord) {
                    record();
                }
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_WATER_MARK_OPEN)) {
                waterMarkInfo(getContext(), 1);
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_WATER_MARK_CLOSE)) {
                waterMarkInfo(getContext(), 0);
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_GET_PLAY_INFORMATION)) {
                StringBuffer message = new StringBuffer();
                allPlayers.get(selectPlayNum).getVideoReportInformation(message);
                try {
                    JSONObject jsonObject = new JSONObject(message.toString());
                    jsonObject.toString();
                } catch (JSONException e) {
                    e.printStackTrace();
                }
                Log.d("lf123456", "getDeviceReportInformation: " + message.toString());
            } else if (controlBean.getLabel().equals(ControlType.CONTROL_GET_VIDEO_INFORMATION)) {
                StringBuffer message = new StringBuffer();
                allPlayers.get(selectPlayNum).getDeviceReportInformation(message);
                try {
                    JSONObject jsonObject = new JSONObject(message.toString());
                    jsonObject.toString();
                } catch (JSONException e) {
                    e.printStackTrace();
                }
                Log.d("lf123456", "getVideoReportInformation: " + message.toString());
            }
        });
        LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        linearLayoutManager.setOrientation(RecyclerView.HORIZONTAL);
        ryControl.setLayoutManager(linearLayoutManager);
        ryControl.setAdapter(controlAdapter);
    }

    /**
     * 展示搜索对话框
     */
    private void showLocalResourceSelect() {
        if (PlayConstant.userLogin == null) {
            toast("请先登录");
            return;
        }
        allPlayers.get(selectPlayNum).stop();
        allPlayers.get(selectPlayNum).localStop();
        if (null == localResourceDialog) {
            localResourceDialog = new LocalResourceDialog(getActivity(), localSource -> allPlayers.get(selectPlayNum).playLocalUrl(localSource));
            localResourceDialog.create();
        }
        localResourceDialog.show();
        StringBuffer buffer = new StringBuffer();
        buffer.append(Environment.getExternalStorageDirectory()).append("/DCIM/").append(BuildConfig.APPLICATION_ID).append("/").append(PlayConstant.userLogin.getUserInfo().getAccount()).append("/");
        String urlLocal = buffer.toString();
        AppExecutors.getInstance().diskIO().execute(() -> {
            File[] files = (new File(urlLocal)).listFiles();
            List<String> stringList = new ArrayList<>();
            if (files != null && files.length > 0) {
                for (File file : files) {
                    if (file.getPath().contains(".mp4")) {
                        stringList.add(file.getPath());
                    }
                }
            }
            if (null != localResourceDialog) {
                localResourceDialog.setDate(stringList);
            }
        });

//        // 自己添加其他MP4的文件
//        StringBuffer local = new StringBuffer();
//        local.append(Environment.getExternalStorageDirectory())
//                .append("/video")
//                .append("/")
//                .append("temp_1657164323_121_1.mp4");
//        String newUrlLocal = buffer.toString();
//        if (null != localResourceDialog) {
//            localResourceDialog.getLocalResourceAdapter().getData().add(newUrlLocal);
//        }
    }

    /**
     * 搜索设备
     */
    private void searchDevice(View view) {
        if (null == PlayConstant.userLogin) {
            toast("请先登录");
            return;
        }
        Intent intent = new Intent();
        intent.setClass(getActivity(), SearchActivity.class);
        startActivityForResult(intent, REQUEST_CAMERA_CODE);
    }

    private boolean checkIsRecord() {
        if (isRecord) {
            toast(R.string.please_stop_record);
            return true;
        }
        return false;
    }

    /**
     * 改变播放视图的播放状态
     */
    private void changPlayViewStatus() {
        if (allPlayers.get(selectPlayNum).isPalying()) {
            // 如果当前是播放状态并且不是历史回放
            if (allPlayers.get(selectPlayNum).isVideoRecord()) {
                allPlayers.get(selectPlayNum).pause(true);
            } else {
                toast(R.string.please_stop_video);
            }
        } else {
            if (allPlayers.get(selectPlayNum).getPlayStatus() == MediaStatus.PAUSE) {
                allPlayers.get(selectPlayNum).resume(true);
            } else {
                if (null == selectPlayLoadBean) {
                    toast(R.string.please_select_video_device);
                    return;
                }
                if (allPlayers.get(selectPlayNum).isVideoRecord()) {
                    allPlayers.get(selectPlayNum).replayCamera(PlayConstant.recordListBean, 0);
                    allPlayers.get(selectPlayNum).setVideoRecord(true);
                } else {
                    allPlayers.get(selectPlayNum).playCamera(selectPlayLoadBean.getDeviceId());
                    allPlayers.get(selectPlayNum).setVideoRecord(false);
                }
            }
        }
    }

    /**
     * 初始化跳转提示框
     */
    private void initMessageDialog() {
        messageDialog = new MessageDialog.Builder(this).setMessage("1").setListener(baseDialog -> {
            Intent intent = new Intent();
            intent.putExtra("deviceId", selectPlayLoadBean.getDeviceId());
            FragmentBaseActivity.startFragmentBaseActivityForResult(RecordFragment.class, intent, RECORD_CODE);
        });
    }

    /**
     * 初始化popWindow选择框
     */
    private void initPopWindow() {
        BasePopupWindow.Builder builder = new BasePopupWindow.Builder(this);
        builder.setContentView(R.layout.popwindow_select_platform_local);
        builder.setText(R.id.tv_title, getText(R.string.please_select_platform_or_local));
        builder.setGravity(Gravity.CENTER);
        TextView tvPlatView = (TextView) builder.findViewById(R.id.tv_platform);
        TextView tvLocalView = (TextView) builder.findViewById(R.id.tv_local);
        tvPlatView.setBackgroundColor(isSelectPlatform ? getResources().getColor(R.color.color92C1FF) : getResources().getColor(R.color.bg_color));
        tvLocalView.setBackgroundColor(!isSelectPlatform ? getResources().getColor(R.color.color92C1FF) : getResources().getColor(R.color.bg_color));
        builder.setOnClickListener(R.id.tv_platform, (BasePopupWindow.OnClickListener<View>) (basePopupWindow, view) -> {
            // 选择了平台
            isSelectPlatform = true;
            tvPlatView.setBackgroundColor(isSelectPlatform ? getResources().getColor(R.color.color92C1FF) : getResources().getColor(R.color.bg_color));
            tvLocalView.setBackgroundColor(!isSelectPlatform ? getResources().getColor(R.color.color92C1FF) : getResources().getColor(R.color.bg_color));
        });
        builder.setOnClickListener(R.id.tv_local, (BasePopupWindow.OnClickListener<View>) (basePopupWindow, view) -> {
            // 选择了本地
            isSelectPlatform = false;
            tvPlatView.setBackgroundColor(isSelectPlatform ? getResources().getColor(R.color.color92C1FF) : getResources().getColor(R.color.bg_color));
            tvLocalView.setBackgroundColor(!isSelectPlatform ? getResources().getColor(R.color.color92C1FF) : getResources().getColor(R.color.bg_color));
        });
        builder.setOnClickListener(R.id.tv_sure, (BasePopupWindow.OnClickListener<View>) (basePopupWindow, view) -> {
            // 确认
            checkPermission(true);
        });
        builder.setOnClickListener(R.id.tv_cancel, (BasePopupWindow.OnClickListener<View>) (basePopupWindow, view) -> {
            // 取消
            if (null != basePopupWindow && basePopupWindow.isShowing()) {
                basePopupWindow.dismiss();
            }
        });
        builder.setBackgroundDimAmount(0.5f);
        builder.setWidth(DisplayUtil.getScreenWidth(this) - DisplayUtil.dp2px(40, this));
        builder.setHeight(DisplayUtil.dp2px(400, this));
        basePopupWindow = builder.create();
    }

    /**
     * 展示选择的popWindow
     */
    private void showPopWindow() {
        if (null == basePopupWindow) {
            initPopWindow();
        }
        if (!basePopupWindow.isShowing()) {
            basePopupWindow.showAtLocation(ryControl, Gravity.CENTER, 0, 0);
        }
    }

    /**
     * 检查权限
     *
     * @param isSnap 是否是抓拍的操作，true是，false是本地录像的操作
     */
    private void checkPermission(boolean isSnap) {
        // 首先判断本地是否有存储权限
        boolean haveLocal = XXPermissions.isGrantedPermission(this, Manifest.permission.MANAGE_EXTERNAL_STORAGE);
        if (haveLocal) {
            // 直接操作
            if (isSnap) {
                snap();
            } else {
                record();
            }
        } else {
            // 请求权限
            requestPermission(new String[]{Manifest.permission.MANAGE_EXTERNAL_STORAGE}, new OnPermissionsCallBack() {
                @Override
                public void onGranted() {
                    // 直接操作
                    if (isSnap) {
                        snap();
                    } else {
                        record();
                    }
                }

                @Override
                public void onDenied() {
                    toast(R.string.permission_lack_tip);
                }
            });
        }
    }

    /**
     * 录像操作
     */
    private void record() {
        if (!allPlayers.get(selectPlayNum).isPalying()) {
            toast(R.string.please_play_video_first);
            return;
        }
        if (isRecord) {
            allPlayers.get(selectPlayNum).endRecord();
            timingEnd();
            isRecord = false;
        } else {
            allPlayers.get(selectPlayNum).startRecord(Utils.getVideoTempLocation());
            timingBegins();
            isRecord = true;
        }
        changControlPlayStatus(selectPlayNum, ControlType.CONTROL_TYPE_VIDEO);
    }

    @Override
    protected void onStop() {

        super.onStop();
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        if (null != deviceListView) {
            deviceListView.onDestroyView();
        }
        super.onDestroy();

    }

    /**
     * 初始化播放器的view
     *
     * @param constraintLayout
     */
    private void initPlayView(ConstraintLayout constraintLayout) {
        int childView = constraintLayout.getChildCount();
        bussinessIds = new int[childView];
        allPlayers.clear();
        for (int i = 0; i < childView; i++) {
            if (constraintLayout.getChildAt(i) instanceof AllPlayer) {
                AllPlayer allPlayer = (AllPlayer) constraintLayout.getChildAt(i);
                allPlayer.setAllPlayerListener(this);
                allPlayer.setOnClickListener(this::playViewSelect);
                allPlayers.add(allPlayer);
                bussinessIds[i] = allPlayer.getPlayBussinessId();
            }
        }
        if (havePlayView()) {
            setSelect(0);
            changeSplit(isSingle);
        } else {
            toast(R.string.common_layout_error);
        }
    }

    /**
     * 设置选中的屏幕
     */
    private void setSelect(int index) {
        if (havePlayView()) {
            for (int i = 0; i < allPlayers.size(); i++) {
                if (index != i) {
                    allPlayers.get(i).setBackground(getResources().getDrawable(R.drawable.shape_playview_bg_unselect));
                }
            }
            allPlayers.get(index).setBackground(getResources().getDrawable(R.drawable.shape_playview_bg));
            changControlPlayStatus(index, ControlType.CONTROL_TYPE_PLAY);
            changControlPlayStatus(index, ControlType.CONTROL_TYPE_VOICE);
            selectPlayNum = index;
        } else {
            toast(R.string.common_layout_error);
        }
    }

    /**
     * 改变控制器上的属性展示
     *
     * @param index 选中的播放视图
     */
    private void changControlPlayStatus(int index, String label) {
        if (null != controlAdapter) {
            List<ControlBean> controlBeans = controlAdapter.getData();
            for (ControlBean controlBean : controlBeans) {
                if (controlBean.getLabel().equals(label)) {
                    switch (label) {
                        case ControlType.CONTROL_TYPE_PLAY:
                            controlBean.setContent(allPlayers.get(index).isPalying() ? getString(R.string.common_pause) : getString(R.string.common_play));
                            controlAdapter.notifyDataSetChanged();
                            break;
                        case ControlType.CONTROL_TYPE_VIDEO:
                            controlBean.setContent(isRecord ? getString(R.string.common_video_end) : getString(R.string.common_video));
                            controlBean.setColorId(isRecord ? R.color.red : R.color.color92C1FF);
                            controlAdapter.notifyDataSetChanged();
                            break;
                        case ControlType.CONTROL_TYPE_VOICE:
                            allPlayers.get(selectPlayNum).setHaveVoice(!allPlayers.get(selectPlayNum).isHaveVoice());
                            allPlayers.get(selectPlayNum).switchVoice(allPlayers.get(selectPlayNum).isHaveVoice());
                            controlBean.setColorId(allPlayers.get(selectPlayNum).isHaveVoice() ? R.color.red : R.color.color92C1FF);
                            controlAdapter.notifyDataSetChanged();
                            break;
                        default:
                            break;
                    }
                    return;
                }
            }
        }
    }

    /**
     * 切换单屏或者四分屏
     *
     * @param isSingle true是单屏，false是四分屏
     */
    private void changeSplit(boolean isSingle) {
        if (havePlayView()) {
            if (isSingle) {
                // 单屏幕
                for (int i = 0; i < allPlayers.size(); i++) {
                    if (selectPlayNum != i) {
                        allPlayers.get(i).stop();
                        allPlayers.get(i).setVisibility(View.GONE);
                    }
                }
            } else {
                // 多屏幕
                for (int i = 0; i < allPlayers.size(); i++) {
                    if (selectPlayNum != i) {
                        allPlayers.get(i).setVisibility(View.VISIBLE);
                    }
                }
            }
        }
        this.isSingle = isSingle;
    }

    /**
     * 设置ip输入框，只允许输入ip地址
     */
    private void initEditInputAddress() {
        edInputAddress.setInputType(InputType.TYPE_CLASS_NUMBER);
        String digits = "0123456789.:";
        edInputAddress.setKeyListener(DigitsKeyListener.getInstance(digits));
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

    }

    /**
     * 点击了哪个播放视图
     *
     * @param view
     */
    public void playViewSelect(View view) {
        if (view instanceof AllPlayer) {
            if (checkIsRecord()) return;
            for (int i = 0; i < bussinessIds.length; i++) {
                if (((AllPlayer) view).getPlayBussinessId() == bussinessIds[i]) {
                    setSelect(i);
                    break;
                }
            }
        }
    }

    //抓拍
    public void snap() {
        if (!allPlayers.get(selectPlayNum).isPalying()) {
            toast(R.string.please_play_video_first);
            return;
        }
        if (isSelectPlatform) {
            allPlayers.get(selectPlayNum).platformSnap(selectPlayLoadBean.getDeviceId());
        } else {
            String local = Utils.getLocalSnapPath();
            allPlayers.get(selectPlayNum).localSnap(local);
            toast(getString(R.string.local_snap_success, local));
        }
        basePopupWindow.dismiss();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (null != allPlayers && allPlayers.size() > 0) {
            if (isSingle) {
                if (allPlayers.get(selectPlayNum).getPlayStatus() == MediaStatus.PAUSE) {
                    allPlayers.get(selectPlayNum).resume(true);
                }
            } else {
                for (int i = 0; i < allPlayers.size(); i++) {
                    if (allPlayers.get(i).getPlayStatus() == MediaStatus.PAUSE) {
                        allPlayers.get(i).resume(true);
                    }
                }
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (null != allPlayers && allPlayers.size() > 0) {
            if (isSingle) {
                if (allPlayers.get(selectPlayNum).isVideoRecord()) {
                    allPlayers.get(selectPlayNum).pause(true);
                } else {
                    allPlayers.get(selectPlayNum).stop();
                }
            } else {
                for (int i = 0; i < allPlayers.size(); i++) {
                    if (allPlayers.get(i).isVideoRecord()) {
                        allPlayers.get(i).pause(true);
                    } else {
                        allPlayers.get(i).stop();
                    }
                }
            }
        }
    }

    /**
     * 初始化设备列表的view
     */
    private void initDeviceList() {
        deviceListView.setDeviceListViewCallBack(new DeviceListView.OnDeviceListViewCallBack() {
            @Override
            public void initializationSuccess() {

            }

            @Override
            public void itemClick(PayloadBean payloadBean) {
                if (checkIsRecord()) return;
                selectPlayLoadBean = payloadBean;
                allPlayers.get(selectPlayNum).playCamera(payloadBean.getDeviceId());
                allPlayers.get(selectPlayNum).setVideoRecord(false);
            }

            @Override
            public void onDetailClick(PayloadBean payloadBean, int i) {

            }

            @Override
            public void getDeviceTreeLocation(boolean b, String s, String s1) {

            }

            @Override
            public void getDeviceListSuccess(boolean b, String s, String s1) {

            }
        });
    }


    /**
     * 刷新验证码
     *
     * @param view
     */
    public void onClickFreshVerification(View view) {
        mPresenter.getVerificationImage();
    }

    /**
     * 登录
     *
     * @param view
     */
    public void onClickLogin(View view) {
        String userName = edInputUserName.getText().toString();
        String password = edInputUserPassword.getText().toString();
        String verify = edInputVerification.getText().toString();
        if (TextUtils.isEmpty(userName)) {
            toast(R.string.please_input_user_name);
            return;
        }
        if (TextUtils.isEmpty(password)) {
            toast(R.string.please_input_user_password);
            return;
        }
        if (TextUtils.isEmpty(verify)) {
            toast(R.string.please_input_verification);
            return;
        }
        Drawable drawable = ivVerification.getDrawable();
        if (null == drawable) {
            toast(R.string.please_confirm_ip_address);
            return;
        }
        mPresenter.getLogin(userName, password, verify);
    }

    /**
     * 登录页面的展示和隐藏
     */
    public void showHideLoginView(View view) {
        loginGroup.setVisibility(loginGroup.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE);
        tvSelect.setSelected(loginGroup.getVisibility() != View.VISIBLE);
    }

    /**
     * 确定ip的按钮
     *
     * @param view
     */
    public void ipSure(View view) {
        String ipAddress = edInputAddress.getText().toString();
        if (TextUtils.isEmpty(ipAddress)) {
            toast(R.string.please_input_ip);
            return;
        }

        if (ipAddress.contains(":")) {
            String[] strings = ipAddress.split(":");
            AllcamApi.getInstance().init(this, strings[0], Integer.valueOf(strings[1]));
            MMKVUtils.get().putString(PlayConstant.MMKV_IP, strings[0]);
            MMKVUtils.get().putString(PlayConstant.MMKV_PORT, strings[1]);
        } else {
            AllcamApi.getInstance().init(this, ipAddress, 10002);
            MMKVUtils.get().putString(PlayConstant.MMKV_IP, ipAddress);
            MMKVUtils.get().putString(PlayConstant.MMKV_PORT, "");
        }

        onClickFreshVerification(view);
    }

    @Override
    protected boolean isStatusBarEnabled() {
        return true;
    }


    @Override
    public void onDateCallBack(String api, boolean isSuccess, BaseBean baseBean) {
        if (isSuccess) {
            if (null != baseBean) {
                if (AcProtocol.API_USER_LOGIN_VERIF_GET.equals(api)) {
                    // 获取验证码
                    setVerificationData(baseBean);
                } else if (AcProtocol.API_USER_LOGIN.equals(api)) {
                    // 登录接口
                    setUserLogin(baseBean);
                }
            } else {
                toast(getString(R.string.common_data_error, api));
            }
        } else {
            if (null != baseBean) {
                toast(baseBean.getResultDesc());
            }
        }
    }

    @Override
    public void onControlItem(List<ControlBean> controlBeans) {
        controlAdapter.setData(controlBeans);
    }

    @Override
    public void onRightClick(View view) {
        if (null == selectPlayLoadBean) {
            toast(R.string.common_not_select_device);
            return;
        }
        String deviceName = selectPlayLoadBean.getDeviceName();
        String message = getString(R.string.common_select_device, deviceName);
        messageDialog.setMessage(message);
        messageDialog.show();
    }

    /**
     * 用户登录成功的处理
     *
     * @param baseBean
     */
    private void setUserLogin(BaseBean baseBean) {
        if (baseBean instanceof UserLogin) {
            PlayConstant.userLogin = (UserLogin) baseBean;
        }
        toast(R.string.common_login_success);
        MMKVUtils.get().putString(PlayConstant.MMKV_USER_NAME, edInputUserName.getText().toString());
        MMKVUtils.get().putString(PlayConstant.MMKV_USER_PASSWORD, edInputUserPassword.getText().toString());
        getDeviceList("");
    }

    /**
     * 获取设备列表
     *
     * @param s
     */
    private void getDeviceList(String s) {
        if (null != deviceListView) {
            deviceListView.getDeviceTree(s);
        }
    }

    /**
     * 刷新验证码数据
     *
     * @param baseBean
     */
    private void setVerificationData(BaseBean baseBean) {
        if (baseBean instanceof VerificationBean) {
            if (!TextUtils.isEmpty(((VerificationBean) baseBean).getCaptchaData())) {
                String imageUrl = ((VerificationBean) baseBean).getCaptchaData();
                if (imageUrl.contains("base64")) {
                    if (imageUrl.contains(",")) {
                        String[] contents = imageUrl.split(",");
                        if (contents.length > 1) {
                            byte[] decodedString = Base64.decode(imageUrl.split(",")[1], Base64.DEFAULT);
                            Bitmap decodedByte = BitmapFactory.decodeByteArray(decodedString, 0, decodedString.length);
                            Glide.with(this).load(decodedByte).into(ivVerification);
                        }
                    }
                } else {
                    Glide.with(this).load(imageUrl).into(ivVerification);
                }
            }
        }
    }

    /**
     * 读取MMkv的缓存
     */
    private void readMMKV() {
        String ip = MMKVUtils.get().getString(PlayConstant.MMKV_IP, "123.60.129.96");
        String port = MMKVUtils.get().getString(PlayConstant.MMKV_PORT, "10002");
        String userName = MMKVUtils.get().getString(PlayConstant.MMKV_USER_NAME, "admin");
        String password = MMKVUtils.get().getString(PlayConstant.MMKV_USER_PASSWORD, "123456");
        edInputAddress.setText(TextUtils.isEmpty(port) ? ip : ip + ":" + port);
        edInputUserName.setText(userName);
        edInputUserPassword.setText(password);

        int ipPort = TextUtils.isEmpty(port) ? 10000 : Integer.valueOf(port);
        AllcamApi.getInstance().init(this, ip, ipPort);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            if (RECORD_CODE == requestCode) {
                if (null != data) {
                    boolean isSelect = data.getBooleanExtra("select", false);
                    if (isSelect) {
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                try {
                                    Thread.sleep(1000);
                                } catch (InterruptedException e) {
                                    e.printStackTrace();
                                }
                                runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        allPlayers.get(selectPlayNum).replayCamera(PlayConstant.recordListBean, 0);
                                        allPlayers.get(selectPlayNum).setVideoRecord(true);
                                    }
                                });
                            }
                        }).start();
                        /*new Handler().postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        allPlayers.get(selectPlayNum).replayCamera(PlayConstant.recordListBean);
                                        allPlayers.get(selectPlayNum).setVideoRecord(true);
                                    }
                                });
                            }
                        }, 1000);*/
                    }
                }
            } else if (REQUEST_CAMERA_CODE == requestCode && data != null) {
                String searchCameraId = data.getStringExtra(DeviceConfig.NODE_BEAN_ID);
                String searchCameraName = data.getStringExtra(DeviceConfig.NODE_BEAN_NAME);
                // 传递过来的节点类型
                String searchType = data.getStringExtra(DeviceConfig.NODE_BEAN_TYPE);
                if (null != deviceListView) {
                    deviceListView.getSearchDeviceTree(searchCameraId, searchType, searchCameraName);
                }
                PayloadBean payloadBean = new PayloadBean();
                payloadBean.setDeviceId(searchCameraId);
                payloadBean.setDeviceName(searchCameraName);
                payloadBean.setStatus(data.getIntExtra(DeviceConfig.NODE_BEAN_STATUS, 0));
                payloadBean.setDeviceType(data.getIntExtra(DeviceConfig.NODE_BEAN_DEVICETYPE, 0));
                selectPlayLoadBean = payloadBean;
            }
        }
    }

    @Override
    public void cameraHttpListener(String camera, String url, int playBussinessId) {

    }

    @Override
    public void onStateChanged(int state, String msg, int playBussinessId) {
        if (playBussinessId == bussinessIds[selectPlayNum]) {
            changControlPlayStatus(selectPlayNum, ControlType.CONTROL_TYPE_PLAY);
        }
    }

    @Override
    public void platformSnap(boolean success, int playBussinessId) {
        if (playBussinessId == bussinessIds[selectPlayNum]) {
            toast(success ? R.string.platform_snap_success : R.string.platform_snap_error);
        }
    }

    @Override
    public void recordPath(String path, int playBussinessId) {
        if (playBussinessId == bussinessIds[selectPlayNum]) {
            Log.e("TAG", "recordPath: " + path);
            toast(getString(R.string.record_success, path));
        }
    }

//    @Override
//    public void startTalkState(int state) {
//
//    }

    @Override
    public void startTalkState(int state) {
        mAudioCollector = new AudioCollector(new IMediaRecorder() {
            @Override
            public void startMux() {
                Log.e("lhf", "startMux  --->");
            }

            @Override
            public void onAudioError(int what, String message) {
                Log.e("lhf", "onAudioError  --->" + message);
            }

            @Override
            public void receiveAudioData(byte[] sampleBuffer, int dataSize) {
                Log.e("lhf", "receiveAudioData  --->" + sampleBuffer.length);
                //allPlayers.get(selectPlayNum).sendAudioFrame(sampleBuffer, dataSize);
                AllPlayerJni.sendAudio(sampleBuffer, dataSize);

            }
        });
        mAudioCollector.start();
    }

    @Override
    public void recordError() {
        record();
    }

    @Override
    public void recordEnd() {
        timingEnd();
        isRecord = false;
        changControlPlayStatus(selectPlayNum, ControlType.CONTROL_TYPE_VIDEO);
    }

    /**
     * 是否有播放视图的数据
     *
     * @return
     */
    private boolean havePlayView() {
        return null != allPlayers && allPlayers.size() > 0;
    }

    /**
     * 请求权限
     */
    private void requestPermission(String[] permissions, OnPermissionsCallBack callBack) {
        XXPermissions.with(this).permission(permissions).request((list, b) -> {
            if (null != callBack) {
                if (b) {
                    callBack.onGranted();
                } else {
                    callBack.onDenied();
                }
            }
        });
    }


    private interface OnPermissionsCallBack {
        void onGranted();// 允许

        void onDenied();//拒绝
    }

    private void timingBegins() {
        chronometer.setBase(System.currentTimeMillis());
        chronometer.setOnChronometerTickListener(cArg -> {
            long time = System.currentTimeMillis() - cArg.getBase();
            int h = (int) (time / 3600000);
            int m = (int) (time - h * 3600000) / 60000;
            int s = (int) (time - h * 3600000 - m * 60000) / 1000;
            String hh = h < 10 ? "0" + h : h + "";
            String mm = m < 10 ? "0" + m : m + "";
            String ss = s < 10 ? "0" + s : s + "";
            String timeFormat = hh + ":" + mm + ":" + ss;
            chronometer.setText(timeFormat);
        });
        chronometer.setVisibility(View.VISIBLE);
        chronometer.start();
    }

    private void timingEnd() {
        chronometer.stop();
        long time = System.currentTimeMillis() - chronometer.getBase();
        chronometer.setVisibility(View.GONE);
    }

    private void waterMarkInfo(Context context, int type) {
        VideoWaterMark videoWaterMark = new VideoWaterMark();
        videoWaterMark.setFontSize(30);
        videoWaterMark.setFontColor("0x578DFF");
        videoWaterMark.setLocalTime(0);
        videoWaterMark.setPosition(0);
        videoWaterMark.setAlpha(1.0);
        videoWaterMark.setRenderOn(type);
        videoWaterMark.setText("测试123456\n测试13456");
        int code = AllPlayerJni.setWaterMarkInfo(context, 123456, videoWaterMark);
        if (code != 0) {
            toast("水印初始化失败");
        }
    }
}