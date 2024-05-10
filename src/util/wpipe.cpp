// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/wpipe.h>

#include <logging.h>
#include <util/edge.h>
#include <util/sock.h>

WakeupPipe::WakeupPipe(EdgeTriggeredEvents* edge_trig_events)
    : m_edge_trig_events{edge_trig_events}
{
#ifdef USE_WAKEUP_PIPE
    if (pipe(m_pipe.data()) != 0) {
        LogPrintf("Unable to initialize WakeupPipe, pipe() for m_pipe failed\n");
        return;
    }
    for (size_t idx = 0; idx < m_pipe.size(); idx++) {
        int flags = fcntl(m_pipe[idx], F_GETFL, 0);
        if (fcntl(m_pipe[idx], F_SETFL, flags | O_NONBLOCK) == -1) {
            LogPrintf("Unable to initialize WakeupPipe, fcntl for O_NONBLOCK on m_pipe[%d] failed\n", idx);
            return;
        }
    }
    if (edge_trig_events && !edge_trig_events->RegisterPipe(m_pipe[0])) {
        LogPrintf("Unable to initialize WakeupPipe, EdgeTriggeredEvents::RegisterPipe() failed\n");
        return;
    }
    m_valid = true;
#else
    LogPrintf("Attempting to initialize WakeupPipe without support compiled in!\n");
#endif /* USE_WAKEUP_PIPE */
}

WakeupPipe::~WakeupPipe()
{
    if (m_valid) {
#ifdef USE_WAKEUP_PIPE
        if (m_edge_trig_events && !m_edge_trig_events->UnregisterPipe(m_pipe[0])) {
            LogPrintf("Destroying WakeupPipe instance, EdgeTriggeredEvents::UnregisterPipe() failed\n");
        }
        for (size_t idx = 0; idx < m_pipe.size(); idx++) {
            if (close(m_pipe[idx]) != 0) {
                LogPrintf("Destroying WakeupPipe instance, close() failed for m_pipe[%d] = %d with error %s\n",
                          idx, m_pipe[idx], NetworkErrorString(WSAGetLastError()));
            }
        }
#else
        assert(false);
#endif /* USE_WAKEUP_PIPE */
    }
}

void WakeupPipe::Drain() const
{
#ifdef USE_WAKEUP_PIPE
    assert(m_valid && m_pipe[0] != -1);

    int ret{0};
    std::array<uint8_t, 128> buf;
    do {
        ret = read(m_pipe[0], buf.data(), buf.size());
    } while (ret > 0);
#else
    assert(false);
#endif /* USE_WAKEUP_PIPE */
}

void WakeupPipe::Write()
{
#ifdef USE_WAKEUP_PIPE
    assert(m_valid && m_pipe[1] != -1);

    std::array<uint8_t, 1> buf;
    if (write(m_pipe[1], buf.data(), buf.size()) != 1) {
        LogPrintf("Write to m_pipe[1] failed\n");
    }

    m_need_wakeup = false;
#else
    assert(false);
#endif /* USE_WAKEUP_PIPE */
}
