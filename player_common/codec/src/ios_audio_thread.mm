//
//  ios_audio_thread.cpp
//  playerSDK
//
//  Created by 秦骏 on 2021/12/30.
//

#include "ios_audio_thread.h"
#include "openAL_player.h"

IOSAudioThread::IOSAudioThread(VideoState *ic):
AvsAudioThread(ic)
{
    [[OpenalPlayer sharedInstance]  initOpenAL];
}

int IOSAudioThread::processFrame(AVFrame* frame)
{
    if (!frame) {
        return AVERROR(EINVAL);
    }
    if (m_needDisplay){
        int resampled_data_size = 0, ret = 0;
        if (frame->format == (AVSampleFormat)AV_SAMPLE_FMT_FLTP) {
            if ((ret = initResample(frame->channel_layout, AV_SAMPLE_FMT_S16, frame->sample_rate, frame->nb_samples) < 0)) {
                return ret;
            }
            uint8_t ** converted_input_samples = nullptr;
    	    if ((ret = resample(frame, resampled_data_size, &converted_input_samples) < 0)) {
		        return ret;
	        }

            [[OpenalPlayer sharedInstance]openAudioFromQueue:(char*)converted_input_samples[0] andWithDataSize:resampled_data_size andWithSampleRate:frame->sample_rate andWithAbit:16 andWithAchannel:frame->channels];
            if (converted_input_samples) {
		        av_freep(&converted_input_samples[0]);
		        free(converted_input_samples);
             }
        } else {
            uint8_t* tempData;
            tempData = frame->data[0];
            int linesize = frame->linesize[0];
            [[OpenalPlayer sharedInstance]openAudioFromQueue:(char*)tempData andWithDataSize:linesize andWithSampleRate:8000 andWithAbit:16 andWithAchannel:1];
        }
    }
    return 0;
}

