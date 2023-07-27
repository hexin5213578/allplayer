package com.allcam.allplayer.playConfig;

/**
 * @ClassName PlayConfig
 * @Description 这个是从Java层传递到C++层，需要的播放参数的数据Bean，进行收拢,目前主要收拢的是实时浏览和录像回放的数据
 * @Author liufang
 * @Date 2022/7/14 09:18
 * @Version 1.0
 */
public class PlayConfig {

    private String url;// 播放的url

    private long bussinessId;// 视图层绑定的bussinessId

    private boolean real;// 是否是正在实时浏览 true为实时浏览，false为录像回放

    /**
     * 播放器选择播放的模式
     * 0 是实时优先，不会缓存帧数
     * 1 是画质优先，会缓存帧数
     */
    private int playMode;

    /**
     * 这里对应的是1s25帧，所以，传递的范围是200ms-10000ms
     * 也就是5-25帧
     */
    private int crashFrames;

    /**
     * 设备的平台类型，录像回放的时候去做业务处理
     */
    private int devicePlatType;

    public PlayConfig(String url, int bussinessId, boolean real, int playMode, int crashFrames, int devicePlatType) {
        this.url = url;
        this.bussinessId = bussinessId;
        this.real = real;
        this.playMode = playMode;
        this.crashFrames = crashFrames;
        this.devicePlatType = devicePlatType;
    }

    public PlayConfig() {
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public long getBussinessId() {
        return bussinessId;
    }

    public void setBussinessId(long bussinessId) {
        this.bussinessId = bussinessId;
    }

    public boolean isReal() {
        return real;
    }

    public void setReal(boolean real) {
        this.real = real;
    }

    public int getPlayMode() {
        return playMode;
    }

    public void setPlayMode(int playMode) {
        this.playMode = playMode;
    }

    public int getCrashFrames() {
        return crashFrames;
    }

    public void setCrashFrames(int crashFrames) {
        this.crashFrames = crashFrames;
    }

    public int getDevicePlatType() {
        return devicePlatType;
    }

    public void setDevicePlatType(int devicePlatType) {
        this.devicePlatType = devicePlatType;
    }
}
