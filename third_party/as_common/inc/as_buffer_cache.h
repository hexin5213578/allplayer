#ifndef __AS_BUFFER_CACHE_INCLUDE_H__
#define __AS_BUFFER_CACHE_INCLUDE_H__

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
extern "C" {
#include  "as_mutex.h"
#include  "as_thread.h"
}
#include <list>
#include <map>

#define AS_CACHE_MIN_ALLOC_COUNT   100
#define AS_CACHE_MAX_ALLOC_COUNT   150

class as_data
{
public:
   friend class as_cache;
public:
    virtual ~as_data();
    char*    base();
    char*    rd_ptr();
    char*    wr_ptr();
    void     rd_ptr(int32_t len);
    void     wr_ptr(int32_t len);
    uint32_t size();
    uint32_t length();
    int32_t  copy(char* data,uint32_t len);
protected:
    as_data(uint32_t size);
    as_data();
    void inc_ref();
    void dec_ref();
    uint32_t get_ref();
private:
    char*    m_pData;
    char*    m_rd_ptr;
    char*    m_wr_ptr;
    uint32_t m_ulSize;
    uint32_t m_ulRefCount;
};

class as_thread_allocator;
class as_cache
{
public:
    friend  class as_thread_allocator;    
public:
    as_cache(uint32_t size);
    virtual ~as_cache();
    char*    base();
    char*    rd_ptr();
    char*    wr_ptr();
    void     rd_ptr(int32_t len);
    void     wr_ptr(int32_t len);
    uint32_t size();
    uint32_t length();
    int32_t  copy(char* data,uint32_t len);
    int32_t  copy(as_cache* pCache);
    as_cache* duplicate();
    void     release();
protected:
    as_cache();
    as_cache(as_data* pData,as_thread_allocator* pAllocator);
    void set_allocator(as_thread_allocator* pAllocator);
    uint32_t get_ref();
protected:
    as_data*             m_pData;
    as_thread_allocator* m_pAllocator;
};

enum AS_CACHE_SIZE_DEFINE
{
    AS_CACHE_SIZE_DEFINE_128BYTE     = 0,
    AS_CACHE_SIZE_DEFINE_256BYTE     = 1,
    AS_CACHE_SIZE_DEFINE_512BYTE     = 2,
    AS_CACHE_SIZE_DEFINE_1KBYTE      = 3,
    AS_CACHE_SIZE_DEFINE_2KBYTE      = 4,
    AS_CACHE_SIZE_DEFINE_4KBYTE      = 5,
    AS_CACHE_SIZE_DEFINE_8KBYTE      = 6,
    AS_CACHE_SIZE_DEFINE_16KBYTE     = 7,
    AS_CACHE_SIZE_DEFINE_32KBYTE     = 8,
    AS_CACHE_SIZE_DEFINE_64KBYTE     = 9,
    AS_CACHE_SIZE_DEFINE_128KBYTE    = 10,
    AS_CACHE_SIZE_DEFINE_256KBYTE    = 11,
    AS_CACHE_SIZE_DEFINE_512KBYTE    = 12,
    AS_CACHE_SIZE_DEFINE_1MBYTE      = 13,
    AS_CACHE_SIZE_DEFINE_2MBYTE      = 14,
    AS_CACHE_SIZE_DEFINE_4MBYTE      = 15,
    AS_CACHE_SIZE_DEFINE_MAX
};

static uint32_t as_cache_size_array [] = {
       128,
       256,
       512,
      1024,
      2048,
      4096,
      8192,
     16384,
     32768,
     65536,
    131072,
    262144,
    524288,
   1048576,
   2097152,
   4194304
};
typedef std::list<as_cache*>     AS_CACHE_LIST;
typedef AS_CACHE_LIST::iterator  AS_CACHE_LIST_ITER;

class as_thread_allocator
{
public:
    friend class as_buffer_cache;
    friend class as_buffer_allocator;
    friend class as_cache;
public:
    virtual ~as_thread_allocator();
protected:
    as_thread_allocator(uint32_t ulThreadId);
    uint32_t  threadId();
    as_cache* allocate(uint32_t ulSize);
    void      free(as_cache* cache);
private:
    void      alloc_from_pool(uint32_t index);
    void      free_to_pool(uint32_t index);
private:
    uint32_t               m_ulThreadId;
    uint32_t               m_ulRefCount;
    AS_CACHE_LIST          m_Cachelist[AS_CACHE_SIZE_DEFINE_MAX];
};

class as_buffer_allocator
{
public:
    as_buffer_allocator();
    virtual ~as_buffer_allocator();
    as_cache* allocate(uint32_t ulSize);
private:
    as_thread_allocator* m_pThreadAllocator;
};


class as_buffer_cache
{
public:
    friend class as_thread_allocator;
    friend class as_buffer_allocator;
public:
    static as_buffer_cache& instance()
    {
        static as_buffer_cache objBufferCache;
        return objBufferCache;
    };
    virtual ~as_buffer_cache();
    int32_t   init(uint32_t config[],uint32_t size = AS_CACHE_SIZE_DEFINE_MAX);
    void      release();
protected:
    as_buffer_cache();
    as_cache* allocate(uint32_t ulIndex);
    void      free(uint32_t ulIndex,as_cache* cache);
    as_thread_allocator* find_thread_allocator();
    void release_thread_allocator(as_thread_allocator* pAllocator);
private:
    AS_CACHE_LIST          m_CachePool[AS_CACHE_SIZE_DEFINE_MAX];
    typedef std::map<uint32_t,as_thread_allocator*>     THREAD_ALLOCATOR_MAP;
    THREAD_ALLOCATOR_MAP   m_ThreadAllocMap;
};


#endif /* __AS_BUFFER_CACHE_INCLUDE_H__ */
