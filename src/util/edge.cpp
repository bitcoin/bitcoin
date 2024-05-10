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

bool EdgeTriggeredEvents::AddSocket(SOCKET socket) const
{
    assert(m_valid);

    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        epoll_event event;
        event.data.fd = socket;
        event.events = EPOLLIN;
        if (epoll_ctl(m_fd, EPOLL_CTL_ADD, socket, &event) != 0) {
            LogPrintf("Failed to add socket to epoll fd (epoll_ctl returned error %s)\n",
                      NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        struct kevent event;
        EV_SET(&event, socket, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(m_fd, &event, 1, nullptr, 0, nullptr) != 0) {
            LogPrintf("Failed to add socket to kqueue fd (kevent returned error %s)\n",
                      NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_KQUEUE */
    } else {
        assert(false);
    }
    return true;
}

bool EdgeTriggeredEvents::RemoveSocket(SOCKET socket) const
{
    assert(m_valid);

    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        if (epoll_ctl(m_fd, EPOLL_CTL_DEL, socket, nullptr) != 0) {
            LogPrintf("Failed to remove socket from epoll fd (epoll_ctl returned error %s)\n",
                      NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        struct kevent event;
        EV_SET(&event, socket, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        if (kevent(m_fd, &event, 1, nullptr, 0, nullptr) != 0) {
            LogPrintf("Failed to remove socket from kqueue fd (kevent returned error %s)\n",
                      NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_KQUEUE */
    } else {
        assert(false);
    }
    return true;
}
