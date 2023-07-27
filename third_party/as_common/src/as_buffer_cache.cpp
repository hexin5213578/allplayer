#include "as_buffer_cache.h"


as_data::as_data()
{
    m_pData      = NULL;
    m_rd_ptr     = NULL;
    m_wr_ptr     = NULL;
    m_ulSize     = 0;
    m_ulRefCount = 0;
}

as_data::as_data(uint32_t size)
{
    m_ulSize = size;
    char* pData = NULL;
    try {
        pData = new char[size];
    }
    catch(...)
    {
        pData = NULL;
    }
    m_pData = m_rd_ptr = m_wr_ptr = pData;
    m_ulRefCount = 1;
}

as_data::~as_data()
{
    if(NULL != m_pData) 
    {
        try {
            delete []m_pData;
        }
        catch(...) {

        }
        m_pData = NULL;
    }
    m_rd_ptr     = NULL;
    m_wr_ptr     = NULL;
    m_ulSize     = 0;
    m_ulRefCount = 0;
}

char*    as_data::base()
{
    return m_pData;
}

char*    as_data::rd_ptr()
{
    return m_rd_ptr;
}

char*    as_data::wr_ptr()
{
    return m_wr_ptr;
}

void     as_data::rd_ptr(int32_t len)
{
    m_rd_ptr += len;
    return;
}

void     as_data::wr_ptr(int32_t len)
{
    m_wr_ptr += len;
    return;
}

uint32_t as_data::size()
{
    return m_ulSize;
}

uint32_t as_data::length()
{
    if(m_wr_ptr <= m_rd_ptr) {
        return 0;
    }
    return (m_wr_ptr - m_rd_ptr);
}

int32_t  as_data::copy(char* data,uint32_t len)
{
    uint32_t size = m_ulSize - (m_wr_ptr - m_pData);
    if(size < len) {
        return AS_ERROR_CODE_MEM;
    }
    memcpy(m_wr_ptr,data,len);
    m_wr_ptr += len;
    return AS_ERROR_CODE_OK;
}

void as_data::inc_ref()
{
    m_ulRefCount++;
}

void as_data::dec_ref()
{
    m_ulRefCount--;
}
uint32_t as_data::get_ref()
{
    return m_ulRefCount;
}

as_cache::as_cache()
{
    m_pData      = NULL;
    m_pAllocator = NULL;
}

as_cache::as_cache(uint32_t size)
{
    try {
        m_pData = new as_data(size);
    }
    catch(...) {
        m_pData = NULL;
    }
    m_pAllocator = NULL;
}

as_cache::as_cache(as_data* pData,as_thread_allocator* pAllocator)
{
    m_pData = pData;
    m_pAllocator = pAllocator;
    if(NULL != m_pData) {
        m_pData->inc_ref();
    }
}

void as_cache::set_allocator(as_thread_allocator* pAllocator) 
{
    m_pAllocator = pAllocator;
}

as_cache::~as_cache()
{
    if(NULL == m_pData) {
        return;
    }

    m_pData->dec_ref();
    if(0 ==  m_pData->get_ref()) 
    {
        try {
            delete m_pData;
        }
        catch(...) {

        }
        m_pData = NULL;
    }
}

char*    as_cache::base()
{
    if(NULL == m_pData) {
        return NULL;
    }
    return m_pData->base();
}

char*    as_cache::rd_ptr()
{
    if(NULL == m_pData) {
        return NULL;
    }
    return m_pData->rd_ptr();
}

char*    as_cache::wr_ptr()
{
    if(NULL == m_pData) {
        return NULL;
    }
    return m_pData->wr_ptr();
}

void     as_cache::rd_ptr(int32_t len)
{
    if(NULL == m_pData) {
        return ;
    }
    m_pData->rd_ptr(len);
}

void     as_cache::wr_ptr(int32_t len)
{
    if(NULL == m_pData) {
        return ;
    }
    m_pData->wr_ptr(len);
}

uint32_t as_cache::size()
{
    if(NULL == m_pData) {
        return 0;
    }
    return m_pData->size();
}

uint32_t as_cache::length()
{
    if(NULL == m_pData) {
        return 0;
    }
    return m_pData->length();
}

int32_t  as_cache::copy(char* data,uint32_t len)
{
    if(NULL == m_pData) {
        return AS_ERROR_CODE_MEM;
    }
    return m_pData->copy(data,len);
}

int32_t  as_cache::copy(as_cache* pCache)
{
    if(NULL == m_pData) {
        return AS_ERROR_CODE_MEM;
    }
    return m_pData->copy(pCache->rd_ptr(),pCache->length());
}

as_cache* as_cache::duplicate()
{
    as_cache* pCache = NULL;

    if(NULL == m_pData) {
        return NULL;
    }
    
    try {
        pCache = new as_cache(m_pData,m_pAllocator);
    }
    catch(...) {
        return NULL;
    }
    return pCache;
}

uint32_t as_cache::get_ref()
{
    if(NULL == m_pData) {
        return 0;
    }
    return m_pData->get_ref();
}

void as_cache::release()
{
    if(NULL == m_pData) {
        return;
    }

    if(NULL !=m_pAllocator) 
    {
        m_pAllocator->free(this);
    }
    else 
    {
        m_pData->dec_ref();
        if(0 ==  m_pData->get_ref()) 
        {
            try {
                delete m_pData;
            }
            catch(...) {

            }
            m_pData = NULL;
        }

        delete this;    
    }
}
/*****************************************************************************
 ****************************************************************************/

as_thread_allocator::as_thread_allocator(uint32_t ulThreadId)
{
    m_ulRefCount = 1;
    m_ulThreadId = ulThreadId;
    for(uint32_t i = 0 ; i < AS_CACHE_SIZE_DEFINE_MAX;i++) 
    {
        m_Cachelist[i].clear();
    }
}
as_thread_allocator::~as_thread_allocator()
{
    as_cache* pCache = NULL;
    for(uint32_t i = 0 ; i < AS_CACHE_SIZE_DEFINE_MAX;i++) {
        while (0 < m_Cachelist[i].size())
        {
            pCache = m_Cachelist[i].front();
            m_Cachelist[i].pop_front();
            if(NULL != pCache) {
                //pCache->release();
                //TODO: free back to pool
                as_buffer_cache::instance().free(i,pCache);
            }
            pCache = NULL;
        }
        
    }
}
uint32_t  as_thread_allocator::threadId()
{
    return m_ulThreadId;
}

as_cache* as_thread_allocator::allocate(uint32_t ulSize)
{
    as_cache* pCache = NULL;
    uint32_t ulMin = 0;
    uint32_t ulMax = 0;
    uint32_t i = 0;
    uint32_t index = AS_CACHE_SIZE_DEFINE_MAX;
    for(i = 0; i < AS_CACHE_SIZE_DEFINE_MAX; i++) 
    {
        ulMax = as_cache_size_array[i];
        if((ulMin < ulSize)&&(ulMax >= ulSize)) {
            index = i;
            break;
        }
        ulMin = ulMax;
    }

    if(AS_CACHE_SIZE_DEFINE_MAX == index) 
    {
        return NULL;
    }

    if(0 == m_Cachelist[index].size()) 
    {
        return NULL;
    }

    pCache = m_Cachelist[index].front();
    m_Cachelist[index].pop_front();
    pCache->set_allocator(this);
    return pCache;
}

void      as_thread_allocator::free(as_cache* cache)
{
    if(1 != cache->get_ref()) 
    {
        cache->set_allocator(NULL);
        cache->release();
        return;
    }
    uint32_t ulSize = cache->size();
    uint32_t ulMin = 0;
    uint32_t ulMax = 0;
    uint32_t i = 0;
    uint32_t index = AS_CACHE_SIZE_DEFINE_MAX;
    for(i = 0;i  < AS_CACHE_SIZE_DEFINE_MAX;i++) 
    {
        ulMax = as_cache_size_array[i];
        if((ulMin < ulSize)&&(ulMax >= ulSize)) {
            index = i;
            break;
        }
        ulMin = ulMax;
    }
    if(AS_CACHE_SIZE_DEFINE_MAX == index) 
    {
        return;
    }
    m_Cachelist[index].push_back(cache);
    if(AS_CACHE_MAX_ALLOC_COUNT <= m_Cachelist[index].size()) 
    {
        free_to_pool(index);
    }
}
void  as_thread_allocator::alloc_from_pool(uint32_t index)
{
    as_cache* pCache = NULL;
    for(uint32_t i = 0 ;i < AS_CACHE_MIN_ALLOC_COUNT;i++) 
    {
        pCache = as_buffer_cache::instance().allocate(index);
        if(NULL == pCache) {
            break;
        }
        m_Cachelist[index].push_back(pCache);
        pCache->set_allocator(this);
    }
}

void  as_thread_allocator::free_to_pool(uint32_t index)
{
    as_cache* pCache = NULL;
    for(uint32_t i = 0 ;i < AS_CACHE_MIN_ALLOC_COUNT;i++) {
        if( 0 == m_Cachelist[index].size()) {
            break;
        }
        pCache = m_Cachelist[index].front();
        m_Cachelist[index].pop_front();
        as_buffer_cache::instance().free(index,pCache);
    }
}

as_buffer_allocator::as_buffer_allocator()
{
    m_pThreadAllocator = NULL;
}
as_buffer_allocator::~as_buffer_allocator()
{
    if(NULL != m_pThreadAllocator) {
        as_buffer_cache::instance().release_thread_allocator(m_pThreadAllocator);
        m_pThreadAllocator = NULL;
    }
}
as_cache* as_buffer_allocator::allocate(uint32_t ulSize)
{
    if(NULL == m_pThreadAllocator) {
        m_pThreadAllocator = as_buffer_cache::instance().find_thread_allocator();
    }

    if(NULL == m_pThreadAllocator) {
        return NULL;
    }

    return m_pThreadAllocator->allocate(ulSize);
}


as_buffer_cache::as_buffer_cache()
{

}
as_buffer_cache::~as_buffer_cache()
{

}
int32_t   as_buffer_cache::init(uint32_t config[],uint32_t size)
{
    as_cache* pCache = NULL;
    if(size > AS_CACHE_SIZE_DEFINE_MAX) {
        return AS_ERROR_CODE_PARAM;
    }
    for(uint32_t i = 0;i < size;i++) {
        uint32_t size = as_cache_size_array[i];
        for(uint32_t j = 0; j< config[i];j++) {
            try {
                pCache = new as_cache(size);
            }
            catch(...) {
                return AS_ERROR_CODE_MEM;
            }
            m_CachePool[i].push_back(pCache);
        }
    }
    return AS_ERROR_CODE_OK;
}
void      as_buffer_cache::release()
{
    as_cache* pCache = NULL;
    for(uint32_t i = 0 ; i < AS_CACHE_SIZE_DEFINE_MAX;i++) {
        while (0 < m_CachePool[i].size())
        {
            pCache = m_CachePool[i].front();
            m_CachePool[i].pop_front();
            pCache->release();
            pCache = NULL;
        }
        
    }
    return;
}

as_cache* as_buffer_cache::allocate(uint32_t ulIndex)
{
    as_cache* pCache = NULL;
    if(ulIndex >= AS_CACHE_SIZE_DEFINE_MAX) {
        return NULL;
    }
    if(0 == m_CachePool[ulIndex].size()) {
        return NULL;
    }
    pCache = m_CachePool[ulIndex].front();
    m_CachePool[ulIndex].pop_front();

    return pCache;
}
void      as_buffer_cache::free(uint32_t ulIndex,as_cache* cache)
{
    if(ulIndex >= AS_CACHE_SIZE_DEFINE_MAX) {
        return;
    }
    m_CachePool[ulIndex].push_back(cache);
    return ;
}
as_thread_allocator* as_buffer_cache::find_thread_allocator()
{
    uint32_t ulCurThreadId = as_get_threadid();
    as_thread_allocator* pAllocator = NULL;
    THREAD_ALLOCATOR_MAP::iterator iter = m_ThreadAllocMap.find(ulCurThreadId);
    if(iter != m_ThreadAllocMap.end()) {
        pAllocator = iter->second;
        pAllocator->m_ulRefCount++; 
    }
    try {
        pAllocator = new as_thread_allocator(ulCurThreadId);
    }
    catch(...) {
        pAllocator = NULL;
    }
    if(NULL != pAllocator) {
        m_ThreadAllocMap.insert(THREAD_ALLOCATOR_MAP::value_type(ulCurThreadId,pAllocator));
    }
    return pAllocator;
}
void as_buffer_cache::release_thread_allocator(as_thread_allocator* pAllocator)
{
    uint32_t ulThreadId = pAllocator->threadId();
    pAllocator->m_ulRefCount--;
    if(0 < pAllocator->m_ulRefCount) {
        return;
    }

    THREAD_ALLOCATOR_MAP::iterator iter = m_ThreadAllocMap.find(ulThreadId);
    if(iter != m_ThreadAllocMap.end()) {
        m_ThreadAllocMap.erase(iter);
    }
    try {
        delete pAllocator ;
    }
    catch(...) {
        
    }
    pAllocator = NULL;
    return;
}