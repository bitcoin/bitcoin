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
#include "Message.h"

#include <cassert>

Message::Message(int serviceId, int messageId)
    : remote(-1),
    m_start(0),
    m_bodyStart(0),
    m_end(0)
{
    setServiceId(serviceId);
    setMessageId(messageId);
}

Message::Message(const std::shared_ptr<char> &sharedBuffer, const char *start, const char * bodyStart, const char *end)
    : remote(-1),
    m_rawData(sharedBuffer),
    m_start(start),
    m_bodyStart(bodyStart),
    m_end(end)
{
}

Message::Message(const std::shared_ptr<char> &sharedBuffer, int totalSize, int headerSize)
    : remote(-1),
    m_rawData(sharedBuffer),
    m_start(m_rawData.get()),
    m_bodyStart(m_start + headerSize),
    m_end(m_start + totalSize)
{
}

Message::Message(const Streaming::ConstBuffer &payload, int serviceId, int messageId)
    : remote(-1),
    m_rawData(payload.internal_buffer()),
    m_start(payload.begin()),
    m_bodyStart(m_start),
    m_end(payload.end())
{
    setServiceId(serviceId);
    setMessageId(messageId);
    assert(payload.size() == body().size());
}

Streaming::ConstBuffer Message::body() const
{
    return Streaming::ConstBuffer(m_rawData, m_bodyStart, m_end);
}

Streaming::ConstBuffer Message::header() const
{
    return Streaming::ConstBuffer(m_rawData, m_start + 4, m_bodyStart);
}

Streaming::ConstBuffer Message::rawData() const
{
    return Streaming::ConstBuffer(m_rawData, m_start, m_end);
}

bool Message::hasHeader() const
{
    return m_start != m_bodyStart;
}
