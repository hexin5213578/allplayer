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
import com.allcam.player.bean.ControlBean;

/**
 * Created on 3/3/21.
 *
 * @author lhf
 * <p>
 * Des：
 */
public class ControlAdapter extends AppAdapter<ControlBean> {
    public ControlAdapter(@NonNull Context context) {
        super(context);
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        return new ViewHolder();
    }

    private final class ViewHolder extends AppAdapter.ViewHolder {
        private TextView mTvContent;

        public ViewHolder() {
            super(R.layout.adapter_control_item);
            mTvContent = (TextView) findViewById(R.id.tv_content);
        }

        @Override
        public void onBindView(int position) {
            ControlBean controlBean = getItem(position);
            Integer colorId = controlBean.getColorId();
            mTvContent.setText(controlBean.getContent());
            if (null != colorId) {
                mTvContent.setTextColor(getResources().getColor(colorId));
            }
        }
    }
}
