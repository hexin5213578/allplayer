#pragma once
#include "mk_client_connection.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_rtsp_rtp_frame_organizer.h"
#include "mk_rtsp_service.h"

#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_IOS)
#include "as_frame.h"
#endif

//从字符串中提取子字符串
std::string FindField(const char* buf, const char* start, const char* end, size_t bufSize = 0);

int fmtp_next_attr_value(const char** p, char* attr, int attr_size, char* value, int value_size);

typedef struct PayloadContext PayloadContext;

//enum MKStreamParseType 
//{
//	MKSTREAM_PARSE_NONE,
//	MKSTREAM_PARSE_FULL,       /**< full parsing and repack */
//	MKSTREAM_PARSE_HEADERS,    /**< Only parse headers, do not repack. */
//	MKSTREAM_PARSE_TIMESTAMPS, /**< full parsing and interpolation of timestamps for frames not starting on a packet boundary */
//	MKSTREAM_PARSE_FULL_ONCE,  /**< full parsing and repack of the first frame only, only implemented for H.264 currently */
//	MKSTREAM_PARSE_FULL_RAW,   /**< full parsing and repack with timestamp and position generation by parser for raw */
//};

class Track : public mk_rtp_frame_handler
{
public:
	struct MultiDataInfo {
		MediaDataInfo info;
		uint32_t rtplen;
	};

	Track(mk_client_connection* conn, int st) : m_conn(conn), m_streamId(st) { 
		if (!m_conn) {
			throw std::runtime_error("connection is null when construct track.");
		}
	}
	virtual ~Track() = default;

	int init();

	virtual bool ready() { return true; }

	virtual int packConfigFrame(uint8_t* data, uint32_t& data_size, uint32_t total_size, uint32_t pts, double ntp) { return 0; }

	void appendRtpStatistics(RTP_PACKET_STAT_INFO& statistic);

	virtual int handleRtpPacket(as_msg_block* block) = 0;

	int handleRtpFrame(uint8_t payloadType, RTP_PACK_QUEUE& rtpFrameList) override;

	virtual std::list<ASFrame::Ptr>	getConfigFrames() { return std::list<ASFrame::Ptr>(); }

protected:
	int checkValid(mk_rtp_packet& rtp);

	virtual int parse_sdp_a_line() {
		return 0;
	}

	virtual int32_t handleRtp(uint8_t* rtpData, uint32_t rtpLen) = 0;

	virtual int32_t handleRtpList(RTP_PACK_QUEUE& rtpFrameList) { return 0; }

	virtual int isConfigFrame(int type) { return 0; }

	virtual int isKeyFrame(int type) { return 0; }

	virtual int packMediaInfo(MediaDataInfo& dataInfo, int key, uint32_t pts, double ntp = 0.0);

	int32_t checkFrameTotalDataLen(RTP_PACK_QUEUE& rtpFrameList);

	MK_Stream* getTrackStream();

public:
	int					   m_needParsing;

protected:
	mk_rtp_frame_organizer	m_organizer;
	mk_client_connection*	m_conn;
	int						m_streamId;
};
