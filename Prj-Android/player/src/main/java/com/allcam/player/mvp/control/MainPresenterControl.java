package com.allcam.player.mvp.control;

import com.allcam.basemodule.base.mvp.presenter.IBasePresenter;
import com.allcam.basemodule.base.mvp.view.IBaseView;
import com.allcam.http.model.HttpData;
import com.allcam.http.protocol.base.BaseBean;
import com.allcam.http.protocol.device.DeviceTree;
import com.allcam.player.bean.ControlBean;

import java.util.List;

/**
 * @ClassName MainPresenterControl
 * @Description TODO
 * @Author liufang
 * @Date 2021/12/13 9:42 上午
 * @Version 1.0
 */
public interface MainPresenterControl {

    interface MainView extends IBaseView {
        void onDateCallBack(String api, boolean isSuccess, BaseBean baseBean);

        void onControlItem(List<ControlBean> controlBeans);
    }

    interface MainPresenter extends IBasePresenter {
        /**
         * 获取登录的图形验证码
         */
        void getVerificationImage();

        /**
         * 登录
         *
         * @param userName 用户名
         * @param password 密码
         * @param verify   验证码
         */
        void getLogin(String userName, String password, String verify);

        /**
         * 获取控制器的内容
         */
        void getControlItem();
    }
}
