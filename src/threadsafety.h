// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADSAFETY_H
#define BITCOIN_THREADSAFETY_H

#include <mutex>

#ifdef __clang__
// TL;DR Add GUARDED_BY(mutex) to member variables. The others are
// rarely necessary. Ex: int nFoo GUARDED_BY(cs_foo);
//
// See http://clang.llvm.org/docs/LanguageExtensions.html#threadsafety
// for documentation.  The clang compiler can do advanced static analysis
// of locking when given the -Wthread-safety option.
#define LOCKABLE __attribute__((lockable))
#define SCOPED_LOCKABLE __attribute__((scoped_lockable))
#define GUARDED_BY(x) __attribute__((guarded_by(x)))
#define GUARDED_VAR __attribute__((guarded_var))
#define PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))
#define PT_GUARDED_VAR __attribute__((pt_guarded_var))
#define ACQUIRED_AFTER(...) __attribute__((acquired_after(__VA_ARGS__)))
#define ACQUIRED_BEFORE(...) __attribute__((acquired_before(__VA_ARGS__)))
#define EXCLUSIVE_LOCK_FUNCTION(...) __attribute__((exclusive_lock_function(__VA_ARGS__)))
#define SHARED_LOCK_FUNCTION(...) __attribute__((shared_lock_function(__VA_ARGS__)))
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) __attribute__((exclusive_trylock_function(__VA_ARGS__)))
#define SHARED_TRYLOCK_FUNCTION(...) __attribute__((shared_trylock_function(__VA_ARGS__)))
#define UNLOCK_FUNCTION(...) __attribute__((unlock_function(__VA_ARGS__)))
#define LOCK_RETURNED(x) __attribute__((lock_returned(x)))
#define LOCKS_EXCLUDED(...) __attribute__((locks_excluded(__VA_ARGS__)))
#define EXCLUSIVE_LOCKS_REQUIRED(...) __attribute__((exclusive_locks_required(__VA_ARGS__)))
#define SHARED_LOCKS_REQUIRED(...) __attribute__((shared_locks_required(__VA_ARGS__)))
#define NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#define ASSERT_EXCLUSIVE_LOCK(...) __attribute((assert_exclusive_lock(__VA_ARGS__)))
#else
#define LOCKABLE
#define SCOPED_LOCKABLE
#define GUARDED_BY(x)
#define GUARDED_VAR
#define PT_GUARDED_BY(x)
#define PT_GUARDED_VAR
#define ACQUIRED_AFTER(...)
#define ACQUIRED_BEFORE(...)
#define EXCLUSIVE_LOCK_FUNCTION(...)
#define SHARED_LOCK_FUNCTION(...)
#define EXCLUSIVE_TRYLOCK_FUNCTION(...)
#define SHARED_TRYLOCK_FUNCTION(...)
#define UNLOCK_FUNCTION(...)
#define LOCK_RETURNED(x)
#define LOCKS_EXCLUDED(...)
#define EXCLUSIVE_LOCKS_REQUIRED(...)
#define SHARED_LOCKS_REQUIRED(...)
#define NO_THREAD_SAFETY_ANALYSIS
#define ASSERT_EXCLUSIVE_LOCK(...)
#endif // __GNUC__

// StdMutex provides an annotated version of std::mutex for us,
// and should only be used when sync.h Mutex/LOCK/etc are not usable.
class LOCKABLE StdMutex : public std::mutex
{
public:
#ifdef __clang__
    //! For negative capabilities in the Clang Thread Safety Analysis.
    //! A negative requirement uses the EXCLUSIVE_LOCKS_REQUIRED attribute, in conjunction
    //! with the ! operator, to indicate that a mutex should not be held.
    const StdMutex& operator!() const { return *this; }
#endif // __clang__
};

// StdLockGuard provides an annotated version of std::lock_guard for us,
// and should only be used when sync.h Mutex/LOCK/etc are not usable.
class SCOPED_LOCKABLE StdLockGuard : public std::lock_guard<StdMutex>
{
public:
    explicit StdLockGuard(StdMutex& cs) EXCLUSIVE_LOCK_FUNCTION(cs) : std::lock_guard<StdMutex>(cs) {}
    ~StdLockGuard() UNLOCK_FUNCTION() {}
};

#endif // BITCOIN_THREADSAFETY_H
