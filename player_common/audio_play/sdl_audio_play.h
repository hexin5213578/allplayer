#include "avs_audio_play.h"
#include "as_config.h"

class SDLAudioPlay : public AvsAudioPlay
{
public:
    SDLAudioPlay();

    void pause(bool isPause) override;

    int open(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate) override;

    void close() override;

protected:
    void callback(unsigned char* stream, int len) override;

private:
    long long               m_curPts; //当前播放位置
    uint16_t                m_audioFormat;
    int                     m_deviceId;
};
