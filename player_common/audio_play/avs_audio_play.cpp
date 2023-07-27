
#include "sdl_audio_play.h"

extern "C"
{
#include <libavformat/avformat.h>
}
#include "as.h"

uint8_t AvsAudioPlay::audioBuffer[AUIDO_MAX_SIZE] = { 0 };

AvsAudioPlay* AvsAudioPlay::GetInstance()
{
#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
    static SDLAudioPlay sdlPlayer;
    return &sdlPlayer;
#endif
    return nullptr;
}

AvsAudioPlay* AvsAudioPlay::CreateInstance()
{
#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
    SDLAudioPlay* sdlPlayer = AS_NEW<SDLAudioPlay>(sdlPlayer);
    return sdlPlayer;
#endif
    return nullptr;
}

void AvsAudioPlay::DestroyInstance(AvsAudioPlay* play)
{
    if (play) {
        play->close();
        AS_DELETE(play);
    }
}

void AvsAudioPlay::push(AVFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return;
    }

    int data_size, resampled_data_size;
    data_size = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);

    int64_t dec_channel_layout = (frame->channel_layout && frame->channels == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
        frame->channel_layout : av_get_default_channel_layout(frame->channels);

    if (frame->format != m_videoState->audio_src.fmt ||
        dec_channel_layout != m_videoState->audio_src.channel_layout ||
		frame->sample_rate != m_videoState->audio_src.freq ||
        !m_videoState->swr_ctx) {
        swr_free(&m_videoState->swr_ctx);
      
        m_videoState->swr_ctx = swr_alloc_set_opts(NULL,
            m_videoState->audio_tgt.channel_layout, m_videoState->audio_tgt.fmt, m_videoState->audio_tgt.freq,
            dec_channel_layout, (AVSampleFormat)frame->format, frame->sample_rate,
            0, NULL);

        if (!m_videoState->swr_ctx || swr_init(m_videoState->swr_ctx) < 0) {
            swr_free(&m_videoState->swr_ctx);
        }

        m_videoState->audio_src.channel_layout = dec_channel_layout;
        m_videoState->audio_src.channels = frame->channels;
        m_videoState->audio_src.freq = frame->sample_rate;
        m_videoState->audio_src.fmt = (AVSampleFormat)frame->format;
    }

    if (m_videoState->swr_ctx) {
        const uint8_t** in = (const uint8_t**)frame->extended_data;
        uint8_t** out = &m_videoState->audio_buf1;
        int out_count = (int64_t)frame->nb_samples * m_videoState->audio_tgt.freq / frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, m_videoState->audio_tgt.channels, out_count, m_videoState->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            return;
        }
        av_fast_malloc(&m_videoState->audio_buf1, &m_videoState->audio_buf1_size, out_size);
        if (!m_videoState->audio_buf1) {
            return;
        }
        len2 = swr_convert(m_videoState->swr_ctx, out, out_count, in, frame->nb_samples);
        if (len2 < 0) {
            return;
        }
        if (len2 == out_count) {
            if (swr_init(m_videoState->swr_ctx) < 0)
                swr_free(&m_videoState->swr_ctx);
        }
        resampled_data_size = len2 * m_videoState->audio_tgt.channels * av_get_bytes_per_sample(m_videoState->audio_tgt.fmt);
        push(m_videoState->audio_buf1, resampled_data_size, frame->pts);
    }
    else {
        push(frame->data[0], data_size, frame->pts);
    }
}

void AvsAudioPlay::push(const unsigned char* data, int size, int64_t pts)
{
    m_audioCache.Write((const char*)data, size);
    //AS_LOG(AS_LOG_INFO, "push %d size to cache, total %d.", size, m_audioCache.GetDataSize());
}

void AvsAudioPlay::setSpeed(float speed)
{
    m_speed = speed;
    auto spec = m_audioParams;
    auto old_freq = spec.freq;
    spec.freq *= speed;
	open(spec.channel_layout, spec.channels, spec.freq);
    m_audioParams.freq = old_freq;
}

as_ring_cache* AvsAudioPlay::getPCMCache()
{
    return &m_audioCache;
}

void AvsAudioPlay::setVolume(int16_t volum)
{
    m_volume = volum;
}

AvsAudioPlay::AvsAudioPlay():
    m_speed(1.0), m_volume(100),
    m_inited(false), m_videoState(nullptr)
{
	memset(&m_audioParams, 0, sizeof(m_audioParams));
}

void AvsAudioPlay::clear() {
    m_audioCache.Clear();
    //setSpeed(m_speed);
}
