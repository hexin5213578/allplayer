#pragma once 

#include "track.h"

class HEVCTrack : public Track {
public:
	HEVCTrack(mk_client_connection* conn, int st) : Track(conn, st) { 
		m_needParsing = 1;
	}
	virtual ~HEVCTrack() = default;

	int handleRtpPacket(as_msg_block* block) override;

	int32_t handleRtp(uint8_t* rtpData, uint32_t rtpLen) override;

	int32_t handleRtpList(RTP_PACK_QUEUE& rtpFrameList) override;

	int parse_sdp_a_line() override;

	bool ready() override;

	int packConfigFrame(uint8_t* data, uint32_t& data_size, uint32_t total_size, uint32_t pts, double ntp) override;

	std::list<ASFrame::Ptr>	getConfigFrames() override {
		std::list<ASFrame::Ptr> lst;
		if (ready()) {
			lst.emplace_back(m_vps);
			lst.emplace_back(m_sps);
			lst.emplace_back(m_pps);
		}
		return lst;
	}

private:
	int isConfigFrame(int type) override;

	int isKeyFrame(int type) override;

	int32_t handleSingleFrame(RTP_PACK_QUEUE& rtpFrameList);

	int32_t handleMultiFrames(RTP_PACK_QUEUE& rtpFrameList);

private:
	ASFrame::Ptr	m_vps;
	ASFrame::Ptr	m_sps;
	ASFrame::Ptr	m_pps;
};