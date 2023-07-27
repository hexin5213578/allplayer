package com.allcam.player.control;

import android.content.Context;

/**
 * @ClassName LoginControl
 * @Description 所有接口的调用回调机制
 * @Author liufang
 * @Date 2021/12/10 4:04 下午
 * @Version 1.0
 */
public class ApiControl {

    private Context context;

    public ApiControl(Context context) {
        this.context = context;
    }


    public interface APiCallBack {

    }
}
