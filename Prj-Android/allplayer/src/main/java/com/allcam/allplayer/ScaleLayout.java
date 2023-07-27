package com.allcam.allplayer;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.util.AttributeSet;
import android.widget.FrameLayout;

import androidx.annotation.Nullable;

import com.allcam.allplayer.utils.DisplayUtil;


/**
 * Created by huyuwen on 16/5/21.
 */
public class ScaleLayout extends FrameLayout {
    private float scale = 9f / 16;
    private int height;
    private Activity mContext;

    public ScaleLayout(Context context) {
        super(context);
    }

    public ScaleLayout(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ScaleLayout(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        this.mContext = getActivityByContext(context);
    }

    public void setScale(float scale, boolean landscape) {
        setScale(scale);
        height = landscape ? DisplayUtil.getScreenWidth(mContext) : DisplayUtil.getScreenHeight(mContext);
    }

    public void setScale(float scale) {
        this.scale = scale;
        invalidate();
        height = 0;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        /*int widthSize = getDefaultSize(0, widthMeasureSpec);
        int heightSize = (int) (widthSize * scale);
        setMeasuredDimension(widthSize, heightSize);
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);*/

        setMeasuredDimension(getDefaultSize(0, widthMeasureSpec), getDefaultSize(0, heightMeasureSpec));

        int widthSize = getMeasuredWidth();
        int heightSize = (int) (widthSize * scale);

        //如果高度计算后超出屏幕高度，将高度设置为屏幕高度
        if (height > 0 && heightSize > height) {
            heightSize = height;
        }
        widthMeasureSpec = MeasureSpec.makeMeasureSpec(widthSize, MeasureSpec.EXACTLY);
        heightMeasureSpec = MeasureSpec.makeMeasureSpec(heightSize, MeasureSpec.EXACTLY);
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
    public static Activity getActivityByContext(Context context){
        while(context instanceof ContextWrapper){
            if(context instanceof Activity){
                return (Activity) context;
            }
            context = ((ContextWrapper) context).getBaseContext();
        }
        return null;
    }
}
