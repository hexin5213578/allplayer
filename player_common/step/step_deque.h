#pragma once
#include <deque>
#include <mutex>

struct AVPacket;

class StepDeque
{
public:
    enum PktsDeque_ERRNO
    {
        ERR_AGAIN = -11,
        ERR_OUTRANG = -10,
        ERR_WINDEND = -9,   //清理完毕
        ERR_CLRAR = -8,
        NOERR = 0,
    };

    enum STATUS {
        REALTIME = 0,       //实时，无单帧缓冲
        BUFFERING = 1,      //缓冲单帧状态
        STEPPING = 2,       //单帧播放状态
        WINDINGUP = 3,      //正常退出.exit或resume,进行收尾
    };

    StepDeque();

    virtual ~StepDeque() { }

    void setStepCapcity(int step_capcity);

    int getQueueSize();

    int getCacheSize();
    /**
     * \param forward 前向或后向
     * \return 是否满足单帧条件
     */
    bool stepSatify(int8_t stepDirect);

    STATUS getDequeStatus();

    int forward(AVPacket** pkt);

    int backward(AVPacket** pkt);

    int push(AVPacket* pkt);

    int pop(AVPacket** pkt);

    //缓冲大小是否满足暂停条件
    bool checkLevel();

    //清理前向缓冲至规定值
    void stopStep();

    void clear();

private:
    std::mutex              m_listMtx;
    std::deque<AVPacket*>   m_pkts_queue;
    int                     m_step_capcity = 0;    //前向缓冲帧容量
    int                     m_backward_size = 0;
    int                     m_forward_size = 0;
    int                     m_cur_index = 0;
    STATUS                  m_status;
    int                     m_pktSize = 0;
};