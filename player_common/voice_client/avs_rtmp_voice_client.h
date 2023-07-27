#pragma once

#include "avs_voice_client.h"
#include "avs_thread_base.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include "libavutil/audio_fifo.h"
	#include "libswresample/swresample.h"
}

enum rtmpErrorCode {
	kConnectError = 301,            //rtmp连接失败
	kColleterError = 302,           //麦克风采集失败
	kPushFaild = 303,				//rtmp打包发送失败(编码,网络等原因)
	kErrorMax = 9999,
};

class rtmp_voice_client : public avs_voice_client, public AvsThreadBase
{
public:
	static rtmp_voice_client* get_audio_talk_instance();

	static rtmp_voice_client* get_voice_broadcast_instance();

	int init(VoiceSt& params) override;

	int start_1() override { startRun();  return 0; }

	void stop_1() override;

protected:
	virtual void mainProcess() override { start_2(); }
	int start_2();
	void cleanup();

	int initConvertSamples(uint8_t*** converted_input_samples, int frame_size);
	int readConvertAndStore(uint8_t **data, int frame_size);

	int loadEncodeAndWrite();
	int encodeAudioFrame(AVFrame* frame, int * data_present);

	rtmp_voice_client(): avs_voice_client() {}
	~rtmp_voice_client() = default;

private:
	AVFormatContext*	m_ctx = nullptr;
	AVCodecContext*		m_encodeCtx = nullptr;
	SwrContext*			m_resampler = nullptr;
	AVStream*			m_audioStream = nullptr;
	AVAudioFifo*		m_fifo = nullptr;
	int64_t				m_pts = 0;
	std::atomic<bool>	m_stopFlag{false};
};