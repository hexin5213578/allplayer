package com.allcam.player.fragment;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.allcam.basemodule.base.BaseAdapter;
import com.allcam.basemodule.base.BaseDialog;
import com.allcam.basemodule.base.BaseFragment;
import com.allcam.basemodule.base.FragmentBaseActivity;
import com.allcam.basemodule.base.action.StatusAction;
import com.allcam.basemodule.base.action.ToastAction;
import com.allcam.basemodule.base.mvp.presenter.BasePresenter;
import com.allcam.basemodule.dialog.DateTimeDialog;
import com.allcam.basemodule.dialog.MessageDialog;
import com.allcam.basemodule.titlebar.TitleBar;
import com.allcam.basemodule.widget.ClearEditText;
import com.allcam.basemodule.widget.StatusLayout;
import com.allcam.basemodule.widget.WrapRecyclerView;
import com.allcam.http.AllcamApi;
import com.allcam.http.protocol.base.PageInfo;
import com.allcam.http.protocol.record.EventBean;
import com.allcam.http.protocol.record.RecordBean;
import com.allcam.http.protocol.record.SearchInfo;
import com.allcam.player.R;
import com.allcam.player.adapter.RecordAdapter;
import com.allcam.player.constant.PlayConstant;
import com.allcam.player.util.TimeUtil;
import com.hjq.http.lifecycle.ApplicationLifecycle;
import com.hjq.http.listener.OnHttpListener;
import com.scwang.smartrefresh.layout.SmartRefreshLayout;
import com.scwang.smartrefresh.layout.api.RefreshLayout;
import com.scwang.smartrefresh.layout.listener.OnLoadMoreListener;
import com.scwang.smartrefresh.layout.listener.OnRefreshListener;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;

/**
 * @ClassName RecordFragment
 * @Description 录像回放列表
 * @Author liufang
 * @Date 2021/10/19 7:48 下午
 * @Version 1.0
 */
public class RecordFragment extends BaseFragment implements StatusAction, ToastAction, OnRefreshListener, OnLoadMoreListener {

    private TitleBar titleBar;
    // 输入设备Id
    private ClearEditText edInputDeviceId;
    // 搜索的view
    private TextView tvSearchView;
    // 刷新头控件
    private SmartRefreshLayout smartRefreshLayout;
    // 录像回放的list列表
    private WrapRecyclerView ryRecordList;
    // 开始时间选择框
    private TextView tvStartTime;
    // 结束时间选择框
    private TextView tvEndTime;
    private String beginTime;
    private String endTime;
    private String deviceId;
    private int pageNumber = 1;
    private int pageSize = 30;
    private RecordAdapter recordAdapter;
    private StatusLayout mHintLayout;
    private MessageDialog.Builder messageDialog;

    @Override
    protected BasePresenter onCreatePresenter() {
        return null;
    }

    @Override
    protected boolean needEventBus() {
        return false;
    }

    @Override
    protected int getLayoutId() {
        return R.layout.fragment_video_list;
    }

    @Override
    protected void initView() {
        titleBar = (TitleBar) findViewById(R.id.title_bar);
        edInputDeviceId = (ClearEditText) findViewById(R.id.ed_input_device);
        tvSearchView = (TextView) findViewById(R.id.tv_search);
        tvSearchView.setOnClickListener(this);

        smartRefreshLayout = (SmartRefreshLayout) findViewById(R.id.rl_status_refresh);
        smartRefreshLayout.setOnRefreshListener(this::onRefresh);
        smartRefreshLayout.setOnLoadMoreListener(this::onLoadMore);
        ryRecordList = (WrapRecyclerView) findViewById(R.id.rv_record_list);
        recordAdapter = new RecordAdapter(getContext());
        recordAdapter.setOnItemLongClickListener((recyclerView, view, i) -> {
            messageDialog.setMessage("你当前选取的是第" + i + "个item数据，是否确定");
            messageDialog.show();
            PlayConstant.recordListBean = recordAdapter.getItem(i);
            return false;
        });
        ryRecordList.setAdapter(recordAdapter);

        tvStartTime = (TextView) findViewById(R.id.tv_start_time);
        tvStartTime.setOnClickListener(this);
        tvEndTime = (TextView) findViewById(R.id.tv_end_time);
        tvEndTime.setOnClickListener(this);
        mHintLayout = (StatusLayout) findViewById(com.allcam.view.R.id.hl_layout);
    }

    @Override
    protected void initData() {
        String deviceId = getActivity().getIntent().getStringExtra("deviceId");
        if (!TextUtils.isEmpty(deviceId)) {
            edInputDeviceId.setText(deviceId);
            edInputDeviceId.setSelection(deviceId.length());
        }
        endTime = TimeUtil.getTodayTime(TimeUtil.DATE_FORMAT_TYPE_ONE);
        Calendar nowCalendar = TimeUtil.calendarFrom(endTime);
        nowCalendar.add(Calendar.DAY_OF_YEAR, -1);
        beginTime = TimeUtil.getFormatTime(nowCalendar.getTime(), TimeUtil.DATE_FORMAT_TYPE_ONE);
        tvStartTime.setText(beginTime);
        tvEndTime.setText(endTime);
        initMessageDialog();// 初始化对话框
    }

    private void initMessageDialog() {
        messageDialog = new MessageDialog.Builder(getContext()).setMessage("1")
                .setListener(baseDialog -> {
                    Intent intent = new Intent();
                    intent.putExtra("select", true);
                    getActivity().setResult(Activity.RESULT_OK, intent);
                    messageDialog.dismiss();
                    finish();
                });
        messageDialog.create();
    }

    @Override
    public boolean isStatusBarEnabled() {
        return true;
    }

    @Override
    public void onLeftClick(View view) {
        finish();
    }

    @Override
    public void onClick(View view) {
        if (R.id.tv_search == view.getId()) {
            // 查询按钮
            doSearch();
        } else if (R.id.tv_start_time == view.getId()) {
            // 开始时间
            selectTime(true);
        } else if (R.id.tv_end_time == view.getId()) {
            // 结束时间
            selectTime(false);
        }
    }

    /**
     * 做查询操作
     */
    private void doSearch() {
        deviceId = edInputDeviceId.getText().toString();
        if (TextUtils.isEmpty(deviceId)) {
            toast(R.string.please_input_device_id);
            return;
        }

        if (TextUtils.isEmpty(beginTime)) {
            toast(R.string.record_please_input_start_time);
            return;
        }

        if (TextUtils.isEmpty(endTime)) {
            toast(R.string.record_please_input_end_time);
            return;
        }
        int compare = TimeUtil.compareCalendar(TimeUtil.calendarFrom(beginTime), TimeUtil.calendarFrom(endTime));
        if (compare == 1) {
            toast("开始时间不能大于结束时间");
            return;
        }
        smartRefreshLayout.autoRefresh();
    }

    /**
     * 开始查询
     */
    private void startSearch(int pageNumber) {
        PageInfo pageInfo = new PageInfo();
        pageInfo.setPageNum(pageNumber);
        pageInfo.setPageSize(pageSize);

        List<EventBean> eventBeans = new ArrayList<>();
        eventBeans.add(new EventBean(PlayConstant.SEARCH_ALL));

        SearchInfo searchInfo = new SearchInfo();
        searchInfo.setBeginTime(beginTime);
        searchInfo.setEndTime(endTime);
        searchInfo.setFrom(PlayConstant.SEARCH_PLATFORM);
        searchInfo.setEventList(eventBeans);

        AllcamApi.getInstance().getRecordList(this, pageInfo, searchInfo
                , deviceId, new OnHttpListener<RecordBean>() {
                    @Override
                    public void onSucceed(RecordBean recordBean) {
                        smartRefreshLayout.finishRefresh();
                        if (0 == recordBean.getCode()) {
                            // 成功
                            PageInfo pageInfo1 = recordBean.getPageInfo();
                            if (pageInfo1.getPageNum() == 1) {
                                if (null != recordBean.getRecordList()) {
                                    recordAdapter.setData(recordBean.getRecordList());
                                }
                            } else {
                                if (null != recordBean.getRecordList()) {
                                    recordAdapter.addData(recordBean.getRecordList());
                                }
                            }

                            if (null == recordBean.getRecordList() || recordBean.getRecordList().size() < pageSize) {
                                smartRefreshLayout.finishLoadMore(0, true, false);
                                smartRefreshLayout.setEnableLoadMore(false);
                            } else {
                                smartRefreshLayout.finishLoadMore();
                                smartRefreshLayout.setEnableLoadMore(true);
                            }
                        } else {
                            toast(R.string.common_data_error);
                        }
                    }

                    @Override
                    public void onFail(Exception e) {
                        smartRefreshLayout.finishRefresh();
                        smartRefreshLayout.finishLoadMore();
                        toast(e.getMessage());
                    }
                });
    }

    /**
     * 选择时间
     *
     * @param isStart 是否是选择开始时间，true 是，false 不是
     */
    private void selectTime(boolean isStart) {
        new DateTimeDialog.Builder(getContext())
                .setTitle(getString(R.string.date_title))
                // 确定按钮文本
                .setConfirm(getString(R.string.common_confirm))
                // 设置 null 表示不显示取消按钮
                .setCancel(getString(R.string.common_cancel))
                .setListener(new DateTimeDialog.OnListener() {
                    @Override
                    public void onSelected(BaseDialog baseDialog, int year, int month, int day, int hour, int minute, int second) {
                        Calendar calendar = Calendar.getInstance();
                        calendar.set(Calendar.YEAR, year);
                        // 月份从零开始，所以需要减 1
                        calendar.set(Calendar.MONTH, month - 1);
                        calendar.set(Calendar.DAY_OF_MONTH, day);
                        calendar.set(Calendar.HOUR_OF_DAY, hour);
                        calendar.set(Calendar.SECOND, second);
                        calendar.set(Calendar.MINUTE, minute);
                        String selectTime = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(calendar.getTime());
                        if (isStart) {
                            tvStartTime.setText(selectTime);
                            beginTime = selectTime;
                        } else {
                            tvEndTime.setText(selectTime);
                            endTime = selectTime;
                        }
                    }

                    @Override
                    public void onCancel(BaseDialog dialog) {
                        toast("cancel");
                    }
                }).show();
    }

    @Override
    public void onRefresh(@NonNull RefreshLayout refreshLayout) {
        pageNumber = 1;
        startSearch(pageNumber);
    }

    @Override
    public void onLoadMore(@NonNull RefreshLayout refreshLayout) {
        pageNumber++;
        startSearch(pageNumber);
    }

    @Override
    public StatusLayout getStatusLayout() {
        return mHintLayout;
    }
}
