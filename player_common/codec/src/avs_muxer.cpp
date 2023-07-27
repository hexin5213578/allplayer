#include "avs_muxer.h"
#include "as_log.h"
#include "as_common.h"
#include "util.h"
#include "avs_player_factory.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_IOS)
#include "as_frame.h"
#endif

static const int kThresholdDuation = 2500;

AvsMuxer::AvsMuxer(long bizId, int32_t bizType)
    :avs_egress(bizId, bizType) {
	m_videoSt = { 0 };
	m_videoSt.input_index = -1;
	m_audioSt = { 0 };
	m_audioSt.input_index = -1;

	if (WaterMarkManager::get_instance()->get_watermark_on(false)) {
		m_waterMark = WaterMarkManager::get_instance()->produce_watermark(m_bizId);
		if (!m_waterMark) {
			throw std::runtime_error("produce watermark failed");
		}
	}
}

AvsMuxer::~AvsMuxer() {
	AS_DELETE(m_waterMark);
}

int32_t AvsMuxer::addStream(OutputStream* ost, MK_Stream* stream)
{
	int ret = 0;
	MK_CodecParameters* codecpar = stream->codecpar;
	AVCodecID codecId = (AVCodecID)codecpar->codec_id;
	if (AV_CODEC_ID_NONE == codecId)
		return AVERROR(EINVAL);

	ost->st = avformat_new_stream(m_pFormatCtx, nullptr);
	if (!ost->st) {
		AS_LOG(AS_LOG_ERROR, "Could not allocate stream");
		return AVERROR(ENOMEM);
	}
	ost->st->codec->codec_tag = 0;

	switch (codecpar->codec_type) {
	case MEDIA_TYPE_VIDEO: {
		ost->st->codecpar->codec_id = codecId;
		ost->st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		ost->st->time_base.den  = 1000 ;
		ost->st->time_base.num = 1;

		int res = codecpar->width * codecpar->height;
		if (m_waterMark && !ost->dec) {
			if (!(ost->dec = AS_NEW(ost->dec))) {
				ret = AVERROR(ENOMEM);
				break;
			}

			if ((ret = ost->dec->initMK(stream)) < 0)
				break;
		}
		
		if (res > 0) {
			ost->st->codecpar->width = codecpar->width;
			ost->st->codecpar->height = codecpar->height;
			m_resolution = codecpar->width * codecpar->height;
		}
	}
	break;

	case MEDIA_TYPE_AUDIO: {
		ost->st->codecpar->codec_id = AV_CODEC_ID_AAC;
		ost->st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
		ost->st->codecpar->format = AV_SAMPLE_FMT_FLTP;
		ost->st->codecpar->channel_layout = codecpar->channel_layout;
		ost->st->codecpar->channels = av_get_channel_layout_nb_channels(codecpar->channel_layout);
		ost->st->codecpar->sample_rate = codecpar->sample_rate;

		if (!ost->dec) {
			if (!(ost->dec = AS_NEW(ost->dec))) {
				ret = AVERROR(ENOMEM);
				break;
			}
		}
		
		if ((ret = ost->dec->initMK(stream)) < 0) {
			break;
		}

		if (!ost->enc) {
			AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
			if (!codec) {
				ret = AVERROR(EEXIST);
				break;
			}

			if (!(ost->enc = avcodec_alloc_context3(codec))) {
				ret = AVERROR(EINVAL);
				break;
			}

			ost->enc->sample_fmt = AV_SAMPLE_FMT_FLTP;
			ost->enc->channel_layout = codecpar->channel_layout;
			ost->enc->channels = av_get_channel_layout_nb_channels(codecpar->channel_layout);
			ost->enc->sample_rate = codecpar->sample_rate;
			
			ost->st->time_base.den = codecpar->sample_rate;
			ost->st->time_base.num = 1;
			/* Some container formats (like MP4) require global headers to be present.
			* Mark the encoder so that it behaves accordingly. */
			if (m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
				ost->enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			if (ret = avcodec_open2(ost->enc, codec, nullptr)) {
				AS_LOG(AS_LOG_WARNING, "biz[%ld] open aac encoder fail, %d.", ret);
				break;
			}
			if (ret = avcodec_parameters_from_context(ost->st->codecpar, ost->enc)) {
				break;
			}
		}

		if (!ost->resampler && !(ost->resampler = AS_NEW(ost->resampler))) {
			return AVERROR(ENOMEM);
		}
		ost->resampler->setParameters(ost->enc->channel_layout, ost->enc->sample_fmt, ost->enc->sample_rate, ost->enc->frame_size);
		
		if (!ost->fifo && !(ost->fifo = av_audio_fifo_alloc(ost->enc->sample_fmt, ost->enc->channels, 1))) {
			return AVERROR(ENOMEM);
		}
	}
	break;

	default:
		break;
	}

	if (ret >= 0) {
		if (!ost->frame && !(ost->frame = av_frame_alloc())) {
			ret = AVERROR(ENOMEM);
		}
	}
	
	if (ret < 0) {
		char buf[FFMPEG_ERR_BUFF_LEN] = { 0 };
		av_strerror(ret, buf, FFMPEG_ERR_BUFF_LEN - 1);
		AS_LOG(AS_LOG_WARNING, "biz[%ld] add stream fail, %s.", buf);
		ost->close();
		return ret;
	}

	ost->output_init = 1;
	return ret;
}

int32_t AvsMuxer::init(std::string& output, MK_Format_Contex* format) {
	int ret = 0;
	m_outPath = output;
	m_format = format;
	m_videoSt.input_index = m_format->video_stream;
	m_audioSt.input_index = m_format->audio_stream;

	if (-1 == m_videoSt.input_index) {
		ret = AVERROR(EINVAL);
	}

	MK_Stream* vid_st;
	if (m_format && (vid_st = m_format->streams.at(m_format->video_stream))) {
		auto codecpar = vid_st->codecpar;
		m_resolution = codecpar->width * codecpar->height;
	}

	if (m_resolution) {
		ret = allocMuxerContext();
	}
	return ret;
}

int32_t AvsMuxer::decodeConvertStore(AVPacket* intputPacket)
{
	int ret = AVERROR_EXIT;
	intputPacket->stream_index = m_audioSt.st->index;
	if (ret = m_audioSt.dec->sendPkt(intputPacket)) {
		return ret;
	}
	const int output_frame_size = m_audioSt.enc->frame_size;
	
	while (ret >= 0) {
		ret = m_audioSt.dec->recvFrameInt(m_audioSt.frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			ret = 0;
			break;
		}
		else if (ret < 0) {
			break;
		}
		uint8_t** converted_input_samples = nullptr;
		int ret1 = 0;
		do {
			ret1 = m_audioSt.resampler->resample(m_audioSt.frame, &converted_input_samples);
			if (ret1 < 0 ) {
				ret = ret1;
				AS_LOG(AS_LOG_ERROR, "Could not resample to aac.");
				break;
			}
			int nb_samples = m_audioSt.frame->nb_samples;
			av_frame_unref(m_audioSt.frame);
			if (av_audio_fifo_size(m_audioSt.fifo) < output_frame_size) {
				if ((ret = av_audio_fifo_realloc(m_audioSt.fifo, av_audio_fifo_size(m_audioSt.fifo) + nb_samples)) < 0) {
					AS_LOG(AS_LOG_ERROR, "Could not reallocate FIFO.");
					break;
				}
				if ((ret = av_audio_fifo_write(m_audioSt.fifo, (void**)converted_input_samples, nb_samples)) < 0) {
					AS_LOG(AS_LOG_ERROR, "Could not write data to FIFO.");
					break;
				}
			}
		} while (0);
		
		if (converted_input_samples) {
			av_freep(&converted_input_samples[0]);
			free(converted_input_samples);
		}
		av_frame_unref(m_audioSt.frame);
	}
	av_frame_unref(m_audioSt.frame);
	return ret;
}

int32_t AvsMuxer::allocMuxerContext()
{
	int ret = 0;
	char errBuf[FFMPEG_ERR_BUFF_LEN] = { 0 };
	if (m_videoSt.input_index < 0)
		return AVERROR(EINVAL);

	do {
		if ((ret = avformat_alloc_output_context2(&m_pFormatCtx, nullptr, nullptr, m_outPath.data())) < 0) {
			break;
		}

		AVCodecID codecId = AV_CODEC_ID_NONE;
		
		MK_Stream* stream = m_format->streams[m_videoSt.input_index];
		if (stream && stream->codecpar) {
			MK_CodecParameters* codecpar = stream->codecpar;
			codecId = (AVCodecID)codecpar->codec_id;
			if (AV_CODEC_ID_NONE == codecId) {
				ret = AVERROR(EINVAL);
				break;
			}

			m_pFormatCtx->oformat->video_codec = codecId;
			if (ret = addStream(&m_videoSt, stream)) {
				break;
			}
		}
		
		if (m_audioSt.input_index >= 0) {
			stream = m_format->streams[m_audioSt.input_index];
			if (stream && stream->codecpar) {
				m_pFormatCtx->oformat->audio_codec = AV_CODEC_ID_AAC;
				if (ret = addStream(&m_audioSt, stream)) {
					break;
				}
			}
		}

		/* open the output file, if needed */
		if (!(m_pFormatCtx->flags & AVFMT_NOFILE)) {
			ret = avio_open(&m_pFormatCtx->pb, m_outPath.c_str(), AVIO_FLAG_WRITE);
			if (ret < 0) {
				break;
			}
		}

		// write header
		if (ret = avformat_write_header(m_pFormatCtx, nullptr) < 0)
			break;

	} while (0);

	if (ret < 0) {
		av_strerror(ret, errBuf, sizeof(errBuf) - 1);
		AS_LOG(AS_LOG_ERROR, " alloc muxer ctx fail, %s!", errBuf);
		avformat_free_context(m_pFormatCtx);
		m_pFormatCtx = nullptr;
	}
	return ret;
}

int32_t AvsMuxer::encodeAudioFrame(AVFrame* frame, int* dataPresent)
{
	int ret = avcodec_send_frame(m_audioSt.enc, frame);
	if (ret < 0) {
		return ret;
	}
	AVPacket output_packet;
	av_init_packet(&output_packet);
	output_packet.data = NULL;
	output_packet.size = 0;

	while (ret >= 0) {
		ret = avcodec_receive_packet(m_audioSt.enc, &output_packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			ret = 0;
			break;
		}
		else if (ret < 0) {
			break;
		}

		*dataPresent = 1;
		output_packet.stream_index = m_audioSt.st->index;
		output_packet.pts = m_audioSt.next_pts;
		av_packet_rescale_ts(&output_packet, AVRational{ 1, 8000 }, m_audioSt.st->time_base);
		m_audioSt.next_pts += frame->nb_samples;
		int out_size = output_packet.size;
		if ((ret = av_interleaved_write_frame(m_pFormatCtx, &output_packet)) < 0) {
			if (AVERROR(EINVAL) == ret) {
				av_packet_unref(&output_packet);
				return 0;
			}
		}
		m_fileSize += out_size;
		av_packet_unref(&output_packet);
	}
	av_packet_unref(&output_packet);
	return ret;
}

int32_t AvsMuxer::loadEncodeWrite()
{
	int error;
	AVFrame* output_frame = av_frame_alloc();
	const int frame_size = FFMIN(av_audio_fifo_size(m_audioSt.fifo), m_audioSt.enc->frame_size);
	int data_written;

	output_frame->nb_samples = frame_size;
	output_frame->channel_layout = m_audioSt.enc->channel_layout;
	output_frame->format = m_audioSt.enc->sample_fmt;
	output_frame->sample_rate = m_audioSt.enc->sample_rate;

	if ((error = av_frame_get_buffer(output_frame, 0)) < 0) {
		av_frame_free(&output_frame);
		AS_LOG(AS_LOG_ERROR, "Could not allocate output frame samples (error '%d')", error);
		return error;
	}

	if (av_audio_fifo_read(m_audioSt.fifo, (void**)output_frame->data, frame_size) < frame_size) {
		AS_LOG(AS_LOG_ERROR, "Could not read data from FIFO\n");
		av_frame_free(&output_frame);
		return AVERROR_EXIT;
	}

	int dataWritten;
	encodeAudioFrame(output_frame, &dataWritten);
	av_frame_free(&output_frame);
	return 0;
}

int32_t AvsMuxer::addWaterMark()
{
	if (!m_waterMark || !m_videoSt.frame || m_resolution <= 0) {
		return AVERROR(EINVAL);
	}

	int ret = 0;

	if (!m_videoSt.enc) {
		AVCodec* codec = avcodec_find_encoder(m_videoSt.st->codecpar->codec_id);
		//AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec) {
			return AVERROR(EEXIST);
		}
		
		if (!(m_videoSt.enc = avcodec_alloc_context3(codec))) {
			return AVERROR(EINVAL);
		}

		m_videoSt.enc->width = m_videoSt.st->codecpar->width;
		m_videoSt.enc->height = m_videoSt.st->codecpar->height;

		m_videoSt.enc->time_base = { 1, 1000 };
		m_videoSt.enc->framerate = { 25, 1 };
		m_videoSt.enc->pix_fmt = AV_PIX_FMT_YUV420P;

		AVDictionary* opts = nullptr;
		ret = av_dict_set(&opts, "preset", "fast", 0);
		ret = av_dict_set(&opts, "bf", "0", 0);
		ret = av_dict_set(&opts, "g", "60", 0);
		ret = av_dict_set(&opts, "b_strategy", "0", 0);

		if ((ret = avcodec_open2(m_videoSt.enc, codec, &opts)) < 0) {
			avcodec_free_context(&m_videoSt.enc);
			av_dict_free(&opts);
			AS_LOG(AS_LOG_WARNING, "biz[%ld] open video encoder fail, %d.", ret);
			return ret;
		}
		//m_videoSt
		/*if (ret = avcodec_parameters_from_context(m_videoSt.st->codecpar, m_videoSt.enc))
			return ret;*/
		av_dict_free(&opts);
	}
	return ret;
}

int32_t AvsMuxer::encodeVideoFrame(AVFrame* frame)
{
	char errBuf[FFMPEG_ERR_BUFF_LEN] = { 0 };
	int ret = avcodec_send_frame(m_videoSt.enc, frame);
	if (ret < 0) {
		av_strerror(ret, errBuf, sizeof(errBuf) - 1);
		AS_LOG(AS_LOG_WARNING, "biz[%ld] send frame error: %s", m_bizId, errBuf);
		return ret;
	}

	AVPacket output_packet;
	av_init_packet(&output_packet);
	output_packet.data = NULL;
	output_packet.size = 0;

	while (ret >= 0) {
		ret = avcodec_receive_packet(m_videoSt.enc, &output_packet);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				ret = 0;
			}
			else {
				av_strerror(ret, errBuf, sizeof(errBuf) - 1);
				AS_LOG(AS_LOG_WARNING, "biz[%ld] receive packet error: %s", m_bizId, errBuf);
			}
			break;
		}

		output_packet.stream_index = m_videoSt.st->index;
		output_packet.pts = m_videoSt.next_pts;
		output_packet.dts = m_videoSt.next_pts;

		if (!frame) {
			m_videoSt.next_pts += (m_videoSt.last_duration > 0 ? m_videoSt.last_duration : 40);
		}
		else {
			int64_t cur_pts = frame->pts;
			if (INT64_MIN == m_lastVPts) {
				//first frame
			}
			else if (m_lastVPts != cur_pts) {
				auto duration = cur_pts - m_lastVPts;
				if (duration > 0 && duration < kThresholdDuation) {
					m_videoSt.next_pts += duration;
					m_videoSt.last_duration = duration;
				}
				else {
					m_videoSt.next_pts += 40;
					m_videoSt.last_duration = 40;
				}
			}
			m_lastVPts = cur_pts;
		}
		
		av_packet_rescale_ts(&output_packet, AVRational{ 1, 1000 }, m_videoSt.st->time_base);
		int out_size = output_packet.size;
		ret = av_interleaved_write_frame(m_pFormatCtx, &output_packet);
		if (AVERROR(EINVAL) == ret) {
			av_packet_unref(&output_packet);
			return 0;
		}

		if (ret < 0) {
			av_strerror(ret, errBuf, sizeof(errBuf) - 1);
			AS_LOG(AS_LOG_WARNING, "biz[%ld] av_interleaved write video frame error: %s", m_bizId, errBuf);
			break;
		}
		m_fileSize += out_size;
		if(!m_writeFrame) {
			m_writeFrame = true;
		}
		av_packet_unref(&output_packet);
	}
	av_packet_unref(&output_packet);
	return ret;
}

int AvsMuxer::do_egress(AVPacket* pkt)
{
	if (!pkt || !pkt->size) {
		return 0;
	}

	int ret = 0;
	if (!m_videoSt.output_init)
		return AVERROR(EINVAL);

	if (ret = processPkt(pkt)) {
		return ret;
	}
	return ret;
}

int32_t AvsMuxer::processPkt(AVPacket* pkt)
{
	int ret = 0;
	char errBuf[FFMPEG_ERR_BUFF_LEN] = { 0 };
	if (0 == pkt->stream_index) {
		if (!m_videoSt.output_init) {
			AS_LOG(AS_LOG_WARNING, "process video while outstream not init.");
			return AVERROR(EINVAL);
		}

		pkt->stream_index = m_videoSt.st->index;
		if (m_waterMark && m_videoSt.dec) {
			if ((ret = m_videoSt.dec->sendPkt(pkt)) < 0) {
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					ret = 0;
				}
				else {
					av_strerror(ret, errBuf, sizeof(errBuf) - 1);
					//AS_LOG(AS_LOG_WARNING, "biz[%ld] send packet error: %s", m_bizId, errBuf);
					return 0;
				}
			}

			while (ret >= 0) {	
				if ((ret = m_videoSt.dec->recvFrameInt(m_videoSt.frame)) < 0) {
					if (m_writeFrame) {
						ret = 0;
					}
					else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						ret = 0;
					}
					else {
						av_strerror(ret, errBuf, sizeof(errBuf) - 1);
						AS_LOG(AS_LOG_WARNING, "biz[%ld] remuxer decode error %s", m_bizId, errBuf);
					}
					break;
				}

				if (m_resolution > 0 && 
					( (m_videoSt.st->codecpar->width != m_videoSt.frame->width) ||
					(  m_videoSt.st->codecpar->height != m_videoSt.frame->height ))
					)
				{
					ret = AVERROR(EINVAL);
					break;
				}

				m_videoSt.st->codecpar->width = m_videoSt.frame->width;
				m_videoSt.st->codecpar->height = m_videoSt.frame->height;
				int resolution = m_videoSt.frame->width * m_videoSt.frame->height;
				if (!m_waterMark) {
					av_frame_unref(m_videoSt.frame);
				}
				
				if (m_waterMark) {
					ret = m_waterMark->filter_add_frame(m_videoSt.frame);
					if (ret < 0) {
						if ((AVERROR(EAGAIN) != ret) && (AVERROR_EOF != ret)) {
							av_frame_unref(m_videoSt.frame);
							av_strerror(ret, errBuf, sizeof(errBuf) - 1);
							AS_LOG(AS_LOG_WARNING, "biz[%ld] filter_add_frame error: %s", m_bizId, errBuf);
							break;
						}
					}

					while (ret >= 0) {
						ret = m_waterMark->filter_get_frame(m_videoSt.frame);
						if (ret < 0) {
							if ((AVERROR(EAGAIN) != ret) && (AVERROR_EOF != ret)) {
								av_strerror(ret, errBuf, sizeof(errBuf) - 1);
								AS_LOG(AS_LOG_WARNING, "biz[%ld] error %s in filter_get_frame", m_bizId, errBuf);
							}
							else {
								ret = 0;
							}
							break;
						}
							
						if ((ret = addWaterMark()) < 0) {
							break;
						}

						if ((ret = encodeVideoFrame(m_videoSt.frame)) < 0) {
							break;
						}
					}
					av_frame_unref(m_videoSt.frame);
				}
			}
		}

		if (!m_waterMark) {
			int64_t cur_pts = pkt->pts;
			pkt->pts = m_videoSt.next_pts;
			pkt->dts = pkt->pts;

			if (INT64_MIN == m_lastVPts) {
				//first frame
			}
			else if (m_lastVPts != cur_pts) {
				auto duration = cur_pts - m_lastVPts;
				if ((duration > 0) && (duration < kThresholdDuation)) {
					m_videoSt.next_pts += duration;
					m_videoSt.last_duration = duration;
				}
				else {
					m_videoSt.next_pts += 40;
					m_videoSt.last_duration = 40;
				}
			}
			m_lastVPts = cur_pts;
			int out_size = pkt->size;
			av_packet_rescale_ts(pkt, AVRational{ 1, 1000 }, m_videoSt.st->time_base);
			ret = av_interleaved_write_frame(m_pFormatCtx, pkt);
			if (ret < 0) {
				if (AVERROR(EINVAL) == ret) {
					return 0;
				}
				av_strerror(ret, errBuf, sizeof(errBuf) - 1);
				AS_LOG(AS_LOG_WARNING, "biz[%ld] interleaved_write_frame error: %s", m_bizId, errBuf);
			}
			else {
				m_fileSize += out_size;
				if (!m_writeFrame) {
					m_writeFrame = true;
				}
			}
		}
	}
	else if (m_audioSt.output_init) {
		ret = decodeConvertStore(pkt);
		const int output_frame_size = m_audioSt.enc->frame_size;
		while (av_audio_fifo_size(m_audioSt.fifo) >= output_frame_size)  {
			if ((ret = loadEncodeWrite()) < 0) {
				ret = 0;
				break;
			}
		}
	}
	return ret;
}

int AvsMuxer::write_trailer()
{
	m_fileSize = 0;
	if (!m_pFormatCtx) {
		return 0;
	}

	int ret = 0;
	if (m_audioSt.output_init && av_audio_fifo_size(m_audioSt.fifo) > 0) {
		loadEncodeWrite();
	}

	if (m_videoSt.output_init && m_waterMark && m_videoSt.enc) {
		encodeVideoFrame(nullptr);
	}

	char errBuf[FFMPEG_ERR_BUFF_LEN] = { 0 };
	if (m_writeFrame) {
		ret = av_write_trailer(m_pFormatCtx);
		if (ret < 0) {
			av_strerror(ret, errBuf, sizeof(errBuf) - 1);
			AS_LOG(AS_LOG_WARNING, "biz[%ld] av_write_trailer error %s", m_bizId, errBuf);
		}
	}
	else {
		AS_LOG(AS_LOG_WARNING, "biz[%ld] capture none complete frame, delete file.", m_bizId);
		ret = -1;
	}

	avio_closep(&m_pFormatCtx->pb);
	if (ret < 0 && m_pFormatCtx->url) {
		int ret1 = avpriv_io_delete(m_pFormatCtx->url);
		if (ret1 < 0) {
			av_strerror(ret1, errBuf, sizeof(errBuf) - 1);
			AS_LOG(AS_LOG_WARNING, "biz[%ld] avpriv_io_delete error %s", m_bizId, errBuf);
		}
	}

	if (m_pFormatCtx) {
		avformat_free_context(m_pFormatCtx);
		m_pFormatCtx = nullptr;
	}

	m_writeFrame = false;
	m_resolution = 0;
	m_lastVPts = INT64_MIN;
	m_videoSt.reclaim();
	m_audioSt.reclaim();
	return ret;
}

int AvsMuxer::stop()
{
	int ret = write_trailer();
	m_videoSt.close();
	m_audioSt.close();
	return ret;
}

