

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

        uint32_t GetUsingPercent() const;

        uint32_t SetCacheSize(uint32_t ulCacheSize);

        uint32_t GetCacheSize() const;

        uint32_t Peek(char* pBuf, uint32_t ulPeekLen);

        uint32_t Read(char* pBuf, uint32_t ulReadLen);

        uint32_t Write(const char* pBuf, uint32_t ulWriteLen);

        uint32_t GetDataSize() const;

        uint32_t GetEmptySize() const;

        void Clear();
    private:
        as_mutex_t*      m_pMutex;

        char*            m_pBuffer;        
        uint32_t    m_ulBufferSize;
        uint32_t    m_ulDataSize;

        uint32_t    m_ulReader;
        uint32_t    m_ulWriter;
};

#endif
