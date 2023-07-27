package com.allcam.player.mvp.presenter;

import androidx.lifecycle.LifecycleOwner;

import com.allcam.basemodule.base.mvp.presenter.BasePresenter;
import com.allcam.http.AllcamApi;
import com.allcam.http.constant.Constant;
import com.allcam.http.protocol.AcProtocol;
import com.allcam.http.protocol.Imageverification.GetVerificationApi;
import com.allcam.http.protocol.Imageverification.VerificationBean;
import com.allcam.http.protocol.Login.UserLogin;
import com.allcam.http.protocol.Login.UserLoginApi;
import com.allcam.http.protocol.base.BaseBean;
import com.allcam.player.R;
import com.allcam.player.bean.ControlBean;
import com.allcam.player.bean.ControlType;
import com.allcam.player.mvp.control.MainPresenterControl;
import com.hjq.http.lifecycle.ApplicationLifecycle;
import com.hjq.http.listener.OnHttpListener;

import java.util.ArrayList;
import java.util.List;

/**
 * @ClassName MainPresenter
 * @Description TODO
 * @Author liufang
 * @Date 2021/12/13 9:44 上午
 * @Version 1.0
 */
public class MainPresenter extends BasePresenter<MainPresenterControl.MainView> implements MainPresenterControl.MainPresenter {

    public MainPresenter(MainPresenterControl.MainView mView) {
        super(mView);
    }

    @Override
    public void getVerificationImage() {
        GetVerificationApi getVerificationApi = new GetVerificationApi();
        String api = AcProtocol.API_USER_LOGIN;
        String[] splits = api.split("api");
        api = "/api" + splits[splits.length - 1];
        getVerificationApi.setApi(api);
        AllcamApi.getInstance().getVerification((LifecycleOwner) mContext, getVerificationApi, new OnHttpListener<VerificationBean>() {
            @Override
            public void onSucceed(VerificationBean verificationBean) {
                if (isAttach()) {
                    mView.onDateCallBack(AcProtocol.API_USER_LOGIN_VERIF_GET, true, verificationBean);
                }
            }

            @Override
            public void onFail(Exception e) {
                if (isAttach()) {
                    BaseBean baseBean = new BaseBean();
                    baseBean.setResultCode(Constant.NET_ERROR);
                    baseBean.setResultDesc(e.getMessage());
                    mView.onDateCallBack(AcProtocol.API_USER_LOGIN_VERIF_GET, false, baseBean);
                }
            }
        });
    }

    @Override
    public void getLogin(String userName, String password, String verify) {
        AllcamApi.getInstance().updateUserAuth(userName, password);
        UserLoginApi userLoginApi = new UserLoginApi();
        userLoginApi.setVerifCode(verify);
        AllcamApi.getInstance().userLogin((LifecycleOwner) mContext, userLoginApi, new OnHttpListener<UserLogin>() {
            @Override
            public void onSucceed(UserLogin userLogin) {
                if (isAttach()) {
                    mView.onDateCallBack(AcProtocol.API_USER_LOGIN, true, userLogin);
                }
            }

            @Override
            public void onFail(Exception e) {
                if (isAttach()) {
                    BaseBean baseBean = new BaseBean();
                    baseBean.setResultCode(Constant.NET_ERROR);
                    baseBean.setResultDesc(e.getMessage());
                    mView.onDateCallBack(AcProtocol.API_USER_LOGIN, false, baseBean);
                }
            }
        });
    }

    @Override
    public void getControlItem() {
        List<ControlBean> controlBeans = new ArrayList<>();
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_SPLIT, mContext.getResources().getString(R.string.common_mulit_screen)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_PLAY, mContext.getResources().getString(R.string.common_play)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_STOP, mContext.getResources().getString(R.string.common_stop)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_SNAP, mContext.getResources().getString(R.string.common_snap)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_VIDEO, mContext.getResources().getString(R.string.common_video)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_VOICE, mContext.getResources().getString(R.string.common_voice)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_DEFAULT, mContext.getResources().getString(R.string.common_default)));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_LISTENER, "监听"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_TYPE_DRAG_FORWARD, "拖拽"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_START_RECORD, "音频开始"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_STOP_RECORD, "音频结束"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_LOCAL_PLAY, "本地播放开始"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_LOCAL_STOP, "结束本地播放"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_WATER_MARK_OPEN, "开启水印"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_WATER_MARK_CLOSE, "关闭水印"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_GET_PLAY_INFORMATION, "获取播放时候的详细信息"));
        controlBeans.add(new ControlBean(ControlType.CONTROL_GET_VIDEO_INFORMATION, "获取设备播放的信息"));
        mView.onControlItem(controlBeans);
    }
}
