// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/edge.h>

#include <logging.h>
#include <util/sock.h>

#include <assert.h>

#ifdef USE_EPOLL
#include <sys/epoll.h>
#endif

#ifdef USE_KQUEUE
#include <sys/event.h>
#endif

EdgeTriggeredEvents::EdgeTriggeredEvents(SocketEventsMode events_mode)
    : m_mode(events_mode)
{
    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        m_fd = epoll_create1(0);
        if (m_fd == -1) {
            LogPrintf("Unable to initialize EdgeTriggeredEvents, epoll_create1 returned -1\n");
            return;
        }
#else
        LogPrintf("Attempting to initialize EdgeTriggeredEvents for epoll without support compiled in!\n");
        return;
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        m_fd = kqueue();
        if (m_fd == -1) {
            LogPrintf("Unable to initialize EdgeTriggeredEvents, kqueue returned -1\n");
            return;
        }
#else
        LogPrintf("Attempting to initialize EdgeTriggeredEvents for kqueue without support compiled in!\n");
        return;
#endif /* USE_KQUEUE */
    } else {
        assert(false);
    }
    m_valid = true;
}

EdgeTriggeredEvents::~EdgeTriggeredEvents()
{
    if (m_valid) {
#if defined(USE_KQUEUE) || defined(USE_EPOLL)
        close(m_fd);
#else
        assert(false);
#endif /* defined(USE_KQUEUE) || defined(USE_EPOLL) */
    }
}
