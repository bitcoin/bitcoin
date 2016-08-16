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
#ifndef NETWORKCONNECTION_H
#define NETWORKCONNECTION_H

#include "NetworkEndPoint.h"

#include <memory>
#include <functional>

class NetworkManager;
class NetworkManagerConnection;
class Message;

/**
 * An instance gives access to a client-server connection.
 * The NetworkManager has getters to create NetworkConnection instances, once
 * one is successfully created you can use it like a remote server that you can
 * post messages to, get messages from (using setOnIncomingMessage()) and it will
 * automatically reconnect and resend messages on failures.
 *
 * The NetworkConnection is a very light-weight class for
 * easy access to a certain connection.  It is used as a value-class and as such it has
 * an isValid() method to allow checking if the connection it represents still exists.
 * For instance when the NetworkManager that owns all the connections is deleted, this
 * will return false.
 *
 * Callbacks are registered with one instance of a NetworkConnection, and when that
 * instance goes out of scope the callbacks will be destructed as well.
 * Simplest usage is thus something like this;
 * @code
class MyServiceHandler
{
public:
    MyServiceHandler(NetworkConnection && connection)
        : m_connection(std::move(connection))
    {
        m_connection.setOnConnected(std::bind(&MyServiceHandler::onConnected, this));
        m_connection.setOnDisconnected(std::bind(&MyServiceHandler::onDisconnected, this));
        m_connection.setOnIncomingMessage(std::bind(&MyServiceHandler::onIncomingMessage, this, std::placeholders::_1));
    }

private:
    void onConnected();
    void onDisconnected();
    void onIncomingMessage(const Message &message);

private:
    NetworkConnection m_connection;
};
    @endcode
*/
class NetworkConnection
{
public:
    /// create an invalid default connection object.
    NetworkConnection();
    /**
     * Create a NetworkConnection representing a certain connection-id.
     * This constructor can be used to create a connection from a Message by using
     * the Message::remote member as the \a id.
     */
    NetworkConnection(NetworkManager *parent, int id);
    /// called by the NetworkManager
    NetworkConnection(std::shared_ptr<NetworkManagerConnection> &parent, int id);
    NetworkConnection(NetworkConnection && other);
    ~NetworkConnection();

    /// returns true if we have an actual socket connection to the remote()
    bool isConnected() const;
    /// initiates a (re)connect
    void connect();
    void disconnect();
    /// Return the current remote we are connected to, or an empty one if we are not connected
    EndPoint endPoint() const;

    inline int connectionId() const {
        return m_id;
    }

    /// accepts an incoming connection
    void accept();


    enum MessagePriority {
        NormalPriority,
        HighPriority
    };

    /// send a message, automatically connecting if needed.
    void send(const Message &message, MessagePriority priority = NormalPriority);

    /// return true if this object can be operated on and represents a real connection
    bool isValid() const;

    /**
     * Clears the networkconnection and all related callbacks.
     * isValid() will return false after this is called.
     */
    void clear();

    NetworkConnection& operator=(NetworkConnection && other);

    /**
      * Sets a callback to be called every time we (re)connect at the socket level.
      * Notice that callbacks are owned by the connection object, the callback will be
      *  garbage collected when the instance of the NetworkConnection goes out of scope.
      * You can have only one callback per NetworkConnection instance, but you can have
      * many instances representing the same physical connection.
      */
    void setOnConnected(const std::function<void(const EndPoint&)> &callback);

    /**
      * Sets a callback to be called every time we have a disconnect.
      * Notice that callbacks are owned by the connection object, the callback will be
      *  garbage collected when the instance of the NetworkConnection goes out of scope.
      * You can have only one callback per NetworkConnection instance, but you can have
      * many instances representing the same physical connection.
      */
    void setOnDisconnected(const std::function<void(const EndPoint&)> &callback);

    /**
      * Sets a callback for incoming messages, one message per call.
      * Notice that callbacks are owned by the connection object, the callback will be
      *  garbage collected when the instance of the NetworkConnection goes out of scope.
      * You can have only one callback per NetworkConnection instance, but you can have
      * many instances representing the same physical connection.
      */
    void setOnIncomingMessage(const std::function<void(const Message&)> &callback);

    /**
     * Punish a node that misbehaves (for instance if it breaks your protocol).
     * A node that gathers a total of 1000 points is banned for 24 hours,
     * every hour 100 points are subtracted from a each node's punishment-score.
     */
    void punishPeer(int punishment);

    /**
      * Posts a task on the underlying strand for it to be executed serially.
      */
    void postOnStrand(const std::function<void()> &task);

private:
    NetworkConnection(const NetworkConnection&);
    NetworkConnection& operator=(NetworkConnection&);

    std::weak_ptr<NetworkManagerConnection> m_parent;
    int m_id;
    int m_callbacksId;
};

#endif
