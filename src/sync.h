// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_SYNC_H
#define BITCOIN_SYNC_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/tss.hpp>




/** Extend boost::shared_mutex to have recursive support */
class CCriticalSection
{
    boost::thread_specific_ptr<int> nHasExclusive;
    boost::thread_specific_ptr<int> nHasUpgrade;
    boost::thread_specific_ptr<int> nHadUpgrade;
    boost::thread_specific_ptr<int> nHasShared;
    boost::thread_specific_ptr<int> nHadShared;
    boost::shared_mutex mutex;
public:
    CCriticalSection();

    void lock();
    bool try_lock();
    void unlock();

    void lock_upgrade();
    void unlock_upgrade();

    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();
};

/** RAII wrapper around CCriticalSection */
class CCriticalBlock
{
public:
    enum LockType { UNIQUE, UPGRADE, SHARED };
private:
    CCriticalSection* pmutex;
    bool fOwnsLock;
    LockType lockType;
public:
    void Enter(const char* pszName, const char* pszFile, int nLine);
    void Leave();
    bool TryEnter(const char* pszName, const char* pszFile, int nLine);

    CCriticalBlock(CCriticalSection& mutexIn, const char* pszName, const char* pszFile, int nLine, bool fTry = false, LockType lockTypeIn = UNIQUE)
        : pmutex(&mutexIn), fOwnsLock(false), lockType(lockTypeIn)
    {
        if (fTry)
            TryEnter(pszName, pszFile, nLine);
        else
            Enter(pszName, pszFile, nLine);
    }

    ~CCriticalBlock()
    {
        if (fOwnsLock)
            Leave();
    }

    operator bool()
    {
        return fOwnsLock;
    }
};

#define LOCK(cs) CCriticalBlock criticalblock(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1,cs2) CCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__),criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs,name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, true)

#define SHARED_LOCK(cs) CCriticalBlock shared_criticalblock(cs, #cs, __FILE__, __LINE__, false, CCriticalBlock::SHARED)
#define TRY_SHARED_LOCK(cs,name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, true, CCriticalBlock::SHARED)

#define UPGRADE_LOCK(cs) CCriticalBlock shared_criticalblock(cs, #cs, __FILE__, __LINE__, false, CCriticalBlock::UPGRADE)

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false);
void LeaveCritical();
#else
void static inline EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false) {}
void static inline LeaveCritical() {}
#endif

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

