//
// detail/buffer_sequence_adapter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_BUFFER_SEQUENCE_ADAPTER_HPP
#define BOOST_ASIO_DETAIL_BUFFER_SEQUENCE_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/array_fwd.hpp>
#include <boost/asio/detail/socket_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class buffer_sequence_adapter_base
{
#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
public:
  // The maximum number of buffers to support in a single operation.
  enum { max_buffers = 1 };

protected:
  typedef Windows::Storage::Streams::IBuffer^ native_buffer_type;

  BOOST_ASIO_DECL static void init_native_buffer(
      native_buffer_type& buf,
      const boost::asio::mutable_buffer& buffer);

  BOOST_ASIO_DECL static void init_native_buffer(
      native_buffer_type& buf,
      const boost::asio::const_buffer& buffer);
#elif defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
public:
  // The maximum number of buffers to support in a single operation.
  enum { max_buffers = 64 < max_iov_len ? 64 : max_iov_len };

protected:
  typedef WSABUF native_buffer_type;

  static void init_native_buffer(WSABUF& buf,
      const boost::asio::mutable_buffer& buffer)
  {
    buf.buf = static_cast<char*>(buffer.data());
    buf.len = static_cast<ULONG>(buffer.size());
  }

  static void init_native_buffer(WSABUF& buf,
      const boost::asio::const_buffer& buffer)
  {
    buf.buf = const_cast<char*>(static_cast<const char*>(buffer.data()));
    buf.len = static_cast<ULONG>(buffer.size());
  }
#else // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
public:
  // The maximum number of buffers to support in a single operation.
  enum { max_buffers = 64 < max_iov_len ? 64 : max_iov_len };

protected:
  typedef iovec native_buffer_type;

  static void init_iov_base(void*& base, void* addr)
  {
    base = addr;
  }

  template <typename T>
  static void init_iov_base(T& base, void* addr)
  {
    base = static_cast<T>(addr);
  }

  static void init_native_buffer(iovec& iov,
      const boost::asio::mutable_buffer& buffer)
  {
    init_iov_base(iov.iov_base, buffer.data());
    iov.iov_len = buffer.size();
  }

  static void init_native_buffer(iovec& iov,
      const boost::asio::const_buffer& buffer)
  {
    init_iov_base(iov.iov_base, const_cast<void*>(buffer.data()));
    iov.iov_len = buffer.size();
  }
#endif // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
};

// Helper class to translate buffers into the native buffer representation.
template <typename Buffer, typename Buffers>
class buffer_sequence_adapter
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = false };

  explicit buffer_sequence_adapter(const Buffers& buffer_sequence)
    : count_(0), total_buffer_size_(0)
  {
    buffer_sequence_adapter::init(
        boost::asio::buffer_sequence_begin(buffer_sequence),
        boost::asio::buffer_sequence_end(buffer_sequence));
  }

  native_buffer_type* buffers()
  {
    return buffers_;
  }

  std::size_t count() const
  {
    return count_;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const Buffers& buffer_sequence)
  {
    return buffer_sequence_adapter::all_empty(
        boost::asio::buffer_sequence_begin(buffer_sequence),
        boost::asio::buffer_sequence_end(buffer_sequence));
  }

  static void validate(const Buffers& buffer_sequence)
  {
    buffer_sequence_adapter::validate(
        boost::asio::buffer_sequence_begin(buffer_sequence),
        boost::asio::buffer_sequence_end(buffer_sequence));
  }

  static Buffer first(const Buffers& buffer_sequence)
  {
    return buffer_sequence_adapter::first(
        boost::asio::buffer_sequence_begin(buffer_sequence),
        boost::asio::buffer_sequence_end(buffer_sequence));
  }

  enum { linearisation_storage_size = 8192 };

  static Buffer linearise(const Buffers& buffer_sequence,
      const boost::asio::mutable_buffer& storage)
  {
    return buffer_sequence_adapter::linearise(
        boost::asio::buffer_sequence_begin(buffer_sequence),
        boost::asio::buffer_sequence_end(buffer_sequence), storage);
  }

private:
  template <typename Iterator>
  void init(Iterator begin, Iterator end)
  {
    Iterator iter = begin;
    for (; iter != end && count_ < max_buffers; ++iter, ++count_)
    {
      Buffer buffer(*iter);
      init_native_buffer(buffers_[count_], buffer);
      total_buffer_size_ += buffer.size();
    }
  }

  template <typename Iterator>
  static bool all_empty(Iterator begin, Iterator end)
  {
    Iterator iter = begin;
    std::size_t i = 0;
    for (; iter != end && i < max_buffers; ++iter, ++i)
      if (Buffer(*iter).size() > 0)
        return false;
    return true;
  }

  template <typename Iterator>
  static void validate(Iterator begin, Iterator end)
  {
    Iterator iter = begin;
    for (; iter != end; ++iter)
    {
      Buffer buffer(*iter);
      buffer.data();
    }
  }

  template <typename Iterator>
  static Buffer first(Iterator begin, Iterator end)
  {
    Iterator iter = begin;
    for (; iter != end; ++iter)
    {
      Buffer buffer(*iter);
      if (buffer.size() != 0)
        return buffer;
    }
    return Buffer();
  }

  template <typename Iterator>
  static Buffer linearise(Iterator begin, Iterator end,
      const boost::asio::mutable_buffer& storage)
  {
    boost::asio::mutable_buffer unused_storage = storage;
    Iterator iter = begin;
    while (iter != end && unused_storage.size() != 0)
    {
      Buffer buffer(*iter);
      ++iter;
      if (buffer.size() == 0)
        continue;
      if (unused_storage.size() == storage.size())
      {
        if (iter == end)
          return buffer;
        if (buffer.size() >= unused_storage.size())
          return buffer;
      }
      unused_storage += boost::asio::buffer_copy(unused_storage, buffer);
    }
    return Buffer(storage.data(), storage.size() - unused_storage.size());
  }

  native_buffer_type buffers_[max_buffers];
  std::size_t count_;
  std::size_t total_buffer_size_;
};

template <typename Buffer>
class buffer_sequence_adapter<Buffer, boost::asio::mutable_buffer>
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = true };

  explicit buffer_sequence_adapter(
      const boost::asio::mutable_buffer& buffer_sequence)
  {
    init_native_buffer(buffer_, Buffer(buffer_sequence));
    total_buffer_size_ = buffer_sequence.size();
  }

  native_buffer_type* buffers()
  {
    return &buffer_;
  }

  std::size_t count() const
  {
    return 1;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const boost::asio::mutable_buffer& buffer_sequence)
  {
    return buffer_sequence.size() == 0;
  }

  static void validate(const boost::asio::mutable_buffer& buffer_sequence)
  {
    buffer_sequence.data();
  }

  static Buffer first(const boost::asio::mutable_buffer& buffer_sequence)
  {
    return Buffer(buffer_sequence);
  }

  enum { linearisation_storage_size = 1 };

  static Buffer linearise(const boost::asio::mutable_buffer& buffer_sequence,
      const Buffer&)
  {
    return Buffer(buffer_sequence);
  }

private:
  native_buffer_type buffer_;
  std::size_t total_buffer_size_;
};

template <typename Buffer>
class buffer_sequence_adapter<Buffer, boost::asio::const_buffer>
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = true };

  explicit buffer_sequence_adapter(
      const boost::asio::const_buffer& buffer_sequence)
  {
    init_native_buffer(buffer_, Buffer(buffer_sequence));
    total_buffer_size_ = buffer_sequence.size();
  }

  native_buffer_type* buffers()
  {
    return &buffer_;
  }

  std::size_t count() const
  {
    return 1;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const boost::asio::const_buffer& buffer_sequence)
  {
    return buffer_sequence.size() == 0;
  }

  static void validate(const boost::asio::const_buffer& buffer_sequence)
  {
    buffer_sequence.data();
  }

  static Buffer first(const boost::asio::const_buffer& buffer_sequence)
  {
    return Buffer(buffer_sequence);
  }

  enum { linearisation_storage_size = 1 };

  static Buffer linearise(const boost::asio::const_buffer& buffer_sequence,
      const Buffer&)
  {
    return Buffer(buffer_sequence);
  }

private:
  native_buffer_type buffer_;
  std::size_t total_buffer_size_;
};

#if !defined(BOOST_ASIO_NO_DEPRECATED)

template <typename Buffer>
class buffer_sequence_adapter<Buffer, boost::asio::mutable_buffers_1>
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = true };

  explicit buffer_sequence_adapter(
      const boost::asio::mutable_buffers_1& buffer_sequence)
  {
    init_native_buffer(buffer_, Buffer(buffer_sequence));
    total_buffer_size_ = buffer_sequence.size();
  }

  native_buffer_type* buffers()
  {
    return &buffer_;
  }

  std::size_t count() const
  {
    return 1;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const boost::asio::mutable_buffers_1& buffer_sequence)
  {
    return buffer_sequence.size() == 0;
  }

  static void validate(const boost::asio::mutable_buffers_1& buffer_sequence)
  {
    buffer_sequence.data();
  }

  static Buffer first(const boost::asio::mutable_buffers_1& buffer_sequence)
  {
    return Buffer(buffer_sequence);
  }

  enum { linearisation_storage_size = 1 };

  static Buffer linearise(const boost::asio::mutable_buffers_1& buffer_sequence,
      const Buffer&)
  {
    return Buffer(buffer_sequence);
  }

private:
  native_buffer_type buffer_;
  std::size_t total_buffer_size_;
};

template <typename Buffer>
class buffer_sequence_adapter<Buffer, boost::asio::const_buffers_1>
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = true };

  explicit buffer_sequence_adapter(
      const boost::asio::const_buffers_1& buffer_sequence)
  {
    init_native_buffer(buffer_, Buffer(buffer_sequence));
    total_buffer_size_ = buffer_sequence.size();
  }

  native_buffer_type* buffers()
  {
    return &buffer_;
  }

  std::size_t count() const
  {
    return 1;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const boost::asio::const_buffers_1& buffer_sequence)
  {
    return buffer_sequence.size() == 0;
  }

  static void validate(const boost::asio::const_buffers_1& buffer_sequence)
  {
    buffer_sequence.data();
  }

  static Buffer first(const boost::asio::const_buffers_1& buffer_sequence)
  {
    return Buffer(buffer_sequence);
  }

  enum { linearisation_storage_size = 1 };

  static Buffer linearise(const boost::asio::const_buffers_1& buffer_sequence,
      const Buffer&)
  {
    return Buffer(buffer_sequence);
  }

private:
  native_buffer_type buffer_;
  std::size_t total_buffer_size_;
};

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

template <typename Buffer, typename Elem>
class buffer_sequence_adapter<Buffer, boost::array<Elem, 2> >
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = false };

  explicit buffer_sequence_adapter(
      const boost::array<Elem, 2>& buffer_sequence)
  {
    init_native_buffer(buffers_[0], Buffer(buffer_sequence[0]));
    init_native_buffer(buffers_[1], Buffer(buffer_sequence[1]));
    total_buffer_size_ = buffer_sequence[0].size() + buffer_sequence[1].size();
  }

  native_buffer_type* buffers()
  {
    return buffers_;
  }

  std::size_t count() const
  {
    return 2;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const boost::array<Elem, 2>& buffer_sequence)
  {
    return buffer_sequence[0].size() == 0 && buffer_sequence[1].size() == 0;
  }

  static void validate(const boost::array<Elem, 2>& buffer_sequence)
  {
    buffer_sequence[0].data();
    buffer_sequence[1].data();
  }

  static Buffer first(const boost::array<Elem, 2>& buffer_sequence)
  {
    return Buffer(buffer_sequence[0].size() != 0
        ? buffer_sequence[0] : buffer_sequence[1]);
  }

  enum { linearisation_storage_size = 8192 };

  static Buffer linearise(const boost::array<Elem, 2>& buffer_sequence,
      const boost::asio::mutable_buffer& storage)
  {
    if (buffer_sequence[0].size() == 0)
      return Buffer(buffer_sequence[1]);
    if (buffer_sequence[1].size() == 0)
      return Buffer(buffer_sequence[0]);
    return Buffer(storage.data(),
        boost::asio::buffer_copy(storage, buffer_sequence));
  }

private:
  native_buffer_type buffers_[2];
  std::size_t total_buffer_size_;
};

#if defined(BOOST_ASIO_HAS_STD_ARRAY)

template <typename Buffer, typename Elem>
class buffer_sequence_adapter<Buffer, std::array<Elem, 2> >
  : buffer_sequence_adapter_base
{
public:
  enum { is_single_buffer = false };

  explicit buffer_sequence_adapter(
      const std::array<Elem, 2>& buffer_sequence)
  {
    init_native_buffer(buffers_[0], Buffer(buffer_sequence[0]));
    init_native_buffer(buffers_[1], Buffer(buffer_sequence[1]));
    total_buffer_size_ = buffer_sequence[0].size() + buffer_sequence[1].size();
  }

  native_buffer_type* buffers()
  {
    return buffers_;
  }

  std::size_t count() const
  {
    return 2;
  }

  std::size_t total_size() const
  {
    return total_buffer_size_;
  }

  bool all_empty() const
  {
    return total_buffer_size_ == 0;
  }

  static bool all_empty(const std::array<Elem, 2>& buffer_sequence)
  {
    return buffer_sequence[0].size() == 0 && buffer_sequence[1].size() == 0;
  }

  static void validate(const std::array<Elem, 2>& buffer_sequence)
  {
    buffer_sequence[0].data();
    buffer_sequence[1].data();
  }

  static Buffer first(const std::array<Elem, 2>& buffer_sequence)
  {
    return Buffer(buffer_sequence[0].size() != 0
        ? buffer_sequence[0] : buffer_sequence[1]);
  }

  enum { linearisation_storage_size = 8192 };

  static Buffer linearise(const std::array<Elem, 2>& buffer_sequence,
      const boost::asio::mutable_buffer& storage)
  {
    if (buffer_sequence[0].size() == 0)
      return Buffer(buffer_sequence[1]);
    if (buffer_sequence[1].size() == 0)
      return Buffer(buffer_sequence[0]);
    return Buffer(storage.data(),
        boost::asio::buffer_copy(storage, buffer_sequence));
  }

private:
  native_buffer_type buffers_[2];
  std::size_t total_buffer_size_;
};

#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/buffer_sequence_adapter.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_DETAIL_BUFFER_SEQUENCE_ADAPTER_HPP
