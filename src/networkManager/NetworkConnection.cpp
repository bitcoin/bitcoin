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
#include "NetworkConnection.h"
#include "NetworkManager.h"
#include "NetworkManager_p.h"
#include "Message.h"


NetworkConnection::NetworkConnection()
    : m_id(-1),
    m_callbacksId(-1)
{
}

NetworkConnection::NetworkConnection(NetworkManager *parent, int id)
    : m_id(id),
    m_callbacksId(-1)
{
    auto priv = parent->priv().lock();
    if (priv)
        m_parent = priv->connections.at(id);
}

NetworkConnection::NetworkConnection(std::shared_ptr<NetworkManagerConnection> &parent, int id)
    : m_parent(parent),
    m_id(id),
    m_callbacksId(-1)
{
}

NetworkConnection::NetworkConnection(NetworkConnection &&other)
    : m_parent(std::move(other.m_parent)),
    m_id(other.m_id),
    m_callbacksId(other.m_callbacksId)
{
    other.m_id = -1;
    other.m_parent.reset();
}

NetworkConnection& NetworkConnection::operator=(NetworkConnection && other)
{
    if (other.m_id != m_id && m_callbacksId != -1)
        clear();
    m_parent = std::move(other.m_parent);
    other.m_parent.reset();
    m_id = other.m_id;
    other.m_id = -1;
    m_callbacksId = other.m_callbacksId;
    return *this;
}

void NetworkConnection::clear()
{
    auto d = m_parent.lock();
    if (d) {
        if (m_callbacksId >= 0) {
            if (d->m_strand.running_in_this_thread())
                d->removeAllCallbacksFor(m_callbacksId);
            else
                d->m_strand.post(std::bind(&NetworkManagerConnection::removeAllCallbacksFor, d, m_callbacksId));
        }
        m_callbacksId = -1;
        m_parent.reset();
    }
}

NetworkConnection::~NetworkConnection()
{
    clear();
}

bool NetworkConnection::isValid() const
{
    if (m_id <= 0) // default constructor called
        return false;
    auto d = m_parent.lock();
    return d != nullptr;
}

bool NetworkConnection::isConnected() const
{
    auto d = m_parent.lock();
    if (d)
        return d->isConnected();
    return false;
}

EndPoint NetworkConnection::endPoint() const
{
    auto d = m_parent.lock();
    if (d && d->isConnected())
        return d->endPoint();
    return EndPoint();
}

void NetworkConnection::accept()
{
    auto d = m_parent.lock();
    if (d && d->isConnected())
        d->accept();
}

void NetworkConnection::connect()
{
    auto d = m_parent.lock();
    if (d.get())
        d->connect();
}

void NetworkConnection::disconnect()
{
    auto d = m_parent.lock();
    if (d) {
        if (d->m_strand.running_in_this_thread())
            d->disconnect();
        else
            d->m_strand.post(std::bind(&NetworkManagerConnection::disconnect, d));
    }
}

void NetworkConnection::send(const Message &message, MessagePriority priority)
{
    auto d = m_parent.lock();
    if (d)
        d->queueMessage(message, priority);
}

void NetworkConnection::setOnConnected(const std::function<void(const EndPoint&)> &callback)
{
    auto d = m_parent.lock();
    if (d) {
        if (m_callbacksId < 0)
            m_callbacksId = d->nextCallbackId();
        if (d->m_strand.running_in_this_thread())
            d->addOnConnectedCallback(m_callbacksId, callback);
        else
            d->m_strand.post(std::bind(&NetworkManagerConnection::addOnConnectedCallback, d, m_callbacksId, callback));
    }
}

void NetworkConnection::setOnDisconnected(const std::function<void(const EndPoint&)> &callback)
{
    auto d = m_parent.lock();
    if (d) {
        if (m_callbacksId < 0)
            m_callbacksId = d->nextCallbackId();
        if (d->m_strand.running_in_this_thread())
            d->addOnDisconnectedCallback(m_callbacksId, callback);
        else
            d->m_strand.post(std::bind(&NetworkManagerConnection::addOnDisconnectedCallback, d, m_callbacksId, callback));
    }
}

void NetworkConnection::setOnIncomingMessage(const std::function<void(const Message&)> &callback)
{
    auto d = m_parent.lock();
    if (d) {
        if (m_callbacksId < 0)
            m_callbacksId = d->nextCallbackId();
        if (d->m_strand.running_in_this_thread())
            d->addOnIncomingMessageCallback(m_callbacksId, callback);
        else
            d->m_strand.post(std::bind(&NetworkManagerConnection::addOnIncomingMessageCallback, d, m_callbacksId, callback));
    }
}

void NetworkConnection::punishPeer(int punishment)
{
    auto d = m_parent.lock();
    if (d.get()) {
        d->punish(punishment);
    }
}

void NetworkConnection::postOnStrand(const std::function<void()> &task)
{
    auto d = m_parent.lock();
    if (d)
    {
        if (d->m_strand.running_in_this_thread())
            task();
        else
            d->runOnStrand(task);
    }
}
