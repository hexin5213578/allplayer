package com.allcam.allplayer.utils;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Created on 5/17/21.
 *
 * @author Des：
 */
public class SnapFileUtils {
    //保存方法
    public static void saveFile( Bitmap bm, String fileName, Context mContext) throws IOException {
//        File foder = new File(subForder);
//        if (!foder.exists()) foder.mkdirs();

        File myCaptureFile = new File( fileName);

        if (!myCaptureFile.exists()) myCaptureFile.createNewFile();

        BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(myCaptureFile));
        bm.compress(Bitmap.CompressFormat.JPEG, 100, bos);
        bos.flush();
        bos.close();

        //发送广播通知系统
        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        Uri uri = Uri.fromFile(myCaptureFile);
        intent.setData(uri);
        mContext.sendBroadcast(intent);
        Log.e("lhf","myCaptureFile = "+myCaptureFile.getAbsolutePath());
    }
}
