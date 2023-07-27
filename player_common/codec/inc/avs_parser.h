#pragma once

#include "libMediaService.h"
#include "util.h"
#include "avs_rtsp_ingress.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

class AvsStreamParser;

class AvsParser {
public:
	AvsParser() = default;
	virtual ~AvsParser();

	int init(AvsStreamParser* streamParser, MK_Stream* stream);
	void setFragIndex(uint16_t idx) {
		m_fragidx = idx;
	}
	void setAudio(bool audio) { m_audio = audio; }

	int parseRawData(uint8_t* data, int size, int64_t pts, int64_t dts, int flush, bool is_key = false);
	void close();

private:
	int withoutPareser(uint8_t* data, int size, int64_t pts, int64_t dts, bool is_key);

private:
	AVCodecContext*				m_avctx = nullptr;
	AVCodecParserContext*		m_parser = nullptr;
	AvsStreamParser*			m_streamParser = nullptr;
	int							m_stindx = -1;
	uint16_t					m_fragidx = 0;
	AVPacket*					m_parse_pkt = nullptr;
	bool						m_audio = false;
};

class AvsStreamParser
{
public:
	AvsStreamParser() = default;
	virtual ~AvsStreamParser() {
		close();
	};

	friend class AvsParser;

	int init(MK_Format_Contex* format);

	int praseConfigData(uint8_t* data, int size, int64_t pts, int64_t dts);

	void addTask(Task* task) {
		std::unique_lock<decltype(m_tasksMutex)> lck(m_tasksMutex);
		if (!task->m_forward) {
			m_tasks.emplace((void*)task, task);
		}
		else {
			m_forwardTasks.emplace((void*)task, task);
		}
	}

	void deleteTask(Task* task) {
		std::unique_lock<decltype(m_tasksMutex)> lck(m_tasksMutex);
		if (!task->m_forward) {
			m_tasks.erase((void*)task);
		}
		else {
			m_forwardTasks.erase((void*)task);
		}
	}

	int parseMediaData(MediaInfoMsg* msg);
	void close();

private:
	void clearTasks() {
		std::unique_lock<decltype(m_tasksMutex)> lck(m_tasksMutex);
		m_tasks.clear();
		m_forwardTasks.clear();
	}

private:
	std::mutex				m_tasksMutex;
	std::map<void*, Task*>	m_tasks;
	std::map<void*, Task*>	m_forwardTasks;
	MK_Format_Contex*		m_fmtctx = nullptr;
	AvsParser*				m_vidParser = nullptr;
	AvsParser*				m_audParser = nullptr;
};