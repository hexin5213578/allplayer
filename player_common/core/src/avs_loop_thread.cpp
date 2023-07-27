#include "avs_loop_thread.h"

avs_loop_thread::avs_loop_thread(const char* threadName, void* context, void(*messageHanler)(unifyMsgStruct msg, void* data))
{
	this->m_threadName = threadName;
	this->m_context = context;
	this->m_msgHanler = messageHanler;
}

void* threadCallback(void* context) {
	avs_loop_thread* looperThread = (avs_loop_thread*)context;
	looperThread->loop();
	return 0;//使用 return 更好的回收一些资源
}

void avs_loop_thread::start() {
	if (!m_pMsgCbThread) {
		if (AS_ERROR_CODE_OK != as_create_thread((AS_THREAD_FUNC)threadCallback,
			this, &m_pMsgCbThread, AS_DEFAULT_STACK_SIZE)) {
			AS_LOG(AS_LOG_ERROR, "create msgcb thread fail.");
		}
	}
}

void avs_loop_thread::loop() {
    unifyMsgStruct msg;
    while (!m_bExit) {
        std::unique_lock<std::mutex> lck(m_mtx);
        while (m_msgQueue.size() == 0) {
			m_cv.wait(lck);
            if (m_bExit) {
				lck.unlock();
                m_threadTerminated = true;
                return;
            }
        }

		msg = m_msgQueue.front();
		m_msgQueue.pop();
        m_msgHanler(msg, m_context);
    }
    m_threadTerminated = true;
}

void avs_loop_thread::stop() {
	if (!m_bExit) {
		shutdown();
	}
	releaseQueue();
}

void avs_loop_thread::sendMessage(unifyMsgStruct& msg) {
	std::unique_lock<std::mutex> lck(m_mtx);
	m_msgQueue.push(msg);
	m_cv.notify_all();
}

void avs_loop_thread::clearMessage() {
	std::unique_lock<std::mutex> lck(m_mtx);
	while (!m_msgQueue.empty()) {
		m_msgQueue.pop();
	}
}

/**
 * 一定要在所有消息处理函数完成后才能调用
 */
void avs_loop_thread::shutdown()
{
	m_bExit = true;
	m_cv.notify_all();
	int sleepCount = 0;
	as_join_thread(m_pMsgCbThread);

	while (!m_threadTerminated) {
		if (sleepCount > 10) {
			break;
		}
		++sleepCount;
		as_sleep(10);
	}
}

void avs_loop_thread::releaseQueue() {
	clearMessage();
	std::queue<unifyMsgStruct> empty;
	swap(empty, m_msgQueue);
}
