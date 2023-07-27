package com.allcam.player;

import android.app.Application;
import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.core.content.ContextCompat;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.LifecycleRegistry;

import com.alibaba.android.arouter.launcher.ARouter;
import com.allcam.allplayer.AllPlayerJni;
import com.allcam.basemodule.manager.ActivityManager;
import com.allcam.basemodule.titlebar.TitleBar;
import com.allcam.basemodule.titlebar.initializer.LightBarInitializer;
import com.allcam.basemodule.toast.ToastInterceptor;
import com.allcam.basemodule.toast.ToastUtils;
import com.allcam.basemodule.utils.MMKVUtils;
import com.allcam.http.AllcamApi;

import org.libsdl.app.SDL;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.RandomAccessFile;

//import com.squareup.leakcanary.LeakCanary;

/**
 * Created on 5/11/21.
 *
 * @author lhf
 * <p>
 * Des：
 */
public class MyApplication extends Application implements LifecycleOwner {
    private final LifecycleRegistry mLifecycle = new LifecycleRegistry(this);

    @Override
    public void onCreate() {
        super.onCreate();


        AllPlayerJni.loadLibrariesOnce(null, this);
        // Set up JNI

        File log = getExternalFilesDir("Caches");
        String logfile = log.getAbsolutePath() + "/playLog.log";
        File file = new File(logfile);
        String content = "播放器日志：" + "\r\n";
        if (!log.exists()) {
            log.mkdir();
        }
        if (!file.exists()) {
            try {
                file.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
            RandomAccessFile raf = null;
            try {
                raf = new RandomAccessFile(file, "rwd");
                raf.seek(file.length());
                raf.write(content.getBytes());
                raf.close();
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        Log.e("lhflhf", "log path = " + file.getAbsolutePath());
        AllPlayerJni.init(file.getAbsolutePath(), 7);
        mLifecycle.handleLifecycleEvent(Lifecycle.Event.ON_CREATE);
        ActivityManager.getInstance().init(this);
        initSdk(this);
//        LeakCanary.install(this);

    }


    private void initSdk(MyApplication myApplication) {
        // 吐司工具类
        ToastUtils.init(myApplication);
        //初始化sdk的地方
        AllcamApi.getInstance().setEnableLog(true);
        // 设置 Toast 拦截器
        ToastUtils.setToastInterceptor(new ToastInterceptor() {
            @Override
            public boolean intercept(Toast toast, CharSequence text) {
                boolean intercept = super.intercept(toast, text);
                if (intercept) {
                    Log.e("Toast", "空 Toast");
                } else {
                    Log.i("Toast", text.toString());
                }
                return intercept;
            }
        });

        // 设置标题栏初始化器
        TitleBar.setDefaultInitializer(new LightBarInitializer() {

            @Override
            public Drawable getBackgroundDrawable(Context context) {
                return new ColorDrawable(ContextCompat.getColor(myApplication, R.color.common_primary_color));
            }

            @Override
            public Drawable getBackIcon(Context context) {
                return ContextCompat.getDrawable(context, R.drawable.arrows_left_ic);
            }

            @Override
            protected TextView createTextView(Context context) {
                return new AppCompatTextView(context);
            }
        });

        /**
         * 腾讯mmkv初始化
         */
        MMKVUtils.get().init(myApplication);
        /**
         * 阿里路由
         */
        initARouter(myApplication);
    }

    private void initARouter(MyApplication myApplication) {
        if (BuildConfig.DEBUG) {           // 这两行必须写在init之前，否则这些配置在init过程中将无效
            ARouter.openLog();     // 打印日志
            ARouter.openDebug();   // 开启调试模式(如果在InstantRun模式下运行，必须开启调试模式！线上版本需要关闭,否则有安全风险)
        }
        ARouter.init(myApplication); // 尽可能早，推荐在Application中初始化
    }

    @NonNull
    @Override
    public Lifecycle getLifecycle() {
        return mLifecycle;
    }
}
