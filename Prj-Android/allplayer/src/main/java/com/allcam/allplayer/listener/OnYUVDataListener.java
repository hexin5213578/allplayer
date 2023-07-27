package com.allcam.allplayer.listener;

/**
 * Created on 5/25/21.
 *
 * @author Des：
 */
public interface OnYUVDataListener {
    void onYUVData(int width, int height, byte[] y, byte[] u, byte[] v);
}
