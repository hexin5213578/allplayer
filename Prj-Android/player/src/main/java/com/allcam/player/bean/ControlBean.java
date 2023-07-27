package com.allcam.player.bean;

/**
 * @ClassName ControlBean
 * @Description 表示控制按钮的类别和属性
 * @Author liufang
 * @Date 2021/12/14 9:40 上午
 * @Version 1.0
 */
public class ControlBean {

    private String label;// 控制器按钮的类别
    private String content;// 控制器按钮的内容
    private Integer colorId; // 渲染颜色用的

    public ControlBean(String label, String content) {
        this.label = label;
        this.content = content;
    }

    public String getLabel() {
        return label;
    }

    public void setLabel(String label) {
        this.label = label;
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }

    public Integer getColorId() {
        return colorId;
    }

    public void setColorId(Integer colorId) {
        this.colorId = colorId;
    }
}
