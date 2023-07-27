
#include "as_lock_guard.h"

as_lock_guard::as_lock_guard(as_mutex_t *pMutex)
{
    m_pMutex = NULL;

    if(NULL == pMutex)
    {
        return;
    }

    m_pMutex = pMutex;

    (void)as_mutex_lock(m_pMutex);
}

as_lock_guard::~as_lock_guard()
{
    if(NULL == m_pMutex)
    {
        return;
    }
    (void)as_mutex_unlock(m_pMutex);

    m_pMutex = NULL;
}


void as_lock_guard::lock(as_mutex_t *pMutex)
{
    if(NULL == pMutex)
    {
        return;
    }
    (void)as_mutex_lock(pMutex);
}

void as_lock_guard::unlock(as_mutex_t *pMutex)
{
    if(NULL == pMutex)
    {
        return;
    }
    (void)as_mutex_unlock(pMutex);
}


