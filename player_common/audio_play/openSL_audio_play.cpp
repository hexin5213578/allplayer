////
//// Created by HP1 on 2021/5/25.
////
//
//#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
//
//#include "ac_audio_play.h"
//#include "openSL_audio_play.h"
//#include "audio_error.h"
//#include "../../Prj-Android/allplayer/src/main/cpp/AndroidLog.h"
//
//void pcmBufferCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
//
//    OpenSLAudioPlay *player = (OpenSLAudioPlay *) context;
//    if (player == NULL) {
//        LOGE("OpenSLPlayer", "pcmBufferCallback cast context to OpenSLPlayer result is NULL")
//        return;
//    }
//
//    player->enqueueFinished = false;
//    int size = 0;
//
//    while(size <=0) {
//        auto cache = player->getPCMCache();
//        int dataSize = cache->GetDataSize();
//        if(dataSize <=0) {
//            MSleep(10);
//            continue;
//        }
//        size = cache->Read((char*)player->audioBuffer,dataSize);
//    }
//
//    if (size <= 0) {// 已经播放完成或不在播放状态了
//        LOGW("AllCamAudioPlay", "pcmBufferCallback111 getPcmData size=%d", size);
//        player->enqueueFailed = true;// 用于暂停后可能异步获取失败恢复播放时主动喂一次数据
//        player->enqueueFinished = true;
//        return;
//    }
//
//    SLresult result;
//    result = (*player->pcmBufferQueue)->Enqueue(
//            player->pcmBufferQueue, player->audioBuffer, size);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE("OpenSLPlayer", "pcmBufferCallback222 BufferQueue Enqueue exception!");
//        player->enqueueFailed = true;
//        if (player->autoEnqueCount < OpenSLAudioPlay::MAX_AUTO_ENQUE_COUNT) {
//            player->autoEnqueCount++;
//            pcmBufferCallback(bq, context);
//        }
//    } else {
//        player->enqueueFailed = false;
//        player->autoEnqueCount = 0;
//    }
//    //LOGW("AllCamAudioPlay", "pcmBufferCallback333 getPcmData size=%d", size);
//    player->enqueueFinished = true;
//}
//
//OpenSLAudioPlay::OpenSLAudioPlay() {
//    init();
//}
//
//bool OpenSLAudioPlay::open(ACAudioSpec &spec) {
//
//    if (!m_inited) {
//        LOGE(LOG_TAG, "Can't create player because init not success!");
//        return E_CODE_AUD_ILLEGAL_CALL;
//    }
//
//    int ret;
//    // 使用引擎创建数据源为缓冲队列的播放器
//    if ((ret = createBufferQueueAudioPlayer(spec)) != NO_ERROR) {
//        destroy();
//        return ret;
//    }
//    // 设置播放器缓冲队列回调函数
//    if ((ret = setBufferQueueCallback(pcmBufferCallback, this)) != NO_ERROR) {
//        destroy();
//        return ret;
//    }
//    startPlay();
//    return NO_ERROR;
//}
//
//void OpenSLAudioPlay::startPlay() {
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "startPlay");
//    }
//
//    if(!m_bPlay) {
//        m_bPlay = true;
//        // 设置播放状态为正在播放
//        setPlayState(SL_PLAYSTATE_PLAYING);
//        (*pcmBufferQueue)->Enqueue(pcmBufferQueue,"",1);
//        LOGD(LOG_TAG, "startPlay success!");
//        // 首次启动时需要主动调用缓冲队列回调函数开始播放
//        //pcmBufferCallback(pcmBufferQueue, this);
//    }
//}
//
//void OpenSLAudioPlay::pause(bool isPause) {
//
//}
//
//void OpenSLAudioPlay::close() {
//    ACAudioPlay::clear();
//
//    setPlayState(SL_PLAYSTATE_STOPPED);
//
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "stopPlay");
//    }
//    destroy();
//}
//
//int OpenSLAudioPlay::init() {
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "init");
//    }
//    int ret;
//
//    // 初始化引擎
//    if ((ret = initEngine()) != NO_ERROR) {
//        destroy();
//        return ret;
//    }
//
//    // 使用引擎创建混音器
//    if ((ret = initOutputMix()) != NO_ERROR) {
//        destroy();
//        return ret;
//    }
//    // 使用混音器设置混音效果
//    setEnvironmentalReverb();
//    return NO_ERROR;
//}
//
//int OpenSLAudioPlay::initEngine() {
//    // create engine object
//    SLresult result;
//    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "slCreateEngine exception!");
//        return E_CODE_AUD_CREATE_ENGINE;
//    }
//
//    // realize the engine object
//    (void) result;
//    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "engineObject Realize exception!");
//        destroyEngine();
//        return E_CODE_AUD_REALIZE_ENGINE;
//    }
//
//    // get the engine interface, which is needed in order to create other objects
//    (void) result;
//    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "GetInterface SLEngineItf exception!");
//        destroyEngine();
//        return E_CODE_AUD_GETITF_ENGINE;
//    }
//
//    return NO_ERROR;
//}
//
//int OpenSLAudioPlay::initOutputMix() {
//    // create output mix, with environmental reverb specified as a non-required interface
//    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
//    const SLboolean reqs[1] = {SL_BOOLEAN_FALSE};
//    SLresult result;
//    result = (*engine)->CreateOutputMix(engine, &outputMixObject, 1, ids, reqs);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "CreateOutputMix exception!");
//        return E_CODE_AUD_CREATE_OUTMIX;
//    }
//
//    // realize the output mix
//    (void) result;
//    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "outputMixObject Realize exception!");
//        destroyOutputMixer();
//        return E_CODE_AUD_REALIZE_OUTMIX;
//    }
//
//    return NO_ERROR;
//}
//
//int OpenSLAudioPlay::setPlayState(SLuint32 state) {
//    if (playController == NULL) {
//        LOGE(LOG_TAG, "SetPlayState %d failed because playController is NULL !", state);
//        return E_CODE_AUD_SET_PLAYSTATE;
//    }
//
//    SLresult result;
//    result = (*playController)->SetPlayState(playController, state);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "SetPlayState %d exception!", state);
//        return E_CODE_AUD_SET_PLAYSTATE;
//    }
//
//    return NO_ERROR;
//}
//
//void OpenSLAudioPlay::destroy() {
//    if (LOG_DEBUG) {
//        LOGD(LOG_TAG, "destroy");
//    }
//    m_inited = false;
//    destroyBufferQueueAudioPlayer();
//    destroyOutputMixer();
//    destroyEngine();
//}
//
//int OpenSLAudioPlay::setEnvironmentalReverb() {
//    // get the environmental reverb interface
//    // this could fail if the environmental reverb effect is not available,
//    // either because the feature is not present, excessive CPU load, or
//    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
//    SLresult result;
//    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
//                                              &outputMixEnvironmentalReverb);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "GetInterface SLEnvironmentalReverbItf exception!");
//        return E_CODE_AUD_GETITF_ENVRVB;
//    }
//
//    // aux effect on the output mix, used by the buffer queue player
//    const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
//    result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
//            outputMixEnvironmentalReverb, &reverbSettings);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "SetEnvironmentalReverbProperties exception!");
//        return E_CODE_AUD_SETPROP_ENVRVB;
//    }
//
//    return NO_ERROR;
//}
//
//int OpenSLAudioPlay::createBufferQueueAudioPlayer(ACAudioSpec& spec) {
//    // configure audio source
//    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator = {
//            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
//    };
//    SLDataFormat_PCM pcmFormat = {
//            SL_DATAFORMAT_PCM,// 数据格式：pcm
//            spec.channels,// 声道数
//            convertToOpenSLSampleRate(spec.freq),// 采样率
//            SL_PCMSAMPLEFORMAT_FIXED_16,// bitsPerSample
//            SL_PCMSAMPLEFORMAT_FIXED_16,// containerSize：和采样位数一致就行
//            SL_SPEAKER_FRONT_CENTER,// 声道布局
//            SL_BYTEORDER_LITTLEENDIAN// 字节排列顺序：小端 little-endian，将低序字节存储在起始地址
//    };
//    SLDataSource audioSrc = {&bufferQueueLocator, &pcmFormat};
//
//    // configure audio sink
//    SLDataLocator_OutputMix outputMixLocator = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
//    SLDataSink audioSnk = {&outputMixLocator, NULL};
//
//    /*
//     * create audio player:
//     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
//     *     for fast audio case
//     */
//    const int ID_COUNT = 2;
//    // 如果某个功能接口没注册 id 和写为 SL_BOOLEAN_TRUE，后边通过 GetInterface 就获取不到这个接口
//    // 打开 SL_IID_PLAYBACKRATE 功能是为了解决变速变调卡顿问题
//    const SLInterfaceID ids[ID_COUNT] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
//    const SLboolean reqs[ID_COUNT] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
//    SLresult result;
//    result = (*engine)->CreateAudioPlayer(
//            engine, &playerObject, &audioSrc, &audioSnk, ID_COUNT, ids, reqs);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "CreateAudioPlayer exception!");
//        return E_CODE_AUD_CREATE_AUDIOPL;
//    }
//
//    // realize the player
//    (void) result;
//    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "playerObject Realize exception!");
//        destroyBufferQueueAudioPlayer();
//        return E_CODE_AUD_REALIZ_AUDIOPL;
//    }
//
//    // get play controller
//    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playController);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "GetInterface SLPlayItf exception!");
//        destroyBufferQueueAudioPlayer();
//        return E_CODE_AUD_GETITF_PLAY;
//    }
//
//    // get volume controller
//    result = (*playerObject)->GetInterface(playerObject, SL_IID_VOLUME, &volumeController);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "GetInterface SLVolumeItf exception!");
//        destroyBufferQueueAudioPlayer();
//        return E_CODE_AUD_GETITF_VOLUME;
//    }
//    setVolumePercent(volumePercent);
//    return NO_ERROR;
//}
//
//int OpenSLAudioPlay::setBufferQueueCallback(slAndroidSimpleBufferQueueCallback callback,
//                                            void *pContext) {
//    // get the buffer queue interface
//    SLresult result;
//    result = (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "GetInterface SLAndroidSimpleBufferQueueItf exception!");
//        destroyBufferQueueAudioPlayer();
//        return E_CODE_AUD_GETITF_BUFQUE;
//    }
//
//    // register callback on the buffer queue
//    (void) result;
//    result = (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, callback, pContext);
//    if (SL_RESULT_SUCCESS != result) {
//        LOGE(LOG_TAG, "pcmBufferQueue RegisterCallback exception!");
//        destroyBufferQueueAudioPlayer();
//        return E_CODE_AUD_REGCALL_BUFQUE;
//    }
//
//    return NO_ERROR;
//}
//
//void OpenSLAudioPlay::destroyBufferQueueAudioPlayer() {
//    if (playerObject != NULL) {
//
//        playController = NULL;
//        volumeController = NULL;
//        muteSoloController = NULL;
//
//        if(pcmBufferQueue && (*pcmBufferQueue)) {
//            (*pcmBufferQueue)->Clear(pcmBufferQueue);
//            pcmBufferQueue = NULL;
//        }
//
//        (*playerObject)->Destroy(playerObject);
//        playerObject = NULL;
//
//        enqueueBuffer = NULL;
//    }
//}
//
//void OpenSLAudioPlay::destroyOutputMixer() {
//    if (outputMixObject != NULL) {
//        (*outputMixObject)->Destroy(outputMixObject);
//        outputMixObject = NULL;
//        outputMixEnvironmentalReverb = NULL;
//    }
//}
//
//void OpenSLAudioPlay::destroyEngine() {
//    if (engineObject != NULL) {
//        (*engineObject)->Destroy(engineObject);
//        engineObject = NULL;
//        engine = NULL;
//    }
//}
//
//SLuint32 OpenSLAudioPlay::convertToOpenSLSampleRate(int sampleRate) {
//    SLuint32 rate = 0;
//    switch (sampleRate) {
//        case 8000:
//            rate = SL_SAMPLINGRATE_8;
//            break;
//        case 11025:
//            rate = SL_SAMPLINGRATE_11_025;
//            break;
//        case 12000:
//            rate = SL_SAMPLINGRATE_12;
//            break;
//        case 16000:
//            rate = SL_SAMPLINGRATE_16;
//            break;
//        case 22050:
//            rate = SL_SAMPLINGRATE_22_05;
//            break;
//        case 24000:
//            rate = SL_SAMPLINGRATE_24;
//            break;
//        case 32000:
//            rate = SL_SAMPLINGRATE_32;
//            break;
//        case 44100:
//            rate = SL_SAMPLINGRATE_44_1;
//            break;
//        case 48000:
//            rate = SL_SAMPLINGRATE_48;
//            break;
//        case 64000:
//            rate = SL_SAMPLINGRATE_64;
//            break;
//        case 88200:
//            rate = SL_SAMPLINGRATE_88_2;
//            break;
//        case 96000:
//            rate = SL_SAMPLINGRATE_96;
//            break;
//        case 192000:
//            rate = SL_SAMPLINGRATE_192;
//            break;
//        default:
//            rate = SL_SAMPLINGRATE_44_1;
//    }
//    return rate;
//}
//
//OpenSLAudioPlay::~OpenSLAudioPlay() {
//    destroy();
//}
//
//void OpenSLAudioPlay::setVolumePercent(float percent) {
//    if (percent < 0) {
//        percent = 0;
//    } else if (percent > 1.0) {
//        percent = 1.0;
//    }
//    volumePercent = percent;
//
//    if (volumeController == NULL || !m_inited) {
//        LOGW(LOG_TAG, "setVolume %f but volumeController is %d and init %d", volumePercent,
//             volumeController, m_inited);
//        return;
//    }
//    // 第 2 个参数有效范围是：0 ~ -5000，其中 0 表示最大音量，-5000 表示静音
//    if (volumePercent > 0.30) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -20);
//    } else if (volumePercent > 0.25) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -22);
//    } else if (volumePercent > 0.20) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -25);
//    } else if (volumePercent > 0.15) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -28);
//    } else if (volumePercent > 0.10) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -30);
//    } else if (volumePercent > 0.05) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -34);
//    } else if (volumePercent > 0.03) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -37);
//    } else if (volumePercent > 0.01) {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -40);
//    } else {
//        (*volumeController)->SetVolumeLevel(volumeController, 100 * (1 - volumePercent) * -100);
//    }
//}
//
//#endif