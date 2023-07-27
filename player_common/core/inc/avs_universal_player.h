#pragma once

#include <mutex>

#include "avs_thread_base.h"
#include "avs_player.h"
#include "avs_refresh_loop.h"

class AvsDemuxer;

class AvsUniversalPlayer : public AvsThreadBase, public AvsPlayer
{
public:
	AvsUniversalPlayer(BizParams& params);
	virtual ~AvsUniversalPlayer() = default;

	int play() override;
	
	void seek(double pos) override;

	void pause(bool pause, bool clear = false) override;

	void stop() override { stopRun(); }

	void stopRun() override;

protected:
	void mainProcess() override;

	int doTask(AVPacket* pkt) override { return 0; }
	
private:
	void playRoute();
	int readUntil(int64_t target, AVPacket* pkt);

	int openInput();
	void closeInput();
    
private:
	std::string			m_strUrl;
	AvsDemuxer*			m_demux = nullptr;
	int					m_infinitBuffer = -1;
};

