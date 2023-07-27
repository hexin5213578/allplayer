#include "ac_audio_play.h"
//#include <SDL2/SDL.h>
#include "SDL.h"
extern "C"
{
#include <libavformat/avformat.h>
}
#include "as.h"
class SDLAudioPlay : public ACAudioPlay
{
public:
    void pause(bool isPause) override {
        if (isPause) {
            SDL_PauseAudio(isPause);
            m_pauseMs = NowMs();
        }
        else {
            if (m_pauseMs > 0) {
                m_lastMs += (NowMs() - m_pauseMs);
            }
            SDL_PauseAudio(isPause);
        }
    }

    bool open(ACAudioSpec& spec) override {
        m_spec = spec;
        SDL_AudioSpec sdlSpec;
        sdlSpec.freq = spec.freq;
        sdlSpec.format = spec.format;
        sdlSpec.channels = spec.channels;
        sdlSpec.samples = spec.samples;
        sdlSpec.silence = 0;
        sdlSpec.userdata = this;
        sdlSpec.callback = AudioCallback;
        if (SDL_OpenAudio(&sdlSpec, nullptr) < 0) {
            auto errBuf = SDL_GetError();
            return false;
        }
        SDL_PauseAudio(0);
        return true;
    }

    void close() override {
//        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        std::unique_lock<std::mutex> lock(m_Mtx);
        m_audioDatas.clear();
        m_curPts = 0; 
        m_lastMs = 0;  
        m_pauseMs = 0;
    }

    void Callback(unsigned char* stream, int len) {
        SDL_memset(stream, 0, len);
        std::unique_lock<std::mutex> lock(m_Mtx);
        if (m_audioDatas.empty()) {
            return;
        }

        auto buf = m_audioDatas.front();
        // 1 buf 大于stream缓冲  offset记录位置
        // 2 buf 小于stream 缓冲  拼接
        int mixedSize = 0;     //已经处理的字节数
        int needSize = len;    //需要处理的字节数

        while (mixedSize < len) {
            if (m_audioDatas.empty()) {
                break;
            }

            buf = m_audioDatas.front();
            int size = buf.data.size() - buf.offset;
            if (size > needSize) {
                size = needSize;
            }

            SDL_MixAudio(stream + mixedSize,buf.data.data() + buf.offset,size, m_volume);
            needSize -= size;
            mixedSize += size;
            buf.offset += size;
            if (buf.offset >= buf.data.size()) {
                m_audioDatas.pop_front();
            }
        }
    }

private:
    long long m_curPts = 0; //当前播放位置
    long long m_lastMs = 0;  //上次的时间戳
    long long m_pauseMs = 0;//暂停开始时间戳
};

ACAudioPlay* ACAudioPlay::GetInstance()
{
    static SDLAudioPlay sdlPlayer;
    return &sdlPlayer;
}

void ACAudioPlay::push(AVFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return;
    }

    std::vector<unsigned char> buf;
    int sampleSize = av_get_bytes_per_sample((AVSampleFormat)frame->format);
    int channels = frame->channels;
    unsigned char* L = frame->data[0];
    unsigned char* R = frame->data[1];
    unsigned char* data = nullptr;
    if (channels == 1) {
        push(frame->data[0], frame->nb_samples * sampleSize, frame->pts);
        return;
    }

    switch (frame->format) {
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_FLTP:
        buf.resize(frame->linesize[0]);
        data = buf.data();
        for (int i = 0; i < frame->nb_samples; i++) {
            memcpy(data + i * sampleSize * channels, L + i * sampleSize, sampleSize);
            memcpy(data + i * sampleSize * channels + sampleSize, R + i * sampleSize, sampleSize);
        }
        push(data, frame->linesize[0], frame->pts);
        return;
    default:
        break;
    }
    push(frame->data[0], frame->linesize[0], frame->pts);
}

bool ACAudioPlay::initG711Para() {
    if (m_inited) {
        return false;
    }
    ACAudioSpec spec;
    spec.channels = 1;
    spec.freq = 8000 + 800;
    spec.format = AUDIO_S16;
    m_inited = true;
    return open(spec);
}

bool ACAudioPlay::initAudioPara(AVFrame* frame)
{
    if (!frame || m_inited) {
        return false;
    }

    ACAudioSpec spec;
    spec.channels = frame->channels;
    spec.freq = frame->sample_rate;

    switch (frame->format)
    {
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        spec.format = AUDIO_S16;
        break;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        spec.format = AUDIO_S32;
        break;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        spec.format = AUDIO_F32;
        break;
    default:
        break;
    }
    m_inited = true;
    return open(spec);
}

void ACAudioPlay::push(const unsigned char* data, int size, long long pts)
{
    std::unique_lock<std::mutex> lock(m_Mtx);
    ACAudioData audioData;
    audioData.pts = pts;
    audioData.data.assign(data, data + size);
    m_audioDatas.push_back(audioData);
}

void ACAudioPlay::setSpeed(float speed)
{
    m_speed = speed;
    auto spec = m_spec;
    auto old_freq = spec.freq;
    spec.freq *= speed;
    open(spec);
    m_spec.freq = old_freq;
}

void ACAudioPlay::setVolume(int volum)
{
    m_volume = volum;
}

ACAudioPlay::ACAudioPlay()
{
    SDL_Init(SDL_INIT_AUDIO);
}
