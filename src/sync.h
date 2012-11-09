// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SYNC_H
#define BITCOIN_SYNC_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include "threadsafety.h"




/** Wrapped boost mutex: supports recursive locking, but no waiting  */

// Recursive mutexes are wrong in 99.999% of normal use-cases.  We
// should try to get away from using them.  The problem is unlock() of
// a recursive mutex.  When unlocking, the invariants of the protected
// data should be established.  In the case where the mutex is
// recursive, this means that the invariant is explicitly established
// while holding the lock.  That is very fragile.  For recursive
// functions that use mutexes, it is better to do a wrapper/worker
// transform where the wrapper does the locking.  It is both faster,
// clearer, and semantically cleaner.
class LOCKABLE CCriticalSection : public boost::recursive_mutex
{
private:
   typedef boost::recursive_mutex parent;
public:
    void lock() EXCLUSIVE_LOCK_FUNCTION()
    {
      parent::lock();
    }

    void unlock() UNLOCK_FUNCTION()
    {
      parent::unlock();
    }

    bool try_lock() EXCLUSIVE_TRYLOCK_FUNCTION(true)
    {
      return parent::try_lock();
    }
};

/** Wrapped boost mutex: supports waiting but not recursive locking */
class LOCKABLE CWaitableCriticalSection : public boost::mutex
{
private:
   typedef boost::mutex parent;
public:
    void lock() EXCLUSIVE_LOCK_FUNCTION()
    {
      parent::lock();
    }

    void unlock() UNLOCK_FUNCTION()
    {
      parent::unlock();
    }

    bool try_lock() EXCLUSIVE_TRYLOCK_FUNCTION(true)
    {
      return parent::try_lock();
    }
};

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false);
void LeaveCritical();
#else
void static inline EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false) {}
void static inline LeaveCritical() {}
#endif

#ifdef DEBUG_LOCKCONTENTION
void PrintLockContention(const char* pszName, const char* pszFile, int nLine);
#endif

/** Wrapper around boost::unique_lock */
template<typename Mutex>
class CMutexLockBase
{
protected:
    boost::unique_lock<Mutex> lock;

public:
    CMutexLockBase(Mutex& mutexIn) : lock(mutexIn, boost::defer_lock) {}
    ~CMutexLockBase() {}
    operator bool() const
{
        return lock.owns_lock();
    }
};

template<typename Mutex>
class SCOPED_LOCKABLE CMutexLock : public CMutexLockBase<Mutex>
{
private:
    void Enter(const char* pszName, const char* pszFile, int nLine)
    {
        assert(!this->lock.owns_lock());
	EnterCritical(pszName, pszFile, nLine, (void*)(this->lock.mutex()));
#ifdef DEBUG_LOCKCONTENTION
	if (!this->lock.try_lock())
        {
	    PrintLockContention(pszName, pszFile, nLine);
#endif
	    this->lock.lock();
#ifdef DEBUG_LOCKCONTENTION
	}
#endif
    }

public:
    CMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine)  EXCLUSIVE_LOCK_FUNCTION(&mutexIn)
      : CMutexLockBase<Mutex>(mutexIn)
    {
        Enter(pszName, pszFile, nLine);
    }

    ~CMutexLock() UNLOCK_FUNCTION()
    {
        LeaveCritical();
    }
};

template<typename Mutex>
class SCOPED_LOCKABLE CMutexTryLock : public CMutexLockBase<Mutex>
{
private:
    void TryEnter(const char* pszName, const char* pszFile, int nLine)
    {
        assert(!this->lock.owns_lock());
        EnterCritical(pszName, pszFile, nLine, (void*)(this->lock.mutex()), true);
        if (!this->lock.try_lock())
            LeaveCritical();
    }

public:
    CMutexTryLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine) 
      : CMutexLockBase<Mutex>(mutexIn)
    {
        TryEnter(pszName, pszFile, nLine);
    }

    ~CMutexTryLock()
    {
        if (this->lock.owns_lock())
            LeaveCritical();
    }
};

typedef CMutexLock<CCriticalSection> CCriticalBlock;
typedef CMutexTryLock<CCriticalSection> CCriticalTryBlock;

#define LOCK(cs) CCriticalBlock criticalblock(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1,cs2) CCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__),criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs,name) CCriticalTryBlock name(cs, #cs, __FILE__, __LINE__)

#define ENTER_CRITICAL_SECTION(cs) \
    { \
        EnterCritical(#cs, __FILE__, __LINE__, (void*)(&cs)); \
        (cs).lock(); \
    }

#define LEAVE_CRITICAL_SECTION(cs) \
    { \
        (cs).unlock(); \
        LeaveCritical(); \
    }

class CSemaphore
{
private:
    boost::condition_variable condition;
    boost::mutex mutex;
    int value;

public:
    CSemaphore(int init) : value(init) {}

    void wait() {
        boost::unique_lock<boost::mutex> lock(mutex);
        while (value < 1) {
            condition.wait(lock);
        }
        value--;
    }

    bool try_wait() {
        boost::unique_lock<boost::mutex> lock(mutex);
        if (value < 1)
            return false;
        value--;
        return true;
    }

    void post() {
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            value++;
        }
        condition.notify_one();
    }
};

/** RAII-style semaphore lock */
class CSemaphoreGrant
{
private:
    CSemaphore *sem;
    bool fHaveGrant;

public:
    void Acquire() {
        if (fHaveGrant)
            return;
        sem->wait();
        fHaveGrant = true;
    }

    void Release() {
        if (!fHaveGrant)
            return;
        sem->post();
        fHaveGrant = false;
    }

    bool TryAcquire() {
        if (!fHaveGrant && sem->try_wait())
            fHaveGrant = true;
        return fHaveGrant;
    }

    void MoveTo(CSemaphoreGrant &grant) {
        grant.Release();
        grant.sem = sem;
        grant.fHaveGrant = fHaveGrant;
        sem = NULL;
        fHaveGrant = false;
    }

    CSemaphoreGrant() : sem(NULL), fHaveGrant(false) {}

    CSemaphoreGrant(CSemaphore &sema, bool fTry = false) : sem(&sema), fHaveGrant(false) {
        if (fTry)
            TryAcquire();
        else
            Acquire();
    }

    ~CSemaphoreGrant() {
        Release();
    }

    operator bool() {
        return fHaveGrant;
    }
};
#endif

