package com.allcam.allplayer.utils;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.graphics.Point;
import android.media.SoundPool;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

public class DisplayUtil {
    private static final int ALARM_VIBRATOR_TIME = 300;
    private static final int ALARM_TIP_TIME_SPACE = 5000;

    private static DisplayMetrics mDisplayMetrics;
    private static long lastTipTime;
    private static int screenWidth, screenHeight;
    private static SoundPool soundPool;

    public static DisplayMetrics getDisplayMetrics(Activity mActivity) {
        if (null == mDisplayMetrics) {
            mDisplayMetrics = new DisplayMetrics();
            if (null != mActivity) {
                mActivity.getWindowManager().getDefaultDisplay().getMetrics(mDisplayMetrics);
            }
        }
        return mDisplayMetrics;
    }

    private static void initScreenSize(Activity activity) {
        if (null != activity) {
            Point size = new Point();
            activity.getWindowManager().getDefaultDisplay().getSize(size);


            screenWidth = size.x;
            screenHeight = size.y;
        }
    }

    public static int getScreenWidth(Activity activity) {
        if (0 == screenWidth) {
            initScreenSize(activity);
        }
        return screenWidth;
    }

    public static int getScreenHeight(Activity activity) {
        if (0 == screenHeight) {
            initScreenSize(activity);
        }
        return screenHeight;
    }

    public static int dp2px(float dp,Activity activity) {
        float scale = getDisplayMetrics(activity).density;
        return (int) (dp * scale + 0.5F);
    }

    public static float sp2px(float sp,Activity activity) {
        float scale = getDisplayMetrics(activity).scaledDensity;
        return sp * scale;
    }


    public static int getGridImageSize(int totalNum, int column,Activity activity) {
        int size;

        if (totalNum < column) {
            size = getScreenWidth(activity) / (totalNum + 2);
        } else {
            size = getScreenWidth(activity) / (column + 2);
        }

        return size;
    }

    /**
     * 显示软键盘
     */
    public static void showSoftInput(Context activity, View view) {
        if (null != activity && null != view) {
            InputMethodManager imm = (InputMethodManager) activity.getSystemService(Activity.INPUT_METHOD_SERVICE);
            view.requestFocus();
            imm.showSoftInput(view, InputMethodManager.SHOW_IMPLICIT);
        }
    }

    /**
     * 隐藏软键盘
     */
    public static void hideSoftInput(Context activity, View currentFocus) {
        if (null != activity && null != currentFocus) {
            InputMethodManager imm = (InputMethodManager) activity.getSystemService(Activity.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(currentFocus.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
            currentFocus.clearFocus();
        }
    }

    @TargetApi(19)
    public static void setTranslucentStatus(Window win, boolean on) {
        if (null != win && Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            WindowManager.LayoutParams winParams = win.getAttributes();
            final int bits = WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS;
            if (on) {
                winParams.flags |= bits;
            } else {
                winParams.flags &= ~bits;
            }
            winParams.flags &= ~WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION;
            win.setAttributes(winParams);
        }
    }


}
