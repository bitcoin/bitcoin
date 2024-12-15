// Copyright (c) 2020-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_EDGE_H
#define BITCOIN_UTIL_EDGE_H

#include <compat/compat.h>

#include <assert.h>
#include <cstdint>
#include <string>

enum class SocketEventsMode : int8_t;

/**
 * A manager for abstracting logic surrounding edge-triggered socket events
 * modes like kqueue and epoll.
 */
// TODO: simplify this class to 2-3 flags; kick out everything else to Sock/~Sock and inherited classes
class EdgeTriggeredEvents
{
public:
    explicit EdgeTriggeredEvents(SocketEventsMode events_mode);
    ~EdgeTriggeredEvents();

    bool IsValid() const { return m_valid; }
    int GetFileDescriptor() const { assert(m_fd != -1); return m_fd; }

    /* Add socket to interest list */
    bool AddSocket(SOCKET socket) const;
    /* Remove socket from interest list */
    bool RemoveSocket(SOCKET socket) const;

    /* Register events for socket */
    bool RegisterEvents(SOCKET socket) const;
    /* Unregister events for socket */
    bool UnregisterEvents(SOCKET socket) const;

private:
    friend class WakeupPipe;
    /* Register wakeup pipe with EdgeTriggeredEvents instance */
    bool RegisterPipe(int wakeup_pipe);
    /* Unregister wakeup pipe with EdgeTriggeredEvents instance */
    bool UnregisterPipe(int wakeup_pipe);

private:
    bool RegisterEntity(int entity, const std::string& entity_name) const;
    bool UnregisterEntity(int entity, const std::string& entity_name) const;

private:
    /* Flag set if pipe has been registered with instance */
    bool m_pipe_registered{false};
    /* Instance validity flag set during construction */
    bool m_valid{false};
    /* Flag for storing selected socket events mode */
    SocketEventsMode m_mode;
    /* File descriptor used to interact with events mode */
    int m_fd{-1};
};

#endif // BITCOIN_UTIL_EDGE_H
