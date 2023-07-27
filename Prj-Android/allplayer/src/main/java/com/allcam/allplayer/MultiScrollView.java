package com.allcam.allplayer;

import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.FrameLayout;
import android.widget.OverScroller;

/**
 * 使用自定义触摸处理的ScrollView
 * Created by Administrator on 2015/8/13.
 */
public class MultiScrollView extends FrameLayout {
    //处理滑动的Scroller 这里如果是api 9以上最好使用OverScroller
    //Scroller的速滑效果很差
    private OverScroller mScroller;
    //判断滑动速度
    private VelocityTracker mVelocityTracker;
    //滑动的阀值
    private int mTouchSlop;
    //滑动速度
    private int mMaxVelocity, mMinVelocity;
    //滑动锁
    private boolean mDragging = false;

    //上一次移动事件的位置
    private float mLastX, mLastY;

    private boolean scrollEnable = false;
    private boolean inScaling = false;

    private ScaleHandler scaleHandler;
    private ScaleGestureDetector scaleGestureDetector;

    private OnScaleChangedListener onScaleChangedListener;

    public MultiScrollView(Context context) {
        super(context);
        init(context);
    }

    public MultiScrollView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public MultiScrollView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    public void init(Context context) {
        mScroller = new OverScroller(context);
        mVelocityTracker = VelocityTracker.obtain();
        //获取系统的触摸阀值
        //可以认为用户是在滑动的距离
        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
        //滑动的最快速度
        mMaxVelocity = ViewConfiguration.get(context).getScaledMaximumFlingVelocity();
        //滑动的最慢速度
        mMinVelocity = ViewConfiguration.get(context).getScaledMinimumFlingVelocity();

        scaleHandler = new ScaleHandler();
        scaleGestureDetector = new ScaleGestureDetector(context, scaleHandler);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            scaleGestureDetector.setQuickScaleEnabled(false);
        }
    }

    public void startScroll() {
        if (!scrollEnable) {
            scrollEnable = true;
            scaleContent(2f);
            post(new Runnable() {
                @Override
                public void run() {
                    scrollTo(getWidth() / 2, getHeight() / 2);
                }
            });
        }
    }

    public void stopScroll() {
        if (scrollEnable) {
            scrollEnable = false;
            scaleContent(0f);
        }
//        scrollTo(0, 0);
    }

    public void scaleContent(float scale) {
        scaleHandler.forceScale(scale);
    }

    //这里的方案是不测量保证视图尽可能按自己的大小来 如果不复写父视图的默认方案会强制子视图和父视图一样大 也就是按父视图的方案来实现
    @Override
    protected void measureChild(View child, int parentWidthMeasureSpec, int parentHeightMeasureSpec) {
        int childWidthMeasureSpec;
        int childHeightMeasureSpec;
        childWidthMeasureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        childHeightMeasureSpec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        child.measure(childWidthMeasureSpec, childHeightMeasureSpec);
    }

    //与上面一样
    @Override
    protected void measureChildWithMargins(View child, int parentWidthMeasureSpec, int widthUsed, int parentHeightMeasureSpec, int heightUsed) {
        MarginLayoutParams lp = (MarginLayoutParams) child.getLayoutParams();
        int childWidthMeasureSpec;
        int childHeightMeasureSpec;
        childWidthMeasureSpec = MeasureSpec.makeMeasureSpec(lp.leftMargin + lp.rightMargin, MeasureSpec.UNSPECIFIED);
        childHeightMeasureSpec = MeasureSpec.makeMeasureSpec(lp.topMargin + lp.bottomMargin, MeasureSpec.UNSPECIFIED);
        child.measure(childWidthMeasureSpec, childHeightMeasureSpec);
    }

    //computeScroll会被定期调用 判断滑动状态
    @Override
    public void computeScroll() {
        //判断滑动状态 返回true表示没有完成
        //使用这个方法保证滑动动画的完成
        if (mScroller.computeScrollOffset()) {
            int oldx = getScrollX();
            int oldy = getScrollY();
            //现在滚动到的x的位置
            int x = mScroller.getCurrX();
            //现在滚总到的y位置
            int y = mScroller.getCurrY();

            if (getChildCount() > 0) {
                View child = getChildAt(0);
                x = clamp(x, getWidth() - getPaddingLeft() - getPaddingRight(), child.getWidth());
                y = clamp(y, getHeight() - getPaddingTop() - getPaddingBottom(), child.getHeight());
                if (x != oldx || y != oldy) {
                    scrollTo(x, y);
                }
            }
            //滑动完成之前一直绘制 就是保证这个方法还会进来
            postInvalidate();
        }
    }

    @Override
    public void scrollTo(int x, int y) {
        //依赖View.ScrollBy方法调用ScrollTo
        if (getChildCount() > 0) {
            View child = getChildAt(0);
            //边界检查
            x = clamp(x, getWidth() - getPaddingLeft() - getPaddingRight(), child.getWidth());
            y = clamp(y, getHeight() - getPaddingTop() - getPaddingBottom(), child.getHeight());
            //如果x== getScrollX()滚动已经完成??
            if (x != getScrollX() || y != getScrollY()) {
                super.scrollTo(x, y);
            }
        }

    }

    //处理快速滑动的方法 参数是滑动速度
    public void fling(int VelocityX, int VelocityY) {
        if (getChildCount() > 0) {
            int height = getHeight() - getPaddingTop() - getPaddingBottom();
            int width = getWidth() - getPaddingLeft() - getPaddingRight();
            int bottom = getChildAt(0).getHeight();
            int right = getChildAt(0).getWidth();

            mScroller.fling(getScrollX(), getScrollY(), VelocityX, VelocityY, 0, Math.max(0, right - width), 0, Math.max(0, bottom - height));
            invalidate();
        }
    }

    //辅助方法判断是否超过边界
    private int clamp(int n, int my, int child) {
        //子View小于父视图或者滑动小于0 不滑动
        if (my >= child || n < 0) {
            return 0;
        }
        //滚动超过了子View的边界,直接滑到边界
        if ((my + n) > child) {
            return child - my;
        }
        return n;
    }

    //监控传递给子视图的触摸事件 一旦进行拖拽就拦截
    //如果子视图是可交互的，允许子视图接收事件
    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (!scrollEnable) {
            return false;
        }

        if (inScaling) {
            if (mDragging) {
                mDragging = false;
                mVelocityTracker.clear();
            }
            return true;
        }

        switch (ev.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                //终止正在进行的滑动
                if (!mScroller.isFinished()) {
                    mScroller.abortAnimation();
                }
                //还原速度追踪器
                mVelocityTracker.clear();
                mVelocityTracker.addMovement(ev);
                //保存初始触点
                mLastX = ev.getX();
                mLastY = ev.getY();
                break;
            case MotionEvent.ACTION_MOVE:
                final float x = ev.getX();
                final float y = ev.getY();
                final int DiffX = (int) Math.abs(x - mLastX);
                final int DiffY = (int) Math.abs(y - mLastY);
                //检查x或者Y方向是否达到了滑动的阀值
                if (DiffX > mTouchSlop || DiffY > mTouchSlop) {
                    mDragging = true;
                    mVelocityTracker.addMovement(ev);
                    //开始自己捕捉触摸事件
                    return true;
                }
                break;
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:
                mDragging = false;
                mVelocityTracker.clear();
                break;
        }

        return super.onInterceptTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!scrollEnable) {
            return false;
        }

        ViewParent parent = getParent();
        if (parent != null) {
            parent.requestDisallowInterceptTouchEvent(true);
        }

        scaleGestureDetector.onTouchEvent(event);
        if (scaleGestureDetector.isInProgress()) {
            return true;
        }

        if (inScaling) {
            if (event.getActionMasked() == MotionEvent.ACTION_UP && event.getPointerCount() == 1) {
                inScaling = false;
            }
            return true;
        }

        //这里所有的事件都会交给检测器
        mVelocityTracker.addMovement(event);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                //已经保留了初始的触点，如果这里不返回true，后续的触摸事件就不会再传递
                return true;
            case MotionEvent.ACTION_MOVE:
                final float x = event.getX();
                final float y = event.getY();
                final float DeltaX = mLastX - x;
                final float DeltaY = mLastY - y;
                //判断阀值
                if ((Math.abs(DeltaX) > mTouchSlop || Math.abs(DeltaY) > mTouchSlop) && !mDragging) {
                    mDragging = true;
                }
                if (mDragging) {
                    //滚动视图
                    scrollBy((int) DeltaX, (int) DeltaY);
                    //更新坐标
                    mLastX = x;
                    mLastY = y;
                }
                break;
            case MotionEvent.ACTION_CANCEL:
                //终止滑动
                mDragging = false;
                if (!mScroller.isFinished()) {
                    mScroller.abortAnimation();
                }
                break;
            case MotionEvent.ACTION_UP:
                mDragging = false;
                //处理快速滑动的情况
                mVelocityTracker.computeCurrentVelocity(1000, mMaxVelocity);
                final int VelocityX = (int) mVelocityTracker.getXVelocity();
                final int VelocityY = (int) mVelocityTracker.getYVelocity();
                if (Math.abs(VelocityX) > mMinVelocity || Math.abs(VelocityY) > mMinVelocity) {
                    //为什么要取负值？ 因为滑动的时候 正值是向上滑动 负值是向下滑动
                    fling(-VelocityX, -VelocityY);
                }
                break;
        }

        return super.onTouchEvent(event);
    }

    public void setOnScaleChangedListener(OnScaleChangedListener listener) {
        this.onScaleChangedListener = listener;
    }

    private void onContentSizeChanged(int newWidth, int newHeight) {
        if (null != onScaleChangedListener) {
            onScaleChangedListener.onContentSizeChanged(newWidth, newHeight);
        }
    }

    private class ScaleHandler extends ScaleGestureDetector.SimpleOnScaleGestureListener {
        private static final float SCALE_MIN = 1f;
        private static final float SCALE_MAX = 3f;

        private View child;
        private float scale = 1f;
        private float lastScale;
        private float scaleX, scaleY;
        private boolean isInit = false;

        @Override
        public boolean onScaleBegin(ScaleGestureDetector detector) {
            if (!inScaling) {
                inScaling = true;
                mScroller.forceFinished(true);
                this.scaleX = detector.getFocusX();
                this.scaleY = detector.getFocusY();

                if (!isInit) {
                    initScale();
                }
            }
            return inScaling;
        }

        @Override
        public boolean onScale(ScaleGestureDetector detector) {


            if (!isInit || !inScaling) {
                return false;
            }

            float s = detector.getScaleFactor();
            float gap = 1f - s;
            if (Math.abs(gap) <= 0.02f) {
                return false;
            }

            makeScale(s, this.scaleX, this.scaleY);
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {


        }

        private void initScale() {
            if (getChildCount() != 1) {
                isInit = false;
                return;
            }

            child = getChildAt(0);
            isInit = true;
        }

        private void makeScale(float currScale, float scaleX, float scaleY) {
            int orgW = getWidth(), orgH = getHeight();
            if (orgW == 0 || orgH == 0) {
                return;
            }

            boolean clamp = false;
            this.scale *= currScale;

            if (scale < SCALE_MIN) {
                clamp = true;
                scale = SCALE_MIN;
            } else if (scale > SCALE_MAX) {
                clamp = true;
                scale = SCALE_MAX;
            }

            if (lastScale == scale) {
                return;
            }

            if (clamp) {
                currScale = this.scale / lastScale;
            }
            lastScale = scale;

            int newWidth = (int) (orgW * scale);
            int newHeight = (int) (orgH * scale);

            float centerX = getScrollX() + scaleX;
            float centerY = getScrollY() + scaleY;
            int newLeft = (int) (centerX * currScale - scaleX);
            int newTop = (int) (centerY * currScale - scaleY);

            if (newLeft > newWidth - orgW)
                newLeft = newWidth - orgW;
            if (newTop > newHeight - orgH)
                newTop = newHeight - orgH;

            setChildSize(child, newWidth, newHeight);
            scrollTo(newLeft, newTop);

            onContentSizeChanged(newWidth, newHeight);
        }

        private void setChildSize(View view, int width, int height) {
            ViewGroup.LayoutParams vLp = view.getLayoutParams();
            if (null != vLp) {
                vLp.width = width;
                vLp.height = height;
                view.setLayoutParams(vLp);
            }

            //view.layout(0, 0, width, height);
            if (view instanceof ViewGroup) {
                ViewGroup vg = (ViewGroup) view;
                if (vg.getChildCount() > 0) {
                    View vgChild = vg.getChildAt(0);
                    setChildSize(vgChild, width, height);
                }
            }
        }

        void forceScale(float s) {
            if (!isInit) {
                initScale();
            }
            makeScale(s, getWidth() / 2, getHeight() / 2);
        }
    }

    public interface OnScaleChangedListener {
        void onContentSizeChanged(int newWidth, int newHeight);
    }
}
