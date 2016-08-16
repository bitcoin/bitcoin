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
#ifndef CONSTBUFFER_H
#define CONSTBUFFER_H

#include <memory>
#include <boost/asio/buffer.hpp>

namespace Streaming
{

/**
 * Class for holding a reference to an slice of the buffer in buffer_base
 * Since it has a reference to the actual buffer with a shared_array
 * the underlying memory will not be deallocated until all references are removed
 */
class ConstBuffer
{
public:
    /// creates an invalid buffer
    ConstBuffer();

    bool isValid() const {
        return m_start != nullptr && m_stop != nullptr;
    }

    /// Construct from already allocated storage.
    /// Keep in mind that a shared_ptr can have a custom dtor if we want to send something special
    explicit ConstBuffer(std::shared_ptr<char> m_buffer, char const* m_start, char const* m_stop);

    char const* begin() const;
    inline char const* cbegin() const {
        return begin();
    }
    char const* end() const;
    inline char const* cend() const {
        return end();
    }

    /// standard indexing operator - assumes begin() + idx < end()
    char operator[](size_t idx) const;

    char const* constData() const;
    /// returns end - begin
    int size() const;

    /// Implement ConvertibleToConstBuffer for asio
    operator boost::asio::const_buffer() const;

    std::shared_ptr<char> internal_buffer() const;

private:
    std::shared_ptr<char> m_buffer;
    char const* m_start;
    char const* m_stop;
};
}

#endif
