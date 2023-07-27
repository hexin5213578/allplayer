#ifndef SVS_COMMON_H_INCLUDE
#define SVS_COMMON_H_INCLUDE

#include <stdlib.h>

template<class T>
T* AS_NEW(T* &m, uint32_t ulMuili = 0)
{
    try
    {
        if (ulMuili == 0)
        {
            m = new T;
        }
        else
        {
            m = new T[ulMuili];
        }
        return m;
    }
    catch(...)
    {
        m = NULL;
        return NULL;
    }
};

enum DELETE_MULTI
{
    SINGLE = 0,
    MULTI = 1
};

template<class T>
void AS_DELETE(T* &m, uint32_t ulMuili = 0)
{
    if(NULL == m)
    {
        return;
    }

    try
    {
        if (0 == ulMuili)
        {
            delete m;
        }
        else
        {
            delete[] m;
        }
    }
    catch(...)
    {
    }

    m = NULL;
};

template<class TBASE, class TREAL>
TBASE* AS_NEW_REAL(TBASE* &m)
{
    try
    {
        m = new TREAL;
    }
    catch(...)
    {
        m = NULL;
    }

    return m;
};


template<class TBASE, class TREAL>
void AS_DELETE_REAL(TBASE* &m)
{
    try
    {
        TREAL* p = (TREAL*)m; 
        delete p;
    }
    catch(...)
    {
        m = NULL;
    }

    m = NULL;
};


template<class T>
T* AS_CAST(void* pVoid)
{
    T* pReal = NULL;

    try
    {
        pReal = dynamic_cast<T*>((T*)pVoid);
    }
    catch(...)
    {
        pReal = NULL;
    }

    return pReal;
};

#endif //SVS_COMMON_H_INCLUDE
