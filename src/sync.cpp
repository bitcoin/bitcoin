// Copyright (c) 2011-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sync.h"
#include "util.h"

#include <boost/foreach.hpp>

#ifdef DEBUG_LOCKCONTENTION
void PrintLockContention(const char* pszName, const char* pszFile, int nLine)
{
    printf("LOCKCONTENTION: %s\n", pszName);
    printf("Locker: %s:%d\n", pszFile, nLine);
}
#endif /* DEBUG_LOCKCONTENTION */

#ifdef DEBUG_LOCKORDER
//
// Early deadlock detection.
// Problem being solved:
//    Thread 1 locks  A, then B, then C
//    Thread 2 locks  D, then C, then A
//     --> may result in deadlock between the two threads, depending on when they run.
// Solution implemented here:
// Keep track of pairs of locks: (A before B), (A before C), etc.
// Complain if any thread tries to lock in a different order.
//

struct CLockLocation
{
    CLockLocation(const char* pszName, const char* pszFile, int nLine)
    {
        mutexName = pszName;
        sourceFile = pszFile;
        sourceLine = nLine;
    }

    std::string ToString() const
    {
        return mutexName+"  "+sourceFile+":"+itostr(sourceLine);
    }

private:
    std::string mutexName;
    std::string sourceFile;
    int sourceLine;
};

typedef std::vector< std::pair<void*, CLockLocation> > LockStack;

static boost::mutex dd_mutex;
static std::map<std::pair<void*, void*>, LockStack> lockorders;
static boost::thread_specific_ptr<LockStack> lockstack;


static void potential_deadlock_detected(const std::pair<void*, void*>& mismatch, const LockStack& s1, const LockStack& s2)
{
    printf("POTENTIAL DEADLOCK DETECTED\n");
    printf("Previous lock order was:\n");
    BOOST_FOREACH(const PAIRTYPE(void*, CLockLocation)& i, s2)
    {
        if (i.first == mismatch.first) printf(" (1)");
        if (i.first == mismatch.second) printf(" (2)");
        printf(" %s\n", i.second.ToString().c_str());
    }
    printf("Current lock order is:\n");
    BOOST_FOREACH(const PAIRTYPE(void*, CLockLocation)& i, s1)
    {
        if (i.first == mismatch.first) printf(" (1)");
        if (i.first == mismatch.second) printf(" (2)");
        printf(" %s\n", i.second.ToString().c_str());
    }
}

static void push_lock(void* c, const CLockLocation& locklocation, bool fTry)
{
    if (lockstack.get() == NULL)
        lockstack.reset(new LockStack);

    if (fDebug) printf("Locking: %s\n", locklocation.ToString().c_str());
    dd_mutex.lock();

    (*lockstack).push_back(std::make_pair(c, locklocation));

    if (!fTry) {
        BOOST_FOREACH(const PAIRTYPE(void*, CLockLocation)& i, (*lockstack)) {
            if (i.first == c) break;

            std::pair<void*, void*> p1 = std::make_pair(i.first, c);
            if (lockorders.count(p1))
                continue;
            lockorders[p1] = (*lockstack);

            std::pair<void*, void*> p2 = std::make_pair(c, i.first);
            if (lockorders.count(p2))
            {
                potential_deadlock_detected(p1, lockorders[p2], lockorders[p1]);
                break;
            }
        }
    }
    dd_mutex.unlock();
}

static void pop_lock()
{
    if (fDebug) 
    {
        const CLockLocation& locklocation = (*lockstack).rbegin()->second;
        printf("Unlocked: %s\n", locklocation.ToString().c_str());
    }
    dd_mutex.lock();
    (*lockstack).pop_back();
    dd_mutex.unlock();
}

void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry)
{
    push_lock(cs, CLockLocation(pszName, pszFile, nLine), fTry);
}

void LeaveCritical()
{
    pop_lock();
}

#endif /* DEBUG_LOCKORDER */


static void do_nothing (int* pBool) {}

CCriticalSection::CCriticalSection() : nHasExclusive(&do_nothing), 
nHasUpgrade(&do_nothing), nHadUpgrade(&do_nothing), 
nHasShared(&do_nothing), nHadShared(&do_nothing) {}

void CCriticalSection::lock()
{
    if (nHasExclusive.get() > (int*)0)
    {
        nHasExclusive.reset(nHasExclusive.get() + 1);
        return;
    }

    if (nHasShared.get() > (int*)0)
    {
        mutex.unlock_shared();
        mutex.lock();

        nHadShared.reset(nHasShared.get());
        nHasShared.reset((int*) 0);
    }
    else if (nHasUpgrade.get() > (int*)0)
    {
        mutex.unlock_upgrade_and_lock();

        nHadUpgrade.reset(nHasUpgrade.get());
        nHasUpgrade.reset((int*) 0);
    }
    else
        mutex.lock();

    nHasExclusive.reset((int*) 1);
}

bool CCriticalSection::try_lock()
{
    if (nHasExclusive.get() > (int*)0)
    {
        nHasExclusive.reset(nHasExclusive.get() + 1);
        return true;
    }

    if (nHasShared.get() > (int*)0)
    {
        mutex.unlock_shared();
        if (mutex.try_lock())
        {
            nHadShared.reset(nHasShared.get());
            nHasShared.reset((int*) 0);
            nHasExclusive.reset((int*) 1);
            return true;
        }
        else
        {
            mutex.lock_shared();
            return false;
        }
    }
    else if (nHasUpgrade.get() > (int*)0)
    {
        mutex.unlock_upgrade();
        if (mutex.try_lock())
        {
            nHadUpgrade.reset(nHasUpgrade.get());
            nHasUpgrade.reset((int*) 0);
            nHasExclusive.reset((int*) 1);
            return true;
        }
        else
        {
            mutex.lock_upgrade();
            return false;
        }
    }
    else
    {
        if (mutex.try_lock())
        {
            nHasExclusive.reset((int*) 1);
            return true;
        }
        else
            return false;
    }
}

void CCriticalSection::unlock()
{
    if (nHasExclusive.get() == (int*)0)
        return;
    else if (nHasExclusive.get() > (int*)1)
    {
        nHasExclusive.reset(nHasExclusive.get() - 1);
        return;
    }

    if (nHadUpgrade.get() > (int*)0)
    {
        mutex.unlock_and_lock_upgrade();

        nHasUpgrade.reset(nHadUpgrade.get());
        nHadUpgrade.reset((int*) 0);
    }
    else if (nHadShared.get() > (int*)0)
    {
        mutex.unlock();
        mutex.lock_shared();

        nHasShared.reset(nHadShared.get());
        nHadShared.reset((int*) 0);
    }
    else
        mutex.unlock();

    nHasExclusive.reset((int*) 0);
}

void CCriticalSection::lock_upgrade()
{
    if (nHasExclusive.get() > (int*)0)
    {
        nHadUpgrade.reset(nHadUpgrade.get() + 1);
        return;
    }
    else if (nHasUpgrade.get() > (int*)0)
    {
        nHasUpgrade.reset(nHasUpgrade.get() + 1);
        return;
    }

    if (nHasShared.get() > (int*)0)
    {
        mutex.unlock_shared();
        mutex.lock_upgrade();

        nHadShared.reset(nHasShared.get());
        nHasShared.reset((int*) 0);
    }
    else
        mutex.lock_upgrade();

    nHasUpgrade.reset((int*) 1);
}

void CCriticalSection::unlock_upgrade()
{
    if (nHasExclusive.get() > (int*)0)
    {
        if (nHadUpgrade.get() > (int*)0)
            nHadUpgrade.reset(nHadUpgrade.get() - 1);
        return;
    }
    else if (nHasUpgrade.get() > (int*)1)
    {
        nHasUpgrade.reset(nHasUpgrade.get() - 1);
        return;
    }

    if (nHadShared.get() > (int*)0)
    {
        mutex.unlock_upgrade_and_lock_shared();

        nHasShared.reset(nHadShared.get());
        nHadShared.reset((int*) 0);
    }
    else
        mutex.unlock_upgrade();

    nHasUpgrade.reset((int*) 0);
}

void CCriticalSection::lock_shared()
{
    if (nHasExclusive.get() > (int*)0 || nHasUpgrade.get() > (int*)0)
    {
        nHadShared.reset(nHadShared.get() + 1);
        return;
    }
    else if (nHasShared.get() > (int*)0)
    {
        nHasShared.reset(nHasShared.get() + 1);
        return;
    }

    mutex.lock_shared();
    nHasShared.reset((int*) 1);
}

bool CCriticalSection::try_lock_shared()
{
    if (nHasExclusive.get() > (int*)0 || nHasUpgrade.get() > (int*)0)
    {
        nHadShared.reset(nHadShared.get() + 1);
        return true;
    }
    else if (nHasShared.get() > (int*)0)
    {
        nHasShared.reset(nHasShared.get() + 1);
        return true;
    }

    if (mutex.try_lock_shared())
    {
        nHasShared.reset((int*) 1);
        return true;
    }
    else
        return false;
}

void CCriticalSection::unlock_shared()
{
    if (nHasExclusive.get() > (int*)0 || nHasUpgrade.get() > (int*)0)
    {
        if (nHadShared.get() > (int*)0)
            nHadShared.reset(nHadShared.get() - 1);
        return;
    }
    else if (nHasShared.get() > (int*)1)
    {
        nHasShared.reset(nHasShared.get() - 1);
        return;
    }

    mutex.unlock_shared();
    nHasShared.reset((int*) 0);
}




void CCriticalBlock::Enter(const char* pszName, const char* pszFile, int nLine)
{
    if (!fOwnsLock)
    {
        EnterCritical(pszName, pszFile, nLine, (void*)pmutex);
#ifdef DEBUG_LOCKCONTENTION
        bool fLocked;
        switch (lockType)
        {
        case UNIQUE:
            fLocked = pmutex->try_lock();
            break;
        case UPGRADE:
            pmutex->lock_upgrade();
            fLocked = true;
            break;
        case SHARED:
            fLocked = pmutex->try_lock_shared();
            break;
        }
        if (!fLocked)
        {
            PrintLockContention(pszName, pszFile, nLine);
#endif
        switch (lockType)
        {
        case UNIQUE:
            pmutex->lock();
            break;
        case UPGRADE:
            pmutex->lock_upgrade();
            break;
        case SHARED:
            pmutex->lock_shared();
            break;
        }
#ifdef DEBUG_LOCKCONTENTION
        }
#endif
        fOwnsLock = true;
    }
}

void CCriticalBlock::Leave()
{
    if (fOwnsLock)
    {
        switch (lockType)
        {
        case UNIQUE:
            pmutex->unlock();
            break;
        case UPGRADE:
            pmutex->unlock_upgrade();
            break;
        case SHARED:
            pmutex->unlock_shared();
            break;
        }
        LeaveCritical();
    }
}

bool CCriticalBlock::TryEnter(const char* pszName, const char* pszFile, int nLine)
{
    if (!fOwnsLock)
    {
        EnterCritical(pszName, pszFile, nLine, (void*)pmutex, true);
        switch (lockType)
        {
        case UNIQUE:
            fOwnsLock = pmutex->try_lock();
            break;
        case UPGRADE:
            pmutex->lock_upgrade();
            fOwnsLock = true;
            break;
        case SHARED:
            fOwnsLock = pmutex->try_lock_shared();
            break;
        }
        if (!fOwnsLock)
            LeaveCritical();
    }
    return fOwnsLock;
}

