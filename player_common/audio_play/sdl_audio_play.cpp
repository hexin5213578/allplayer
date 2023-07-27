//#ifdef _WIN32
#include "sdl_audio_play.h"
#include "as_log.h"
#include <SDL2/SDL.h>

const int tKb = 2048;

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

SDLAudioPlay::SDLAudioPlay()
    :m_curPts(0),
    m_deviceId(-1)
{
    //SDL_Init(SDL_INIT_AUDIO);
}

void SDLAudioPlay::pause(bool isPause)
{
    if (isPause) {
        SDL_PauseAudioDevice(m_deviceId, 1);
    }
    else {
        SDL_PauseAudioDevice(m_deviceId, 0);
    }
}

int SDLAudioPlay::open(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate)
{
    SDL_AudioSpec wanted_spec, sdl_spec;
    static const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
    static const int next_sample_rates[] = { 0, 44100, 48000, 96000, 192000 };
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

    const char* env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) {
        wanted_nb_channels = atoi(env);
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    }
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;
	wanted_spec.freq = wanted_sample_rate;
	if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
		AS_LOG(AS_LOG_WARNING,"SDLAudioPlay::open failed, wanted_spec.freq=%d, wanted_spec.channels=%d", 
            wanted_spec.freq, wanted_spec.channels);
		return -1;
	}

    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    
	wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.userdata = this;
    wanted_spec.callback = audioCallback;

    if (m_deviceId > 0) {
        SDL_CloseAudioDevice(m_deviceId);
        m_deviceId = -1;
    }

    while (!(m_deviceId = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &sdl_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
        const char* err = SDL_GetError();
        AS_LOG(AS_LOG_ERROR, "audio output device open failed : %s", SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                AS_LOG(AS_LOG_ERROR, "No more combinations to try, audio open failed");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }

    AS_LOG(AS_LOG_INFO, "m_deviceId:%d", m_deviceId);
    if (m_deviceId <= 0) {
        return -1;
    }

    if (sdl_spec.format != AUDIO_S16SYS) {
		AS_LOG(AS_LOG_ERROR, "SDL advised audio format %d is not supported!", sdl_spec.format);
        return -1;
    }
    
    if (sdl_spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(sdl_spec.channels);
        if (!wanted_channel_layout) {
            AS_LOG(AS_LOG_ERROR, "SDL advised channel count %d is not supported!\n", sdl_spec.channels);
            return -1;
        }
    }

	m_audioParams.fmt = AV_SAMPLE_FMT_S16;
	m_audioParams.freq = sdl_spec.freq;
	m_audioParams.channel_layout = wanted_channel_layout;
	m_audioParams.channels = sdl_spec.channels;
	m_audioParams.frame_size = av_samples_get_buffer_size(NULL, m_audioParams.channels, 1, m_audioParams.fmt, 1);
	m_audioParams.bytes_per_sec = av_samples_get_buffer_size(NULL, m_audioParams.channels, m_audioParams.freq, m_audioParams.fmt, 1);
	if (m_audioParams.bytes_per_sec <= 0 || m_audioParams.frame_size <= 0) {
		AS_LOG(AS_LOG_ERROR, "av_samples_get_buffer_size failed\n");
		return -1;
	}
    
    if (m_videoState) {
        m_videoState->audio_src = m_videoState->audio_tgt = m_audioParams;
    }
    m_audioFormat = sdl_spec.format;
    SDL_PauseAudioDevice(m_deviceId, 0);
    return sdl_spec.size;
}

void SDLAudioPlay::close()
{
    AvsAudioPlay::clear();
    if(m_deviceId > 0) {
        SDL_CloseAudioDevice(m_deviceId);
    }
    memset(&m_audioParams, 0, sizeof(m_audioParams));
    m_curPts = 0;
}

void SDLAudioPlay::callback(unsigned char* stream, int len) {
    SDL_memset(stream, 0, len);
   
    if (0 == m_audioCache.GetDataSize()) {
        return;
    }

    int audio_size,len1;

    while (len > 0) {
        len1 = m_audioCache.Read((char*)audioBuffer, len);
        if (len1 == 0) {
            break;
        }
        
        if (len1 > len) {
            len1 = len;
        }

        if (len1 > 0 && SDL_MIX_MAXVOLUME == m_volume) {
            memcpy(stream, audioBuffer, len1);
        }
        else {
            memset(stream, 0, len1);
            SDL_MixAudioFormat(stream, audioBuffer, m_audioFormat, len1,  m_volume);
        }
        len -= len1;
        stream += len1;
    }

    //normal when occur jitter
    /* 
    int cache = m_audioCache.GetDataSize();
    if (cache >= m_audioCache.GetCacheSize() - tKb) {
        AS_LOG(AS_LOG_INFO, "remain %d size", cache);
    }
    */
}

//#endif
