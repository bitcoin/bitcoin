// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_STDMUTEX_H
#define BITCOIN_UTIL_STDMUTEX_H

// This header declares threading primitives compatible with Clang
// Thread Safety Analysis and provides appropriate annotation macros.
#include <threadsafety.h> // IWYU pragma: export

#include <util/macros.h>

#include <mutex>

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

    // StdMutex::Guard provides an annotated version of std::lock_guard for us.
    class SCOPED_LOCKABLE Guard : public std::lock_guard<StdMutex>
    {
    public:
        explicit Guard(StdMutex& cs) EXCLUSIVE_LOCK_FUNCTION(cs) : std::lock_guard<StdMutex>(cs) {}
        ~Guard() UNLOCK_FUNCTION() = default;
    };

    static inline StdMutex& CheckNotHeld(StdMutex& cs) EXCLUSIVE_LOCKS_REQUIRED(!cs) LOCK_RETURNED(cs) { return cs; }
};

// Provide STDLOCK(..) wrapper around StdMutex::Guard that checks the lock is not already held
#define STDLOCK(cs) StdMutex::Guard UNIQUE_NAME(criticalblock){StdMutex::CheckNotHeld(cs)}

using StdLockGuard = StdMutex::Guard; // TODO: remove, provided for backwards compat only

#endif // BITCOIN_UTIL_STDMUTEX_H
