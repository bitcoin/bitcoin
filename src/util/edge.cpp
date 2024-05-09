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

bool EdgeTriggeredEvents::RegisterEvents(SOCKET socket) const
{
    assert(m_valid && socket != INVALID_SOCKET);

    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        epoll_event e;
        // We're using edge-triggered mode, so it's important that we drain sockets even if no signals come in
        e.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
        e.data.fd = socket;

        if (epoll_ctl(m_fd, EPOLL_CTL_ADD, socket, &e) != 0) {
            LogPrintf("Failed to register events for socket -- epoll_ctl(%d, %d, %d, ...) returned error: %s\n",
                      m_fd, EPOLL_CTL_ADD, socket, NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        struct kevent events[2];
        EV_SET(&events[0], socket, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        EV_SET(&events[1], socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, nullptr);

        if (kevent(m_fd, events, 2, nullptr, 0, nullptr) != 0) {
            LogPrintf("Failed to register events for socket -- kevent(%d, %d, %d, ...) returned error: %s\n",
                      m_fd, EV_ADD, socket, NetworkErrorString(WSAGetLastError()));
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

bool EdgeTriggeredEvents::UnregisterEvents(SOCKET socket) const
{
    assert(m_valid);

    if (socket == INVALID_SOCKET) {
        LogPrintf("Cannot unregister events for invalid socket\n");
        return false;
    }

    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        if (epoll_ctl(m_fd, EPOLL_CTL_DEL, socket, nullptr) != 0) {
            LogPrintf("Failed to unregister events for socket -- epoll_ctl(%d, %d, %d, ...) returned error: %s\n",
                      m_fd, EPOLL_CTL_DEL, socket, NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        struct kevent events[2];
        EV_SET(&events[0], socket, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        EV_SET(&events[1], socket, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        if (kevent(m_fd, events, 2, nullptr, 0, nullptr) != 0) {
            LogPrintf("Failed to unregister events for socket -- kevent(%d, %d, %d, ...) returned error: %s\n",
                      m_fd, EV_DELETE, socket, NetworkErrorString(WSAGetLastError()));
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
