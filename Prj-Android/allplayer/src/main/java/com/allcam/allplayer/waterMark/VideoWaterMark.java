package com.allcam.allplayer.waterMark;

/**
 * @ClassName VideoWaterMark
 * @Description 视频水印传递给C++层的bean
 * @Author liufang
 * @Date 2022/10/11 09:56
 * @Version 1.0
 */
public class VideoWaterMark {
    /**
     * 传递过去需要操作的类型标识
     */
    private String businessType;

    /**
     * 需要渲染的内容
     */
    private String text;

    /**
     * 字体颜色
     */
    private String fontColor;

    /**
     * 字体大小
     */
    private int fontSize;

    /**
     * 透明度
     */
    private Double alpha;

    /**
     * 水印渲染的位置
     * kTopLeft = 0,
     * kTopRight = 1,
     * kBottomLeft = 2,
     * kBottomRight = 3,
     */
    private int position;

    /**
     * 水印是否含有本地时间 0-不含有，默认  1-含有
     */
    private int localTime;

    /**
     * 渲染时水印是否开启， 0-关闭，默认， 1-开启
     */
    private int renderOn;


    /**
     * 视频水印渲染的时候 内容格式布局，0：上下，默认， 1：左右
     */
    private int layout;


    public String getBusinessType() {
        return businessType;
    }

    public void setBusinessType(String businessType) {
        this.businessType = businessType;
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }

    public String getFontColor() {
        return fontColor;
    }

    public void setFontColor(String fontColor) {
        this.fontColor = fontColor;
    }

    public int getFontSize() {
        return fontSize;
    }

    public void setFontSize(int fontSize) {
        this.fontSize = fontSize;
    }

    public Double getAlpha() {
        return alpha;
    }

    public void setAlpha(Double alpha) {
        this.alpha = alpha;
    }

    public int getPosition() {
        return position;
    }

    public void setPosition(int position) {
        this.position = position;
    }

    public int getLocalTime() {
        return localTime;
    }

    public void setLocalTime(int localTime) {
        this.localTime = localTime;
    }

    public int getRenderOn() {
        return renderOn;
    }

    public void setRenderOn(int renderOn) {
        this.renderOn = renderOn;
    }

    public int getLayout() {
        return layout;
    }

    public void setLayout(int layout) {
        this.layout = layout;
    }
}
