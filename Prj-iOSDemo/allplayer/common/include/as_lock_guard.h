
#ifndef CLOCKGUARD_H_INCLUDE
#define CLOCKGUARD_H_INCLUDE    

extern "C"{
#include  "as_mutex.h"
}
class as_lock_guard
{
  public:
    as_lock_guard(as_mutex_t *pMutex);
    virtual ~as_lock_guard();
    
  public:
    static void lock(as_mutex_t *pMutex);
    static void unlock(as_mutex_t *pMutex);
    
 private:
    as_mutex_t *m_pMutex;
};

#endif // CLOCKGUARD_H_INCLUDE


