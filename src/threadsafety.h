// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADSAFETY_H
#define BITCOIN_THREADSAFETY_H

#include <mutex>

#ifdef __clang__
// TL;DR Add TS_ITCOIN_GUARDED_BY(mutex) to member variables. The others are
// rarely necessary. Ex: int nFoo TS_ITCOIN_GUARDED_BY(cs_foo);
//
// See https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
// for documentation.  The clang compiler can do advanced static analysis
// of locking when given the -Wthread-safety option.
#define TS_ITCOIN_LOCKABLE __attribute__((lockable))
#define TS_ITCOIN_SCOPED_LOCKABLE __attribute__((scoped_lockable))
#define TS_ITCOIN_GUARDED_BY(x) __attribute__((guarded_by(x)))
#define TS_ITCOIN_PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))
#define TS_ITCOIN_ACQUIRED_AFTER(...) __attribute__((acquired_after(__VA_ARGS__)))
#define TS_ITCOIN_ACQUIRED_BEFORE(...) __attribute__((acquired_before(__VA_ARGS__)))
#define TS_ITCOIN_EXCLUSIVE_LOCK_FUNCTION(...) __attribute__((exclusive_lock_function(__VA_ARGS__)))
#define TS_ITCOIN_SHARED_LOCK_FUNCTION(...) __attribute__((shared_lock_function(__VA_ARGS__)))
#define TS_ITCOIN_EXCLUSIVE_TRYLOCK_FUNCTION(...) __attribute__((exclusive_trylock_function(__VA_ARGS__)))
#define TS_ITCOIN_SHARED_TRYLOCK_FUNCTION(...) __attribute__((shared_trylock_function(__VA_ARGS__)))
#define TS_ITCOIN_UNLOCK_FUNCTION(...) __attribute__((unlock_function(__VA_ARGS__)))
#define TS_ITCOIN_LOCK_RETURNED(x) __attribute__((lock_returned(x)))
#define TS_ITCOIN_LOCKS_EXCLUDED(...) __attribute__((locks_excluded(__VA_ARGS__)))
#define TS_ITCOIN_EXCLUSIVE_LOCKS_REQUIRED(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#define TS_ITCOIN_SHARED_LOCKS_REQUIRED(...) __attribute__((shared_locks_required(__VA_ARGS__)))
#define TS_ITCOIN_NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#define TS_ITCOIN_ASSERT_EXCLUSIVE_LOCK(...) __attribute__((assert_exclusive_lock(__VA_ARGS__)))
#else
#define TS_ITCOIN_LOCKABLE
#define TS_ITCOIN_SCOPED_LOCKABLE
#define TS_ITCOIN_GUARDED_BY(x)
#define TS_ITCOIN_PT_GUARDED_BY(x)
#define TS_ITCOIN_ACQUIRED_AFTER(...)
#define TS_ITCOIN_ACQUIRED_BEFORE(...)
#define TS_ITCOIN_EXCLUSIVE_LOCK_FUNCTION(...)
#define TS_ITCOIN_SHARED_LOCK_FUNCTION(...)
#define TS_ITCOIN_EXCLUSIVE_TRYLOCK_FUNCTION(...)
#define TS_ITCOIN_SHARED_TRYLOCK_FUNCTION(...)
#define TS_ITCOIN_UNLOCK_FUNCTION(...)
#define TS_ITCOIN_LOCK_RETURNED(x)
#define TS_ITCOIN_LOCKS_EXCLUDED(...)
#define TS_ITCOIN_EXCLUSIVE_LOCKS_REQUIRED(...)
#define TS_ITCOIN_SHARED_LOCKS_REQUIRED(...)
#define TS_ITCOIN_NO_THREAD_SAFETY_ANALYSIS
#define TS_ITCOIN_ASSERT_EXCLUSIVE_LOCK(...)
#endif // __GNUC__

// StdMutex provides an annotated version of std::mutex for us,
// and should only be used when sync.h Mutex/LOCK/etc are not usable.
class TS_ITCOIN_LOCKABLE StdMutex : public std::mutex
{
public:
#ifdef __clang__
    //! For negative capabilities in the Clang Thread Safety Analysis.
    //! A negative requirement uses the TS_ITCOIN_EXCLUSIVE_LOCKS_REQUIRED attribute, in conjunction
    //! with the ! operator, to indicate that a mutex should not be held.
    const StdMutex& operator!() const { return *this; }
#endif // __clang__
};

// StdLockGuard provides an annotated version of std::lock_guard for us,
// and should only be used when sync.h Mutex/LOCK/etc are not usable.
class TS_ITCOIN_SCOPED_LOCKABLE StdLockGuard : public std::lock_guard<StdMutex>
{
public:
    explicit StdLockGuard(StdMutex& cs) TS_ITCOIN_EXCLUSIVE_LOCK_FUNCTION(cs) : std::lock_guard<StdMutex>(cs) {}
    ~StdLockGuard() TS_ITCOIN_UNLOCK_FUNCTION() {}
};

#endif // BITCOIN_THREADSAFETY_H
