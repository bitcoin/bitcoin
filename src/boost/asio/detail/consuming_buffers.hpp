//
// detail/consuming_buffers.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_CONSUMING_BUFFERS_HPP
#define BOOST_ASIO_DETAIL_CONSUMING_BUFFERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/detail/limits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Helper template to determine the maximum number of prepared buffers.
template <typename Buffers>
struct prepared_buffers_max
{
  enum { value = buffer_sequence_adapter_base::max_buffers };
};

template <typename Elem, std::size_t N>
struct prepared_buffers_max<boost::array<Elem, N> >
{
  enum { value = N };
};

#if defined(BOOST_ASIO_HAS_STD_ARRAY)

template <typename Elem, std::size_t N>
struct prepared_buffers_max<std::array<Elem, N> >
{
  enum { value = N };
};

#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)

// A buffer sequence used to represent a subsequence of the buffers.
template <typename Buffer, std::size_t MaxBuffers>
struct prepared_buffers
{
  typedef Buffer value_type;
  typedef const Buffer* const_iterator;

  enum { max_buffers = MaxBuffers < 16 ? MaxBuffers : 16 };

  prepared_buffers() : count(0) {}
  const_iterator begin() const { return elems; }
  const_iterator end() const { return elems + count; }

  Buffer elems[max_buffers];
  std::size_t count;
};

// A proxy for a sub-range in a list of buffers.
template <typename Buffer, typename Buffers, typename Buffer_Iterator>
class consuming_buffers
{
public:
  typedef prepared_buffers<Buffer, prepared_buffers_max<Buffers>::value>
    prepared_buffers_type;

  // Construct to represent the entire list of buffers.
  explicit consuming_buffers(const Buffers& buffers)
    : buffers_(buffers),
      total_consumed_(0),
      next_elem_(0),
      next_elem_offset_(0)
  {
    using boost::asio::buffer_size;
    total_size_ = buffer_size(buffers);
  }

  // Determine if we are at the end of the buffers.
  bool empty() const
  {
    return total_consumed_ >= total_size_;
  }

  // Get the buffer for a single transfer, with a size.
  prepared_buffers_type prepare(std::size_t max_size)
  {
    prepared_buffers_type result;

    Buffer_Iterator next = boost::asio::buffer_sequence_begin(buffers_);
    Buffer_Iterator end = boost::asio::buffer_sequence_end(buffers_);

    std::advance(next, next_elem_);
    std::size_t elem_offset = next_elem_offset_;
    while (next != end && max_size > 0 && (result.count) < result.max_buffers)
    {
      Buffer next_buf = Buffer(*next) + elem_offset;
      result.elems[result.count] = boost::asio::buffer(next_buf, max_size);
      max_size -= result.elems[result.count].size();
      elem_offset = 0;
      if (result.elems[result.count].size() > 0)
        ++result.count;
      ++next;
    }

    return result;
  }

  // Consume the specified number of bytes from the buffers.
  void consume(std::size_t size)
  {
    total_consumed_ += size;

    Buffer_Iterator next = boost::asio::buffer_sequence_begin(buffers_);
    Buffer_Iterator end = boost::asio::buffer_sequence_end(buffers_);

    std::advance(next, next_elem_);
    while (next != end && size > 0)
    {
      Buffer next_buf = Buffer(*next) + next_elem_offset_;
      if (size < next_buf.size())
      {
        next_elem_offset_ += size;
        size = 0;
      }
      else
      {
        size -= next_buf.size();
        next_elem_offset_ = 0;
        ++next_elem_;
        ++next;
      }
    }
  }

  // Get the total number of bytes consumed from the buffers.
  std::size_t total_consumed() const
  {
    return total_consumed_;
  }

private:
  Buffers buffers_;
  std::size_t total_size_;
  std::size_t total_consumed_;
  std::size_t next_elem_;
  std::size_t next_elem_offset_;
};

// Base class of all consuming_buffers specialisations for single buffers.
template <typename Buffer>
class consuming_single_buffer
{
public:
  // Construct to represent the entire list of buffers.
  template <typename Buffer1>
  explicit consuming_single_buffer(const Buffer1& buffer)
    : buffer_(buffer),
      total_consumed_(0)
  {
  }

  // Determine if we are at the end of the buffers.
  bool empty() const
  {
    return total_consumed_ >= buffer_.size();
  }

  // Get the buffer for a single transfer, with a size.
  Buffer prepare(std::size_t max_size)
  {
    return boost::asio::buffer(buffer_ + total_consumed_, max_size);
  }

  // Consume the specified number of bytes from the buffers.
  void consume(std::size_t size)
  {
    total_consumed_ += size;
  }

  // Get the total number of bytes consumed from the buffers.
  std::size_t total_consumed() const
  {
    return total_consumed_;
  }

private:
  Buffer buffer_;
  std::size_t total_consumed_;
};

template <>
class consuming_buffers<mutable_buffer, mutable_buffer, const mutable_buffer*>
  : public consuming_single_buffer<BOOST_ASIO_MUTABLE_BUFFER>
{
public:
  explicit consuming_buffers(const mutable_buffer& buffer)
    : consuming_single_buffer<BOOST_ASIO_MUTABLE_BUFFER>(buffer)
  {
  }
};

template <>
class consuming_buffers<const_buffer, mutable_buffer, const mutable_buffer*>
  : public consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>
{
public:
  explicit consuming_buffers(const mutable_buffer& buffer)
    : consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>(buffer)
  {
  }
};

template <>
class consuming_buffers<const_buffer, const_buffer, const const_buffer*>
  : public consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>
{
public:
  explicit consuming_buffers(const const_buffer& buffer)
    : consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>(buffer)
  {
  }
};

#if !defined(BOOST_ASIO_NO_DEPRECATED)

template <>
class consuming_buffers<mutable_buffer,
    mutable_buffers_1, const mutable_buffer*>
  : public consuming_single_buffer<BOOST_ASIO_MUTABLE_BUFFER>
{
public:
  explicit consuming_buffers(const mutable_buffers_1& buffer)
    : consuming_single_buffer<BOOST_ASIO_MUTABLE_BUFFER>(buffer)
  {
  }
};

template <>
class consuming_buffers<const_buffer, mutable_buffers_1, const mutable_buffer*>
  : public consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>
{
public:
  explicit consuming_buffers(const mutable_buffers_1& buffer)
    : consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>(buffer)
  {
  }
};

template <>
class consuming_buffers<const_buffer, const_buffers_1, const const_buffer*>
  : public consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>
{
public:
  explicit consuming_buffers(const const_buffers_1& buffer)
    : consuming_single_buffer<BOOST_ASIO_CONST_BUFFER>(buffer)
  {
  }
};

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

template <typename Buffer, typename Elem>
class consuming_buffers<Buffer, boost::array<Elem, 2>,
    typename boost::array<Elem, 2>::const_iterator>
{
public:
  // Construct to represent the entire list of buffers.
  explicit consuming_buffers(const boost::array<Elem, 2>& buffers)
    : buffers_(buffers),
      total_consumed_(0)
  {
  }

  // Determine if we are at the end of the buffers.
  bool empty() const
  {
    return total_consumed_ >=
      Buffer(buffers_[0]).size() + Buffer(buffers_[1]).size();
  }

  // Get the buffer for a single transfer, with a size.
  boost::array<Buffer, 2> prepare(std::size_t max_size)
  {
    boost::array<Buffer, 2> result = {{
      Buffer(buffers_[0]), Buffer(buffers_[1]) }};
    std::size_t buffer0_size = result[0].size();
    result[0] = boost::asio::buffer(result[0] + total_consumed_, max_size);
    result[1] = boost::asio::buffer(
        result[1] + (total_consumed_ < buffer0_size
          ? 0 : total_consumed_ - buffer0_size),
        max_size - result[0].size());
    return result;
  }

  // Consume the specified number of bytes from the buffers.
  void consume(std::size_t size)
  {
    total_consumed_ += size;
  }

  // Get the total number of bytes consumed from the buffers.
  std::size_t total_consumed() const
  {
    return total_consumed_;
  }

private:
  boost::array<Elem, 2> buffers_;
  std::size_t total_consumed_;
};

#if defined(BOOST_ASIO_HAS_STD_ARRAY)

template <typename Buffer, typename Elem>
class consuming_buffers<Buffer, std::array<Elem, 2>,
    typename std::array<Elem, 2>::const_iterator>
{
public:
  // Construct to represent the entire list of buffers.
  explicit consuming_buffers(const std::array<Elem, 2>& buffers)
    : buffers_(buffers),
      total_consumed_(0)
  {
  }

  // Determine if we are at the end of the buffers.
  bool empty() const
  {
    return total_consumed_ >=
      Buffer(buffers_[0]).size() + Buffer(buffers_[1]).size();
  }

  // Get the buffer for a single transfer, with a size.
  std::array<Buffer, 2> prepare(std::size_t max_size)
  {
    std::array<Buffer, 2> result = {{
      Buffer(buffers_[0]), Buffer(buffers_[1]) }};
    std::size_t buffer0_size = result[0].size();
    result[0] = boost::asio::buffer(result[0] + total_consumed_, max_size);
    result[1] = boost::asio::buffer(
        result[1] + (total_consumed_ < buffer0_size
          ? 0 : total_consumed_ - buffer0_size),
        max_size - result[0].size());
    return result;
  }

  // Consume the specified number of bytes from the buffers.
  void consume(std::size_t size)
  {
    total_consumed_ += size;
  }

  // Get the total number of bytes consumed from the buffers.
  std::size_t total_consumed() const
  {
    return total_consumed_;
  }

private:
  std::array<Elem, 2> buffers_;
  std::size_t total_consumed_;
};

#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)

// Specialisation for null_buffers to ensure that the null_buffers type is
// always passed through to the underlying read or write operation.
template <typename Buffer>
class consuming_buffers<Buffer, null_buffers, const mutable_buffer*>
  : public boost::asio::null_buffers
{
public:
  consuming_buffers(const null_buffers&)
  {
    // No-op.
  }

  bool empty()
  {
    return false;
  }

  null_buffers prepare(std::size_t)
  {
    return null_buffers();
  }

  void consume(std::size_t)
  {
    // No-op.
  }

  std::size_t total_consumed() const
  {
    return 0;
  }
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_CONSUMING_BUFFERS_HPP
