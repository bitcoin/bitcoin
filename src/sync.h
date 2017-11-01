// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SYNC_H
#define BITCOIN_SYNC_H

#include "threadsafety.h"
#include "util.h"
#include "utiltime.h"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>


////////////////////////////////////////////////
//                                            //
// THE SIMPLE DEFINITION, EXCLUDING DEBUG CODE //
//                                            //
////////////////////////////////////////////////

/*
CCriticalSection mutex;
    boost::recursive_mutex mutex;

LOCK(mutex);
    boost::unique_lock<boost::recursive_mutex> criticalblock(mutex);

LOCK2(mutex1, mutex2);
    boost::unique_lock<boost::recursive_mutex> criticalblock1(mutex1);
    boost::unique_lock<boost::recursive_mutex> criticalblock2(mutex2);

TRY_LOCK(mutex, name);
    boost::unique_lock<boost::recursive_mutex> name(mutex, boost::try_to_lock_t);

ENTER_CRITICAL_SECTION(mutex); // no RAII
    mutex.lock();

LEAVE_CRITICAL_SECTION(mutex); // no RAII
    mutex.unlock();
 */

///////////////////////////////
//                           //
// THE ACTUAL IMPLEMENTATION //
//                           //
///////////////////////////////

/**
 * Template mixin that adds -Wthread-safety locking
 * annotations to a subset of the mutex API.
 */
template <typename PARENT>
class LOCKABLE AnnotatedMixin : public PARENT
{
public:
    void lock() EXCLUSIVE_LOCK_FUNCTION() { PARENT::lock(); }
    void unlock() UNLOCK_FUNCTION() { PARENT::unlock(); }
    bool try_lock() EXCLUSIVE_TRYLOCK_FUNCTION(true) { return PARENT::try_lock(); }
};

/**
 * Wrapped boost mutex: supports recursive locking, but no waiting
 * TODO: We should move away from using the recursive lock by default.
 */
#ifndef DEBUG_LOCKORDER
typedef AnnotatedMixin<boost::recursive_mutex> CCriticalSection;
#else // BU we need to remove the critical section from the lockorder map when destructed
class CCriticalSection : public AnnotatedMixin<boost::recursive_mutex>
{
public:
    const char *name;
    CCriticalSection(const char *name);
    CCriticalSection();
    ~CCriticalSection();
};
#endif

/** Wrapped boost mutex: supports waiting but not recursive locking */
typedef AnnotatedMixin<boost::mutex> CWaitableCriticalSection;

/** Just a typedef for boost::condition_variable, can be wrapped later if desired */
typedef boost::condition_variable CConditionVariable;

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char *pszName, const char *pszFile, int nLine, void *cs, bool fTry = false);
void LeaveCritical();
// BU if a CCriticalSection is allocated on the heap we need to clean it from the lockorder map upon destruction because
// another CCriticalSection could be created on top of it.
void DeleteCritical(const void *cs);
std::string LocksHeld();
void AssertLockHeldInternal(const char *pszName, const char *pszFile, int nLine, void *cs);
#else
void static inline EnterCritical(const char *pszName, const char *pszFile, int nLine, void *cs, bool fTry = false) {}
void static inline LeaveCritical() {}
void static inline AssertLockHeldInternal(const char *pszName, const char *pszFile, int nLine, void *cs) {}
#endif
#define AssertLockHeld(cs) AssertLockHeldInternal(#cs, __FILE__, __LINE__, &cs)

#ifdef DEBUG_LOCKCONTENTION
void PrintLockContention(const char *pszName, const char *pszFile, int nLine);
#endif

/** Wrapper around boost::unique_lock<Mutex> */
template <typename Mutex>
class SCOPED_LOCKABLE CMutexLock
{
private:
    boost::unique_lock<Mutex> lock;
    int64_t lockedTime;
    const char *name;
    const char *file;
    int line;

    void Enter(const char *pszName, const char *pszFile, int nLine)
    {
        int64_t startWait = GetTimeMillis();
        name = pszName;
        file = pszFile;
        line = nLine;
        EnterCritical(pszName, pszFile, nLine, (void *)(lock.mutex()));
#ifdef DEBUG_LOCKCONTENTION
        if (!lock.try_lock())
        {
            PrintLockContention(pszName, pszFile, nLine);
#endif
            lock.lock();
#ifdef DEBUG_LOCKCONTENTION
        }
#endif
        lockedTime = GetTimeMillis();
        if (lockedTime - startWait > 500)
        {
            LogPrint("lck", "Lock %s at %s:%d waited for %d ms\n", pszName, pszFile, nLine, (lockedTime - startWait));
        }
    }

    bool TryEnter(const char *pszName, const char *pszFile, int nLine)
    {
        name = pszName;
        file = pszFile;
        line = nLine;
        EnterCritical(pszName, pszFile, nLine, (void *)(lock.mutex()), true);
        lock.try_lock();
        if (!lock.owns_lock())
        {
            lockedTime = 0;
            LeaveCritical();
        }
        else
            lockedTime = GetTimeMillis();
        return lock.owns_lock();
    }

public:
    CMutexLock(Mutex &mutexIn, const char *pszName, const char *pszFile, int nLine, bool fTry = false)
        EXCLUSIVE_LOCK_FUNCTION(mutexIn)
        : lock(mutexIn, boost::defer_lock)
    {
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }

    CMutexLock(Mutex *pmutexIn, const char *pszName, const char *pszFile, int nLine, bool fTry = false)
        EXCLUSIVE_LOCK_FUNCTION(pmutexIn)
    {
        if (!pmutexIn)
            return;

        lock = boost::unique_lock<Mutex>(*pmutexIn, boost::defer_lock);
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }

    ~CMutexLock() UNLOCK_FUNCTION()
    {
        if (lock.owns_lock())
        {
            LeaveCritical();
            int64_t doneTime = GetTimeMillis();
            if (doneTime - lockedTime > 500)
            {
                LogPrint(
                    "lck", "Lock %s at %s:%d remained locked for %d ms\n", name, file, line, doneTime - lockedTime);
            }
        }
    }

    operator bool() { return lock.owns_lock(); }
};

typedef CMutexLock<CCriticalSection> CCriticalBlock;

#define LOCK(cs) CCriticalBlock criticalblock(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1, cs2) \
    CCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__), criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs, name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, true)

#define ENTER_CRITICAL_SECTION(cs)                             \
    {                                                          \
        EnterCritical(#cs, __FILE__, __LINE__, (void *)(&cs)); \
        (cs).lock();                                           \
    }

#define LEAVE_CRITICAL_SECTION(cs) \
    {                              \
        (cs).unlock();             \
        LeaveCritical();           \
    }

class CSemaphore
{
private:
    boost::condition_variable condition;
    boost::mutex mutex;
    int value;

public:
    CSemaphore(int init) : value(init) {}
    void wait()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        while (value < 1)
        {
            condition.wait(lock);
        }
        value--;
    }

    bool try_wait()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        if (value < 1)
            return false;
        value--;
        return true;
    }

    void post()
    {
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
    void Acquire()
    {
        if (fHaveGrant)
            return;
        sem->wait();
        fHaveGrant = true;
    }

    void Release()
    {
        if (!fHaveGrant)
            return;
        sem->post();
        fHaveGrant = false;
    }

    bool TryAcquire()
    {
        if (!fHaveGrant && sem->try_wait())
            fHaveGrant = true;
        return fHaveGrant;
    }

    void MoveTo(CSemaphoreGrant &grant)
    {
        grant.Release();
        grant.sem = sem;
        grant.fHaveGrant = fHaveGrant;
        sem = NULL;
        fHaveGrant = false;
    }

    CSemaphoreGrant() : sem(NULL), fHaveGrant(false) {}
    CSemaphoreGrant(CSemaphore &sema, bool fTry = false) : sem(&sema), fHaveGrant(false)
    {
        if (fTry)
            TryAcquire();
        else
            Acquire();
    }

    ~CSemaphoreGrant() { Release(); }
    operator bool() { return fHaveGrant; }
};

// BU move from sync.c because I need to create these in globals.cpp
struct CLockLocation
{
    CLockLocation(const char *pszName, const char *pszFile, int nLine, bool fTryIn);
    std::string ToString() const;
    std::string MutexName() const;

    bool fTry;

private:
    std::string mutexName;
    std::string sourceFile;
    int sourceLine;
};

typedef std::vector<std::pair<void *, CLockLocation> > LockStack;
typedef std::map<std::pair<void *, void *>, LockStack> LockStackMap;
#endif // BITCOIN_SYNC_H
