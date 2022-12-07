//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <atomic>
#include <thread>

// This has been shamelessly copied from boost...
#if defined(_MSC_VER) && _MSC_VER >= 1310 &&                                   \
    (defined(_M_IX86) || defined(_M_X64)) && !defined(__c2__)
extern "C" void _mm_pause();
#define IMMER_SMT_PAUSE _mm_pause()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define IMMER_SMT_PAUSE __asm__ __volatile__("rep; nop" : : : "memory")
#endif

namespace immer {

// This is an atomic spinlock similar to the one used by boost to provide
// "atomic" shared_ptr operations.  It also does not differ much from the one
// from libc++ or libstdc++...
struct spinlock_policy
{
    std::atomic_flag v_{};

    bool try_lock() { return !v_.test_and_set(std::memory_order_acquire); }

    void lock()
    {
        for (auto k = 0u; !try_lock(); ++k) {
            if (k < 4)
                continue;
#ifdef IMMER_SMT_PAUSE
            else if (k < 16)
                IMMER_SMT_PAUSE;
#endif
            else
                std::this_thread::yield();
        }
    }

    void unlock() { v_.clear(std::memory_order_release); }

    struct scoped_lock
    {
        scoped_lock(const scoped_lock&) = delete;
        scoped_lock& operator=(const scoped_lock&) = delete;

        explicit scoped_lock(spinlock_policy& sp)
            : sp_{sp}
        {
            sp.lock();
        }

        ~scoped_lock() { sp_.unlock(); }

    private:
        spinlock_policy& sp_;
    };
};

} // namespace immer
