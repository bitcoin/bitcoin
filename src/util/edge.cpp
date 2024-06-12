// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/edge.h>

#include <logging.h>
#include <util/sock.h>

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
            LogPrintf("Unable to initialize EdgeTriggeredEvents, epoll_create1 returned -1 with error %s\n",
                      NetworkErrorString(WSAGetLastError()));
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
            LogPrintf("Unable to initialize EdgeTriggeredEvents, kqueue returned -1 with error %s\n",
                      NetworkErrorString(WSAGetLastError()));
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
        if (close(m_fd) != 0) {
            LogPrintf("Destroying EdgeTriggeredEvents instance, close() failed for m_fd = %d with error %s\n", m_fd,
                      NetworkErrorString(WSAGetLastError()));
        }
#else
        assert(false);
#endif /* defined(USE_KQUEUE) || defined(USE_EPOLL) */
    }
}

bool EdgeTriggeredEvents::RegisterEntity(int entity, const std::string& entity_name) const
{
    assert(m_valid);

    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        epoll_event event;
        event.data.fd = entity;
        event.events = EPOLLIN;
        if (epoll_ctl(m_fd, EPOLL_CTL_ADD, entity, &event) != 0) {
            LogPrintf("Failed to add %s to epoll fd (epoll_ctl returned error %s)\n", entity_name,
                      NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        struct kevent event;
        EV_SET(&event, entity, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(m_fd, &event, 1, nullptr, 0, nullptr) != 0) {
            LogPrintf("Failed to add %s to kqueue fd (kevent returned error %s)\n", entity_name,
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

bool EdgeTriggeredEvents::UnregisterEntity(int entity, const std::string& entity_name) const
{
    assert(m_valid);

    if (m_mode == SocketEventsMode::EPoll) {
#ifdef USE_EPOLL
        if (epoll_ctl(m_fd, EPOLL_CTL_DEL, entity, nullptr) != 0) {
            LogPrintf("Failed to remove %s from epoll fd (epoll_ctl returned error %s)\n", entity_name,
                      NetworkErrorString(WSAGetLastError()));
            return false;
        }
#else
        assert(false);
#endif /* USE_EPOLL */
    } else if (m_mode == SocketEventsMode::KQueue) {
#ifdef USE_KQUEUE
        struct kevent event;
        EV_SET(&event, entity, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        if (kevent(m_fd, &event, 1, nullptr, 0, nullptr) != 0) {
            LogPrintf("Failed to remove %s from kqueue fd (kevent returned error %s)\n", entity_name,
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

bool EdgeTriggeredEvents::AddSocket(SOCKET socket) const
{
    return RegisterEntity(socket, "socket");
}

bool EdgeTriggeredEvents::RemoveSocket(SOCKET socket) const
{
    return UnregisterEntity(socket, "socket");
}

bool EdgeTriggeredEvents::RegisterPipe(int wakeup_pipe)
{
    if (m_pipe_registered) {
        LogPrintf("Pipe already registered, ignoring new registration request\n");
        return false;
    }
    bool ret = RegisterEntity(wakeup_pipe, "wakeup pipe");
    if (ret) m_pipe_registered = true;
    return ret;
}

bool EdgeTriggeredEvents::UnregisterPipe(int wakeup_pipe)
{
    if (!m_pipe_registered) {
        LogPrintf("No pipe currently registered to unregister, ignoring request\n");
        return false;
    }
    bool ret = UnregisterEntity(wakeup_pipe, "wakeup pipe");
    if (ret) m_pipe_registered = false;
    return ret;
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
