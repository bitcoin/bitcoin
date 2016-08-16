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
#include "ConstBuffer.h"

#include <cassert>

Streaming::ConstBuffer::ConstBuffer()
    : m_buffer(nullptr),
    m_start(nullptr),
    m_stop(nullptr)
{
}

Streaming::ConstBuffer::ConstBuffer(std::shared_ptr<char> buffer, char const* start, char const* stop)
    : m_buffer(buffer),
    m_start(start),
    m_stop(stop)
{
    assert(stop >= start);
    assert(start >= buffer.get());
    assert(stop >= buffer.get());
}

char const* Streaming::ConstBuffer::begin() const
{
    return m_start;
}

char const* Streaming::ConstBuffer::constData() const
{
    return m_start;
}

char const* Streaming::ConstBuffer::end() const
{
    return m_stop;
}

int Streaming::ConstBuffer::size() const
{
    return m_stop - m_start;
}

Streaming::ConstBuffer::operator boost::asio::const_buffer() const
{
    return boost::asio::const_buffer(constData(), size());
}

std::shared_ptr<char> Streaming::ConstBuffer::internal_buffer() const
{
    return m_buffer;
}

char Streaming::ConstBuffer::operator[](size_t idx) const
{
    assert(begin() + idx < end());
    return *(begin() + idx);
}
