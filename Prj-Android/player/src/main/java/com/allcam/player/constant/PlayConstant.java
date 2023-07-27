package com.allcam.player.constant;

import com.allcam.http.protocol.Login.UserLogin;
import com.allcam.http.protocol.record.RecordListBean;

/**
 * @ClassName Constance
 * @Description TODO
 * @Author liufang
 * @Date 2021/12/13 4:29 下午
 * @Version 1.0
 */
public class PlayConstant {

    public static final String MMKV_IP = "MMKV_IP";// 通过MMKV持久化保存的ip地址

    public static final String MMKV_PORT = "MMKV_PORT";// 通过MMKV持久化保存的port端口

    public static final String MMKV_USER_NAME = "MMKV_USER_NAME";// 通过MMKV持久化保存的用户名

    public static final String MMKV_USER_PASSWORD = "MMKV_USER_PASSWORD";// 通过MMKV持久化保存的密码

    public static UserLogin userLogin;// 登录信息

    // 需要录像回放的信息
    public static RecordListBean recordListBean;
    // 查询录像回放需要传递的字段
    public static final String SEARCH_PLATFORM = "PLATFORM";
    public static final String SEARCH_ALL = "ALL";
}
