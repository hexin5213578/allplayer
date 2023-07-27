#include "avs_stream_internal.h"
#include "as_config.h"

#if((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "track.h"
#elif ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "track/track.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "track/track.h"
#endif


int AvsStreamInternal::init()
{
	if (!stream->track)
		return AVERROR(EINVAL);

	if (stream->track->m_needParsing)
	{
		int ret = 0;
		char errBuf[128] = { 0 };
		AVCodec* codec;
		AVCodecID avcid = (AVCodecID)stream->codecpar->codec_id;
		
		switch (avcid)
		{
		case AV_CODEC_ID_H264:
		case AV_CODEC_ID_HEVC:
		{
			if (!(codec = avcodec_find_decoder(avcid)))
				return AVERROR(EINVAL);

			if (!(avctx = avcodec_alloc_context3(codec)))
				return AVERROR(ENOMEM);

			avctx->codec_id = avcid;
			if (!(parser = av_parser_init(avcid)))
			{
				ret = AVERROR(ENOMEM);
				break;
			}
			
			if ((ret = avcodec_open2(avctx, codec, nullptr)) < 0)
				break;

			if(!(parse_pkt = av_packet_alloc()))
				ret = AVERROR(ENOMEM);
		}
		break;
		default:
			break;
		}

		if (ret < 0)
		{
			av_strerror(ret, errBuf, sizeof(errBuf) - 1);
			AS_LOG(AS_LOG_ERROR, "stream internal open codec fail, %s.", errBuf);
			release();
		}
		return ret;
	}
	return 0;
}

void AvsStreamInternal::release()
{
	if (parser)
	{
		av_parser_close(parser);
		parser = nullptr;
	}
	av_packet_free(&parse_pkt);
	avcodec_free_context(&avctx);
}

int AvsStreamInternal::gen_packet(AVPacket* pkt, const uint8_t* buf, int len)
{
	int ret = 0;
	if ((ret = av_new_packet(pkt, len)) < 0)
		return ret;
	memcpy(pkt->data, buf, len);
	return ret;
}

int AvsStreamInternal::parse_packet(AVPacket* pkt)
{
	uint8_t* data = pkt->data;
	int size = pkt->size;
	int ret = 0;
	AVPacket* out_pkt = parse_pkt;

	if (parser)
	{
		while (size > 0)
		{
			int len;
			len = av_parser_parse2(parser, avctx,
				&out_pkt->data, &out_pkt->size, data, size,
				pkt->pts, pkt->dts, pkt->pos);

			pkt->pts = pkt->dts = AV_NOPTS_VALUE;

			data = len ? data + len : data;
			size -= len;

			if (!out_pkt->size)
				continue;

			if (pkt->buf && out_pkt->data == pkt->data) {
				out_pkt->buf = av_buffer_ref(pkt->buf);
				if (!out_pkt->buf) {
					ret = AVERROR(ENOMEM);
					break;
				}
			}
			else {
				ret = av_packet_make_refcounted(out_pkt);
				if (ret < 0)
					break;
			}

			if (pkt->side_data) {
				out_pkt->side_data = pkt->side_data;
				out_pkt->side_data_elems = pkt->side_data_elems;
				pkt->side_data = NULL;
				pkt->side_data_elems = 0;
			}

			out_pkt->stream_index = stream->index;
			out_pkt->pts = parser->pts;
			out_pkt->dts = parser->dts;
			out_pkt->pos = parser->pos;
			out_pkt->flags |= pkt->flags & AV_PKT_FLAG_DISCARD;

			if (parser->key_frame == 1 ||
				(parser->key_frame == -1 &&
					parser->pict_type == AV_PICTURE_TYPE_I))
				out_pkt->flags |= AV_PKT_FLAG_KEY;

			AVPacket* pkt1;
			if (!(pkt1 = av_packet_alloc()))
			{
				ret = AVERROR(ENOMEM);
				break;
			}
			av_packet_move_ref(pkt1, out_pkt);
			parse_queue.emplace_back(pkt1);
		}

		if (ret < 0)
			av_packet_unref(out_pkt);

		av_packet_unref(pkt);
	}
	else
	{
		av_packet_move_ref(out_pkt, pkt);
		parse_queue.emplace_back(out_pkt);
	}
	return ret;
}
