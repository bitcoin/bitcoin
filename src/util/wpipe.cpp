// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/wpipe.h>

#include <logging.h>
#include <util/edge.h>
#include <util/sock.h>

static constexpr int EXPECTED_PIPE_WRITTEN_BYTES = 1;

WakeupPipe::WakeupPipe(EdgeTriggeredEvents* edge_trig_events)
    : m_edge_trig_events{edge_trig_events}
{
#ifdef USE_WAKEUP_PIPE
    if (pipe(m_pipe.data()) != 0) {
        LogPrintf("Unable to initialize WakeupPipe, pipe() for m_pipe failed with error %s\n",
                  NetworkErrorString(WSAGetLastError()));
        return;
    }
    for (size_t idx = 0; idx < m_pipe.size(); idx++) {
        int flags = fcntl(m_pipe[idx], F_GETFL, 0);
        if (fcntl(m_pipe[idx], F_SETFL, flags | O_NONBLOCK) == SOCKET_ERROR) {
            LogPrintf("Unable to initialize WakeupPipe, fcntl for O_NONBLOCK on m_pipe[%d] failed with error %s\n", idx,
                      NetworkErrorString(WSAGetLastError()));
            Close();
            return;
        }
    }
    if (edge_trig_events && !edge_trig_events->RegisterPipe(m_pipe[0])) {
        LogPrintf("Unable to initialize WakeupPipe, EdgeTriggeredEvents::RegisterPipe() failed for m_pipe[0] = %d\n",
                  m_pipe[0]);
        Close();
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
            LogPrintf("Destroying WakeupPipe instance, EdgeTriggeredEvents::UnregisterPipe() failed for m_pipe[0] = %d\n",
                      m_pipe[0]);
        }
        Close();
#else
        assert(false);
#endif /* USE_WAKEUP_PIPE */
    }
}

void WakeupPipe::Close()
{
#ifdef USE_WAKEUP_PIPE
    for (size_t idx{0}; idx < m_pipe.size(); idx++) {
        if (m_pipe[idx] != -1 && close(m_pipe[idx]) != 0) {
            LogPrintf("close() failed for m_pipe[%d] = %d with error %s\n", idx, m_pipe[idx],
                      NetworkErrorString(WSAGetLastError()));
        }
        m_pipe[idx] = -1;
    }
#else
    assert(false);
#endif /* USE_WAKEUP_PIPE */
}

void WakeupPipe::Drain() const
{
#ifdef USE_WAKEUP_PIPE
    assert(m_valid && m_pipe[0] != -1);

    int ret{0};
    std::array<uint8_t, 128> buf{};
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

    std::array<uint8_t, EXPECTED_PIPE_WRITTEN_BYTES> buf{};
    int ret = write(m_pipe[1], buf.data(), buf.size());
    if (ret == SOCKET_ERROR) {
        LogPrintf("write() to m_pipe[1] = %d failed with error %s\n", m_pipe[1], NetworkErrorString(WSAGetLastError()));
    }
    if (ret != EXPECTED_PIPE_WRITTEN_BYTES) {
        LogPrintf("write() to m_pipe[1] = %d succeeded with unexpected result %d (expected %d)\n", m_pipe[1], ret,
                  EXPECTED_PIPE_WRITTEN_BYTES);
    }

    m_need_wakeup = false;
#else
    assert(false);
#endif /* USE_WAKEUP_PIPE */
}
