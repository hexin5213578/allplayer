//
// Created by lhf on 5/10/21.
//
#include <sys/ptrace.h>
#include <jni.h>
#include <stddef.h>
#include <android/native_window_jni.h>

#include <allplayer_manager.h>
#include <JavaListenerContainer.h>
#include "libMediaService.h"
//#include "AndroidLog.h"
#include "AndroidLog.h"
#include "SDL2/SDL_test_common.h"

#define LOG_TAG "AllPlayerJNI"

extern "C"
{
#include "libavcodec/jni.h"
};

#define JNIREG_CLASS "com/allcam/allplayer/AllPlayerJni"//指定要注册的类，实现JNI的动态注册

JavaVM *jvm;

std::map<long, JavaListenerContainer *> g_Listeners;

void status_callback(long lBusiness, long busType, long status, const char *info) {
    if (g_Listeners.find(lBusiness) != g_Listeners.end()) {
        LOGE(LOG_TAG, "执行onNativePlayerState");

        g_Listeners[lBusiness]->onNativePlayerState->callback(1, info, status);
    }
}

void data_callback(long lBusiness, long busType, long current, long total) {
    LOGE(LOG_TAG, "执行onNativePlayerBackData");
    if (g_Listeners.find(lBusiness) != g_Listeners.end()) {
        switch (busType) {
            case TYPE_NETRECORD_START:
                g_Listeners[lBusiness]->onNativePlayerBackData->callback(11, current);
                break;

        }
    }
}


int main(int argc, char *argv[]) {
    LOGE(LOG_TAG, "main！!");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        auto err = SDL_GetError();
        AS_LOG(AS_LOG_ERROR, "Could not initialize SDL - %s\n", SDL_GetError());
        return AS_ERROR_CODE_FAIL;
    }
    LOGE(LOG_TAG, "11111main！!");

    return 0;
}

/**
 * 初始化
 */
void nativie_init(JNIEnv *env, jobject clazz, jstring log_path,
                  jint log_level) {


    int jstrUtf16Len = env->GetStringLength(log_path);
    int jstrUtf8Len = env->GetStringUTFLength(log_path);
    if (LOG_DEBUG) {
        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
    }
    char *source = new char[jstrUtf8Len + 1];// 回收放在 AllPlayer 中
    memset(source, 0, jstrUtf8Len + 1);
    env->GetStringUTFRegion(log_path, 0, jstrUtf16Len, source);
    LOGD(LOG_TAG, "nativie_init: %s, %d", source, log_level);
    int res = ap_lib_init(source,log_level);
    if (LOG_DEBUG) {
        LOGE(LOG_TAG, "allplayer nativieInit allplayerInit = [%d]", res);
    }


    ap_lib_reg_data_callback(data_callback);
    ap_lib_reg_status_callback(status_callback);

    LOGE(LOG_TAG, "allplayer初始化成功");
}

/**
 * 本地播放停止
 */
//void native_stop(JNIEnv *env, jobject thiz, jlong bussiness,
//                 jboolean bReal) {
//    BizParams aStruct;
//    // TODO: implement nativeStop()
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer nativeStop ");
//    }
//    aStruct.BizType = bReal ? TYPE_REALVIDEO_STOP : TYPE_NETRECORD_STOP;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativeStop...allplayerExcuteBusiness  = [%d]", res);
//    }
//}


/**
 * 播放
 */
//int native_play(JNIEnv *env, jobject thiz, jobject playConfig) {
//    BizParams aStruct;
//    jclass play_config = env->GetObjectClass(playConfig);
//    if (NULL == play_config) {
//        jclass exceptionClass = env->FindClass("java/lang/Exception");
//        env->ThrowNew(exceptionClass,
//                      "Can't not find playConfig class");
//        env->DeleteLocalRef(exceptionClass);
//        return -1;
//    }
//
//    jfieldID bussinessIdFieldID = env->GetFieldID(play_config, "bussinessId", "J"); //获取播放属性的视图Id
//    jfieldID urlFieldID = env->GetFieldID(play_config, "url", "Ljava/lang/String;"); // 获得播放属性的url
//    jfieldID realFiledID = env->GetFieldID(play_config, "real", "Z"); // 获取播放属性在那种播放状态
//    jfieldID playModeID = env->GetFieldID(play_config, "playMode", "I");// 获取播放属性当前的播放模式
//    jfieldID playCrashFramesID = env->GetFieldID(play_config, "crashFrames", "I");// 获取播放属性需要缓存的帧数
//    jfieldID devicePlayType = env->GetFieldID(play_config, "devicePlatType", "I");// 获取当前的平台类型
//
//    jlong bussiness = env->GetLongField(playConfig, bussinessIdFieldID);// 获取类型值
//    jstring url = (jstring) env->GetObjectField(playConfig, urlFieldID);// 获取播放rtsp
//    jboolean bReal = env->GetBooleanField(playConfig, realFiledID);// 获取是实时浏览还是录像回放
//    jint playMode = env->GetIntField(playConfig, playModeID);// 获取播放的类型
//    jint playCrashFrames = env->GetIntField(playConfig, playCrashFramesID);// 获取缓存的帧数处理
//    jint devicePlatType = env->GetIntField(playConfig, devicePlayType);// 获取
//    if (LOG_DEBUG) {
//        LOGE("XPlay", "allplayer nativePlay %lld", bussiness);
//    }
//    if (g_Listeners.find(bussiness) != g_Listeners.end()) {
//        auto container = g_Listeners[bussiness];
//        delete container;
//    }
//    JavaListenerContainer *javaListenerContainer = new JavaListenerContainer();
//    javaListenerContainer->onNativePlayerState = new OnNativePlayerState(jvm, env, thiz);
//    javaListenerContainer->onNativePlayerBackData = new OnNativePlayerBackData(jvm, env, thiz);
//
//    g_Listeners[bussiness] = javaListenerContainer;
//    if (url == NULL || env->GetStringUTFLength(url) == 0) {
//        jclass exceptionClass = env->FindClass("java/lang/Exception");
//        env->ThrowNew(exceptionClass,
//                      "Can't set a 'null' string to data source!");
//        env->DeleteLocalRef(exceptionClass);
//        return -1;
//    }
//
//    int jstrUtf16Len = env->GetStringLength(url);
//    int jstrUtf8Len = env->GetStringUTFLength(url);
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
//    }
//    char *source = new char[jstrUtf8Len + 1];// 回收放在 AllPlayer 中
//    env->GetStringUTFRegion(url, 0, jstrUtf16Len, source);
//    source[jstrUtf8Len] = '\0';
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "nativeSetDataSource: %s", source);
//    }
//    aStruct.Url = source;
//    aStruct.WindowsHandle = CAllplayerManager::GetInstance()->getNativeWindow(bussiness);
//    if (!aStruct.WindowsHandle) {
//        LOGE("XPlay", "nativePlay getNativeWindow failed = [%lld]", bussiness);
//    }
//
//    aStruct.BizType = bReal ? TYPE_REALVIDEO_START : TYPE_NETRECORD_START;
//    if (!bReal) { // 如果是录像回放，需要判断平台类型
////        if (devicePlatType == 9) {
////            aStruct.ScaleOrSpeed = 1; // 这里不使用speed
////        } else {
//        aStruct.ScaleOrSpeed = 0;
////        }
//    }
//    aStruct.RealQualityPrior = playMode;
//    aStruct.CacheSize = playMode == 1 ? playCrashFrames : 0;
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE("XPlay", "nativePlay...allplayerExcuteBusiness  = [%d]", res);
//        LOGE("XPlay", "nativePlay...RealQualityPrior = [%d]", playMode);
//        LOGE("XPlay", "nativePlay...CacheSize = [%d]", playCrashFrames);
//    }
//    if (res != 0) {
//        native_stop(env, thiz, bussiness, true);
//    }
//    return res;
//}


/**
 * 停止
 */
//void native_local_stop(JNIEnv *env, jobject thiz, jlong bussiness) {
//    BizParams aStruct;
//    // TODO: implement nativeStop()
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer nativeStop ");
//    }
//    aStruct.BizType = TYPE_URL_STOP;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativeStop...allplayerExcuteBusiness  = [%d]", res);
//    }
//}

/**
 * 本地址播放
 */
//int native_local_play(JNIEnv *env, jobject thiz, jstring url,
//                      jlong bussiness) {
//    BizParams aStruct;
//    if (LOG_DEBUG) {
//        LOGE("XPlay", "allplayer nativePlay %lld", bussiness);
//    }
//    if (g_Listeners.find(bussiness) != g_Listeners.end()) {
//        auto container = g_Listeners[bussiness];
//        delete container;
//    }
//    JavaListenerContainer *javaListenerContainer = new JavaListenerContainer();
//    javaListenerContainer->onNativePlayerState = new OnNativePlayerState(jvm, env, thiz);
//    javaListenerContainer->onNativePlayerBackData = new OnNativePlayerBackData(jvm, env, thiz);
//
//    g_Listeners[bussiness] = javaListenerContainer;
//    if (url == NULL || env->GetStringUTFLength(url) == 0) {
//        jclass exceptionClass = env->FindClass("java/lang/Exception");
//        env->ThrowNew(exceptionClass,
//                      "Can't set a 'null' string to data source!");
//        env->DeleteLocalRef(exceptionClass);
//        return -1;
//    }
//
//    int jstrUtf16Len = env->GetStringLength(url);
//    int jstrUtf8Len = env->GetStringUTFLength(url);
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
//    }
//    char *source = new char[jstrUtf8Len + 1];// 回收放在 AllPlayer 中
//    env->GetStringUTFRegion(url, 0, jstrUtf16Len, source);
//    source[jstrUtf8Len] = '\0';
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "nativeSetDataSource: %s", source);
//    }
//    aStruct.Url = source;
//    aStruct.WindowsHandle = CAllplayerManager::GetInstance()->getNativeWindow(bussiness);
//    if (!aStruct.WindowsHandle) {
//        LOGE("XPlay", "nativePlay getNativeWindow failed = [%lld]", bussiness);
//    }
//
//    aStruct.BizType = TYPE_URL_START;
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE("XPlay", "nativePlay...allplayerExcuteBusiness  = [%d]", res);
//    }
//    if (res != 0) {
//        native_local_stop(env, thiz, bussiness);
//    }
//    return res;
//}


/**
 * 释放 end
 */
void native_release(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeRelease()
    if (LOG_DEBUG) {
        LOGE(LOG_TAG, "allplayer nativeRelease ");
    }
}

/**
 * 倍速播放
 */
//void native_vcr_control(JNIEnv *env, jobject thiz, jlong bussiness,
//                        jdouble start,
//                        jdouble scale) {
//    BizParams aStruct;
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer nativeVcrControl ");
//    }
//    aStruct.BizType = TYPE_NETRECORD_CONTROL;
//    aStruct.Scale = scale;
//    aStruct.Start = start;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativeVcrControl  aStruct.Scale= [%f]  aStruct.Start = [%f]  res = [%d]",
//             aStruct.Scale, aStruct.Start, res);
//    }
//}

/**
 * 清屏
 */
void native_clear(JNIEnv *env, jobject thiz, jlong bussiness) {
    CAllplayerManager::GetInstance()->clearSurface(bussiness);
}

/**
 * 获取窗口句柄
 */
jstring native_getBusinessHwnd(JNIEnv* env,jobject thiz, jlong bussiness) {
    void* nativeWindow = CAllplayerManager::GetInstance()->getNativeWindow(bussiness);
    jlong jp = reinterpret_cast<jlong>(nativeWindow);
    std::string str = std::to_string(jp);
    return env->NewStringUTF(str.c_str());
}

void native_callback(JNIEnv* env,jobject thiz, jlong bussiness) {
    JavaListenerContainer *javaListenerContainer = new JavaListenerContainer();
    javaListenerContainer->onNativePlayerState = new OnNativePlayerState(jvm, env, thiz);
    javaListenerContainer->onNativePlayerBackData = new OnNativePlayerBackData(jvm, env, thiz);
    g_Listeners[bussiness] = javaListenerContainer;
}

/**
 * 重新恢复播放，只有录像回放才有这个状态，bReal true的时候是录像回放的，false是本地播放的
 * @param env
 * @param thiz
 * @param bussiness
 * @param bReal
 */
//void native_resume(JNIEnv *env, jobject thiz, jlong bussiness, jboolean bReplay) {
//    BizParams aStruct;
//    // TODO: implement nativeResume()
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer nativeResume ");
//    }
//    aStruct.BizType = bReplay ? TYPE_NETRECORD_RESUME : TYPE_URL_RESUE;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativeResume  = [%d]", res);
//    }
//}

//void native_pause(JNIEnv *env, jobject thiz, jlong bussiness, jboolean bReplay) {
//    BizParams aStruct;
//    // TODO: implement nativePause()
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer nativePause ");
//    }
//    aStruct.BizType = bReplay ? TYPE_NETRECORD_PAUSE : TYPE_URL_PAUSE;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativePause  = [%d]", res);
//    }
//}

/**
 * 本地资源播放的seek跳转
 */
//void native_local_seek(JNIEnv *env, jobject thiz, jlong bussiness, jdouble percentage) {
//    BizParams aStruct;
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer nativeLocalSeek ");
//    }
//    aStruct.BizType = TYPE_URL_SEEK;
//    //todo: json
//    //aStruct.Position = percentage;
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativePause  = [%d]", res);
//    }
//}

extern "C"
JNIEXPORT void JNICALL
Java_com_allcam_allplayer_AllVideoView_InitView(JNIEnv *env, jobject thiz, jint bussiness_id,
                                                jobject surface) {
    // TODO: implement InitView()
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
    LOGE(LOG_TAG, "ativeSurfaceChange   win : %p", win);
    CAllplayerManager::GetInstance()->setNativeWindow(bussiness_id, win);
}

/**
 * 抓拍
 */
//void capture(JNIEnv *env, jobject thiz, jlong bussiness,
//             jstring path) {
//    BizParams aStruct;
//    // TODO: implement capture()
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer capture ");
//    }
//
//    if (path == NULL || env->GetStringUTFLength(path) == 0) {
//        jclass exceptionClass = env->FindClass("java/lang/Exception");
//        env->ThrowNew(exceptionClass,
//                      "Can't set a 'null' string to data source!");
//        env->DeleteLocalRef(exceptionClass);
//        return;
//    }
//
//    int jstrUtf16Len = env->GetStringLength(path);
//    int jstrUtf8Len = env->GetStringUTFLength(path);
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
//    }
//    char *source = new char[jstrUtf8Len + 1];// 回收放在 AllPlayer 中
//    env->GetStringUTFRegion(path, 0, jstrUtf16Len, source);
//    source[jstrUtf8Len] = '\0';
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "nativecapture: %s", source);
//    }
//
//    aStruct.SnapPath = source;
//    aStruct.BizType = TYPE_CAPTURE;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "nativecapture  = [%d]", res);
//    }
//}


void native_surface_change(JNIEnv *env, jobject thiz, jlong biz_id,
                           jobject surface, jint width, jint height,
                           jboolean surface_change) {
    if (surface_change) {
        ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
        if (win) {
            LOGE(LOG_TAG, "ativeSurfaceChange   win : %p      111", win);
            CAllplayerManager::GetInstance()->changeSurface(biz_id, win, width, height);
        } else {
            LOGE(LOG_TAG, "ativeSurfaceChange   win : nullptr   1111");
        }
    } else {
        auto win = CAllplayerManager::GetInstance()->getNativeWindow(biz_id);
        if (win) {
            LOGE(LOG_TAG, "ativeSurfaceChange   win : %p     222", win);
            CAllplayerManager::GetInstance()->changeSurface(biz_id, win, width, height);
        } else {
            LOGE(LOG_TAG, "ativeSurfaceChange   win : nullptr   222");
        }
    }
}
//
//void start_record(JNIEnv *env, jobject thiz, jlong bussiness,
//                  jstring path) {
//    BizParams aStruct;
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "allplayer startRecord ");
//    }
//
//    if (path == NULL || env->GetStringUTFLength(path) == 0) {
//        jclass exceptionClass = env->FindClass("java/lang/Exception");
//        env->ThrowNew(exceptionClass,
//                      "Can't set a 'null' string to data source!");
//        env->DeleteLocalRef(exceptionClass);
//        return;
//    }
//
//    int jstrUtf16Len = env->GetStringLength(path);
//    int jstrUtf8Len = env->GetStringUTFLength(path);
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
//    }
//    char *source = new char[jstrUtf8Len + 1];// 回收放在 AllPlayer 中
//    env->GetStringUTFRegion(path, 0, jstrUtf16Len, source);
//    source[jstrUtf8Len] = '\0';
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "startRecord: %s", source);
//    }
//
//    aStruct.BizType = TYPE_LOCAL_RECORD_START;
//    aStruct.RecordPath = source;
//    aStruct.MP4CutFormat = 1;
//    aStruct.MP4CutSize = 1024;
//
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "startRecord  = [%d]", res);
//    }
//
//    // TODO: implement startRecord()
//}

//void end_record(JNIEnv *env, jobject thiz, jlong bussiness) {
//    BizParams aStruct;
//    // TODO: implement endRecord()
//    aStruct.BizType = TYPE_LOCAL_RECORD_STOP;
//    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    if (LOG_DEBUG) {
//        LOGE(LOG_TAG, "startRecord  = [%d]", res);
//    }
//}
/**
 * 老的开启声音
 * @param env
 * @param clazz
 * @param data
 * @param data_size
 */
/*void open_voice(JNIEnv *env, jobject thiz, jlong bussiness,
                jboolean open) {
    BizParams aStruct;
    // TODO: implement openVoice()
    aStruct.BizType = TYPE_VOLUME_CONTROL;
    aStruct.VolumeControl = open ? 50 : 0;

    int res = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
    if (LOG_DEBUG) {
        LOGE(LOG_TAG, "openVoice  = [%d]", res);
    }
}*/

void send_audio_frame(JNIEnv *env, jclass clazz, jbyteArray data,
                      jint data_size) {
    // TODO: implement sendAudioFrame()
    int len = env->GetArrayLength(data);
    unsigned char *buf = new unsigned char[len];
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte *>(buf));
    CAllplayerManager::GetInstance()->collectAudioFrame(buf, len);
    //LOGE(LOG_TAG, "sendAudioFrame len = [%d], data_size [%d]", len, data_size);
    delete[] buf;
}

void stop_audio_record(JNIEnv *env, jclass clazz) {
    // TODO: implement stopAudioRecord()
    CAllplayerManager::GetInstance()->stopAudioRecord();
}


void start_talk(JNIEnv *env, jclass thiz, jstring url,
                jlong bussiness) {
//    BizParams aStruct;
//    // TODO: implement startTalk()
//
//    if (url == NULL || env->GetStringUTFLength(url) == 0) {
//        jclass exceptionClass = env->FindClass("java/lang/Exception");
//        env->ThrowNew(exceptionClass,
//                      "Can't set a 'null' string to data source!");
//        env->DeleteLocalRef(exceptionClass);
//        return;
//    }
//
//    int jstrUtf16Len = env->GetStringLength(url);
//    int jstrUtf8Len = env->GetStringUTFLength(url);
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
//    }
//    char *source = new char[jstrUtf8Len + 1];// 回收放在 AllPlayer 中
//    env->GetStringUTFRegion(url, 0, jstrUtf16Len, source);
//    source[jstrUtf8Len] = '\0';
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "nativeSetDataSource: %s", source);
//    }
//    //todo: json
//    aStruct.Url = source;
//    aStruct.BizType = TYPE_AUDIO_TALK_START;
//    CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//    delete source;
}

//void stop_talk(JNIEnv *env, jclass clazz, jlong bussiness) {
//    BizParams aStruct;
//    // TODO: implement stopTalk()
//    // TODO: implement endTalk()
//    aStruct.BizType = TYPE_AUDIO_TALK_STOP;
//    CAllplayerManager::GetInstance()->excuteBussiness(bussiness, aStruct);
//}

jstring stoJstring(JNIEnv *env, const char *pat) {
    jclass strClass = env->FindClass("java/lang/String");
    jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    jbyteArray bytes = env->NewByteArray(strlen(pat));
    env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte *) pat);
    jstring encoding = env->NewStringUTF("gb2312");
    return (jstring) env->NewObject(strClass, ctorID, bytes, encoding);
}


/**
 * 后期统一更改成这个通道，json的方式去传递参数给C++层
 * @param env
 * @param clazz
 * @param bussiness 操作的类型type
 * @param parameters 包含的Json内容
 * @param needCallback 是否需要返回值
 * @param resultCallBack 传递字符串接收返回值。
 * @return
 */
int commonSendParams(JNIEnv *env, jclass clazz, jlong bussiness, jstring parameters,
                     jboolean needCallback,
                     jobject resultCallBack) {
    if (parameters == NULL || env->GetStringUTFLength(parameters) == 0) {
        jclass exceptionClass = env->FindClass("java/lang/Exception");
        env->ThrowNew(exceptionClass,
                      "Can't set a 'null' string to data source!");
        env->DeleteLocalRef(exceptionClass);
        return -1;
    }
    int jstrUtf16Len = env->GetStringLength(parameters);
    int jstrUtf8Len = env->GetStringUTFLength(parameters);
    if (LOG_DEBUG) {
        LOGD(LOG_TAG, "GetString UTF-16 Length: %d; UTF-8 Length: %d", jstrUtf16Len, jstrUtf8Len);
    }
    char *source = new char[jstrUtf8Len + 1];
    env->GetStringUTFRegion(parameters, 0, jstrUtf16Len, source);
    source[jstrUtf8Len] = '\0';
    if (LOG_DEBUG) {
        LOGD(LOG_TAG, "parameters: %s", source);
    }

    jclass stringBufferClass = nullptr;
    jfieldID valueCallBack = nullptr;
    jmethodID appendJmId = nullptr;
    char *callBack = nullptr;
    if (needCallback) {
        stringBufferClass = env->GetObjectClass(resultCallBack);
        if (stringBufferClass == NULL) {
            if (LOG_DEBUG) {
                LOGD(LOG_TAG, "resultCallBack is Null ");
            }
            return -1;
        }

        valueCallBack = env->GetFieldID(stringBufferClass, "value", "[C");
        if (valueCallBack == NULL) {
            if (LOG_DEBUG) {
                LOGD(LOG_TAG, "valueCallBack is Null ");
            }
            return -1;
        }
        appendJmId = env->GetMethodID(stringBufferClass, "append",
                                      "(Ljava/lang/String;)Ljava/lang/StringBuffer;");
        if (appendJmId == NULL) {
            if (LOG_DEBUG) {
                LOGD(LOG_TAG, "appendJmId is Null ");
            }
            return -1;
        }

    }

    if (needCallback && NULL != stringBufferClass && NULL != valueCallBack &&
        NULL != resultCallBack) {
        callBack = new char[512];
    }

    int resultCode = CAllplayerManager::GetInstance()->excuteBussiness(bussiness, source, callBack);
    if (LOG_DEBUG) {
        LOGD(LOG_TAG, "resultCode: %d", resultCode);
    }

    if (needCallback && callBack && strlen(callBack) > 0 && NULL != resultCallBack &&
        NULL != appendJmId) {
        jstring jstring1 = stoJstring(env, callBack);
        env->CallObjectMethod(resultCallBack, appendJmId, jstring1);
//        env->SetCharArrayRegion(resultCallBack, 0, strlen(callBack), (jchar *) callBack);
    }

    delete source;
    delete callBack;
    return resultCode;
}


/**
* Table of methods associated with a single class.
*/
static JNINativeMethod gMethods[] = {
        {"nativieInit",         "(Ljava/lang/String;I)V",                          (void *) nativie_init},// 初始化
//        {"nativePlay",          "(Lcom/allcam/allplayer/playConfig/PlayConfig;)I", (jint *) native_play},// 播放
//        {"nativeStop",          "(JZ)V",                                           (void *) native_stop},// 停止
        {"nativeClear",         "(J)V",                                            (void *) native_clear},// 清屏
        {"nativeBusinessHwnd",  "(J)Ljava/lang/String;",                           (jstring *) native_getBusinessHwnd},// 获取句柄
        {"nativeCallback",      "(J)V",                                               (void *) native_callback},// 获取句柄
        {"nativeSurfaceChange", "(JLjava/lang/Object;IIZ)V",                       (void *) native_surface_change},// surfaceViewChange
        {"nativeRelease",       "()V",                                             (void *) native_release},// 释放
//        {"nativeVcrControl",    "(JDD)V",                                          (void *) native_vcr_control},// 倍速播放
//        {"nativePause",         "(JZ)V",                                           (void *) native_pause},//暂停
//        {"nativeResume",        "(JZ)V",                                           (void *) native_resume},// 恢复播放
//        {"capture",             "(JLjava/lang/String;)V",                          (void *) capture},// 抓拍
//        {"startRecord",         "(JLjava/lang/String;)V",                          (void *) start_record},// 开始录像
//        {"endRecord",           "(J)V",                                            (void *) end_record},// 结束录像
//        {"openVoice",           "(JZ)V",                                           (void *) open_voice},// 开启声音
//        {"stopTalk",            "(J)V",                                            (void *) stop_talk},//结束语音
        {"sendAudioFrame",      "([BI)V",                                          (void *) send_audio_frame},// 一帧帧语音数据传递
        {"stopAudioRecord",     "()V",                                             (void *) stop_audio_record},
//        {"nativeLocalPlay",     "(Ljava/lang/String;J)I",                          (jint *) native_local_play},// 本地播放
//        {"nativeLocalStop",     "(J)V",                                            (void *) native_local_stop},// 本地播放停止
        {"commonSendParams",    "(JLjava/lang/String;ZLjava/lang/Object;)I",       (jint *) commonSendParams},// 公共使用JSON的方式去传参数，后期的话，都是以这个方式去传
};

/*
* Register several native methods for one class.
*/
static int registerNativeMethods(JNIEnv *env, const char *className,
                                 JNINativeMethod *gMethods, int numMethods) {
    jclass clazz;
    clazz = (*env).FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if ((*env).RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*
* Register native methods for all classes we know about.
*/
static int registerNatives(JNIEnv *env) {
    if (!registerNativeMethods(env, JNIREG_CLASS, gMethods,
                               sizeof(gMethods) / sizeof(gMethods[0])))
        return JNI_FALSE;

    return JNI_TRUE;
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
//    ptrace(PTRACE_TRACEME, getpid(), 0, 0);
    LOGD(LOG_TAG, "JNI_OnLoad vm->pid = %d", getpid());
    int ret = av_jni_set_java_vm(vm, 0);
    if (ret < 0) {
        int n = 2;
    }

    jvm = vm;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGE(LOG_TAG, "JNI_OnLoad vm->GetEnv exception！!");
        return -1;
    }

    if (!registerNatives(env)) {//注册
        return -1;
    }
    return JNI_VERSION_1_6;
}