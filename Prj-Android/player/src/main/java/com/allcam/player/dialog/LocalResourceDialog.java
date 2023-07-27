package com.allcam.player.dialog;

import android.app.Activity;
import android.view.Gravity;
import android.view.View;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.allcam.basemodule.base.BaseAdapter;
import com.allcam.basemodule.base.BaseDialog;
import com.allcam.player.R;
import com.allcam.player.adapter.LocalResourceAdapter;

import java.util.List;

/**
 * @ClassName LocalResourceDialog
 * @Description 选在本地资源的路径下的录制的视频
 * @Author liufang
 * @Date 2022/7/21 14:52
 * @Version 1.0
 */
public class LocalResourceDialog extends BaseDialog.Builder {

    private RecyclerView recyclerView;
    private LocalResourceAdapter localResourceAdapter;
    private OnItemClick onItemClick;

    public LocalResourceDialog(Activity activity, OnItemClick onItemClick) {
        super(activity);
        this.onItemClick = onItemClick;
        setContentView(R.layout.dialog_local_resources);
        setGravity(Gravity.CENTER);
        recyclerView = (RecyclerView) findViewById(R.id.ry_local_resouces);
        localResourceAdapter = new LocalResourceAdapter(getActivity());
        recyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));
        localResourceAdapter.setOnItemClickListener((recyclerView, view, i) -> {
            if (null != onItemClick) {
                onItemClick.onClick(localResourceAdapter.getItem(i));
                dismiss();
            }
        });
        recyclerView.setAdapter(localResourceAdapter);
    }

    public void setDate(List<String> dates) {
        if (null != localResourceAdapter) {
            localResourceAdapter.setData(dates);
        }
    }

    public void addDate(List<String> dates) {
        if (null != localResourceAdapter) {
            localResourceAdapter.addData(dates);
        }
    }

    public interface OnItemClick {
        void onClick(String localSource);
    }

    public LocalResourceAdapter getLocalResourceAdapter() {
        return localResourceAdapter;
    }
}
