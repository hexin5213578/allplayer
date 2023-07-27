package com.allcam.player.bean;

/**
 * @ClassName ControlType
 * @Description TODO
 * @Author liufang
 * @Date 2021/12/14 9:42 上午
 * @Version 1.0
 */
public interface ControlType {

    // 四分屏，单屏的切换
    String CONTROL_TYPE_SPLIT = "CONTROL_TYPE_SPLIT";

    // 播放，暂停按钮的切换
    String CONTROL_TYPE_PLAY = "CONTROL_TYPE_PLAY";

    // 抓拍按钮
    String CONTROL_TYPE_SNAP = "CONTROL_TYPE_SNAP";

    // 录像，结束录像的状态
    String CONTROL_TYPE_VIDEO = "CONTROL_TYPE_VIDEO";

    // 声音的控制按钮
    String CONTROL_TYPE_VOICE = "CONTROL_TYPE_VOICE";

    // 停止按钮
    String CONTROL_TYPE_STOP = "CONTROL_TYPE_STOP";

    // 默认的
    String CONTROL_TYPE_DEFAULT = "CONTROL_TYPE_DEFAULT";
    /**
     * 拖拽
     */
    String CONTROL_TYPE_DRAG_FORWARD = "CONTROL_TYPE_DRAG_FORWARD";// 拖拽向前

    String CONTROL_TYPE_DRAG_BACKWARD = "CONTROL_TYPE_DRAG_FORWARD";// 拖拽向后

    String CONTROL_TYPE_LISTENER = "CONTROL_TYPE_LISTENER";
    //开始
    String CONTROL_START_RECORD = "CONTROL_START_RECORD";
    //结束
    String CONTROL_STOP_RECORD = "CONTROL_START_RECORD";

    /**
     * 播放本地的mp4的文件
     */
    String CONTROL_LOCAL_PLAY = "CONTROL_PLAY_LOCAL";

    /**
     * 本地播放mp4文件结束
     */
    String CONTROL_LOCAL_STOP = "CONTROL_STOP_PLAY_LOCAL";

    /**
     * 水印开启
     */
    String CONTROL_WATER_MARK_OPEN = "CONTROL_WATER_MARK_OPEN";

    /**
     * 水印关闭
     */
    String CONTROL_WATER_MARK_CLOSE = "CONTROL_WATER_MARK_CLOSE";

    /**
     * 获取播放的详细信息
     */
    String CONTROL_GET_PLAY_INFORMATION = "CONTROL_GET_PLAY_INFORMATION";


    /**
     * 获取设备的信息
     */
    String CONTROL_GET_VIDEO_INFORMATION = "CONTROL_GET_VIDEO_INFORMATION";
}
