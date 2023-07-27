#pragma once

#include <thread>

class AvsThreadBase
{
public:
    AvsThreadBase() = default;
    virtual ~AvsThreadBase() = default;

    //�����߳�
    virtual void startRun() {
        static int i = 0;
        i++;
        m_index = i;
        m_bIsExit = false;
        //�����߳�
        m_thread = std::thread(&AvsThreadBase::mainProcess, this);
    }

    bool getStarted() { return !m_bIsExit;}

    //ֹͣ�߳�
    virtual void stopRun() { m_bIsExit = true; }

    //�ȴ��߳��˳�
    virtual void wait() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    //��ͣ���߲���
    virtual void setPause(bool isPause) { m_paused = isPause; }

    bool getPaused() { return m_paused; }

protected:
    //�߳���ں���
    virtual void mainProcess() = 0;

protected:
    bool m_paused = false;
    //��־�߳��˳�
    volatile bool m_bIsExit = true;
    //�߳�������
    int m_index = 0;

private:
    std::thread m_thread;
};
