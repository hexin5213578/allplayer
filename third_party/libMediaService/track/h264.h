#pragma once 

#include "track.h"

class H264Track : public Track
{
public:
	H264Track(mk_client_connection* conn, int st) : Track(conn, st),
		m_timeStamp(0), m_currFrag(0) {
		m_needParsing = 1;
	}
	virtual ~H264Track();

	int parse_sdp_a_line() override;

	int handleRtpPacket(as_msg_block* block) override;

	int32_t handleRtp(uint8_t* rtpData, uint32_t rtpLen) override;

	int32_t handleRtpList(RTP_PACK_QUEUE& rtpFrameList) override;

	bool ready() override { return m_sps && m_pps; }

	int packConfigFrame(uint8_t* data, uint32_t& data_size, uint32_t total_size, uint32_t pts, double ntp) override;

	std::list<ASFrame::Ptr>	getConfigFrames() override {
		std::list<ASFrame::Ptr > lst;
		if (ready()) {
			lst.emplace_back(m_sps);
			lst.emplace_back(m_pps);
		}
		return lst;
	}

private:
	int isConfigFrame(int type) override;

	int isKeyFrame(int type) override;

	int unpackStapA(uint8_t* rtpData, uint32_t rtpLen);

	int32_t handleSingleFrame(RTP_PACK_QUEUE& rtpFrameList);

	int32_t handleMultiFrames(RTP_PACK_QUEUE& rtpFrameList);

	int ff_h264_parse_sprop_parameter_sets(uint8_t** data_ptr, int* size_ptr, const char* value);

	uint32_t				m_timeStamp;
	int16_t					m_currFrag;
	ASFrame::Ptr			m_sps;
	ASFrame::Ptr			m_pps;
};