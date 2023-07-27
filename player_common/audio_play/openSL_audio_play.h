////
//// Created by HP1 on 2021/5/25.
////
//
//#ifndef PRJ_ANDROID_OPENSLAUDIOPLAY_H
//#define PRJ_ANDROID_OPENSLAUDIOPLAY_H
//
//#include "as_config.h"
//#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
//
//#include "ac_audio_play.h"
//extern "C"
//{
//#include <SLES/OpenSLES.h>
//#include <SLES/OpenSLES_Android.h>
//#include <libavutil/channel_layout.h>
//}
//
//class OpenSLAudioPlay : public ACAudioPlay {
//
//public:
//    OpenSLAudioPlay();
//
//    ~OpenSLAudioPlay();
//
//    //暂停
//    void pause(bool isPause) override;
//
//    //打开音频 开始播放 调用回调函数
//    virtual bool open(ACAudioSpec& spec) override;
//
//    void startPlay();
//
//    virtual void close() override;
//
//    void setVolumePercent(float percent);
//
//public:
//    // 播放数据入队 buffer
//    void *enqueueBuffer = NULL;
//    bool enqueueFailed = false;
//    static const int MAX_AUTO_ENQUE_COUNT = 5;
//    int autoEnqueCount = 0;// 入队失败后自动尝试入队次数
//    bool enqueueFinished = true;
//
//    // 播放缓冲队列
//    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;
//
//private:
//    int init();
//
//    int initEngine();
//
//    int initOutputMix();
//
//    int setEnvironmentalReverb();
//
//    int createBufferQueueAudioPlayer(ACAudioSpec& spec);
//
//    int setBufferQueueCallback(slAndroidSimpleBufferQueueCallback callback, void *pContext);
//
//    int setPlayState(SLuint32 state);
//
//    /**
//    * 确保在退出应用时销毁所有对象。
//    * 对象应按照与创建时相反的顺序销毁，因为销毁具有依赖对象的对象并不安全。
//    * 例如，请按照以下顺序销毁：音频播放器和录制器、输出混合，最后是引擎。
//    */
//    void destroy();
//
//    /**
//    * destroy buffer queue audio player object, and invalidate all associated interfaces
//    */
//    void destroyBufferQueueAudioPlayer();
//
//    /**
//     * destroy output mix object, and invalidate all associated interfaces
//     */
//    void destroyOutputMixer();
//
//    /**
//     * destroy engine object, and invalidate all associated interfaces
//     */
//    void destroyEngine();
//
//    /**
//    * 转换采样率（Hz）为 OpenSL ES 定义的采样率
//    * @param sampleRate 待转换的样率（Hz）
//    * @return OpenSL ES 定义的采样率
//    */
//    static SLuint32 convertToOpenSLSampleRate(int sampleRate);
//
//private:
//    SLuint32 channelMask;
//
//    const char *LOG_TAG = "AllCamAudioPlay";
//
//    // 引擎
//    SLObjectItf engineObject = NULL;
//    SLEngineItf engine = NULL;
//
//    // 混音器
//    SLObjectItf outputMixObject = NULL;
//    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
//
//    // 播放器
//    SLObjectItf playerObject = NULL;
//    SLPlayItf playController = NULL;
//    SLVolumeItf volumeController = NULL;
//    SLMuteSoloItf muteSoloController = NULL;
//
//    static const int CHANNEL_RIGHT = 0;
//    static const int CHANNEL_LEFT = 1;
//    static const int CHANNEL_STEREO = 2;
//
//    float volumePercent = 1.0;
//
//    // PCM 数据参数
//    int channelNums;// 声道数量
//    SLuint32 openSLSampleRate;// OpenSL ES 定义的采样率
//    int bitsPerSample;// 每个声道每次采样比特位数
//    SLuint32 openSLChannelLayout;// OpenSL ES 定义的声道布局
//
//    bool m_bPlay = false;
//
//};
//
//#endif
//
//#endif //PRJ_ANDROID_OPENSLAUDIOPLAY_H
