// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sync.h"

#include "util.h"
#include "utilstrencodings.h"

#include <stdio.h>

#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#ifdef DEBUG_LOCKCONTENTION
void PrintLockContention(const char *pszName, const char *pszFile, int nLine)
{
    LogPrintf("LOCKCONTENTION: %s\n", pszName);
    LogPrintf("Locker: %s:%d\n", pszFile, nLine);
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

// BU move to sync.h because I need to create these in globals.cpp
// struct CLockLocation {
CLockLocation::CLockLocation(const char *pszName, const char *pszFile, int nLine, bool fTryIn)
{
    mutexName = pszName;
    sourceFile = pszFile;
    sourceLine = nLine;
    fTry = fTryIn;
}

std::string CLockLocation::ToString() const
{
    return mutexName + "  " + sourceFile + ":" + itostr(sourceLine) + (fTry ? " (TRY)" : "");
}

std::string CLockLocation::MutexName() const { return mutexName; }
#if 0
    bool fTry;
private:
    std::string mutexName;
    std::string sourceFile;
    int sourceLine;
};
#endif

// BU move to .h: typedef std::vector<std::pair<void*, CLockLocation> > LockStack;

// BU control the ctor/dtor ordering
extern boost::mutex dd_mutex;
extern LockStackMap lockorders;
extern boost::thread_specific_ptr<LockStack> lockstack;


static void potential_deadlock_detected(const std::pair<void *, void *> &mismatch,
    const LockStack &s1,
    const LockStack &s2)
{
    // We attempt to not assert on probably-not deadlocks by assuming that
    // a try lock will immediately have otherwise bailed if it had
    // failed to get the lock
    // We do this by, for the locks which triggered the potential deadlock,
    // in either lockorder, checking that the second of the two which is locked
    // is only a TRY_LOCK, ignoring locks if they are reentrant.
    bool firstLocked = false;
    bool secondLocked = false;
    bool onlyMaybeDeadlock = false;

    LogPrintf("POTENTIAL DEADLOCK DETECTED\n");
    LogPrintf("Previous lock order was:\n");
    BOOST_FOREACH (const PAIRTYPE(void *, CLockLocation) & i, s2)
    {
        if (i.first == mismatch.first)
        {
            LogPrintf(" (1)");
            if (!firstLocked && secondLocked && i.second.fTry)
                onlyMaybeDeadlock = true;
            firstLocked = true;
        }
        if (i.first == mismatch.second)
        {
            LogPrintf(" (2)");
            if (!secondLocked && firstLocked && i.second.fTry)
                onlyMaybeDeadlock = true;
            secondLocked = true;
        }
        LogPrintf(" %s\n", i.second.ToString());
    }
    firstLocked = false;
    secondLocked = false;
    LogPrintf("Current lock order is:\n");
    BOOST_FOREACH (const PAIRTYPE(void *, CLockLocation) & i, s1)
    {
        if (i.first == mismatch.first)
        {
            LogPrintf(" (1)");
            if (!firstLocked && secondLocked && i.second.fTry)
                onlyMaybeDeadlock = true;
            firstLocked = true;
        }
        if (i.first == mismatch.second)
        {
            LogPrintf(" (2)");
            if (!secondLocked && firstLocked && i.second.fTry)
                onlyMaybeDeadlock = true;
            secondLocked = true;
        }
        LogPrintf(" %s\n", i.second.ToString());
    }
    assert(onlyMaybeDeadlock);
}

static void push_lock(void *c, const CLockLocation &locklocation, bool fTry)
{
    if (lockstack.get() == NULL)
        lockstack.reset(new LockStack);

    dd_mutex.lock();

    (*lockstack).push_back(std::make_pair(c, locklocation));
    // If this is a blocking lock operation, we want to make sure that the locking order between 2 mutexes is consistent
    // across the program
    if (!fTry)
    {
        BOOST_FOREACH (const PAIRTYPE(void *, CLockLocation) & i, (*lockstack))
        {
            if (i.first == c)
                break;

            std::pair<void *, void *> p1 = std::make_pair(i.first, c);
            // If this order has already been placed into the order map, we've already tested it
            if (lockorders.count(p1))
                continue;
            lockorders[p1] = (*lockstack);
            // check to see if the opposite order has ever occurred, if so flag a possible deadlock
            std::pair<void *, void *> p2 = std::make_pair(c, i.first);
            if (lockorders.count(p2))
                potential_deadlock_detected(p1, lockorders[p1], lockorders[p2]);
        }
    }
    dd_mutex.unlock();
}

static void pop_lock()
{
    dd_mutex.lock();
    (*lockstack).pop_back();
    dd_mutex.unlock();
}

void EnterCritical(const char *pszName, const char *pszFile, int nLine, void *cs, bool fTry)
{
    push_lock(cs, CLockLocation(pszName, pszFile, nLine, fTry), fTry);
}

void LeaveCritical() { pop_lock(); }
void DeleteCritical(const void *cs)
{
    dd_mutex.lock();
    LockStackMap::iterator prev = lockorders.end();
    for (LockStackMap::iterator i = lockorders.begin(); i != lockorders.end(); ++i)
    {
        // if prev is valid and one of its locks is the one we are deleting, then erase the entry
        if ((prev != lockorders.end()) && ((prev->first.first == cs) || (prev->first.second == cs)))
        {
            lockorders.erase(prev);
        }
        prev = i;
    }
    dd_mutex.unlock();
}


std::string LocksHeld()
{
    std::string result;
    BOOST_FOREACH (const PAIRTYPE(void *, CLockLocation) & i, *lockstack)
        result += i.second.ToString() + std::string("\n");
    return result;
}

void AssertLockHeldInternal(const char *pszName, const char *pszFile, int nLine, void *cs)
{
    BOOST_FOREACH (const PAIRTYPE(void *, CLockLocation) & i, *lockstack)
        if (i.first == cs)
            return;
    fprintf(stderr, "Assertion failed: lock %s not held in %s:%i; locks held:\n%s", pszName, pszFile, nLine,
        LocksHeld().c_str());
    abort();
}

// BU normally CCriticalSection is a typedef, but when lockorder debugging is on we need to delete the critical section
// from the lockorder map
#ifdef DEBUG_LOCKORDER
CCriticalSection::CCriticalSection() : name(NULL) {}
CCriticalSection::CCriticalSection(const char *n) : name(n) {}
CCriticalSection::~CCriticalSection()
{
    if (name)
    {
        printf("Destructing %s\n", name);
        fflush(stdout);
    }
    DeleteCritical((void *)this);
}
#endif

#endif /* DEBUG_LOCKORDER */
