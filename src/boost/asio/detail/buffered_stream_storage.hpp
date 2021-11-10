//
// detail/buffered_stream_storage.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_BUFFERED_STREAM_STORAGE_HPP
#define BOOST_ASIO_DETAIL_BUFFERED_STREAM_STORAGE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/assert.hpp>
#include <cstddef>
#include <cstring>
#include <vector>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class buffered_stream_storage
{
public:
  // The type of the bytes stored in the buffer.
  typedef unsigned char byte_type;

  // The type used for offsets into the buffer.
  typedef std::size_t size_type;

  // Constructor.
  explicit buffered_stream_storage(std::size_t buffer_capacity)
    : begin_offset_(0),
      end_offset_(0),
      buffer_(buffer_capacity)
  {
  }

  /// Clear the buffer.
  void clear()
  {
    begin_offset_ = 0;
    end_offset_ = 0;
  }

  // Return a pointer to the beginning of the unread data.
  mutable_buffer data()
  {
    return boost::asio::buffer(buffer_) + begin_offset_;
  }

  // Return a pointer to the beginning of the unread data.
  const_buffer data() const
  {
    return boost::asio::buffer(buffer_) + begin_offset_;
  }

  // Is there no unread data in the buffer.
  bool empty() const
  {
    return begin_offset_ == end_offset_;
  }

  // Return the amount of unread data the is in the buffer.
  size_type size() const
  {
    return end_offset_ - begin_offset_;
  }

  // Resize the buffer to the specified length.
  void resize(size_type length)
  {
    BOOST_ASIO_ASSERT(length <= capacity());
    if (begin_offset_ + length <= capacity())
    {
      end_offset_ = begin_offset_ + length;
    }
    else
    {
      using namespace std; // For memmove.
      memmove(&buffer_[0], &buffer_[0] + begin_offset_, size());
      end_offset_ = length;
      begin_offset_ = 0;
    }
  }

  // Return the maximum size for data in the buffer.
  size_type capacity() const
  {
    return buffer_.size();
  }

  // Consume multiple bytes from the beginning of the buffer.
  void consume(size_type count)
  {
    BOOST_ASIO_ASSERT(begin_offset_ + count <= end_offset_);
    begin_offset_ += count;
    if (empty())
      clear();
  }

private:
  // The offset to the beginning of the unread data.
  size_type begin_offset_;

  // The offset to the end of the unread data.
  size_type end_offset_;
  
  // The data in the buffer.
  std::vector<byte_type> buffer_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_BUFFERED_STREAM_STORAGE_HPP
