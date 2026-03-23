// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_STDMUTEX_H
#define BITCOIN_UTIL_STDMUTEX_H

// This header declares threading primitives compatible with Clang
// Thread Safety Analysis and provides appropriate annotation macros.
#include <threadsafety.h> // IWYU pragma: export

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
};

// StdLockGuard provides an annotated version of std::lock_guard for us,
// and should only be used when sync.h Mutex/LOCK/etc are not usable.
class SCOPED_LOCKABLE StdLockGuard : public std::lock_guard<StdMutex>
{
public:
    explicit StdLockGuard(StdMutex& cs) EXCLUSIVE_LOCK_FUNCTION(cs) : std::lock_guard<StdMutex>(cs) {}
    ~StdLockGuard() UNLOCK_FUNCTION() = default;
};

#endif // BITCOIN_UTIL_STDMUTEX_H
