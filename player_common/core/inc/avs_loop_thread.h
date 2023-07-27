#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "avs_player_common.h"

struct unifyMsgStruct {
	int				msgType;		//0-status,1-progress
	long			bizId;
	long			bizType;
	long			status;
	std::string		description;
	long			current;		//当前播放/下载进度
	long			total;			//总进度
};


class avs_loop_thread
{
public:
	avs_loop_thread(const char* threadName, void* context, void(*messageHanler)(unifyMsgStruct msg, void* data));
	virtual ~avs_loop_thread() = default;

	void start();

	void loop();

	void stop();

	void sendMessage(unifyMsgStruct& msg);

	void clearMessage();

	/**
	 * 一定要在所有消息处理函数完成后才能调用
	 */
	void shutdown();

private:
	void releaseQueue();

private:

	void*							m_context = nullptr;
	void	(*m_msgHanler)(unifyMsgStruct msg, void* context) = nullptr;

	volatile	bool				m_bExit = false;
	bool							m_threadTerminated = false;
	std::mutex						m_mtx;
	std::condition_variable			m_cv;

	std::queue<unifyMsgStruct>		m_msgQueue;
	as_thread_t*					m_pMsgCbThread = nullptr;	//状态，数据等回调处理线程
	const char*						m_threadName = nullptr;
};

