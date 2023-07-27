package com.allcam.player.adapter;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.allcam.basemodule.base.AppAdapter;
import com.allcam.http.protocol.record.RecordListBean;
import com.allcam.player.R;

/**
 * Created on 3/3/21.
 *
 * @author lhf
 * <p>
 * Des：
 */
public class RecordAdapter extends AppAdapter<RecordListBean> {
    public RecordAdapter(@NonNull Context context) {
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
            mTvName.setText(getItem(position).getBeginTime() + "---" + getItem(position).getEndTime());
        }
    }
}
