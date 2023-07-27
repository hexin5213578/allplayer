#include "as_thread.h"
#include "as_config.h"
#include "as_common.h"
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <pthread.h>
#elif AS_APP_OS == AS_OS_WIN32
#endif


int32_t  as_create_thread( AS_THREAD_FUNC pfnThread, void *args, as_thread_t **pstASThread,uint32_t ulStackSize)
{
    as_thread_t *pstThread = NULL ;

    pstThread = (as_thread_t*)(void*)malloc(sizeof(as_thread_t));
    if( NULL == pstThread )
    {
        return AS_ERROR_CODE_MEM;
    }

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    if ( pthread_attr_init(&pstThread->attr) != 0 )
    {
        free(pstThread);
        return AS_ERROR_CODE_FAIL ;
    }

    pthread_attr_setdetachstate(&pstThread->attr, PTHREAD_CREATE_JOINABLE );

    if( 0 == ulStackSize )
    {
        ulStackSize = AS_DEFAULT_STACK_SIZE;
    }
    if (pthread_attr_setstacksize(&pstThread->attr, (size_t)ulStackSize))
    {
        free(pstThread);
        return AS_ERROR_CODE_FAIL ;
    }

    if ( pthread_create(&pstThread->pthead, &pstThread->attr, pfnThread, args) != 0 )
    {
        free(pstThread);
        return AS_ERROR_CODE_FAIL ;
    }
#elif AS_APP_OS == AS_OS_WIN32
    pstThread->pthead = CreateThread(NULL,ulStackSize,pfnThread,args,0,&pstThread->ptheadID);
    if (NULL == pstThread->pthead)
    {
        free(pstThread);
        return AS_ERROR_CODE_FAIL ;
    }
#endif
    *pstASThread = pstThread ;

    return AS_ERROR_CODE_OK;
}
void  as_destory_thread(as_thread_t *pstMKThread)
{
    if(NULL != pstMKThread) {
        free(pstMKThread);
    }
    return;
}
int32_t as_join_thread(as_thread_t *pstMKThread)
{
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    pthread_join(pstMKThread->pthead, 0);
#elif AS_APP_OS == AS_OS_WIN32
    (void)WaitForSingleObject(pstMKThread->pthead, INFINITE);
    (void)CloseHandle(pstMKThread->pthead);
#endif
    return AS_ERROR_CODE_OK;
}

void     as_thread_exit(void *retval)
{
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    pthread_exit(retval);
#elif AS_APP_OS == AS_OS_WIN32
    (void)retval;
    ExitThread(0);
#endif
}
uint32_t as_get_threadid(void)
{
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    pthread_t pThreadID = pthread_self();
    return 0;
#elif AS_APP_OS == AS_OS_WIN32
    return GetCurrentThreadId();
#endif
}

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
pthread_t  as_thread_self(void)
{
    return pthread_self();
}
#elif AS_APP_OS == AS_OS_WIN32
HANDLE as_thread_self(void)
{
    return GetCurrentThread();
}
#endif
