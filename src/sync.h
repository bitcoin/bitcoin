// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SYNC_H
#define BITCOIN_SYNC_H

#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/lock_options.hpp>




/** Wrapped boost mutex: supports recursive locking, but no waiting  */
typedef boost::interprocess::interprocess_recursive_mutex CCriticalSection;

/** Wrapped boost mutex: supports waiting but not recursive locking */
typedef boost::interprocess::interprocess_mutex CWaitableCriticalSection;

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false);
void LeaveCritical();
#else
void static inline EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false) {}
void static inline LeaveCritical() {}
#endif

/** Wrapper around boost::interprocess::scoped_lock */
template<typename Mutex>
class CMutexLock
{
private:
    boost::interprocess::scoped_lock<Mutex> lock;
public:

    void Enter(const char* pszName, const char* pszFile, int nLine)
    {
        if (!lock.owns())
        {
            EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()));
#ifdef DEBUG_LOCKCONTENTION
            if (!lock.try_lock())
            {
                printf("LOCKCONTENTION: %s\n", pszName);
                printf("Locker: %s:%d\n", pszFile, nLine);
#endif
            lock.lock();
#ifdef DEBUG_LOCKCONTENTION
            }
#endif
        }
    }

    void Leave()
    {
        if (lock.owns())
        {
            lock.unlock();
            LeaveCritical();
        }
    }

    bool TryEnter(const char* pszName, const char* pszFile, int nLine)
    {
        if (!lock.owns())
        {
            EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()), true);
            lock.try_lock();
            if (!lock.owns())
                LeaveCritical();
        }
        return lock.owns();
    }

    CMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false) : lock(mutexIn, boost::interprocess::defer_lock)
    {
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }

    ~CMutexLock()
    {
        if (lock.owns())
            LeaveCritical();
    }

    operator bool()
    {
        return lock.owns();
    }

    boost::interprocess::scoped_lock<Mutex> &GetLock()
    {
        return lock;
    }
};

typedef CMutexLock<CCriticalSection> CCriticalBlock;

#define LOCK(cs) CCriticalBlock criticalblock(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1,cs2) CCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__),criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs,name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, true)

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

#ifdef MAC_OSX
// boost::interprocess::interprocess_semaphore seems to spinlock on OSX; prefer polling instead
class CSemaphore
{
private:
    CCriticalSection cs;
    int val;

public:
    CSemaphore(int init) : val(init) {}

    void wait() {
        do {
            {
                LOCK(cs);
                if (val>0) {
                    val--;
                    return;
                }
            }
            Sleep(100);
        } while(1);
    }

    bool try_wait() {
        LOCK(cs);
        if (val>0) {
            val--;
            return true;
        }
        return false;
    }

    void post() {
        LOCK(cs);
        val++;
    }
};
#else
typedef boost::interprocess::interprocess_semaphore CSemaphore;
#endif

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

