// Copyright (c) 2023 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_UTIL_SIGNALINTERRUPT_H
#define TORTOISECOIN_UTIL_SIGNALINTERRUPT_H

#ifdef WIN32
#include <condition_variable>
#include <mutex>
#else
#include <util/tokenpipe.h>
#endif

#include <atomic>
#include <cstdlib>

namespace util {
/**
 * Helper class that manages an interrupt flag, and allows a thread or
 * signal to interrupt another thread.
 *
 * This class is safe to be used in a signal handler. If sending an interrupt
 * from a signal handler is not necessary, the more lightweight \ref
 * CThreadInterrupt class can be used instead.
 */

class SignalInterrupt
{
public:
    SignalInterrupt();
    explicit operator bool() const;
    [[nodiscard]] bool operator()();
    [[nodiscard]] bool reset();
    [[nodiscard]] bool wait();

private:
    std::atomic<bool> m_flag;

#ifndef WIN32
    // On UNIX-like operating systems use the self-pipe trick.
    TokenPipeEnd m_pipe_r;
    TokenPipeEnd m_pipe_w;
#else
    // On windows use a condition variable, since we don't have any signals there
    std::mutex m_mutex;
    std::condition_variable m_cv;
#endif
};
} // namespace util

#endif // TORTOISECOIN_UTIL_SIGNALINTERRUPT_H
