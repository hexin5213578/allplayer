#pragma once 

#include "track.h"

class G711Track : public Track
{
public:
	G711Track(mk_client_connection* conn, int st) :
		Track(conn, st)
	{
		m_needParsing = 0;
	}
	
	~G711Track()
	{}

	int handleRtpPacket(as_msg_block* block) override;

	int32_t handleRtp(uint8_t* rtpData, uint32_t rtpLen) override;

	int32_t handleRtpList(RTP_PACK_QUEUE& rtpFrameList) override;
private:

};