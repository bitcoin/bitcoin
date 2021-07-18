// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADSAFETY_H
#define BITCOIN_THREADSAFETY_H

#include <mutex>

#ifdef __clang__
// TL;DR Add GUARDED_BY(mutex) to member variables. The others are
// rarely necessary. Ex: int nFoo GUARDED_BY(cs_foo);
//
// See https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
// for documentation.  The clang compiler can do advanced static analysis
// of locking when given the -Wthread-safety option.
#define LOCKABLE __attribute__((lockable))
#define SCOPED_LOCKABLE __attribute__((scoped_lockable))
#define GUARDED_BY(x) __attribute__((guarded_by(x)))
#define PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))
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
#define ASSERT_EXCLUSIVE_LOCK(...) __attribute__((assert_exclusive_lock(__VA_ARGS__)))
#else
#define LOCKABLE
#define SCOPED_LOCKABLE
#define GUARDED_BY(x)
#define PT_GUARDED_BY(x)
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

template<typename Mutex>
class LOCKABLE Lockable : public Mutex
{
public:
#ifdef __clang__
    //! For negative capabilities in the Clang Thread Safety Analysis.
    //! A negative requirement uses the EXCLUSIVE_LOCKS_REQUIRED attribute, in conjunction
    //! with the ! operator, to indicate that a mutex should not be held.
    const Lockable& operator!() const { return *this; }
#endif // __clang__
};

template<typename Mutex, template<typename> class Lock>
class SCOPED_LOCKABLE ScopedLockable : public Lock<Mutex>
{
public:
    explicit ScopedLockable(Mutex& mutex) EXCLUSIVE_LOCK_FUNCTION(mutex) : Lock<Mutex>(mutex) {}
    ~ScopedLockable() UNLOCK_FUNCTION() {}
};

// StdMutex, StdLockGuard, and StdUniqueLock provide annotated versions of
// standard synchronization classes, and should only be used when sync.h
// Mutex/LOCK/etc are not usable.
using StdMutex = Lockable<std::mutex>;
using StdLockGuard = ScopedLockable<std::mutex, std::lock_guard>;
using StdUniqueLock = ScopedLockable<std::mutex, std::unique_lock>;

#endif // BITCOIN_THREADSAFETY_H
