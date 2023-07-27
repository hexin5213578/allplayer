#ifndef __AVS_SPLICE_PLAYER_H__
#define __AVS_SPLICE_PLAYER_H__

#include "avs_rtsp_player.h"
#include "avs_rtsp_processor.h"

class AvsSplicePlayer : public AvsPlayer, public AvsRtspProcessor {
public:
	AvsSplicePlayer(BizParams& p);
	virtual ~AvsSplicePlayer() = default;

	int play() override;

	int setMediaAttribute(MK_Format_Contex* format) override;

	int doTask(AVPacket* pkt) override;

	void pause(bool pause, bool clear) override;

	void stop() override;

private:
	std::vector<AvsRtspBasePlayer*>	m_players;
};

#endif // !__AVS_SPLICE_PLAYER_H__