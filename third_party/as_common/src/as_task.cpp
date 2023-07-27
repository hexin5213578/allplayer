#include "as_task.h"

as_task::as_task()
{
    m_pThreadArray             = NULL;
    m_ulCount                  = 0;
}

as_task::~as_task()
{
    as_thread_t*  pHandle = NULL;
    if(NULL != m_pThreadArray) {
        for(uint32_t i = 0; i < m_ulCount;i++) {
            pHandle = m_pThreadArray[i];
            if(NULL != pHandle) {
                as_destory_thread(pHandle);
            }
            m_pThreadArray[i] = NULL;
        }
        try {
            delete[]m_pThreadArray;
        }
        catch(...) {

        }
        m_pThreadArray = NULL;
    }
}


int32_t as_task::activate(uint32_t ulThreadCount,uint32_t ulStackSize)
{
    try {
        m_pThreadArray = new as_thread_t*[ulThreadCount];
    }
    catch(...) {
        m_pThreadArray = NULL;
        return AS_ERROR_CODE_MEM;
    }
    m_ulCount = ulThreadCount;
    uint32_t          i = 0;
    as_thread_t*      pHandle = NULL; 
    for(i = 0; i < ulThreadCount;i++) {
        m_pThreadArray[i] = NULL;
    }
    for(i = 0; i < ulThreadCount;i++) {
        int32_t nRet = as_create_thread(task_thread,this,&pHandle,ulStackSize);
        if (AS_ERROR_CODE_OK != nRet)
        {
            return AS_ERROR_CODE_FAIL;
        }
        m_pThreadArray[i] = pHandle;
    }

    return AS_ERROR_CODE_OK;
}


#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
 void* as_task::task_thread(void *arg)
#elif AS_APP_OS == AS_OS_WIN32
 uint32_t __stdcall as_task::task_thread(void *arg)
#endif
{
    as_task* pTask = (as_task*)arg;
    pTask->svc();
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    return NULL;
#elif AS_APP_OS == AS_OS_WIN32
    return 0;
#endif   
}
