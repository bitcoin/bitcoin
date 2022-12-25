// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/threadinterrupt.h>

#include <sync.h>

CThreadInterrupt::CThreadInterrupt() : flag(false) {}

CThreadInterrupt::operator bool() const
{
    return flag.load(std::memory_order_acquire);
}

void CThreadInterrupt::reset()
{
    flag.store(false, std::memory_order_release);
}

void CThreadInterrupt::operator()()
{
    {
        LOCK(mut);
        flag.store(true, std::memory_order_release);
    }
    cond.notify_all();
}

bool CThreadInterrupt::sleep_for(Clock::duration rel_time)
{
    WAIT_LOCK(mut, lock);
    return !cond.wait_for(lock, rel_time, [this]() { return flag.load(std::memory_order_acquire); });
}
