package com.allcam.player.util;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

import com.allcam.player.R;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * @ClassName TimeUtil
 * @Author jwb
 * @Date 2019/8/24 11:04
 */
public class TimeUtil {
    private static final String TAG = TimeUtil.class.getSimpleName();
    public static final String DATE_FORMAT_TYPE_ONE = "yyyy-MM-dd HH:mm:ss";
    public static final String DATE_FORMAT_TYPE_TWO = "yyyy-MM-dd";
    public static final String DATE_FORMAT_TYPE_THREE = "yyyy.MM.dd";
    public static final String DATE_FORMAT_TYPE_FOUR = "yyyy-MM-dd";
    public static final String DATE_FORMAT_TYPE_FIVE = "yyyyMMddHHmmssSSS";
    public static final String DATE_FORMAT_TYPE_SIX = "yyyy-MM-dd HH:mm";
    public static final String DATE_FORMAT_TYPE_SEVEN = "HH:mm:ss";

    /**
     * 获取指定时间前mount天时间
     *
     * @param time 指定时间
     * @return
     */
    public static String getBeforeTime(String time, int amount) {
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Calendar calendar = calendarFrom(time);
        Date date = calendar.getTime();
        calendar.setTime(date);
        calendar.add(Calendar.DAY_OF_MONTH, -amount);
        date = calendar.getTime();
        return df.format(date);
    }

    /**
     * 获取指定时间前30s
     *
     * @param time 指定时间
     * @return
     */
    public static String getBeforeTime(String time) {
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Calendar calendar = calendarFrom(time);
        Date date = calendar.getTime();
        calendar.setTime(date);
        calendar.add(Calendar.SECOND, -30);
        date = calendar.getTime();
        return df.format(date);
    }

    /**
     * 获取指定时间后30s
     *
     * @param time 指定时间
     * @return
     */
    public static String getAfterTime(String time) {
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Calendar calendar = calendarFrom(time);
        Date date = calendar.getTime();
        calendar.setTime(date);
        calendar.add(Calendar.SECOND, +30);
        date = calendar.getTime();
        return df.format(date);
    }


    /**
     * 获取当天时间
     *
     * @return
     */
    public static String getTodayTime() {
        return getTodayTime(DATE_FORMAT_TYPE_TWO);
    }

    /**
     * 获取当天时间
     *
     * @return
     */
    public static String getTodayTime(String pattern) {
        SimpleDateFormat df = new SimpleDateFormat(pattern);
        Date date = new Date(System.currentTimeMillis());
        return df.format(date);
    }


    public static String getTodayTime1(String pattern) {
        SimpleDateFormat df = new SimpleDateFormat(pattern);
        Date date = new Date(System.currentTimeMillis());
        return df.format(date);
    }


    /**
     * 时间字符串转Calendar
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @return java.util.Calendar
     */
    public static Calendar calendarFrom(String pattern, String time) {
        SimpleDateFormat sdf = new SimpleDateFormat(pattern, Locale.getDefault());
        try {
            Calendar calendar = Calendar.getInstance();
            calendar.setTime(sdf.parse(time));
            return calendar;
        } catch (ParseException e) {
            Log.d(TAG, "time format illegal");
            return null;
        }
    }

    /**
     * 时间字符串转Calendar
     *
     * @param time 时间字符串
     * @return
     */
    public static Calendar calendarFrom(String time) {
        return calendarFrom(DATE_FORMAT_TYPE_ONE, time);
    }

    /**
     * 获取特定时间 月的第一天
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @return
     */
    public static String getFirstDayOfMonth(String pattern, String time) {
        SimpleDateFormat format = new SimpleDateFormat(pattern);
        Calendar cale = calendarFrom(pattern, time);
        cale.add(Calendar.MONTH, 0);
        cale.set(Calendar.DAY_OF_MONTH, 1);
        return format.format(cale.getTime());
    }

    /**
     * 获取特定时间 月的最后一天
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @return
     */
    public static String getLastDayOfMonth(String pattern, String time) {
        SimpleDateFormat format = new SimpleDateFormat(pattern);
        Calendar cale = calendarFrom(pattern, time);
        cale.add(Calendar.MONTH, 1);
        cale.set(Calendar.DAY_OF_MONTH, 0);
        return format.format(cale.getTime());
    }

    /**
     * 获取特定时间 星期的第一天
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @return
     */
    public static String getFirstDayOfWeek(String pattern, String time) {
        SimpleDateFormat format = new SimpleDateFormat(pattern);
        Calendar calendar = calendarFrom(pattern, time);
        calendar.setFirstDayOfWeek(Calendar.MONDAY);
        int firstDayOfWeek = calendar.getFirstDayOfWeek();
        calendar.set(Calendar.DAY_OF_WEEK, firstDayOfWeek);
        return format.format(calendar.getTime());
    }

    /**
     * 获取特定时间 星期的最后一天
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @return
     */
    public static String getLastDayOfWeek(String pattern, String time) {
        SimpleDateFormat format = new SimpleDateFormat(pattern);
        Calendar calendar = calendarFrom(pattern, time);
        calendar.setFirstDayOfWeek(Calendar.MONDAY);
        int firstDayOfWeek = calendar.getFirstDayOfWeek();
        calendar.set(Calendar.DAY_OF_WEEK, firstDayOfWeek);
        calendar.add(Calendar.DATE, 6);
        return format.format(calendar.getTime());
    }

    /**
     * 获取指定时间 前后amount天时间
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @param amount  天数
     * @return
     */
    public static String getTimeOfAmountDay(String pattern, String time, int amount) {
        SimpleDateFormat df = new SimpleDateFormat(pattern);
        Calendar calendar = calendarFrom(pattern, time);
        calendar.add(Calendar.DAY_OF_MONTH, amount);
        return df.format(calendar.getTime());
    }

    /**
     * 获取指定时间 前后amount月时间
     *
     * @param pattern 时间格式
     * @param time    时间字符串
     * @param amount  月数
     * @return
     */
    public static String getTimeOfAmountMonth(String pattern, String time, int amount) {
        SimpleDateFormat df = new SimpleDateFormat(pattern);
        Calendar calendar = calendarFrom(pattern, time);
        calendar.add(Calendar.MONTH, amount);
        return df.format(calendar.getTime());
    }

    /**
     * 将字符串时间格式化为相应的时间串
     *
     * @param time      时间字符串
     * @param inFormat  转入时间格式
     * @param outFormat 转出时间格式
     * @return
     */
    public static String transTimeFormat(String time, String inFormat, String outFormat) {
        Date date = new Date(transTimeToMills(time, inFormat));
        return getFormatTime(date, outFormat);
    }

    /**
     * 获得时间格式化字符串
     *
     * @param date 要格式化的时间
     * @return 格式化后时间字符串(YYYY - MM - DD), 异常则返回空字符串
     */
    public static String getFormatTime(Date date, String formatter) {
        if (null == formatter || null == date) {
            Log.e(TAG, "param is null");
            return "";
        }
        SimpleDateFormat sdf = new SimpleDateFormat(formatter, Locale.getDefault());
        return sdf.format(date);
    }

    /**
     * 获得时间格式化字符串
     *
     * @param time 要格式化的时间
     * @return 格式化后时间字符串(yyyy - MM - dd HH : mm : ss), 异常则返回空字符串
     */
    public static String getFormatTime(long time) {
        Date date = new Date(time);
        return getFormatTime(date, DATE_FORMAT_TYPE_ONE);
    }

    /**
     * 将字符串数据转化为毫秒数
     */
    public static long transTimeToMills(String time, String format) {
        if (TextUtils.isEmpty(time)) {
            return 0;
        }
        if (time.length() > format.length()) {
            time = time.substring(0, format.length());
        }
        long millis;
        Calendar c = Calendar.getInstance();
        try {
            c.setTime(new SimpleDateFormat(format, Locale.getDefault()).parse(time));
            millis = c.getTimeInMillis();
        } catch (ParseException e) {
            millis = 0;
            Log.e(TAG, e.getMessage());
        }
        return millis;
    }

    /**
     * 两个日期比较
     *
     * @param left  日期
     * @param right 日期
     * @return 0相等，如（2019.01.01 和 2019.01.01）
     * 负数(-1)表示left在right之前，如（2019.01.01 和 2019.02.01）
     * 正数(1)表示left在right之后如（2019.02.01 和 2019.01.01）
     */
    public static int compareCalendar(Calendar left, Calendar right) {
        if (null == left || null == right) {
            return 0;
        }
        return left.compareTo(right);
    }

    /**
     * 通过日期number获取星期几
     *
     * @param context
     * @param number  1-7 表示周一到周日
     * @return
     */
    public static String getWeekDayForNumber(Context context, int number) {
        if (context == null) {
            return null;
        }
        String weekDate = null;
        switch (number) {
            case 1:
                weekDate = context.getString(R.string.attendance_statistics_week_monday);
                break;
            case 2:
                weekDate = context.getString(R.string.attendance_statistics_week_tuesday);
                break;
            case 3:
                weekDate = context.getString(R.string.attendance_statistics_week_wednesday);
                break;
            case 4:
                weekDate = context.getString(R.string.attendance_statistics_week_thursday);
                break;
            case 5:
                weekDate = context.getString(R.string.attendance_statistics_week_friday);
                break;
            case 6:
                weekDate = context.getString(R.string.attendance_statistics_week_saturday);
                break;
            case 7:
                weekDate = context.getString(R.string.attendance_statistics_week_sunday);
                break;
            default:
                weekDate = null;
                break;
        }
        return weekDate;
    }

    /**
     * 根据规则判断是否可以打卡
     *
     * @param rule 传入的规则 0000000,1000000， 规则就是数字的下标从0开始，对应的是星期一~星期天，数字0表示不可以
     *             打卡，1是可以打卡。
     * @return true 是可以的，false 是不可以的
     */
    public static boolean isCanAttendance(String rule) {
        if (TextUtils.isEmpty(rule) && rule.length() < 7) {
            return false;
        }
        Calendar calendar = Calendar.getInstance();
        int index = 0;
        if (calendar.get(Calendar.DAY_OF_WEEK) == 1) {
            index = 7;
        } else {
            index = calendar.get(Calendar.DAY_OF_WEEK) - 1;
        }
        StringBuffer stringBuffer = new StringBuffer(rule);
        return stringBuffer.substring(index - 1, index).equals("1");
    }

    public static boolean isOneDay(Date dateStart, Date dateEnd) {
        Calendar calendar = Calendar.getInstance();
        calendar.setTime(dateStart);
        int year = calendar.get(Calendar.YEAR);
        int month = calendar.get(Calendar.MONTH);
        int day = calendar.get(Calendar.DAY_OF_MONTH);
        calendar.setTime(dateEnd);
        return year == calendar.get(Calendar.YEAR) && month == calendar.get(Calendar.MONTH) && day == calendar.get(Calendar.DAY_OF_MONTH);
    }

    public static List<Map<String, String>> getDayFromDates(Date dateStart, Date dateEnd) {
        List<Map<String, String>> list = new ArrayList<>();
        if (isOneDay(dateStart, dateEnd)) {
            Map<String, String> map = new HashMap<>();
            map.put("start", TimeUtil.getFormatTime(dateStart, TimeUtil.DATE_FORMAT_TYPE_ONE));
            map.put("end", TimeUtil.getFormatTime(dateEnd, TimeUtil.DATE_FORMAT_TYPE_ONE));
            list.add(map);
        } else {
            Calendar calendar = Calendar.getInstance();
            calendar.setTime(dateStart);
            Map<String, String> map;
            while (calendar.getTime().getTime() < dateEnd.getTime()) {
                map = new HashMap<>();
                map.put("start", TimeUtil.getFormatTime(calendar.getTime(), TimeUtil.DATE_FORMAT_TYPE_ONE));
                if (isOneDay(calendar.getTime(), dateEnd)) {
                    calendar.setTime(dateEnd);
                } else {
                    calendar.set(Calendar.HOUR_OF_DAY, 23);
                    calendar.set(Calendar.MINUTE, 59);
                    calendar.set(Calendar.SECOND, 59);
                }
                map.put("end", TimeUtil.getFormatTime(calendar.getTime(), TimeUtil.DATE_FORMAT_TYPE_ONE));
                list.add(map);
                calendar.set(Calendar.DAY_OF_MONTH, calendar.get(Calendar.DAY_OF_MONTH) + 1);
                calendar.set(Calendar.HOUR_OF_DAY, 0);
                calendar.set(Calendar.MINUTE, 0);
                calendar.set(Calendar.SECOND, 0);
            }
        }
        return list;
    }

}
