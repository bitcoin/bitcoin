/*
 * This file is part of the bitcoin-classic project
 * Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ADMINSERVER_H
#define ADMINSERVER_H

#include "streaming/BufferPool.h"
#include "networkManager/NetworkManager.h"

#include <univalue/include/univalue.h>
#include <vector>
#include <string>
#include <list>
#include <boost/thread/mutex.hpp>

class NetworkManager;

namespace Admin {

class Server {
public:
    Server(boost::asio::io_service &service);

private:
    void newConnection(NetworkConnection &connection);
    void connectionRemoved(const EndPoint &endPoint);
    void incomingLoginMessage(const Message &message);

    void checkConnections(boost::system::error_code error);

    class Connection {
    public:
        Connection(NetworkConnection && connection);
        void incomingMessage(const Message &message);

        NetworkConnection m_connection;

    private:
        void sendFailedMessage(const Message &origin, const std::string &failReason);

        Streaming::BufferPool m_bufferPool;
    };

    struct NewConnection {
        NetworkConnection connection;
        boost::posix_time::ptime time;
    };

    NetworkManager m_networkManager;
    std::string m_cookie; // for authentication

    mutable boost::mutex m_mutex; // protects the next 4 vars.
    std::list<Connection*> m_connections;
    std::list<NewConnection> m_newConnections;
    bool m_timerRunning;
    boost::asio::deadline_timer m_newConnectionTimeout;
};
}

#endif
