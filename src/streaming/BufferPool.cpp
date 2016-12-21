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
#include "BufferPool.h"
#include <boost/math/constants/constants.hpp>

Streaming::BufferPool::BufferPool(int default_size)
    : m_buffer(std::shared_ptr<char>(new char[default_size], std::default_delete<char[]>())),
    m_readPointer(m_buffer.get()),
    m_writePointer(m_buffer.get()),
    m_defaultSize(default_size),
    m_size(default_size)
{
}

Streaming::BufferPool::BufferPool(BufferPool&& other)
    : m_buffer(std::move(other.m_buffer)),
    m_readPointer(std::move(other.m_readPointer)),
    m_writePointer(std::move(other.m_writePointer)),
    m_defaultSize(std::move(other.m_defaultSize)),
    m_size(std::move(other.m_size))
{
}

Streaming::BufferPool& Streaming::BufferPool::operator=(BufferPool&& rhs)
{
    m_buffer = std::move(rhs.m_buffer);
    m_readPointer = std::move(rhs.m_readPointer);
    m_writePointer = std::move(rhs.m_writePointer);
    m_defaultSize = std::move(rhs.m_defaultSize);
    m_size = std::move(rhs.m_size);
    return *this;
}

int Streaming::BufferPool::capacity() const
{
    assert(m_writePointer <= m_buffer.get() + m_size);
    return std::distance<char const*>(m_writePointer, m_buffer.get() + m_size);
}

void Streaming::BufferPool::forget(int rc)
{
    m_readPointer += rc;
    assert(m_readPointer <= m_buffer.get() + m_size);
}

Streaming::ConstBuffer Streaming::BufferPool::commit(int usedBytes)
{
    assert(usedBytes >= 0);
    m_writePointer += usedBytes;
    assert(m_writePointer <= m_buffer.get() + m_size);
    assert(m_writePointer >= m_readPointer);

    const char * begin = m_readPointer;
    m_readPointer = m_writePointer;
    assert(m_readPointer <= m_buffer.get() + m_size); // Or just less than?
    const char * end = m_writePointer;
    return ConstBuffer(m_buffer, begin, end);
}

int Streaming::BufferPool::size() const
{
    return end() - begin();
}

void Streaming::BufferPool::clear()
{
    m_readPointer = nullptr;
    m_writePointer = nullptr;
    m_buffer.reset();
    m_size = m_defaultSize;
}

void Streaming::BufferPool::writeInt32(unsigned int data)
{
    unsigned int d = data;
    m_writePointer[0] = d & 0xFF;
    d = d >> 8;
    m_writePointer[1] = d & 0xFF;
    d = d >> 8;
    m_writePointer[2] = d & 0xFF;
    d = d >> 8;
    m_writePointer[3] = d & 0xFF;
    markUsed(4);
}

Streaming::ConstBuffer Streaming::BufferPool::createBufferSlice(char const* start, char const* stop) const
{
    assert(stop >= start);
    assert(start >= begin());
    assert(start <= end());
    assert(stop >= begin());
    assert(stop <= end());
    return ConstBuffer(m_buffer, start, stop);
}

void Streaming::BufferPool::change_capacity(int bytes)
{
    int unprocessed = m_writePointer - m_readPointer;
    assert(unprocessed >= 0);
    if (unprocessed + bytes <= m_defaultSize) // unprocessed > buffer_size
    {
        m_size = m_defaultSize;
    }
    else
    {
        // Over reserve by phi ~= 1.62 = (1 + sqrt(5))/2
        // There are some discussions about whether 1.5, 2 or phi is correct. Try this.
        m_size = std::max<size_t>(bytes + unprocessed, static_cast<size_t>(std::ceil(m_size * boost::math::double_constants::phi)));
    }
    m_buffer = std::shared_ptr<char>(new char[m_size], std::default_delete<char[]>());

    std::memcpy(m_buffer.get(), m_readPointer, unprocessed); // Read pointer still points to the old buffer
    m_readPointer = m_buffer.get();
    m_writePointer = m_readPointer + unprocessed;
}

void Streaming::BufferPool::reserve(int bytes)
{
    assert(bytes >= 0);
    if (m_readPointer == nullptr) {
        m_buffer = std::shared_ptr<char>(new char[m_size], std::default_delete<char[]>());
        m_readPointer = m_buffer.get();
        m_writePointer = m_readPointer;
    }
    if (capacity() < bytes)
        change_capacity(bytes);
}

std::shared_ptr<char> Streaming::BufferPool::internal_buffer() const
{
    return m_buffer;
}

char Streaming::BufferPool::operator[](size_t idx) const
{
    assert(begin() + idx < end());
    return *(begin() + idx);
}
