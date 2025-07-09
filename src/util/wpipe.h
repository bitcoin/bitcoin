// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_WPIPE_H
#define BITCOIN_UTIL_WPIPE_H

#include <array>
#include <assert.h>
#include <atomic>

#ifndef WIN32
#define USE_WAKEUP_PIPE
#endif

class EdgeTriggeredEvents;

/**
 * A manager for abstracting logic surrounding wakeup pipes. Supported only on
 * platforms with a POSIX API. Disabled on Windows.
 */
class WakeupPipe
{
private:
    /* Iterate through m_pipe and ::close() them */
    void Close();

public:
    explicit WakeupPipe(EdgeTriggeredEvents* edge_trig_events);
    ~WakeupPipe();

    bool IsValid() const { return m_valid; };

    /* Drain pipe of all contents */
    void Drain() const;
    /* Write a byte to the pipe */
    void Write();

    /* Used to wrap calls around m_need_wakeup toggling */
    template <typename Callable>
    void Toggle(Callable&& func)
    {
        assert(m_valid);

        m_need_wakeup = true;
        func();
        m_need_wakeup = false;
    }

public:
    /* File descriptors for read and write data channels */
    std::array<int, 2> m_pipe{{ -1, -1 }};
    /* Flag used to determine if Write() needs to be called. Used occasionally */
    std::atomic<bool> m_need_wakeup{false};

private:
    /* Instance validity flag set during construction */
    bool m_valid{false};
    /* Pointer to EdgeTriggeredEvents instance used for pipe (de)registration if using supported events modes */
    EdgeTriggeredEvents* m_edge_trig_events{nullptr};
};

#endif // BITCOIN_UTIL_WPIPE_H
