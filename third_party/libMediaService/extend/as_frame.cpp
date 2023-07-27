#include "as_frame.h"

MK_MediaType getMediaType1(MKCodecID codecId) {
	switch (codecId) {
	case MK_CODEC_ID_MJPEG:
	case MK_CODEC_ID_H264:
	case MK_CODEC_ID_HEVC:
		return MEDIA_TYPE_VIDEO;

	case MK_CODEC_ID_PCM_MULAW:
	case MK_CODEC_ID_PCM_ALAW:
	case MK_CODEC_ID_AAC:
		return MEDIA_TYPE_AUDIO;

	default:
		break;
	}
	return MEDIA_TYPE_UNKNOWN;
}

MK_MediaType CodecInfo::getMediaType() const {
	return getMediaType1(this->getCodecId());
}
