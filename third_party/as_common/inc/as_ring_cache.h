

#ifndef _RING_CACHE_H_
#define _RING_CACHE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_mutex.h"
#include "as_event.h"
#include "as_thread.h"
#include "as_time.h"
#include "as_json.h"
}

class as_ring_cache
{
    public:
        as_ring_cache();
        virtual ~as_ring_cache();
    public:

        //获得当前缓冲区中数据长度和缓冲区长度的比例的百分数
        uint32_t GetUsingPercent() const;

        //设置缓冲区大小，返回设置完成后缓冲的大小
        uint32_t SetCacheSize(uint32_t ulCacheSize);

        uint32_t GetCacheSize() const;

        //查看指定长度数据，但缓冲中仍然保存这些数据，返回实际读取数据长度
        uint32_t Peek(char* pBuf, uint32_t ulPeekLen);

        //读取指定长度数据，并将这些数据从缓冲中清理掉，返回实际读取数据长度
        uint32_t Read(char* pBuf, uint32_t ulReadLen);

        //写指定长度数据，返回实际写数据长度，若缓冲区空间不够，禁止写入
        uint32_t Write(const char* pBuf, uint32_t ulWriteLen);

        //获得当前缓冲中数据大小
        uint32_t GetDataSize() const;

        //获得当前空余缓冲大小
        uint32_t GetEmptySize() const;

        void Clear();
    private:
        as_mutex_t*      m_pMutex;

        char*            m_pBuffer;        
        uint32_t    m_ulBufferSize; // 缓冲区大小
        uint32_t    m_ulDataSize;

        //将缓冲区所有字节从0开始编号，到(m_lBufferSize-1)
        //以下两值就是此编号，即缓冲区偏移值
        uint32_t    m_ulReader;     //数据区头部(数据读取端)
        uint32_t    m_ulWriter;     //空余区头部(数据写入端)
};

#endif
