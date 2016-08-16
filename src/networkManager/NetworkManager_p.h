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
#ifndef NETWORKMANAGER_P_H
#define NETWORKMANAGER_P_H

/*
 * WARNING USAGE OF THIS HEADER IS RESTRICTED.
 * This Header file is part of the private API and is meant to be used solely by the NetworkManager component.
 *
 * Usage of this API will likely mean your code will break in interesting ways in the future,
 * or even stop to compile.
 *
 * YOU HAVE BEEN WARNED!!
 */

#include "NetworkEndPoint.h"
#include <Message.h>
#include <streaming/BufferPool.h>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/asio.hpp>
#include <list>
#include <boost/date_time/posix_time/ptime.hpp>
#include <atomic>

class NetworkConnection;

using boost::asio::ip::tcp;

class NetworkManagerConnection
{
public:
    NetworkManagerConnection(const std::shared_ptr<NetworkManagerPrivate> &parent, tcp::socket socket, int connectionId);
    NetworkManagerConnection(const std::shared_ptr<NetworkManagerPrivate> &parent, const EndPoint &remote);
    /// Connects to remote (async)
    void connect();

    int nextCallbackId();

    /// unregister a NetworkConnection. Calls have to be from the strand.
    void removeAllCallbacksFor(int id);

    void queueMessage(const Message &message, NetworkConnection::MessagePriority priority);

    inline bool isConnected() const {
        return m_socket.is_open();
    }

    inline const EndPoint &endPoint() const {
        return m_remote;
    }

    /// add callback, calls have to be on the strand.
    void addOnConnectedCallback(int id, const std::function<void(const EndPoint&)> &callback);
    /// add callback, calls have to be on the strand.
    void addOnDisconnectedCallback(int id, const std::function<void(const EndPoint&)> &callback);
    /// add callback, calls have to be on the strand.
    void addOnIncomingMessageCallback(int id, const std::function<void(const Message&)> &callback);

    /// forcably shut down the connection, soon you should no longer reference this object
    void shutdown(std::shared_ptr<NetworkManagerConnection> me);

    /// only incoming connections need accepting.
    void accept();

    inline void disconnect() {
        close(false);
    }

    boost::asio::strand m_strand;

    /// move a call to the thread that the strand represents
    void runOnStrand(const std::function<void()> &function);

    inline bool acceptedConnection() const {
        return m_acceptedConnection;
    }

    void punish(int amount);

    short m_punishment; // aka ban-sore

private:
    EndPoint m_remote;

    void onAddressResolveComplete(const boost::system::error_code& error, tcp::resolver::iterator iterator);
    void onConnectComplete(const boost::system::error_code& error);

    /// call then to start sending messages to remote. Will do noting if we are already sending messages.
    void runMessageQueue();
    void sentSomeBytes(const boost::system::error_code& error, std::size_t bytes_transferred);
    void receivedSomeBytes(const boost::system::error_code& error, std::size_t bytes_transferred);

    bool processPacket(const std::shared_ptr<char> &buffer, const char *data);
    void close(bool reconnect = true); // close down connection
    void connect_priv(); // thread-unsafe version of connect
    void reconnectWithCheck(const boost::system::error_code& error); // called from the m_reconectDelay timer
    void finalShutdown(std::shared_ptr<NetworkManagerConnection>);
    void sendPing(const boost::system::error_code& error);
    void pingTimeout(const boost::system::error_code& error);

    inline bool isOutgoing() const {
        return m_remote.announcePort == m_remote.peerPort;
    }

    Streaming::ConstBuffer createHeader(const Message &message);

    std::shared_ptr<NetworkManagerPrivate> d;

    std::map<int, std::function<void(const EndPoint&)> > m_onConnectedCallbacks;
    std::map<int, std::function<void(const EndPoint&)> > m_onDisConnectedCallbacks;
    std::map<int, std::function<void(const Message&)> > m_onIncomingMessageCallbacks;

    tcp::socket m_socket;
    tcp::resolver m_resolver;

    std::list<Message> m_messageQueue;
    std::list<Message> m_priorityMessageQueue;
    std::list<Message> m_sentPriorityMessages;
    std::list<Streaming::ConstBuffer> m_sendQHeaders;
    int m_messageBytesSend; // future tense
    int m_messageBytesSent; // past tense

    Streaming::BufferPool m_receiveStream;
    Streaming::BufferPool m_sendHelperBuffer;
    mutable std::atomic<int> m_lastCallbackId;
    std::atomic<bool> m_isClosingDown;
    bool m_firstPacket;
    bool m_isConnecting;
    bool m_sendingInProgress;
    bool m_acceptedConnection;

    short m_reconnectStep;
    boost::asio::deadline_timer m_reconnectDelay;

    // for these I write 'ping' but its 'pong' for server (incoming) connections.
    boost::asio::deadline_timer m_pingTimer;
    Message m_pingMessage;

    // chunked messages can be recombined.
    Streaming::BufferPool m_chunkedMessageBuffer;
    int m_chunkedServiceId;
    int m_chunkedMessageId;
};

class NetworkManagerServer
{
public:
    NetworkManagerServer(const std::shared_ptr<NetworkManagerPrivate> &parent, tcp::endpoint endpoint, const std::function<void(NetworkConnection&)> &callback);

    void shutdown();

private:
    void setupCallback();
    void acceptConnection(boost::system::error_code error);

    std::weak_ptr<NetworkManagerPrivate> d;
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    std::function<void(NetworkConnection&)> onIncomingConnection; // callback
};


struct BannedNode
{
    EndPoint endPoint;
    boost::posix_time::ptime banTimeout;
};

class NetworkManagerPrivate
{
public:
    NetworkManagerPrivate(boost::asio::io_service &service);
    ~NetworkManagerPrivate();

    void punishNode(int connectionId, int punishScore);
    void cronHourly(const boost::system::error_code& error);

    boost::asio::io_service& ioService;

    std::map<int, std::shared_ptr<NetworkManagerConnection> > connections;
    int lastConnectionId;

    boost::recursive_mutex mutex; // to lock access to things like the connections map
    bool isClosingDown;

    std::vector<NetworkManagerServer *> servers;

    std::list<BannedNode> banned;
    boost::asio::deadline_timer m_cronHourly;
};

#endif
