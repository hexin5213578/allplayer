package com.allcam.player.util;

import android.os.Environment;

import com.allcam.basemodule.utils.DateTimeUtil;
import com.allcam.basemodule.utils.VerificationCodeUtil;
import com.allcam.http.protocol.AcProtocol;
import com.allcam.player.BuildConfig;
import com.allcam.player.constant.PlayConstant;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

/**
 * @ClassName Utils
 * @Description TODO
 * @Author liufang
 * @Date 2021/12/10 5:27 下午
 * @Version 1.0
 */
public class Utils {
    /**
     * 获取ClientNonce的方法
     *
     * @param flavor 建议使用唯一标识，渠道名，或者一个固定的字符串
     * @return
     */
    public static String getClientNonce(String flavor) {
        String code = VerificationCodeUtil.getInstance().getCode();
        StringBuffer clientNonce = (new StringBuffer(flavor)).append("0" +
                AcProtocol.CU_TYPE_ANDROID).append(DateTimeUtil.getFormatTime(new Date(),
                "yyyyMMddHHmmssSSS")).append(code);

        return clientNonce.toString();
    }


    /**
     * 根据min和max随机生成一个范围在[min,max]的随机数，包括min和max
     *
     * @param min
     * @param max
     * @return int
     */
    public static int getRandom(int min, int max) {
        Random random = new Random();
        return random.nextInt(max - min + 1) + min;
    }

    /**
     * 根据min和max随机生成count个不重复的随机数组
     *
     * @param min
     * @param max
     * @param count
     * @return int[]
     */
    public static int[] getRandoms(int min, int max, int count) {
        int[] randoms = new int[count];
        List listRandom = new ArrayList();
        if (count > (max - min + 1)) {
            return null;
        }
// 将所有的可能出现的数字放进候选list
        for (int i = min; i <= max; i++) {
            listRandom.add(i);
        }
// 从候选list中取出放入数组，已经被选中的就从这个list中移除
        for (int i = 0; i < count; i++) {
            int index = getRandom(0, listRandom.size() - 1);
            randoms[i] = (int) listRandom.get(index);
            listRandom.remove(index);
        }
        return randoms;
    }


    /***
     * 获取本地抓拍图片地址
     *
     * @return 如果没有用户信息，证明就没有登录，就无法进行本地录像
     */
    public static String getLocalSnapPath() {
        if (null == PlayConstant.userLogin) {
            return "";
        }
        long snapTime = System.currentTimeMillis();
        StringBuilder path = new StringBuilder();
        path.append(Environment.getExternalStorageDirectory())
                .append("/")
                .append(BuildConfig.APPLICATION_ID)
                .append("/")
                .append("image/");
        File file = new File(path.toString());
        if (!file.exists()) {
            file.mkdirs();
        }
        path.append("Snap_")
                .append(DateTimeUtil.getFormatTime(new Date(snapTime), "yyyyMMddHHmmSSS"))
                .append(".jpg");
        file = new File(path.toString());
        if (file.exists()) {
            file.delete();
        }
        try {
            file.createNewFile();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return file.getPath();
    }

    /**
     * 获取录像的播放地址
     *
     * @return 如果没有用户信息，证明就没有登录，就无法进行本地录像
     */
    public static String getVideoTempLocation() {
        if (null == PlayConstant.userLogin) {
            return "";
        }
        String videoTempLocation = "";
        StringBuffer buffer = new StringBuffer();
        buffer.append(Environment.getExternalStorageDirectory())
                .append("/DCIM/")
                .append(BuildConfig.APPLICATION_ID)
                .append("/")
                .append(PlayConstant.userLogin.getUserInfo().getAccount())
                .append("/")
                .append("video/");
        File file = new File(buffer.toString());
        if (!file.exists()) {
            if (file.mkdirs()) {
                videoTempLocation = file.getPath();
            } else {
                videoTempLocation = "";
            }
        } else {
            videoTempLocation = file.getPath();
        }
        return videoTempLocation;
    }
}
