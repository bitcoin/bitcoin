// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SEMAPHORE_GRANT_H
#define BITCOIN_SEMAPHORE_GRANT_H

#include <semaphore>

/** RAII-style semaphore lock */
template <std::ptrdiff_t LeastMaxValue = std::counting_semaphore<>::max()>
class CountingSemaphoreGrant
{
private:
    std::counting_semaphore<LeastMaxValue>* sem;
    bool fHaveGrant;

public:
    void Acquire() noexcept
    {
        if (fHaveGrant) {
            return;
        }
        sem->acquire();
        fHaveGrant = true;
    }

    void Release() noexcept
    {
        if (!fHaveGrant) {
            return;
        }
        sem->release();
        fHaveGrant = false;
    }

    bool TryAcquire() noexcept
    {
        if (!fHaveGrant && sem->try_acquire()) {
            fHaveGrant = true;
        }
        return fHaveGrant;
    }

    // Disallow copy.
    CountingSemaphoreGrant(const CountingSemaphoreGrant&) = delete;
    CountingSemaphoreGrant& operator=(const CountingSemaphoreGrant&) = delete;

    // Allow move.
    CountingSemaphoreGrant(CountingSemaphoreGrant&& other) noexcept
    {
        sem = other.sem;
        fHaveGrant = other.fHaveGrant;
        other.fHaveGrant = false;
        other.sem = nullptr;
    }

    CountingSemaphoreGrant& operator=(CountingSemaphoreGrant&& other) noexcept
    {
        Release();
        sem = other.sem;
        fHaveGrant = other.fHaveGrant;
        other.fHaveGrant = false;
        other.sem = nullptr;
        return *this;
    }

    CountingSemaphoreGrant() noexcept : sem(nullptr), fHaveGrant(false) {}

    explicit CountingSemaphoreGrant(std::counting_semaphore<LeastMaxValue>& sema, bool fTry = false) noexcept : sem(&sema), fHaveGrant(false)
    {
        if (fTry) {
            TryAcquire();
        } else {
            Acquire();
        }
    }

    ~CountingSemaphoreGrant()
    {
        Release();
    }

    explicit operator bool() const noexcept
    {
        return fHaveGrant;
    }
};

using BinarySemaphoreGrant = CountingSemaphoreGrant<1>;

#endif // BITCOIN_SEMAPHORE_GRANT_H
