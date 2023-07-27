#ifndef __AS_TASK_INCLUDE_H__
#define __AS_TASK_INCLUDE_H__

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
extern "C"{
#include  "as_mutex.h"
#include  "as_thread.h"
}

#define AS_TASK_STACK_DEFAULT       (128*1024) 

class as_task
{
public:
    as_task();
    virtual ~as_task();
    virtual int32_t activate(uint32_t ulThreadCount,uint32_t ulStackSize = AS_TASK_STACK_DEFAULT);
    virtual void    svc() = 0;
protected:
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    static  void* task_thread(void *arg);
#elif AS_APP_OS == AS_OS_WIN32
    static  uint32_t __stdcall task_thread(void *arg);
#endif
private:
    as_thread_t**   m_pThreadArray;
    uint32_t        m_ulCount;
};

#endif /*__AS_TASK_INCLUDE_H__*/