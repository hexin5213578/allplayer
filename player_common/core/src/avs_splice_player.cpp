#include "avs_splice_player.h"

AvsSplicePlayer::AvsSplicePlayer(BizParams& p) : AvsPlayer(p) {
	this->play_mode = kRtsp;
	this->pre_cache = 1;

	if (TYPE_MULTI_REALVIDEO_START == this->type) {
		this->realtime = 1;
	}
}

int AvsSplicePlayer::play() {
	int ret = AvsRtspProcessor::init_1(this);
	if (ret < 0) {
		return ret;
	}

	auto& p = m_bizParams;
	for (int i = 0; i < p.StitchInfo.HWNDs.size(); ++i) {
		p.WindowsHandle = p.StitchInfo.HWNDs[i];
		AvsRtspBasePlayer* player = new AvsRtspBasePlayer(p);
		if (!player) {
			return AVERROR(ENOMEM);
		}

		if ((void*)INVALID_WND != p.StitchInfo.HWNDs[i]) {
			player->setHwAcc(m_hwnd);
		}
		m_players.push_back(player);
	}
	return start_1();
}

int AvsSplicePlayer::setMediaAttribute(MK_Format_Contex* format) {
	if (!format) {
		return AVERROR(EINVAL);
	}
	int ret = 0;
	for (int idx = 0; idx < m_players.size(); ++idx) {
		auto player = m_players.at(idx);
		if (ret = player->setMediaAttribute(format))
			break;
	}
	return ret;
}

int AvsSplicePlayer::doTask(AVPacket* pkt) {
	if (!m_formatCtx) {
		return AVERROR(EINVAL);
	}

	int ret = 0;
	int frag_idx = 0;
	if (pkt->stream_index == m_formatCtx->video_stream) {
		frag_idx = pkt->side_data_elems;
		pkt->side_data_elems = 0;
	}

	AvsRtspBasePlayer* player = nullptr;
	if (m_players.size() <= frag_idx || !(player = m_players.at(frag_idx))) {
		AS_LOG(AS_LOG_WARNING, "biz[%ld] frag %d has parsed is invalid in pointed size %d, invalid.",
			this->id, frag_idx, m_players.size());
		return AS_ERROR_CODE_INVALID;
	}
	return player->doTask(pkt);
}

void AvsSplicePlayer::pause(bool pause, bool clear) {
	if (this->realtime) {
		return;
	}

	if (this->paused == pause) {
		return;
	}

	if (this->paused) {
		this->resume_1();
	}
	else {
		this->pause_1();
	}

	for (auto player : m_players) {
		if (player) {
			player->pause(pause, clear);
		}
	}
}

void AvsSplicePlayer::stop() {
	for (auto player : m_players) {
		if (player) {
			player->stop();
		}
	}
	stop_1();
}


