#include "avs_parser.h"
#include "as_log.h"

AvsParser::~AvsParser() {
	close();
}

int AvsParser::init(AvsStreamParser* streamParser, MK_Stream* stream) {
	if (!streamParser || !stream || !stream->codecpar) {
		return AVERROR(EINVAL);
	}
	m_streamParser = streamParser;

	int ret = 0;
	char buf[128] = { 0 };
	if (!(m_parse_pkt = av_packet_alloc())) {
		return AVERROR(ENOMEM);
	}

	AVCodecID codecid = (AVCodecID)stream->codecpar->codec_id;
	switch (codecid) {
	case AV_CODEC_ID_H264:
	case AV_CODEC_ID_HEVC: {
		AVCodec* codec = avcodec_find_decoder(codecid);
		if (!codec) {
			return AVERROR(EINVAL);
		}

		if (!(m_avctx = avcodec_alloc_context3(codec))) {
			return AVERROR(ENOMEM);
		}
		
		m_avctx->codec_id = codecid;
		/*m_avctx->time_base.num = stream->time_base.num;
		m_avctx->time_base.den = stream->time_base.den;*/
		m_avctx->time_base = { 1, 1000 };

		if (!(m_parser = av_parser_init(m_avctx->codec_id))) {
			ret = AVERROR(ENOTSUP);
			break;
		}

		if ((ret = avcodec_open2(m_avctx, nullptr, nullptr)) < 0) {
			break;
		}
	}
		break;
	case AV_CODEC_ID_NONE:
		ret = AVERROR(ENOTSUP);
		break;
	default:
		ret = 0;
		break;
	}
	m_stindx = stream->index;

	if (ret < 0) {
		av_strerror(ret, buf, sizeof(buf) - 1);
		AS_LOG(AS_LOG_ERROR, "parser open failed, %s.", buf);
		if (m_parser) {
			av_parser_close(m_parser);
			m_parser = nullptr;
		}
		avcodec_free_context(&m_avctx);
	}
	return ret;
}

int AvsParser::withoutPareser(uint8_t* data, int size, int64_t pts, int64_t dts, bool is_key) {
	int ret = 0;
	if (0 >= size) {
		return AVERROR(EINVAL);
	}

	if ((ret = av_new_packet(m_parse_pkt, size)) < 0) {
		return ret;
	}

	memcpy(m_parse_pkt->data, data, size);
	m_parse_pkt->stream_index = m_stindx;
	m_parse_pkt->pts = pts;
	if (is_key) {
		m_parse_pkt->flags |= AV_PKT_FLAG_KEY;
	}
	return ret;
}


int AvsParser::parseRawData(uint8_t* data, int size, int64_t pts, int64_t dts, int flush, bool is_key) {
	int ret = 0, got_output = flush;

	std::unique_lock<decltype(m_streamParser->m_tasksMutex)> lck(m_streamParser->m_tasksMutex);

	if (m_audio) {
		ret = withoutPareser(data, size, pts, dts, is_key);
		if (ret < 0) {
			return ret;
		}

		for (auto& iter : m_streamParser->m_forwardTasks) {
			auto task = iter.second;
			if (task) {
				task->doTask(m_parse_pkt);
			}
		}

		for (auto& iter : m_streamParser->m_tasks) {
			auto task = iter.second;
			if (task) {
				task->doTask(m_parse_pkt);
			}
		}
		av_packet_unref(m_parse_pkt);
		return 0;
	}

	if (!m_streamParser->m_forwardTasks.empty()) {
		ret = withoutPareser(data, size, pts, dts, is_key);
		if (ret < 0) {
			return ret;
		}

		for (auto& iter : m_streamParser->m_forwardTasks) {
			auto task = iter.second;
			if (task) {
				task->doTask(m_parse_pkt);
			}
		}
		av_packet_unref(m_parse_pkt);
	}

	if (m_parser && !m_streamParser->m_tasks.empty()) {
		AVPacket* out_pkt = m_parse_pkt;
		while (size > 0 || (flush && got_output)) {
			int len;
			len = av_parser_parse2(m_parser, m_avctx, &out_pkt->data, &out_pkt->size,
				data, size, pts, dts, 0);

			data = len ? data + len : data;
			size -= len;

			got_output = !!out_pkt->size;
			if (!out_pkt->size) {
				continue;
			}

			ret = av_packet_make_refcounted(out_pkt);
			if (ret < 0) {
				break;
			}
			out_pkt->stream_index = m_stindx;
			out_pkt->pts = m_parser->pts;
			out_pkt->dts = m_parser->dts;
			out_pkt->pos = m_parser->pos;
			out_pkt->side_data_elems = m_fragidx;

			if (m_parser->key_frame == 1 || (m_parser->key_frame == -1 && m_parser->pict_type == AV_PICTURE_TYPE_I)) {
				out_pkt->flags |= AV_PKT_FLAG_KEY;
			}

			for (auto& iter : m_streamParser->m_tasks) {
				auto task = iter.second;
				if (task) {
					task->doTask(m_parse_pkt);
				}
			}
			av_packet_unref(m_parse_pkt);
		}
	}
	return ret;
}

void AvsParser::close() {
	if (m_parser) {
		av_parser_close(m_parser);
		m_parser = nullptr;
	}

	avcodec_free_context(&m_avctx);
	av_packet_free(&m_parse_pkt);
}

int AvsStreamParser::init(MK_Format_Contex* format) {
	if (!format) {
		return AVERROR(EINVAL);
	}

	int ret = 0;
	MK_Stream* stream;
	if (format->video_stream >= 0) {
		stream = format->streams.at(format->video_stream);
		if (stream && stream->codecpar) {
			if (!m_vidParser && !(m_vidParser = AS_NEW(m_vidParser))) {
				ret = AVERROR(ENOMEM);
			}
			ret = m_vidParser->init(this, stream);
		}
	}

	if (ret < 0) {
		AS_DELETE(m_vidParser);
		return ret;
	}

	if (format->audio_stream >= 0) {
		stream = format->streams.at(format->audio_stream);
		if (stream && stream->codecpar) {
			if (!m_audParser && !(m_audParser = AS_NEW(m_audParser))) {
				ret = AVERROR(ENOMEM);
			}
			ret = m_audParser->init(this, stream);
		}
	}
	
	if (ret < 0) {
		AS_DELETE(m_audParser);
		ret = 0;
	}
	m_fmtctx = format;
	return ret;
}

int AvsStreamParser::praseConfigData(uint8_t* data, int size, int64_t pts, int64_t dts)
{
	int ret = 0;
	if (m_vidParser) {
		ret = m_vidParser->parseRawData(data, size, pts, pts, 0, 0);
	}
	return ret;
}

int AvsStreamParser::parseMediaData(MediaInfoMsg* msg) {
	if (!msg || !m_fmtctx) {
		return AVERROR(EINVAL);
	}

	int ret = AVERROR(EINVAL);
	int64_t pts = msg->dataInfo.pts;
	bool is_key = msg->dataInfo.is_key;
	
	if (msg->dataInfo.stream_index == m_fmtctx->video_stream) {
		if (m_vidParser) {
			m_vidParser->setFragIndex(msg->dataInfo.fragment);
			ret = m_vidParser->parseRawData((uint8_t*)msg + sizeof(MediaInfoMsg), msg->dataSize, pts, pts, 0, is_key);
		}
	}
	else if (msg->dataInfo.stream_index == m_fmtctx->audio_stream) {
		if (m_audParser) {
			ret = m_audParser->parseRawData((uint8_t*)msg + sizeof(MediaInfoMsg), msg->dataSize, pts, pts, 0, is_key);
			m_audParser->setAudio(true);
		}
	}
	else {
	}
	return ret;
}

void AvsStreamParser::close() {
	if (m_vidParser) {
		m_vidParser->parseRawData(nullptr, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 1);
	}

	if (m_audParser) {
		m_audParser->parseRawData(nullptr, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 1);
	}

	clearTasks();
	AS_DELETE(m_vidParser);
	AS_DELETE(m_audParser);
}


