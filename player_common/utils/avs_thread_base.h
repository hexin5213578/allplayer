#pragma once

#include <thread>

class AvsThreadBase
{
public:
    AvsThreadBase() = default;
    virtual ~AvsThreadBase() = default;

    //启动线程
    virtual void startRun() {
        static int i = 0;
        i++;
        m_index = i;
        m_bIsExit = false;
        //启动线程
        m_thread = std::thread(&AvsThreadBase::mainProcess, this);
    }

    bool getStarted() { return !m_bIsExit;}

    //停止线程
    virtual void stopRun() { m_bIsExit = true; }

    //等待线程退出
    virtual void wait() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    //暂停或者播放
    virtual void setPause(bool isPause) { m_paused = isPause; }

    bool getPaused() { return m_paused; }

protected:
    //线程入口函数
    virtual void mainProcess() = 0;

protected:
    bool m_paused = false;
    //标志线程退出
    volatile bool m_bIsExit = true;
    //线程索引号
    int m_index = 0;

private:
    std::thread m_thread;
};
