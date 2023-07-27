#pragma once 

#include "track.h"

class AACTrack : public Track
{
public:
	AACTrack(mk_client_connection* conn, int st) : 
		Track(conn, st)
	{ 
		m_needParsing = 1;
	}
	
	~AACTrack() 
	{}

	int handleRtpPacket(as_msg_block* block) override;

	int32_t handleRtp(uint8_t* rtpData, uint32_t rtpLen) override;

	int32_t handleRtpList(RTP_PACK_QUEUE& rtpFrameList) override;

private:
	char	recvbuf_[4096];
};