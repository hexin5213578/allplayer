package com.allcam.player.adapter;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.allcam.basemodule.base.AppAdapter;
import com.allcam.player.BuildConfig;
import com.allcam.player.R;

/**
 * @ClassName LocalResourceAdapter
 * @Description 本地录像资源展示
 * @Author liufang
 * @Date 2022/7/21 15:01
 * @Version 1.0
 */
public class LocalResourceAdapter extends AppAdapter<String> {

    public LocalResourceAdapter(@NonNull Context context) {
        super(context);
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        return new ViewHolder();
    }

    private final class ViewHolder extends AppAdapter.ViewHolder {
        private TextView mTvName;
        private ImageView btn_detail;

        public ViewHolder() {
            super(R.layout.item_device);
            getItemView().setBackground(getResources().getDrawable(R.drawable.shape_device_bg));
            mTvName = (TextView) findViewById(R.id.tv_name);
            btn_detail = (ImageView) findViewById(R.id.btn_detail);
        }

        @Override
        public void onBindView(int position) {
            btn_detail.setVisibility(View.INVISIBLE);
            String local = getItem(position);
            String[] strings = local.split(BuildConfig.APPLICATION_ID);
            mTvName.setText(strings[1]);
        }
    }
}
