//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_READ_WRITE_SINGLE_VALUE_HPP
#define BOOST_COMPUTE_DETAIL_READ_WRITE_SINGLE_VALUE_HPP

#include <boost/throw_exception.hpp>

#include <boost/compute/buffer.hpp>
#include <boost/compute/event.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/command_queue.hpp>

namespace boost {
namespace compute {
namespace detail {

// reads and returns a single value at index in the buffer
template<class T>
inline T read_single_value(const buffer &buffer,
                           size_t index,
                           command_queue &queue)
{
    BOOST_ASSERT(index < buffer.size() / sizeof(T));
    BOOST_ASSERT(buffer.get_context() == queue.get_context());

    T value;
    queue.enqueue_read_buffer(buffer,
                              sizeof(T) * index,
                              sizeof(T),
                              &value);
    return value;
}

// reads and returns a the first value in the buffer
template<class T>
inline T read_single_value(const buffer &buffer, command_queue &queue)
{
    return read_single_value<T>(buffer, 0, queue);
}

// writes a single value at index to the buffer
template<class T>
inline event write_single_value(const T &value,
                                const buffer &buffer,
                                size_t index,
                                command_queue &queue)
{
    BOOST_ASSERT(index < buffer.size() / sizeof(T));
    BOOST_ASSERT(buffer.get_context() == queue.get_context());

    return queue.enqueue_write_buffer(buffer,
                                      index * sizeof(T),
                                      sizeof(T),
                                      &value);
}

// writes value to the first location in buffer
template<class T>
inline void write_single_value(const T &value,
                               const buffer &buffer,
                               command_queue &queue)
{
    write_single_value<T>(value, buffer, 0, queue);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_READ_WRITE_SINGLE_VALUE_HPP
