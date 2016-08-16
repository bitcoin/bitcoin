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
#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include "ConstBuffer.h"
#include <cassert>

namespace Streaming {

/**
 * The buffer pool is a utility class that allows you to allocate a big chunk of memory and
 * create any number of buffers from it in order to avoid expensive calls to malloc.
 *
 * The easiest usage is the following pattern;
 @code
   BufferPool pool;
   pool.reserve(myString.size()); //make sure the pool has enough space
   strcpy(pool.data(), myString);
   pool.markUsed(myString.size());
   ConstBuffer buf = pool.commit();
 @endcode
 * Do notice that you likely want to make your pool a member so its a long-lived class and
 * it can create buffers for a long time.
 *
 * The data internally in both the BufferPool and the ConstBuffer uses shared pointers so
 * all memory management is automatically taken care of.
 */
class BufferPool
{
public:
    BufferPool(int m_defaultSize = 256 * 1024);
    BufferPool(BufferPool&& other);
    BufferPool& operator=(BufferPool&& other);

    /// Ensures that there are at least that \a bytes available for writing
    void reserve(int bytes);

    /// Return the available space for writing - NOT the same as size()
    int capacity() const;

    /// usage of storage (by writing to data()) should be registered
    /// so a future call to commit() returns the right data.
    inline void markUsed(int bytecount)
    {
        assert(bytecount >= 0);
        m_writePointer += bytecount;
        assert(m_writePointer <= m_buffer.get() + m_size);
        assert(m_writePointer >= m_readPointer);
    }

    /**
     * This method will create a ConstBuffer of a more flexible size than commit().
     * The start and end pointers have to be within the reserved space of
     * the internal_buffer, which means that after data was marked as possible to
     * forget with forget(), you should not try to create a buffer any longer as
     * that may fail (trigger an assert in debug builds).
     */
    ConstBuffer createBufferSlice(char const* start, char const* stop) const;

    /**
     * After a buffer has been used you should make the pool forget about it so the data can be
     * garbage collected.
     * This probably doesn't have to be called directly, you want to use commit() instead.
     */
    void forget(int read_bytes);

    /**
     * Commit the used part into a ConstBuffer and preparing the pool for a new buffer.
     * This is equivalent to;
     @code
        markUsed(usedBytes);
        ConstBuffer buf = createBufferSlice(begin(), end());
        forget(buf.size());
        return buf;
     @endcode
     */
    ConstBuffer commit(int usedBytes = 0);

    /**
     * Iterator to access the already 'used' part
     * Begin points to the start of the uncommitted memory area.
     * Directly after a commit, but before a markUsed() both begin and end are the same.
     * \see markUsed()
     * \see end()
     */
    char const* begin() const {
        return m_readPointer;
    }
    /// non-const convenience method.
    inline char* begin() {
        return m_readPointer;
    }

    /**
     * Iterator to access the not yet 'used' part
     * End points directly after the end of the uncommitted, but 'used' memory area.
     * Directly after a commit, but before a markUsed() both begin and end are the same.
     * \see markUsed()
     * \see begin()
     * \see data()
     */
    char const* end() const {
        return m_writePointer;
    }
    /// non-const convenience method.
    char* end() {
        return m_writePointer;
    }

    /// indexting operator - assumes begin() + idx < end()
    char operator[](size_t idx) const;

    /**
     * Returns a pointer to the not yet used memory area.
     * This is always equal to end()
     * \see markUsed()
     */
    inline char* data() {
        return m_writePointer;
    }
    /// This is the size available for reading
    /// same as end() - begin()
    int size() const;

    /**
     * Remove all internal storage usage and reset size to the default (initial) size.
     * A pool holds on to a buffer, sometimes a rather large one which may be not useful.
     * Calling clear will do a reset() on the shared pointer and set the begin/end pointers
     * to nullptr.
     * It is essential for the next usage to call reserve() (but that was always the case anyway)
     * before putting anything new in the buffer.
     */
    void clear();

    /**
     * @brief writeInt32 allows you to write 4 bytes at begin() and mark them as used.
     * @param data the 4-byte integer value to write.
     */
    void writeInt32(unsigned int data);

    /// return the shared pointer to the buffer, useful to upgrade the refcount.
    std::shared_ptr<char> internal_buffer() const;
private:
    void change_capacity(int bytes);
    // unimplemented
    BufferPool(BufferPool const&);
    BufferPool& operator=(BufferPool const&);

    std::shared_ptr<char> m_buffer;
    char *m_readPointer;
    char *m_writePointer;
    int m_defaultSize;
    int m_size;
};

}

#endif
