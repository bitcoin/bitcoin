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
#include "AdminServer.h"
#include "AdminRPCBinding.h"
#include "AdminProtocol.h"

#include "streaming/MessageBuilder.h"
#include "streaming/MessageParser.h"

#include "util.h"
#include "utilstrencodings.h"
#include "random.h"
#include "rpcserver.h"

#include <fstream>
#include <functional>

// the amount of seconds after which we disconnect incoming connections that have not logged in yet.
#define LOGIN_TIMEOUT 4

Admin::Server::Server(boost::asio::io_service &service)
    : m_networkManager(service),
      m_timerRunning(false),
      m_newConnectionTimeout(service)
{
    boost::filesystem::path path(GetArg("-admincookiefile", "admin_cookie"));
    if (!path.is_complete())
        path = GetDataDir() / path;

    std::ifstream file;
    file.open(path.string().c_str());
    if (file.is_open()) {
        std::getline(file, m_cookie);
        file.close();
    } else {
        // then we create one.
        uint8_t buf[32];
        GetRandBytes(buf, 32);
        m_cookie = EncodeBase64(&buf[0],32);

        std::ofstream out;
        out.open(path.string().c_str());
        if (!out.is_open()) {
            LogPrintf("Unable to open admin-cookie authentication file %s for writing\n", path.string());
            throw std::runtime_error("Unable to open admin-cookie authentication file.");
        }
        out << m_cookie;
        out.close();
        LogPrintf("Generated admin-authentication cookie %s\n", path.string());
    }

    // TODO find out how to listen to the right ports based on the arguments users type
    boost::asio::ip::tcp::endpoint endpoint4(boost::asio::ip::tcp::v4(), 1235);
    // boost::asio::ip::tcp::endpoint endpoint6(boost::asio::ip::tcp::v6(), 1236);
    m_networkManager.bind(endpoint4, std::bind(&Admin::Server::newConnection, this, std::placeholders::_1));
}

void Admin::Server::newConnection(NetworkConnection &connection)
{
    connection.setOnIncomingMessage(std::bind(&Admin::Server::incomingLoginMessage, this, std::placeholders::_1));
    connection.setOnDisconnected(std::bind(&Admin::Server::connectionRemoved, this, std::placeholders::_1));
    connection.accept();
    NewConnection con;
    con.connection = std::move(connection);
    con.time = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(LOGIN_TIMEOUT);

    boost::mutex::scoped_lock lock(m_mutex);
    m_newConnections.push_back(std::move(con));

    if (!m_timerRunning) {
        m_timerRunning = true;
        m_newConnectionTimeout.expires_from_now(boost::posix_time::seconds(LOGIN_TIMEOUT));
        m_newConnectionTimeout.async_wait(std::bind(&Admin::Server::checkConnections, this, std::placeholders::_1));
    }
}

void Admin::Server::connectionRemoved(const EndPoint &endPoint)
{
    boost::mutex::scoped_lock lock(m_mutex);
    auto iter = m_newConnections.begin();
    while (iter != m_newConnections.end()) {
        if (iter->connection.connectionId() == endPoint.connectionId) {
            m_newConnections.erase(iter);
            break;
        }
        ++iter;
    }

    auto conIter = m_connections.begin();
    while (conIter != m_connections.end()) {
        if ((*conIter)->m_connection.connectionId() == endPoint.connectionId) {
            m_connections.erase(conIter);
            delete *conIter;
            break;
        }
        ++conIter;
    }
}

void Admin::Server::incomingLoginMessage(const Message &message)
{
    bool success = false;
    if (message.messageId() == Login::LoginMessage && message.serviceId() == LoginService) {
        Streaming::MessageParser parser(message.body());
        while (!success && parser.next() == Streaming::FoundTag) {
            if (parser.tag() == Login::CookieData) {
                assert(!m_cookie.empty());
                if (m_cookie == parser.stringData()) {
                    success = true;
                }
            }
        }
    }
    NetworkConnection con(&m_networkManager, message.remote);
    assert(con.isValid());
    if (!success) {
        con.disconnect();
        return;
    }

    con.setOnDisconnected(std::bind(&Admin::Server::connectionRemoved, this, std::placeholders::_1));
    Connection *handler = new Connection(std::move(con));
    boost::mutex::scoped_lock lock(m_mutex);
    m_connections.push_back(handler);

    auto iter = m_newConnections.begin();
    while (iter != m_newConnections.end()) {
        if (iter->connection.connectionId() == message.remote) {
            m_newConnections.erase(iter);
            break;
        }
        ++iter;
    }
}

void Admin::Server::checkConnections(boost::system::error_code error)
{
    if (error.value() == boost::asio::error::operation_aborted)
        return;
    boost::mutex::scoped_lock lock(m_mutex);
    const auto now = boost::posix_time::second_clock::universal_time();
    auto iter = m_newConnections.begin();
    while (iter != m_newConnections.end()) {
        if (iter->time <= now) {
            // LogPrintf("Calling disconnect on connection %d now\n", iter->connection.connectionId());
            iter->connection.disconnect();
            iter = m_newConnections.erase(iter);
        } else {
            ++iter;
        }
    }

    // restart timer if there is still something left.
    if (!m_newConnections.empty()) {
        m_timerRunning = true;
        m_newConnectionTimeout.expires_from_now(boost::posix_time::seconds(1));
        m_newConnectionTimeout.async_wait(std::bind(&Admin::Server::checkConnections, this, std::placeholders::_1));
    } else {
        m_timerRunning = false;
    }
}


Admin::Server::Connection::Connection(NetworkConnection && connection)
    : m_connection(std::move(connection))
{
    m_connection.setOnIncomingMessage(std::bind(&Admin::Server::Connection::incomingMessage, this, std::placeholders::_1));
}

void Admin::Server::Connection::incomingMessage(const Message &message)
{
    std::unique_ptr<AdminRPCBinding::Parser> parser;
    try {
        parser.reset(AdminRPCBinding::createParser(message));
        assert(parser.get()); // createParser should never return a nullptr
    } catch (const std::exception &e) {
        sendFailedMessage(message, e.what());
        return;
    }

    assert(parser.get());
    assert(!parser->method().empty());

    try {
        UniValue request(UniValue::VOBJ);
        parser->createRequest(message, request);
        UniValue result;
        try {
            result = tableRPC.execute(parser->method(), request);
        } catch (UniValue& objError) {
            sendFailedMessage(message, find_value(objError, "message").get_str());
            return;
        } catch(const std::exception &e) {
            sendFailedMessage(message, std::string(e.what()));
            return;
        }
        m_bufferPool.reserve(parser->messageSize(result));
        Streaming::MessageBuilder builder(m_bufferPool);
        parser->parser(builder, result);
        Message reply = builder.message(message.serviceId(), parser->answerMessageId());
        const int requestId = message.headerInt(Admin::RequestId);
        if (requestId != -1)
            reply.setHeaderInt(Admin::RequestId, requestId);
        m_connection.send(reply);
    } catch (const std::exception &e) {
        std::string error = "Interal Error " + std::string(e.what());
        LogPrintf("AdminServer internal error in parsing %s: %s", parser->method(), e.what());
        (void) m_bufferPool.commit(); // make sure the partial message is discarded
        sendFailedMessage(message, error);
        return;
    }
}

void Admin::Server::Connection::sendFailedMessage(const Message &origin, const std::string &failReason)
{
    m_bufferPool.reserve(failReason.size() + 20);
    Streaming::MessageBuilder builder(m_bufferPool);
    builder.add(Control::FailedReason, failReason);
    builder.add(Control::FailedCommandServiceId, origin.serviceId());
    builder.add(Control::FailedCommandId, origin.messageId());
    Message answer = builder.message(ControlService, Control::CommandFailed);
    const int requestId = origin.headerInt(Admin::RequestId);
    if (requestId != -1)
        answer.setHeaderInt(Admin::RequestId, requestId);
    m_connection.send(answer);
}
