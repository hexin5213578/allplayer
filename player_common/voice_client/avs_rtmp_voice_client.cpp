#ifdef _WIN32
#include "avs_rtmp_voice_client.h"
#include "pusher/audio_collect.h"

static FILE* pf_pcm = nullptr;

AVSampleFormat wFormatTagToSampleFmt(WORD wFormatTag) {
	switch (wFormatTag) {
	case WAVE_FORMAT_PCM:
		return AV_SAMPLE_FMT_S16;
	default:
		return AV_SAMPLE_FMT_S16;
		//return AV_SAMPLE_FMT_NONE; // 未知格式
	}
}

rtmp_voice_client* rtmp_voice_client::get_audio_talk_instance() {
	static rtmp_voice_client client;
	return &client;
}

rtmp_voice_client* rtmp_voice_client::get_voice_broadcast_instance() {
	static rtmp_voice_client client;
	return &client;
}

int rtmp_voice_client::init(VoiceSt& params) {
	m_voiceParams = params;
	if (AudioCollect::GetInstance().GetState() > 0) {
		AS_LOG(AS_LOG_ERROR, "microphone is opening or disable.");
		sendStatus(STREAM_VOICE_START_FAIL, std::to_string(kColleterError));
		return AVERROR(EINVAL);
	}

	AVOutputFormat* outputFmt;
	AVCodec* codec;

	char buf[128] = { 0 };
	int ret = 0;

	do {
		// 分配输出媒体上下文
		ret = avformat_alloc_output_context2(&m_ctx, NULL, "flv", params.voiceUrl.data());
		if (!m_ctx) {
			av_strerror(ret, buf, sizeof(buf) - 1);
			AS_LOG(AS_LOG_ERROR, "could not create output context, %s.", buf);
			break;
		}
		outputFmt = m_ctx->oformat;
		// 查找音频编码器
		codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		if (!codec) {
			AS_LOG(AS_LOG_ERROR, "could not find audio encoder: aac");
			ret = -1;
			break;
		}

		m_audioStream = avformat_new_stream(m_ctx, codec);
		if (!m_audioStream) {
			AS_LOG(AS_LOG_ERROR, "could not allocate audio stream");
			ret = AVERROR(ENOMEM);
			break;
		}

		m_encodeCtx = avcodec_alloc_context3(codec);
		m_encodeCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
		//m_encodeCtx->bit_rate = 32000;

		// 获取音频采集类的实例
		AudioCollect& audioCollect = AudioCollect::GetInstance();
		if (audioCollect.Init()) {
			AS_LOG(AS_LOG_ERROR, "microphone init fail");
			ret = AVERROR(EINVAL);
			break;
		}

		AUDIO_FORMAT_INFO audioFormat;
		audioCollect.GetAudioFormat(audioFormat);

		m_encodeCtx->sample_rate = audioFormat.ulSampleRate;
		m_encodeCtx->channel_layout = audioFormat.ucChannel == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
		m_encodeCtx->channels = av_get_channel_layout_nb_channels(m_encodeCtx->channel_layout);

		m_audioStream->time_base.den = m_encodeCtx->sample_rate;
		m_audioStream->time_base.num = 1;

		if (outputFmt->flags & AVFMT_GLOBALHEADER) {
			m_encodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

		// 打开音频编码器
		ret = avcodec_open2(m_encodeCtx, codec, NULL);
		if (ret < 0) {
			av_strerror(ret, buf, sizeof(buf) - 1);
			AS_LOG(AS_LOG_ERROR, "could not open audio codec, %s.", buf);
			break;
		}

		ret = avcodec_parameters_from_context(m_audioStream->codecpar, m_encodeCtx);
		if (ret < 0) {
			av_strerror(ret, buf, sizeof(buf) - 1);
			AS_LOG(AS_LOG_ERROR, "could not initialize stream parameters, %s.", buf);
			break;
		}

		// 打开RTMP连接
		ret = avio_open2(&m_ctx->pb, params.voiceUrl.data(), AVIO_FLAG_WRITE, NULL, NULL);
		if (ret < 0) {
			av_strerror(ret, buf, sizeof(buf) - 1);
			AS_LOG(AS_LOG_ERROR, "could not open RTMP connection, %s.", buf);
			break;
		}

		m_resampler = swr_alloc_set_opts(nullptr, av_get_default_channel_layout(m_encodeCtx->channels),
			m_encodeCtx->sample_fmt,
			m_encodeCtx->sample_rate,
			av_get_default_channel_layout(m_encodeCtx->channels),
			AV_SAMPLE_FMT_S16,
			m_encodeCtx->sample_rate,
			0, nullptr);

		if (!m_resampler) {
			ret = AVERROR(ENOMEM);
			break;
		}

		ret = swr_init(m_resampler);
		if (ret < 0) {
			break;
		}

		if (!(m_fifo = av_audio_fifo_alloc(m_encodeCtx->sample_fmt, m_encodeCtx->channels, 1))) {
			AS_LOG(AS_LOG_ERROR, "could not allocate FIFO");
			ret = AVERROR(ENOMEM);
			break;
		}

		// 写输出文件的头部
		ret = avformat_write_header(m_ctx, NULL);
	} while (0);

	if (ret < 0) {
		sendStatus(STREAM_VOICE_START_FAIL, std::to_string(kConnectError));
		cleanup();
	}
	pf_pcm = fopen("D:\\test.pcm", "wb");

	return ret;
}

int rtmp_voice_client::start_2()
{
	AudioCollect& audioCollect = AudioCollect::GetInstance();
	if (AudioCollect::GetInstance().GetState() < 0) {
		AS_LOG(AS_LOG_WARNING, "start rtmp audio client while collector not init.");
		return -1;
	}

	audioCollect.startIngress();
	int error = 0;

	unsigned char* audio_data = AS_NEW(audio_data, 4096);
	unsigned int data_size = 0;
	const int output_frame_size = m_encodeCtx->frame_size;

	while (!m_stopFlag) {
		while (!m_stopFlag && (av_audio_fifo_size(m_fifo) < output_frame_size)) {
			data_size = 0;
			if (AudioCollect::GetInstance().GetAudioData(audio_data, &data_size) == 0 && data_size > 0) {
				auto nb = data_size / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
#ifdef DEBUG
				if (pf_pcm) {
					fwrite(audio_data, data_size, 1, pf_pcm);
					fflush(pf_pcm);
				}
#endif // DEBUG
				readConvertAndStore(&audio_data, nb);
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
		
		if (AVERROR_EXIT == error) {
			break;
		}

		while (av_audio_fifo_size(m_fifo) >= output_frame_size) {
			error = loadEncodeAndWrite();
			if (AVERROR_EXIT == error) {
				sendStatus(STREAM_VOICE_FAIL, std::to_string(kPushFaild));
				break;
			}
		}

		if (AVERROR_EXIT == error) {
			break;
		}
	}
	
	cleanup();
	AS_DELETE(audio_data);
}

void rtmp_voice_client::cleanup() {
	AudioCollect::GetInstance().stopIngress();
	AudioCollect::GetInstance().Release();

	if (m_fifo) {
		av_audio_fifo_free(m_fifo);
		m_fifo = nullptr;
	}
	
	swr_free(&m_resampler);

	if (m_encodeCtx) {
		avcodec_free_context(&m_encodeCtx);
	}

	if (m_ctx) {
		// 写输出文件的尾部
		av_write_trailer(m_ctx);

		// 关闭编码器
		if (m_audioStream) {
			avcodec_close(m_audioStream->codec);
		}

		// 关闭RTMP连接
		avio_closep(&m_ctx->pb);

		// 释放输出媒体上下文
		avformat_free_context(m_ctx);
		m_ctx = nullptr;
	}
}

int rtmp_voice_client::initConvertSamples(uint8_t*** converted_input_samples, int frame_size) {
	int error;
	if (!(*converted_input_samples = (uint8_t **)calloc(m_encodeCtx->channels, sizeof(**converted_input_samples)))) {
		AS_LOG(AS_LOG_WARNING, "could not allocate converted input sample pointers");
		return AVERROR(ENOMEM);
	}

	if ((error = av_samples_alloc(*converted_input_samples, nullptr, m_encodeCtx->channels, 
		frame_size, m_encodeCtx->sample_fmt, 0)) < 0) {
		AS_LOG(AS_LOG_WARNING, "could not allocate converted input samples(error '%s')");
		av_freep(&(*converted_input_samples)[0]);
		free(*converted_input_samples);
		return error;
	}
	return 0;
}

int rtmp_voice_client::readConvertAndStore(uint8_t** data, int frame_size) {
	uint8_t** converted_input_samples = nullptr;
	int ret = AVERROR_EXIT;
	
	do {
		ret = initConvertSamples(&converted_input_samples, frame_size);
		if (ret) {
			break;
		}

		ret = swr_convert(m_resampler, converted_input_samples, frame_size, (const uint8_t **)data, frame_size);
		if (ret < 0) {
			break;
		}

		ret = av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + frame_size);
		if (ret < 0) {
			break;
		}

		if (av_audio_fifo_write(m_fifo, (void**)converted_input_samples, frame_size) < frame_size) {
			AS_LOG(AS_LOG_ERROR, "could not write data to FIFO");
			ret = AVERROR_EXIT;
			break;
		}
	} while (0);

	
	if (converted_input_samples) {
		av_freep(&converted_input_samples[0]);
		free(converted_input_samples);
	}
	return ret;
}

int rtmp_voice_client::loadEncodeAndWrite() {
	const int frame_size = FFMIN(av_audio_fifo_size(m_fifo), m_encodeCtx->frame_size);
	int data_written;

	AVFrame* frame; 
	if (!(frame = av_frame_alloc())) {
		return AVERROR_EXIT;
	}

	frame->nb_samples = frame_size;
	frame->format = m_encodeCtx->sample_fmt; //smaple_fmt;
	frame->channel_layout = m_encodeCtx->channel_layout;
	frame->sample_rate = m_encodeCtx->sample_rate;

	int error; 
	if ((error = av_frame_get_buffer(frame, 0)) < 0) {
		av_frame_free(&frame);
		AS_LOG(AS_LOG_ERROR, "could not allocate output frame samples (error '%d')", error);
		return AVERROR_EXIT;
	}

	if (av_audio_fifo_read(m_fifo, (void**)frame->data, frame_size) < frame_size) {
		AS_LOG(AS_LOG_ERROR, "could not read data from FIFO");
		av_frame_free(&frame);
		return AVERROR_EXIT;
	}

	int present = 0;
	if (encodeAudioFrame(frame, &present)) {
		av_frame_free(&frame);
		return AVERROR_EXIT;
	}

	av_frame_free(&frame);
	return 0;
}

int rtmp_voice_client::encodeAudioFrame(AVFrame* frame, int* data_present) {
	AVPacket* output_packet = av_packet_alloc();
	if (!output_packet) {
		return  AVERROR(ENOMEM);
	}
	int error;
	char err_buf[64];

	if (frame) {
		frame->pts = m_pts;
		m_pts += frame->nb_samples;
	}

	do {
		error = avcodec_send_frame(m_encodeCtx, frame);
		if (error == AVERROR_EOF) {
			error = 0;
			break;
		}
		else if (error < 0) {
			av_strerror(error, err_buf, sizeof(err_buf) - 1);
			AS_LOG(AS_LOG_WARNING, "could not send packet for encoding (error '%s')", err_buf);
			break;
		}

		/* Receive one encoded frame from the encoder. */
		error = avcodec_receive_packet(m_encodeCtx, output_packet);
		if (error == AVERROR(EAGAIN)) {
			error = 0;
			break;
		}
		else if (error == AVERROR_EOF) {
			error = 0;
			break;
		}
		else if (error < 0) {
			av_strerror(error, err_buf, sizeof(err_buf) - 1);
			AS_LOG(AS_LOG_WARNING, "could not encode frame (error '%s')", err_buf);
			break;
		}
		else {
			*data_present = 1;
		}

		output_packet->stream_index = m_audioStream->index;
		av_packet_rescale_ts(output_packet, m_encodeCtx->time_base, m_audioStream->time_base);

		/* Write one audio frame from the temporary packet to the output file. */
		if (*data_present && (error = av_interleaved_write_frame(m_ctx, output_packet)) < 0) {
			av_strerror(error, err_buf, sizeof(err_buf) - 1);
			AS_LOG(AS_LOG_WARNING, "could not write frame (error '%s')", err_buf);
			break;
		}
	} while (0);

	av_packet_free(&output_packet);
	return error;
}

void rtmp_voice_client::stop_1() {
	m_stopFlag = true;
	wait();

	cleanup();
	m_stopFlag = false;

	if (pf_pcm) {
		fclose(pf_pcm);
	}
}
#endif
