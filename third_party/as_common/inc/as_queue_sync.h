#ifndef __AS_SYNC_QUEUE_INCLUDE__
#define __AS_SYNC_QUEUE_INCLUDE__

#include <queue>
using namespace std;
#include "as_synchronized.h"

template <class Type>
class CSyncQueue:public as_synchronized
{
public:
    CSyncQueue( );    
    virtual ~CSyncQueue();
    int32_t init(int32_t maxQueueLen);
    bool empty(void);
    int32_t size(void);
    int32_t pushBackEv(Type *ev,int32_t timeOut = 0,int32_t mode = QUEUE_MODE_NOWAIT );
    int32_t popFrontEv(Type *&ev,int32_t timeout = 0,int32_t mode = QUEUE_MODE_WAIT );
private:    
    queue<Type *> p_Queue;
    int32_t m_maxQueueLen;
};

template <class Type> 
CSyncQueue<Type>::CSyncQueue()
{
}

template <class Type>
CSyncQueue<Type>::~CSyncQueue()
{
}

template <class Type>
int32_t CSyncQueue<Type>::init(int32_t maxQueueLen)
{
    int32_t result = AS_ERROR_CODE_OK;
    
    m_maxQueueLen = maxQueueLen;
    result = start();
    
    return result ;    
}

template <class Type>
bool CSyncQueue<Type>::empty(void)
{
    bool em;
    
    lock();
    em = p_Queue.empty();
    unlock();
    
    return em ;
}

template <class Type>
int32_t CSyncQueue<Type>::size(void)
{
    int32_t sz;
    
    lock();
    sz = p_Queue.size();
    unlock();
    
    return sz ;
}

template <class Type>
int32_t CSyncQueue<Type>::popFrontEv(Type *&ev,int32_t timeout , int32_t mode )
{
    int32_t result = AS_ERROR_CODE_OK ;

    lock();    
    while( AS_TRUE )
    { 
        result = p_Queue.empty();        
        if( AS_FALSE == result )
        {
            break ;
        }        

        if( QUEUE_MODE_NOWAIT == mode )    
        {
            unlock();
            return AS_ERROR_CODE_QUE_EMPTY ;
        }
        
        result = popWait(timeout);
        if( AS_ERROR_CODE_OK != result )
        {
            unlock();
            return result ;
        }
    }

    ev = p_Queue.front();    
    p_Queue.pop();    
    unlock();

    (void)notifyWrite();


    return result ;
}

template <class Type>
int32_t  CSyncQueue<Type>::pushBackEv(Type *ev, int32_t timeout,int32_t mode)
{
    int32_t result = AS_ERROR_CODE_OK;   
    int32_t queulen = 0 ;    

    lock();
    queulen = p_Queue.size();        
    if( queulen >= m_maxQueueLen )    
    {
        if( QUEUE_MODE_NOWAIT == mode )
        {
            unlock();
            return AS_ERROR_CODE_QUE_LEN;
        }
        
        result = pushWait(timeout);
        if( AS_ERROR_CODE_OK != result )           
        {
            unlock();
            return AS_ERROR_CODE_SYS;
        }
    }

    p_Queue.push(ev);
    unlock();

   (void)notifyRead();

    return result ;
}

#endif /* __AS_SYNC_QUEUE_INCLUDE__ */
