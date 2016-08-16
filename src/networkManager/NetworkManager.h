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
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <string>
#include <cstdint>
#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "NetworkConnection.h"

class NetworkManagerPrivate;

/**
 * The NetworkManager is the main entry-point of this library.
 *
 * Creating a NetworkManager allows you to manage your connections and their message-flows.
 */
class NetworkManager
{
public:
    NetworkManager(boost::asio::io_service &service);
    ~NetworkManager();

    enum ConnectionEnum {
        AutoCreate,
        OnlyExisting    ///< If no existing connection is found, an invalid one is returned.
    };

    /**
     * Find a connection based on explicit data from the remote argument.
     * @param remote the datastructure with all the details of a remote used in the connection.
     * @param connect Indicate what to do when the connection doesn't exist yet.
     */
    NetworkConnection connection(const EndPoint &remote, ConnectionEnum connect = AutoCreate);

    EndPoint endPoint(int remoteId) const;

    /**
     * Punish a node that misbehaves (for instance if it breaks your protocol).
     * A node that gathers a total of 1000 points is banned for 24 hours,
     * every hour 100 points are subtracted from a each node's punishment-score.
     * @see Message.remote
     * @see NetworkManager::punishNode()
     */
    void punishNode(int remoteId, int punishment);

    /**
     * Listen for incoming connections.
     * Adds a callback that will be called when a new connection comes in.
     */
    void bind(boost::asio::ip::tcp::endpoint endpoint, const std::function<void(NetworkConnection&)> &callback);

    std::weak_ptr<NetworkManagerPrivate> priv(); ///< \internal

private:
    std::shared_ptr<NetworkManagerPrivate> d;
};

#endif
