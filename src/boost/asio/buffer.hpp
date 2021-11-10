//
// buffer.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BUFFER_HPP
#define BOOST_ASIO_BUFFER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/asio/detail/array_fwd.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/string_view.hpp>
#include <boost/asio/detail/throw_exception.hpp>
#include <boost/asio/detail/type_traits.hpp>

#if defined(BOOST_ASIO_MSVC) && (BOOST_ASIO_MSVC >= 1700)
# if defined(_HAS_ITERATOR_DEBUGGING) && (_HAS_ITERATOR_DEBUGGING != 0)
#  if !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
#   define BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
#  endif // !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
# endif // defined(_HAS_ITERATOR_DEBUGGING)
#endif // defined(BOOST_ASIO_MSVC) && (BOOST_ASIO_MSVC >= 1700)

#if defined(__GNUC__)
# if defined(_GLIBCXX_DEBUG)
#  if !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
#   define BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
#  endif // !defined(BOOST_ASIO_DISABLE_BUFFER_DEBUGGING)
# endif // defined(_GLIBCXX_DEBUG)
#endif // defined(__GNUC__)

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
# include <boost/asio/detail/functional.hpp>
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

#if defined(BOOST_ASIO_HAS_BOOST_WORKAROUND)
# include <boost/detail/workaround.hpp>
# if !defined(__clang__)
#  if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
#   define BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND
#  endif // BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
# elif BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
#  define BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND
# endif // BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
#endif // defined(BOOST_ASIO_HAS_BOOST_WORKAROUND)

#if defined(BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND)
# include <boost/asio/detail/type_traits.hpp>
#endif // defined(BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

class mutable_buffer;
class const_buffer;

/// Holds a buffer that can be modified.
/**
 * The mutable_buffer class provides a safe representation of a buffer that can
 * be modified. It does not own the underlying data, and so is cheap to copy or
 * assign.
 *
 * @par Accessing Buffer Contents
 *
 * The contents of a buffer may be accessed using the @c data() and @c size()
 * member functions:
 *
 * @code boost::asio::mutable_buffer b1 = ...;
 * std::size_t s1 = b1.size();
 * unsigned char* p1 = static_cast<unsigned char*>(b1.data());
 * @endcode
 *
 * The @c data() member function permits violations of type safety, so uses of
 * it in application code should be carefully considered.
 */
class mutable_buffer
{
public:
  /// Construct an empty buffer.
  mutable_buffer() BOOST_ASIO_NOEXCEPT
    : data_(0),
      size_(0)
  {
  }

  /// Construct a buffer to represent a given memory range.
  mutable_buffer(void* data, std::size_t size) BOOST_ASIO_NOEXCEPT
    : data_(data),
      size_(size)
  {
  }

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  mutable_buffer(void* data, std::size_t size,
      boost::asio::detail::function<void()> debug_check)
    : data_(data),
      size_(size),
      debug_check_(debug_check)
  {
  }

  const boost::asio::detail::function<void()>& get_debug_check() const
  {
    return debug_check_;
  }
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

  /// Get a pointer to the beginning of the memory range.
  void* data() const BOOST_ASIO_NOEXCEPT
  {
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
    if (size_ && debug_check_)
      debug_check_();
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
    return data_;
  }

  /// Get the size of the memory range.
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
    return size_;
  }

  /// Move the start of the buffer by the specified number of bytes.
  mutable_buffer& operator+=(std::size_t n) BOOST_ASIO_NOEXCEPT
  {
    std::size_t offset = n < size_ ? n : size_;
    data_ = static_cast<char*>(data_) + offset;
    size_ -= offset;
    return *this;
  }

private:
  void* data_;
  std::size_t size_;

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  boost::asio::detail::function<void()> debug_check_;
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
};

#if !defined(BOOST_ASIO_NO_DEPRECATED)

/// (Deprecated: Use mutable_buffer.) Adapts a single modifiable buffer so that
/// it meets the requirements of the MutableBufferSequence concept.
class mutable_buffers_1
  : public mutable_buffer
{
public:
  /// The type for each element in the list of buffers.
  typedef mutable_buffer value_type;

  /// A random-access iterator type that may be used to read elements.
  typedef const mutable_buffer* const_iterator;

  /// Construct to represent a given memory range.
  mutable_buffers_1(void* data, std::size_t size) BOOST_ASIO_NOEXCEPT
    : mutable_buffer(data, size)
  {
  }

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  mutable_buffers_1(void* data, std::size_t size,
      boost::asio::detail::function<void()> debug_check)
    : mutable_buffer(data, size, debug_check)
  {
  }
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

  /// Construct to represent a single modifiable buffer.
  explicit mutable_buffers_1(const mutable_buffer& b) BOOST_ASIO_NOEXCEPT
    : mutable_buffer(b)
  {
  }

  /// Get a random-access iterator to the first element.
  const_iterator begin() const BOOST_ASIO_NOEXCEPT
  {
    return this;
  }

  /// Get a random-access iterator for one past the last element.
  const_iterator end() const BOOST_ASIO_NOEXCEPT
  {
    return begin() + 1;
  }
};

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

/// Holds a buffer that cannot be modified.
/**
 * The const_buffer class provides a safe representation of a buffer that cannot
 * be modified. It does not own the underlying data, and so is cheap to copy or
 * assign.
 *
 * @par Accessing Buffer Contents
 *
 * The contents of a buffer may be accessed using the @c data() and @c size()
 * member functions:
 *
 * @code boost::asio::const_buffer b1 = ...;
 * std::size_t s1 = b1.size();
 * const unsigned char* p1 = static_cast<const unsigned char*>(b1.data());
 * @endcode
 *
 * The @c data() member function permits violations of type safety, so uses of
 * it in application code should be carefully considered.
 */
class const_buffer
{
public:
  /// Construct an empty buffer.
  const_buffer() BOOST_ASIO_NOEXCEPT
    : data_(0),
      size_(0)
  {
  }

  /// Construct a buffer to represent a given memory range.
  const_buffer(const void* data, std::size_t size) BOOST_ASIO_NOEXCEPT
    : data_(data),
      size_(size)
  {
  }

  /// Construct a non-modifiable buffer from a modifiable one.
  const_buffer(const mutable_buffer& b) BOOST_ASIO_NOEXCEPT
    : data_(b.data()),
      size_(b.size())
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , debug_check_(b.get_debug_check())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
  {
  }

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  const_buffer(const void* data, std::size_t size,
      boost::asio::detail::function<void()> debug_check)
    : data_(data),
      size_(size),
      debug_check_(debug_check)
  {
  }

  const boost::asio::detail::function<void()>& get_debug_check() const
  {
    return debug_check_;
  }
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

  /// Get a pointer to the beginning of the memory range.
  const void* data() const BOOST_ASIO_NOEXCEPT
  {
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
    if (size_ && debug_check_)
      debug_check_();
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
    return data_;
  }

  /// Get the size of the memory range.
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
    return size_;
  }

  /// Move the start of the buffer by the specified number of bytes.
  const_buffer& operator+=(std::size_t n) BOOST_ASIO_NOEXCEPT
  {
    std::size_t offset = n < size_ ? n : size_;
    data_ = static_cast<const char*>(data_) + offset;
    size_ -= offset;
    return *this;
  }

private:
  const void* data_;
  std::size_t size_;

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  boost::asio::detail::function<void()> debug_check_;
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
};

#if !defined(BOOST_ASIO_NO_DEPRECATED)

/// (Deprecated: Use const_buffer.) Adapts a single non-modifiable buffer so
/// that it meets the requirements of the ConstBufferSequence concept.
class const_buffers_1
  : public const_buffer
{
public:
  /// The type for each element in the list of buffers.
  typedef const_buffer value_type;

  /// A random-access iterator type that may be used to read elements.
  typedef const const_buffer* const_iterator;

  /// Construct to represent a given memory range.
  const_buffers_1(const void* data, std::size_t size) BOOST_ASIO_NOEXCEPT
    : const_buffer(data, size)
  {
  }

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
  const_buffers_1(const void* data, std::size_t size,
      boost::asio::detail::function<void()> debug_check)
    : const_buffer(data, size, debug_check)
  {
  }
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

  /// Construct to represent a single non-modifiable buffer.
  explicit const_buffers_1(const const_buffer& b) BOOST_ASIO_NOEXCEPT
    : const_buffer(b)
  {
  }

  /// Get a random-access iterator to the first element.
  const_iterator begin() const BOOST_ASIO_NOEXCEPT
  {
    return this;
  }

  /// Get a random-access iterator for one past the last element.
  const_iterator end() const BOOST_ASIO_NOEXCEPT
  {
    return begin() + 1;
  }
};

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

/// (Deprecated: Use the socket/descriptor wait() and async_wait() member
/// functions.) An implementation of both the ConstBufferSequence and
/// MutableBufferSequence concepts to represent a null buffer sequence.
class null_buffers
{
public:
  /// The type for each element in the list of buffers.
  typedef mutable_buffer value_type;

  /// A random-access iterator type that may be used to read elements.
  typedef const mutable_buffer* const_iterator;

  /// Get a random-access iterator to the first element.
  const_iterator begin() const BOOST_ASIO_NOEXCEPT
  {
    return &buf_;
  }

  /// Get a random-access iterator for one past the last element.
  const_iterator end() const BOOST_ASIO_NOEXCEPT
  {
    return &buf_;
  }

private:
  mutable_buffer buf_;
};

/** @defgroup buffer_sequence_begin boost::asio::buffer_sequence_begin
 *
 * @brief The boost::asio::buffer_sequence_begin function returns an iterator
 * pointing to the first element in a buffer sequence.
 */
/*@{*/

/// Get an iterator to the first element in a buffer sequence.
template <typename MutableBuffer>
inline const mutable_buffer* buffer_sequence_begin(const MutableBuffer& b,
    typename constraint<
      is_convertible<const MutableBuffer*, const mutable_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return static_cast<const mutable_buffer*>(detail::addressof(b));
}

/// Get an iterator to the first element in a buffer sequence.
template <typename ConstBuffer>
inline const const_buffer* buffer_sequence_begin(const ConstBuffer& b,
    typename constraint<
      is_convertible<const ConstBuffer*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return static_cast<const const_buffer*>(detail::addressof(b));
}

#if defined(BOOST_ASIO_HAS_DECLTYPE) || defined(GENERATING_DOCUMENTATION)

/// Get an iterator to the first element in a buffer sequence.
template <typename C>
inline auto buffer_sequence_begin(C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT -> decltype(c.begin())
{
  return c.begin();
}

/// Get an iterator to the first element in a buffer sequence.
template <typename C>
inline auto buffer_sequence_begin(const C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT -> decltype(c.begin())
{
  return c.begin();
}

#else // defined(BOOST_ASIO_HAS_DECLTYPE) || defined(GENERATING_DOCUMENTATION)

template <typename C>
inline typename C::iterator buffer_sequence_begin(C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return c.begin();
}

template <typename C>
inline typename C::const_iterator buffer_sequence_begin(const C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return c.begin();
}

#endif // defined(BOOST_ASIO_HAS_DECLTYPE) || defined(GENERATING_DOCUMENTATION)

/*@}*/

/** @defgroup buffer_sequence_end boost::asio::buffer_sequence_end
 *
 * @brief The boost::asio::buffer_sequence_end function returns an iterator
 * pointing to one past the end element in a buffer sequence.
 */
/*@{*/

/// Get an iterator to one past the end element in a buffer sequence.
template <typename MutableBuffer>
inline const mutable_buffer* buffer_sequence_end(const MutableBuffer& b,
    typename constraint<
      is_convertible<const MutableBuffer*, const mutable_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return static_cast<const mutable_buffer*>(detail::addressof(b)) + 1;
}

/// Get an iterator to one past the end element in a buffer sequence.
template <typename ConstBuffer>
inline const const_buffer* buffer_sequence_end(const ConstBuffer& b,
    typename constraint<
      is_convertible<const ConstBuffer*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return static_cast<const const_buffer*>(detail::addressof(b)) + 1;
}

#if defined(BOOST_ASIO_HAS_DECLTYPE) || defined(GENERATING_DOCUMENTATION)

/// Get an iterator to one past the end element in a buffer sequence.
template <typename C>
inline auto buffer_sequence_end(C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT -> decltype(c.end())
{
  return c.end();
}

/// Get an iterator to one past the end element in a buffer sequence.
template <typename C>
inline auto buffer_sequence_end(const C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT -> decltype(c.end())
{
  return c.end();
}

#else // defined(BOOST_ASIO_HAS_DECLTYPE) || defined(GENERATING_DOCUMENTATION)

template <typename C>
inline typename C::iterator buffer_sequence_end(C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return c.end();
}

template <typename C>
inline typename C::const_iterator buffer_sequence_end(const C& c,
    typename constraint<
      !is_convertible<const C*, const mutable_buffer*>::value
        && !is_convertible<const C*, const const_buffer*>::value
    >::type = 0) BOOST_ASIO_NOEXCEPT
{
  return c.end();
}

#endif // defined(BOOST_ASIO_HAS_DECLTYPE) || defined(GENERATING_DOCUMENTATION)

/*@}*/

namespace detail {

// Tag types used to select appropriately optimised overloads.
struct one_buffer {};
struct multiple_buffers {};

// Helper trait to detect single buffers.
template <typename BufferSequence>
struct buffer_sequence_cardinality :
  conditional<
    is_same<BufferSequence, mutable_buffer>::value
#if !defined(BOOST_ASIO_NO_DEPRECATED)
      || is_same<BufferSequence, mutable_buffers_1>::value
      || is_same<BufferSequence, const_buffers_1>::value
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)
      || is_same<BufferSequence, const_buffer>::value,
    one_buffer, multiple_buffers>::type {};

template <typename Iterator>
inline std::size_t buffer_size(one_buffer,
    Iterator begin, Iterator) BOOST_ASIO_NOEXCEPT
{
  return const_buffer(*begin).size();
}

template <typename Iterator>
inline std::size_t buffer_size(multiple_buffers,
    Iterator begin, Iterator end) BOOST_ASIO_NOEXCEPT
{
  std::size_t total_buffer_size = 0;

  Iterator iter = begin;
  for (; iter != end; ++iter)
  {
    const_buffer b(*iter);
    total_buffer_size += b.size();
  }

  return total_buffer_size;
}

} // namespace detail

/// Get the total number of bytes in a buffer sequence.
/**
 * The @c buffer_size function determines the total size of all buffers in the
 * buffer sequence, as if computed as follows:
 *
 * @code size_t total_size = 0;
 * auto i = boost::asio::buffer_sequence_begin(buffers);
 * auto end = boost::asio::buffer_sequence_end(buffers);
 * for (; i != end; ++i)
 * {
 *   const_buffer b(*i);
 *   total_size += b.size();
 * }
 * return total_size; @endcode
 *
 * The @c BufferSequence template parameter may meet either of the @c
 * ConstBufferSequence or @c MutableBufferSequence type requirements.
 */
template <typename BufferSequence>
inline std::size_t buffer_size(const BufferSequence& b) BOOST_ASIO_NOEXCEPT
{
  return detail::buffer_size(
      detail::buffer_sequence_cardinality<BufferSequence>(),
      boost::asio::buffer_sequence_begin(b),
      boost::asio::buffer_sequence_end(b));
}

#if !defined(BOOST_ASIO_NO_DEPRECATED)

/** @defgroup buffer_cast boost::asio::buffer_cast
 *
 * @brief (Deprecated: Use the @c data() member function.) The
 * boost::asio::buffer_cast function is used to obtain a pointer to the
 * underlying memory region associated with a buffer.
 *
 * @par Examples:
 *
 * To access the memory of a non-modifiable buffer, use:
 * @code boost::asio::const_buffer b1 = ...;
 * const unsigned char* p1 = boost::asio::buffer_cast<const unsigned char*>(b1);
 * @endcode
 *
 * To access the memory of a modifiable buffer, use:
 * @code boost::asio::mutable_buffer b2 = ...;
 * unsigned char* p2 = boost::asio::buffer_cast<unsigned char*>(b2);
 * @endcode
 *
 * The boost::asio::buffer_cast function permits violations of type safety, so
 * uses of it in application code should be carefully considered.
 */
/*@{*/

/// Cast a non-modifiable buffer to a specified pointer to POD type.
template <typename PointerToPodType>
inline PointerToPodType buffer_cast(const mutable_buffer& b) BOOST_ASIO_NOEXCEPT
{
  return static_cast<PointerToPodType>(b.data());
}

/// Cast a non-modifiable buffer to a specified pointer to POD type.
template <typename PointerToPodType>
inline PointerToPodType buffer_cast(const const_buffer& b) BOOST_ASIO_NOEXCEPT
{
  return static_cast<PointerToPodType>(b.data());
}

/*@}*/

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

/// Create a new modifiable buffer that is offset from the start of another.
/**
 * @relates mutable_buffer
 */
inline mutable_buffer operator+(const mutable_buffer& b,
    std::size_t n) BOOST_ASIO_NOEXCEPT
{
  std::size_t offset = n < b.size() ? n : b.size();
  char* new_data = static_cast<char*>(b.data()) + offset;
  std::size_t new_size = b.size() - offset;
  return mutable_buffer(new_data, new_size
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new modifiable buffer that is offset from the start of another.
/**
 * @relates mutable_buffer
 */
inline mutable_buffer operator+(std::size_t n,
    const mutable_buffer& b) BOOST_ASIO_NOEXCEPT
{
  return b + n;
}

/// Create a new non-modifiable buffer that is offset from the start of another.
/**
 * @relates const_buffer
 */
inline const_buffer operator+(const const_buffer& b,
    std::size_t n) BOOST_ASIO_NOEXCEPT
{
  std::size_t offset = n < b.size() ? n : b.size();
  const char* new_data = static_cast<const char*>(b.data()) + offset;
  std::size_t new_size = b.size() - offset;
  return const_buffer(new_data, new_size
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that is offset from the start of another.
/**
 * @relates const_buffer
 */
inline const_buffer operator+(std::size_t n,
    const const_buffer& b) BOOST_ASIO_NOEXCEPT
{
  return b + n;
}

#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
namespace detail {

template <typename Iterator>
class buffer_debug_check
{
public:
  buffer_debug_check(Iterator iter)
    : iter_(iter)
  {
  }

  ~buffer_debug_check()
  {
#if defined(BOOST_ASIO_MSVC) && (BOOST_ASIO_MSVC == 1400)
    // MSVC 8's string iterator checking may crash in a std::string::iterator
    // object's destructor when the iterator points to an already-destroyed
    // std::string object, unless the iterator is cleared first.
    iter_ = Iterator();
#endif // defined(BOOST_ASIO_MSVC) && (BOOST_ASIO_MSVC == 1400)
  }

  void operator()()
  {
    (void)*iter_;
  }

private:
  Iterator iter_;
};

} // namespace detail
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

/** @defgroup buffer boost::asio::buffer
 *
 * @brief The boost::asio::buffer function is used to create a buffer object to
 * represent raw memory, an array of POD elements, a vector of POD elements,
 * or a std::string.
 *
 * A buffer object represents a contiguous region of memory as a 2-tuple
 * consisting of a pointer and size in bytes. A tuple of the form <tt>{void*,
 * size_t}</tt> specifies a mutable (modifiable) region of memory. Similarly, a
 * tuple of the form <tt>{const void*, size_t}</tt> specifies a const
 * (non-modifiable) region of memory. These two forms correspond to the classes
 * mutable_buffer and const_buffer, respectively. To mirror C++'s conversion
 * rules, a mutable_buffer is implicitly convertible to a const_buffer, and the
 * opposite conversion is not permitted.
 *
 * The simplest use case involves reading or writing a single buffer of a
 * specified size:
 *
 * @code sock.send(boost::asio::buffer(data, size)); @endcode
 *
 * In the above example, the return value of boost::asio::buffer meets the
 * requirements of the ConstBufferSequence concept so that it may be directly
 * passed to the socket's write function. A buffer created for modifiable
 * memory also meets the requirements of the MutableBufferSequence concept.
 *
 * An individual buffer may be created from a builtin array, std::vector,
 * std::array or boost::array of POD elements. This helps prevent buffer
 * overruns by automatically determining the size of the buffer:
 *
 * @code char d1[128];
 * size_t bytes_transferred = sock.receive(boost::asio::buffer(d1));
 *
 * std::vector<char> d2(128);
 * bytes_transferred = sock.receive(boost::asio::buffer(d2));
 *
 * std::array<char, 128> d3;
 * bytes_transferred = sock.receive(boost::asio::buffer(d3));
 *
 * boost::array<char, 128> d4;
 * bytes_transferred = sock.receive(boost::asio::buffer(d4)); @endcode
 *
 * In all three cases above, the buffers created are exactly 128 bytes long.
 * Note that a vector is @e never automatically resized when creating or using
 * a buffer. The buffer size is determined using the vector's <tt>size()</tt>
 * member function, and not its capacity.
 *
 * @par Accessing Buffer Contents
 *
 * The contents of a buffer may be accessed using the @c data() and @c size()
 * member functions:
 *
 * @code boost::asio::mutable_buffer b1 = ...;
 * std::size_t s1 = b1.size();
 * unsigned char* p1 = static_cast<unsigned char*>(b1.data());
 *
 * boost::asio::const_buffer b2 = ...;
 * std::size_t s2 = b2.size();
 * const void* p2 = b2.data(); @endcode
 *
 * The @c data() member function permits violations of type safety, so
 * uses of it in application code should be carefully considered.
 *
 * For convenience, a @ref buffer_size function is provided that works with
 * both buffers and buffer sequences (that is, types meeting the
 * ConstBufferSequence or MutableBufferSequence type requirements). In this
 * case, the function returns the total size of all buffers in the sequence.
 *
 * @par Buffer Copying
 *
 * The @ref buffer_copy function may be used to copy raw bytes between
 * individual buffers and buffer sequences.
*
 * In particular, when used with the @ref buffer_size function, the @ref
 * buffer_copy function can be used to linearise a sequence of buffers. For
 * example:
 *
 * @code vector<const_buffer> buffers = ...;
 *
 * vector<unsigned char> data(boost::asio::buffer_size(buffers));
 * boost::asio::buffer_copy(boost::asio::buffer(data), buffers); @endcode
 *
 * Note that @ref buffer_copy is implemented in terms of @c memcpy, and
 * consequently it cannot be used to copy between overlapping memory regions.
 *
 * @par Buffer Invalidation
 *
 * A buffer object does not have any ownership of the memory it refers to. It
 * is the responsibility of the application to ensure the memory region remains
 * valid until it is no longer required for an I/O operation. When the memory
 * is no longer available, the buffer is said to have been invalidated.
 *
 * For the boost::asio::buffer overloads that accept an argument of type
 * std::vector, the buffer objects returned are invalidated by any vector
 * operation that also invalidates all references, pointers and iterators
 * referring to the elements in the sequence (C++ Std, 23.2.4)
 *
 * For the boost::asio::buffer overloads that accept an argument of type
 * std::basic_string, the buffer objects returned are invalidated according to
 * the rules defined for invalidation of references, pointers and iterators
 * referring to elements of the sequence (C++ Std, 21.3).
 *
 * @par Buffer Arithmetic
 *
 * Buffer objects may be manipulated using simple arithmetic in a safe way
 * which helps prevent buffer overruns. Consider an array initialised as
 * follows:
 *
 * @code boost::array<char, 6> a = { 'a', 'b', 'c', 'd', 'e' }; @endcode
 *
 * A buffer object @c b1 created using:
 *
 * @code b1 = boost::asio::buffer(a); @endcode
 *
 * represents the entire array, <tt>{ 'a', 'b', 'c', 'd', 'e' }</tt>. An
 * optional second argument to the boost::asio::buffer function may be used to
 * limit the size, in bytes, of the buffer:
 *
 * @code b2 = boost::asio::buffer(a, 3); @endcode
 *
 * such that @c b2 represents the data <tt>{ 'a', 'b', 'c' }</tt>. Even if the
 * size argument exceeds the actual size of the array, the size of the buffer
 * object created will be limited to the array size.
 *
 * An offset may be applied to an existing buffer to create a new one:
 *
 * @code b3 = b1 + 2; @endcode
 *
 * where @c b3 will set to represent <tt>{ 'c', 'd', 'e' }</tt>. If the offset
 * exceeds the size of the existing buffer, the newly created buffer will be
 * empty.
 *
 * Both an offset and size may be specified to create a buffer that corresponds
 * to a specific range of bytes within an existing buffer:
 *
 * @code b4 = boost::asio::buffer(b1 + 1, 3); @endcode
 *
 * so that @c b4 will refer to the bytes <tt>{ 'b', 'c', 'd' }</tt>.
 *
 * @par Buffers and Scatter-Gather I/O
 *
 * To read or write using multiple buffers (i.e. scatter-gather I/O), multiple
 * buffer objects may be assigned into a container that supports the
 * MutableBufferSequence (for read) or ConstBufferSequence (for write) concepts:
 *
 * @code
 * char d1[128];
 * std::vector<char> d2(128);
 * boost::array<char, 128> d3;
 *
 * boost::array<mutable_buffer, 3> bufs1 = {
 *   boost::asio::buffer(d1),
 *   boost::asio::buffer(d2),
 *   boost::asio::buffer(d3) };
 * bytes_transferred = sock.receive(bufs1);
 *
 * std::vector<const_buffer> bufs2;
 * bufs2.push_back(boost::asio::buffer(d1));
 * bufs2.push_back(boost::asio::buffer(d2));
 * bufs2.push_back(boost::asio::buffer(d3));
 * bytes_transferred = sock.send(bufs2); @endcode
 */
/*@{*/

#if defined(BOOST_ASIO_NO_DEPRECATED) || defined(GENERATING_DOCUMENTATION)
# define BOOST_ASIO_MUTABLE_BUFFER mutable_buffer
# define BOOST_ASIO_CONST_BUFFER const_buffer
#else // defined(BOOST_ASIO_NO_DEPRECATED) || defined(GENERATING_DOCUMENTATION)
# define BOOST_ASIO_MUTABLE_BUFFER mutable_buffers_1
# define BOOST_ASIO_CONST_BUFFER const_buffers_1
#endif // defined(BOOST_ASIO_NO_DEPRECATED) || defined(GENERATING_DOCUMENTATION)

/// Create a new modifiable buffer from an existing buffer.
/**
 * @returns <tt>mutable_buffer(b)</tt>.
 */
inline BOOST_ASIO_MUTABLE_BUFFER buffer(
    const mutable_buffer& b) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(b);
}

/// Create a new modifiable buffer from an existing buffer.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     b.data(),
 *     min(b.size(), max_size_in_bytes)); @endcode
 */
inline BOOST_ASIO_MUTABLE_BUFFER buffer(const mutable_buffer& b,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(
      mutable_buffer(b.data(),
        b.size() < max_size_in_bytes
        ? b.size() : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
        , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
        ));
}

/// Create a new non-modifiable buffer from an existing buffer.
/**
 * @returns <tt>const_buffer(b)</tt>.
 */
inline BOOST_ASIO_CONST_BUFFER buffer(
    const const_buffer& b) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(b);
}

/// Create a new non-modifiable buffer from an existing buffer.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     b.data(),
 *     min(b.size(), max_size_in_bytes)); @endcode
 */
inline BOOST_ASIO_CONST_BUFFER buffer(const const_buffer& b,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(b.data(),
      b.size() < max_size_in_bytes
      ? b.size() : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , b.get_debug_check()
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new modifiable buffer that represents the given memory range.
/**
 * @returns <tt>mutable_buffer(data, size_in_bytes)</tt>.
 */
inline BOOST_ASIO_MUTABLE_BUFFER buffer(void* data,
    std::size_t size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data, size_in_bytes);
}

/// Create a new non-modifiable buffer that represents the given memory range.
/**
 * @returns <tt>const_buffer(data, size_in_bytes)</tt>.
 */
inline BOOST_ASIO_CONST_BUFFER buffer(const void* data,
    std::size_t size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data, size_in_bytes);
}

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     static_cast<void*>(data),
 *     N * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(PodType (&data)[N]) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data, N * sizeof(PodType));
}
 
/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     static_cast<void*>(data),
 *     min(N * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(PodType (&data)[N],
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data,
      N * sizeof(PodType) < max_size_in_bytes
      ? N * sizeof(PodType) : max_size_in_bytes);
}
 
/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     static_cast<const void*>(data),
 *     N * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const PodType (&data)[N]) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data, N * sizeof(PodType));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     static_cast<const void*>(data),
 *     min(N * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(const PodType (&data)[N],
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data,
      N * sizeof(PodType) < max_size_in_bytes
      ? N * sizeof(PodType) : max_size_in_bytes);
}

#if defined(BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND)

// Borland C++ and Sun Studio think the overloads:
//
//   unspecified buffer(boost::array<PodType, N>& array ...);
//
// and
//
//   unspecified buffer(boost::array<const PodType, N>& array ...);
//
// are ambiguous. This will be worked around by using a buffer_types traits
// class that contains typedefs for the appropriate buffer and container
// classes, based on whether PodType is const or non-const.

namespace detail {

template <bool IsConst>
struct buffer_types_base;

template <>
struct buffer_types_base<false>
{
  typedef mutable_buffer buffer_type;
  typedef BOOST_ASIO_MUTABLE_BUFFER container_type;
};

template <>
struct buffer_types_base<true>
{
  typedef const_buffer buffer_type;
  typedef BOOST_ASIO_CONST_BUFFER container_type;
};

template <typename PodType>
struct buffer_types
  : public buffer_types_base<is_const<PodType>::value>
{
};

} // namespace detail

template <typename PodType, std::size_t N>
inline typename detail::buffer_types<PodType>::container_type
buffer(boost::array<PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  typedef typename boost::asio::detail::buffer_types<PodType>::buffer_type
    buffer_type;
  typedef typename boost::asio::detail::buffer_types<PodType>::container_type
    container_type;
  return container_type(
      buffer_type(data.c_array(), data.size() * sizeof(PodType)));
}

template <typename PodType, std::size_t N>
inline typename detail::buffer_types<PodType>::container_type
buffer(boost::array<PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  typedef typename boost::asio::detail::buffer_types<PodType>::buffer_type
    buffer_type;
  typedef typename boost::asio::detail::buffer_types<PodType>::container_type
    container_type;
  return container_type(
      buffer_type(data.c_array(),
        data.size() * sizeof(PodType) < max_size_in_bytes
        ? data.size() * sizeof(PodType) : max_size_in_bytes));
}

#else // defined(BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND)

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(
    boost::array<PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(
      data.c_array(), data.size() * sizeof(PodType));
}

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(boost::array<PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data.c_array(),
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes);
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(
    boost::array<const PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(), data.size() * sizeof(PodType));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(boost::array<const PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(),
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes);
}

#endif // defined(BOOST_ASIO_ENABLE_ARRAY_BUFFER_WORKAROUND)

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const boost::array<PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(), data.size() * sizeof(PodType));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(const boost::array<PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(),
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes);
}

#if defined(BOOST_ASIO_HAS_STD_ARRAY) || defined(GENERATING_DOCUMENTATION)

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(
    std::array<PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data.data(), data.size() * sizeof(PodType));
}

/// Create a new modifiable buffer that represents the given POD array.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(std::array<PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data.data(),
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes);
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(
    std::array<const PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(), data.size() * sizeof(PodType));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(std::array<const PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(),
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes);
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     data.size() * sizeof(PodType)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const std::array<PodType, N>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(), data.size() * sizeof(PodType));
}

/// Create a new non-modifiable buffer that represents the given POD array.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 */
template <typename PodType, std::size_t N>
inline BOOST_ASIO_CONST_BUFFER buffer(const std::array<PodType, N>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(),
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes);
}

#endif // defined(BOOST_ASIO_HAS_STD_ARRAY) || defined(GENERATING_DOCUMENTATION)

/// Create a new modifiable buffer that represents the given POD vector.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.size() ? &data[0] : 0,
 *     data.size() * sizeof(PodType)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(
    std::vector<PodType, Allocator>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(
      data.size() ? &data[0] : 0, data.size() * sizeof(PodType)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::vector<PodType, Allocator>::iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new modifiable buffer that represents the given POD vector.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.size() ? &data[0] : 0,
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(std::vector<PodType, Allocator>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data.size() ? &data[0] : 0,
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::vector<PodType, Allocator>::iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that represents the given POD vector.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.size() ? &data[0] : 0,
 *     data.size() * sizeof(PodType)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const std::vector<PodType, Allocator>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(
      data.size() ? &data[0] : 0, data.size() * sizeof(PodType)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::vector<PodType, Allocator>::const_iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that represents the given POD vector.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.size() ? &data[0] : 0,
 *     min(data.size() * sizeof(PodType), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any vector operation that would also
 * invalidate iterators.
 */
template <typename PodType, typename Allocator>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const std::vector<PodType, Allocator>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.size() ? &data[0] : 0,
      data.size() * sizeof(PodType) < max_size_in_bytes
      ? data.size() * sizeof(PodType) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::vector<PodType, Allocator>::const_iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new modifiable buffer that represents the given string.
/**
 * @returns <tt>mutable_buffer(data.size() ? &data[0] : 0,
 * data.size() * sizeof(Elem))</tt>.
 *
 * @note The buffer is invalidated by any non-const operation called on the
 * given string object.
 */
template <typename Elem, typename Traits, typename Allocator>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(
    std::basic_string<Elem, Traits, Allocator>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data.size() ? &data[0] : 0,
      data.size() * sizeof(Elem)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::basic_string<Elem, Traits, Allocator>::iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new modifiable buffer that represents the given string.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.size() ? &data[0] : 0,
 *     min(data.size() * sizeof(Elem), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any non-const operation called on the
 * given string object.
 */
template <typename Elem, typename Traits, typename Allocator>
inline BOOST_ASIO_MUTABLE_BUFFER buffer(
    std::basic_string<Elem, Traits, Allocator>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_MUTABLE_BUFFER(data.size() ? &data[0] : 0,
      data.size() * sizeof(Elem) < max_size_in_bytes
      ? data.size() * sizeof(Elem) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::basic_string<Elem, Traits, Allocator>::iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that represents the given string.
/**
 * @returns <tt>const_buffer(data.data(), data.size() * sizeof(Elem))</tt>.
 *
 * @note The buffer is invalidated by any non-const operation called on the
 * given string object.
 */
template <typename Elem, typename Traits, typename Allocator>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const std::basic_string<Elem, Traits, Allocator>& data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(), data.size() * sizeof(Elem)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::basic_string<Elem, Traits, Allocator>::const_iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that represents the given string.
/**
 * @returns A const_buffer value equivalent to:
 * @code const_buffer(
 *     data.data(),
 *     min(data.size() * sizeof(Elem), max_size_in_bytes)); @endcode
 *
 * @note The buffer is invalidated by any non-const operation called on the
 * given string object.
 */
template <typename Elem, typename Traits, typename Allocator>
inline BOOST_ASIO_CONST_BUFFER buffer(
    const std::basic_string<Elem, Traits, Allocator>& data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.data(),
      data.size() * sizeof(Elem) < max_size_in_bytes
      ? data.size() * sizeof(Elem) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename std::basic_string<Elem, Traits, Allocator>::const_iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

#if defined(BOOST_ASIO_HAS_STRING_VIEW) \
  || defined(GENERATING_DOCUMENTATION)

/// Create a new modifiable buffer that represents the given string_view.
/**
 * @returns <tt>mutable_buffer(data.size() ? &data[0] : 0,
 * data.size() * sizeof(Elem))</tt>.
 */
template <typename Elem, typename Traits>
inline BOOST_ASIO_CONST_BUFFER buffer(
    basic_string_view<Elem, Traits> data) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.size() ? &data[0] : 0,
      data.size() * sizeof(Elem)
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename basic_string_view<Elem, Traits>::iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

/// Create a new non-modifiable buffer that represents the given string.
/**
 * @returns A mutable_buffer value equivalent to:
 * @code mutable_buffer(
 *     data.size() ? &data[0] : 0,
 *     min(data.size() * sizeof(Elem), max_size_in_bytes)); @endcode
 */
template <typename Elem, typename Traits>
inline BOOST_ASIO_CONST_BUFFER buffer(
    basic_string_view<Elem, Traits> data,
    std::size_t max_size_in_bytes) BOOST_ASIO_NOEXCEPT
{
  return BOOST_ASIO_CONST_BUFFER(data.size() ? &data[0] : 0,
      data.size() * sizeof(Elem) < max_size_in_bytes
      ? data.size() * sizeof(Elem) : max_size_in_bytes
#if defined(BOOST_ASIO_ENABLE_BUFFER_DEBUGGING)
      , detail::buffer_debug_check<
          typename basic_string_view<Elem, Traits>::iterator
        >(data.begin())
#endif // BOOST_ASIO_ENABLE_BUFFER_DEBUGGING
      );
}

#endif // defined(BOOST_ASIO_HAS_STRING_VIEW)
       //  || defined(GENERATING_DOCUMENTATION)

/*@}*/

/// Adapt a basic_string to the DynamicBuffer requirements.
/**
 * Requires that <tt>sizeof(Elem) == 1</tt>.
 */
template <typename Elem, typename Traits, typename Allocator>
class dynamic_string_buffer
{
public:
  /// The type used to represent a sequence of constant buffers that refers to
  /// the underlying memory.
  typedef BOOST_ASIO_CONST_BUFFER const_buffers_type;

  /// The type used to represent a sequence of mutable buffers that refers to
  /// the underlying memory.
  typedef BOOST_ASIO_MUTABLE_BUFFER mutable_buffers_type;

  /// Construct a dynamic buffer from a string.
  /**
   * @param s The string to be used as backing storage for the dynamic buffer.
   * The object stores a reference to the string and the user is responsible
   * for ensuring that the string object remains valid while the
   * dynamic_string_buffer object, and copies of the object, are in use.
   *
   * @b DynamicBuffer_v1: Any existing data in the string is treated as the
   * dynamic buffer's input sequence.
   *
   * @param maximum_size Specifies a maximum size for the buffer, in bytes.
   */
  explicit dynamic_string_buffer(std::basic_string<Elem, Traits, Allocator>& s,
      std::size_t maximum_size =
        (std::numeric_limits<std::size_t>::max)()) BOOST_ASIO_NOEXCEPT
    : string_(s),
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      size_((std::numeric_limits<std::size_t>::max)()),
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      max_size_(maximum_size)
  {
  }

  /// @b DynamicBuffer_v2: Copy construct a dynamic buffer.
  dynamic_string_buffer(const dynamic_string_buffer& other) BOOST_ASIO_NOEXCEPT
    : string_(other.string_),
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      size_(other.size_),
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      max_size_(other.max_size_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move construct a dynamic buffer.
  dynamic_string_buffer(dynamic_string_buffer&& other) BOOST_ASIO_NOEXCEPT
    : string_(other.string_),
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      size_(other.size_),
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      max_size_(other.max_size_)
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// @b DynamicBuffer_v1: Get the size of the input sequence.
  /// @b DynamicBuffer_v2: Get the current size of the underlying memory.
  /**
   * @returns @b DynamicBuffer_v1 The current size of the input sequence.
   * @b DynamicBuffer_v2: The current size of the underlying string if less than
   * max_size(). Otherwise returns max_size().
   */
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    if (size_ != (std::numeric_limits<std::size_t>::max)())
      return size_;
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    return (std::min)(string_.size(), max_size());
  }

  /// Get the maximum size of the dynamic buffer.
  /**
   * @returns The allowed maximum size of the underlying memory.
   */
  std::size_t max_size() const BOOST_ASIO_NOEXCEPT
  {
    return max_size_;
  }

  /// Get the maximum size that the buffer may grow to without triggering
  /// reallocation.
  /**
   * @returns The current capacity of the underlying string if less than
   * max_size(). Otherwise returns max_size().
   */
  std::size_t capacity() const BOOST_ASIO_NOEXCEPT
  {
    return (std::min)(string_.capacity(), max_size());
  }

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  /// @b DynamicBuffer_v1: Get a list of buffers that represents the input
  /// sequence.
  /**
   * @returns An object of type @c const_buffers_type that satisfies
   * ConstBufferSequence requirements, representing the basic_string memory in
   * the input sequence.
   *
   * @note The returned object is invalidated by any @c dynamic_string_buffer
   * or @c basic_string member function that resizes or erases the string.
   */
  const_buffers_type data() const BOOST_ASIO_NOEXCEPT
  {
    return const_buffers_type(boost::asio::buffer(string_, size_));
  }
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

  /// @b DynamicBuffer_v2: Get a sequence of buffers that represents the
  /// underlying memory.
  /**
   * @param pos Position of the first byte to represent in the buffer sequence
   *
   * @param n The number of bytes to return in the buffer sequence. If the
   * underlying memory is shorter, the buffer sequence represents as many bytes
   * as are available.
   *
   * @returns An object of type @c mutable_buffers_type that satisfies
   * MutableBufferSequence requirements, representing the basic_string memory.
   *
   * @note The returned object is invalidated by any @c dynamic_string_buffer
   * or @c basic_string member function that resizes or erases the string.
   */
  mutable_buffers_type data(std::size_t pos, std::size_t n) BOOST_ASIO_NOEXCEPT
  {
    return mutable_buffers_type(boost::asio::buffer(
          boost::asio::buffer(string_, max_size_) + pos, n));
  }

  /// @b DynamicBuffer_v2: Get a sequence of buffers that represents the
  /// underlying memory.
  /**
   * @param pos Position of the first byte to represent in the buffer sequence
   *
   * @param n The number of bytes to return in the buffer sequence. If the
   * underlying memory is shorter, the buffer sequence represents as many bytes
   * as are available.
   *
   * @note The returned object is invalidated by any @c dynamic_string_buffer
   * or @c basic_string member function that resizes or erases the string.
   */
  const_buffers_type data(std::size_t pos,
      std::size_t n) const BOOST_ASIO_NOEXCEPT
  {
    return const_buffers_type(boost::asio::buffer(
          boost::asio::buffer(string_, max_size_) + pos, n));
  }

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  /// @b DynamicBuffer_v1: Get a list of buffers that represents the output
  /// sequence, with the given size.
  /**
   * Ensures that the output sequence can accommodate @c n bytes, resizing the
   * basic_string object as necessary.
   *
   * @returns An object of type @c mutable_buffers_type that satisfies
   * MutableBufferSequence requirements, representing basic_string memory
   * at the start of the output sequence of size @c n.
   *
   * @throws std::length_error If <tt>size() + n > max_size()</tt>.
   *
   * @note The returned object is invalidated by any @c dynamic_string_buffer
   * or @c basic_string member function that modifies the input sequence or
   * output sequence.
   */
  mutable_buffers_type prepare(std::size_t n)
  {
    if (size() > max_size() || max_size() - size() < n)
    {
      std::length_error ex("dynamic_string_buffer too long");
      boost::asio::detail::throw_exception(ex);
    }

    if (size_ == (std::numeric_limits<std::size_t>::max)())
      size_ = string_.size(); // Enable v1 behaviour.

    string_.resize(size_ + n);

    return boost::asio::buffer(boost::asio::buffer(string_) + size_, n);
  }

  /// @b DynamicBuffer_v1: Move bytes from the output sequence to the input
  /// sequence.
  /**
   * @param n The number of bytes to append from the start of the output
   * sequence to the end of the input sequence. The remainder of the output
   * sequence is discarded.
   *
   * Requires a preceding call <tt>prepare(x)</tt> where <tt>x >= n</tt>, and
   * no intervening operations that modify the input or output sequence.
   *
   * @note If @c n is greater than the size of the output sequence, the entire
   * output sequence is moved to the input sequence and no error is issued.
   */
  void commit(std::size_t n)
  {
    size_ += (std::min)(n, string_.size() - size_);
    string_.resize(size_);
  }
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

  /// @b DynamicBuffer_v2: Grow the underlying memory by the specified number of
  /// bytes.
  /**
   * Resizes the string to accommodate an additional @c n bytes at the end.
   *
   * @throws std::length_error If <tt>size() + n > max_size()</tt>.
   */
  void grow(std::size_t n)
  {
    if (size() > max_size() || max_size() - size() < n)
    {
      std::length_error ex("dynamic_string_buffer too long");
      boost::asio::detail::throw_exception(ex);
    }

    string_.resize(size() + n);
  }

  /// @b DynamicBuffer_v2: Shrink the underlying memory by the specified number
  /// of bytes.
  /**
   * Erases @c n bytes from the end of the string by resizing the basic_string
   * object. If @c n is greater than the current size of the string, the string
   * is emptied.
   */
  void shrink(std::size_t n)
  {
    string_.resize(n > size() ? 0 : size() - n);
  }

  /// @b DynamicBuffer_v1: Remove characters from the input sequence.
  /// @b DynamicBuffer_v2: Consume the specified number of bytes from the
  /// beginning of the underlying memory.
  /**
   * @b DynamicBuffer_v1: Removes @c n characters from the beginning of the
   * input sequence. @note If @c n is greater than the size of the input
   * sequence, the entire input sequence is consumed and no error is issued.
   *
   * @b DynamicBuffer_v2: Erases @c n bytes from the beginning of the string.
   * If @c n is greater than the current size of the string, the string is
   * emptied.
   */
  void consume(std::size_t n)
  {
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    if (size_ != (std::numeric_limits<std::size_t>::max)())
    {
      std::size_t consume_length = (std::min)(n, size_);
      string_.erase(0, consume_length);
      size_ -= consume_length;
      return;
    }
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    string_.erase(0, n);
  }

private:
  std::basic_string<Elem, Traits, Allocator>& string_;
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  std::size_t size_;
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  const std::size_t max_size_;
};

/// Adapt a vector to the DynamicBuffer requirements.
/**
 * Requires that <tt>sizeof(Elem) == 1</tt>.
 */
template <typename Elem, typename Allocator>
class dynamic_vector_buffer
{
public:
  /// The type used to represent a sequence of constant buffers that refers to
  /// the underlying memory.
  typedef BOOST_ASIO_CONST_BUFFER const_buffers_type;

  /// The type used to represent a sequence of mutable buffers that refers to
  /// the underlying memory.
  typedef BOOST_ASIO_MUTABLE_BUFFER mutable_buffers_type;

  /// Construct a dynamic buffer from a vector.
  /**
   * @param v The vector to be used as backing storage for the dynamic buffer.
   * The object stores a reference to the vector and the user is responsible
   * for ensuring that the vector object remains valid while the
   * dynamic_vector_buffer object, and copies of the object, are in use.
   *
   * @param maximum_size Specifies a maximum size for the buffer, in bytes.
   */
  explicit dynamic_vector_buffer(std::vector<Elem, Allocator>& v,
      std::size_t maximum_size =
        (std::numeric_limits<std::size_t>::max)()) BOOST_ASIO_NOEXCEPT
    : vector_(v),
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      size_((std::numeric_limits<std::size_t>::max)()),
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      max_size_(maximum_size)
  {
  }

  /// @b DynamicBuffer_v2: Copy construct a dynamic buffer.
  dynamic_vector_buffer(const dynamic_vector_buffer& other) BOOST_ASIO_NOEXCEPT
    : vector_(other.vector_),
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      size_(other.size_),
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      max_size_(other.max_size_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move construct a dynamic buffer.
  dynamic_vector_buffer(dynamic_vector_buffer&& other) BOOST_ASIO_NOEXCEPT
    : vector_(other.vector_),
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      size_(other.size_),
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
      max_size_(other.max_size_)
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// @b DynamicBuffer_v1: Get the size of the input sequence.
  /// @b DynamicBuffer_v2: Get the current size of the underlying memory.
  /**
   * @returns @b DynamicBuffer_v1 The current size of the input sequence.
   * @b DynamicBuffer_v2: The current size of the underlying vector if less than
   * max_size(). Otherwise returns max_size().
   */
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    if (size_ != (std::numeric_limits<std::size_t>::max)())
      return size_;
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    return (std::min)(vector_.size(), max_size());
  }

  /// Get the maximum size of the dynamic buffer.
  /**
   * @returns @b DynamicBuffer_v1: The allowed maximum of the sum of the sizes
   * of the input sequence and output sequence. @b DynamicBuffer_v2: The allowed
   * maximum size of the underlying memory.
   */
  std::size_t max_size() const BOOST_ASIO_NOEXCEPT
  {
    return max_size_;
  }

  /// Get the maximum size that the buffer may grow to without triggering
  /// reallocation.
  /**
   * @returns @b DynamicBuffer_v1: The current total capacity of the buffer,
   * i.e. for both the input sequence and output sequence. @b DynamicBuffer_v2:
   * The current capacity of the underlying vector if less than max_size().
   * Otherwise returns max_size().
   */
  std::size_t capacity() const BOOST_ASIO_NOEXCEPT
  {
    return (std::min)(vector_.capacity(), max_size());
  }

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  /// @b DynamicBuffer_v1: Get a list of buffers that represents the input
  /// sequence.
  /**
   * @returns An object of type @c const_buffers_type that satisfies
   * ConstBufferSequence requirements, representing the vector memory in the
   * input sequence.
   *
   * @note The returned object is invalidated by any @c dynamic_vector_buffer
   * or @c vector member function that modifies the input sequence or output
   * sequence.
   */
  const_buffers_type data() const BOOST_ASIO_NOEXCEPT
  {
    return const_buffers_type(boost::asio::buffer(vector_, size_));
  }
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

  /// @b DynamicBuffer_v2: Get a sequence of buffers that represents the
  /// underlying memory.
  /**
   * @param pos Position of the first byte to represent in the buffer sequence
   *
   * @param n The number of bytes to return in the buffer sequence. If the
   * underlying memory is shorter, the buffer sequence represents as many bytes
   * as are available.
   *
   * @returns An object of type @c mutable_buffers_type that satisfies
   * MutableBufferSequence requirements, representing the vector memory.
   *
   * @note The returned object is invalidated by any @c dynamic_vector_buffer
   * or @c vector member function that resizes or erases the vector.
   */
  mutable_buffers_type data(std::size_t pos, std::size_t n) BOOST_ASIO_NOEXCEPT
  {
    return mutable_buffers_type(boost::asio::buffer(
          boost::asio::buffer(vector_, max_size_) + pos, n));
  }

  /// @b DynamicBuffer_v2: Get a sequence of buffers that represents the
  /// underlying memory.
  /**
   * @param pos Position of the first byte to represent in the buffer sequence
   *
   * @param n The number of bytes to return in the buffer sequence. If the
   * underlying memory is shorter, the buffer sequence represents as many bytes
   * as are available.
   *
   * @note The returned object is invalidated by any @c dynamic_vector_buffer
   * or @c vector member function that resizes or erases the vector.
   */
  const_buffers_type data(std::size_t pos,
      std::size_t n) const BOOST_ASIO_NOEXCEPT
  {
    return const_buffers_type(boost::asio::buffer(
          boost::asio::buffer(vector_, max_size_) + pos, n));
  }

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  /// @b DynamicBuffer_v1: Get a list of buffers that represents the output
  /// sequence, with the given size.
  /**
   * Ensures that the output sequence can accommodate @c n bytes, resizing the
   * vector object as necessary.
   *
   * @returns An object of type @c mutable_buffers_type that satisfies
   * MutableBufferSequence requirements, representing vector memory at the
   * start of the output sequence of size @c n.
   *
   * @throws std::length_error If <tt>size() + n > max_size()</tt>.
   *
   * @note The returned object is invalidated by any @c dynamic_vector_buffer
   * or @c vector member function that modifies the input sequence or output
   * sequence.
   */
  mutable_buffers_type prepare(std::size_t n)
  {
    if (size () > max_size() || max_size() - size() < n)
    {
      std::length_error ex("dynamic_vector_buffer too long");
      boost::asio::detail::throw_exception(ex);
    }

    if (size_ == (std::numeric_limits<std::size_t>::max)())
      size_ = vector_.size(); // Enable v1 behaviour.

    vector_.resize(size_ + n);

    return boost::asio::buffer(boost::asio::buffer(vector_) + size_, n);
  }

  /// @b DynamicBuffer_v1: Move bytes from the output sequence to the input
  /// sequence.
  /**
   * @param n The number of bytes to append from the start of the output
   * sequence to the end of the input sequence. The remainder of the output
   * sequence is discarded.
   *
   * Requires a preceding call <tt>prepare(x)</tt> where <tt>x >= n</tt>, and
   * no intervening operations that modify the input or output sequence.
   *
   * @note If @c n is greater than the size of the output sequence, the entire
   * output sequence is moved to the input sequence and no error is issued.
   */
  void commit(std::size_t n)
  {
    size_ += (std::min)(n, vector_.size() - size_);
    vector_.resize(size_);
  }
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

  /// @b DynamicBuffer_v2: Grow the underlying memory by the specified number of
  /// bytes.
  /**
   * Resizes the vector to accommodate an additional @c n bytes at the end.
   *
   * @throws std::length_error If <tt>size() + n > max_size()</tt>.
   */
  void grow(std::size_t n)
  {
    if (size() > max_size() || max_size() - size() < n)
    {
      std::length_error ex("dynamic_vector_buffer too long");
      boost::asio::detail::throw_exception(ex);
    }

    vector_.resize(size() + n);
  }

  /// @b DynamicBuffer_v2: Shrink the underlying memory by the specified number
  /// of bytes.
  /**
   * Erases @c n bytes from the end of the vector by resizing the vector
   * object. If @c n is greater than the current size of the vector, the vector
   * is emptied.
   */
  void shrink(std::size_t n)
  {
    vector_.resize(n > size() ? 0 : size() - n);
  }

  /// @b DynamicBuffer_v1: Remove characters from the input sequence.
  /// @b DynamicBuffer_v2: Consume the specified number of bytes from the
  /// beginning of the underlying memory.
  /**
   * @b DynamicBuffer_v1: Removes @c n characters from the beginning of the
   * input sequence. @note If @c n is greater than the size of the input
   * sequence, the entire input sequence is consumed and no error is issued.
   *
   * @b DynamicBuffer_v2: Erases @c n bytes from the beginning of the vector.
   * If @c n is greater than the current size of the vector, the vector is
   * emptied.
   */
  void consume(std::size_t n)
  {
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    if (size_ != (std::numeric_limits<std::size_t>::max)())
    {
      std::size_t consume_length = (std::min)(n, size_);
      vector_.erase(vector_.begin(), vector_.begin() + consume_length);
      size_ -= consume_length;
      return;
    }
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
    vector_.erase(vector_.begin(), vector_.begin() + (std::min)(size(), n));
  }

private:
  std::vector<Elem, Allocator>& vector_;
#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  std::size_t size_;
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  const std::size_t max_size_;
};

/** @defgroup dynamic_buffer boost::asio::dynamic_buffer
 *
 * @brief The boost::asio::dynamic_buffer function is used to create a
 * dynamically resized buffer from a @c std::basic_string or @c std::vector.
 */
/*@{*/

/// Create a new dynamic buffer that represents the given string.
/**
 * @returns <tt>dynamic_string_buffer<Elem, Traits, Allocator>(data)</tt>.
 */
template <typename Elem, typename Traits, typename Allocator>
inline dynamic_string_buffer<Elem, Traits, Allocator> dynamic_buffer(
    std::basic_string<Elem, Traits, Allocator>& data) BOOST_ASIO_NOEXCEPT
{
  return dynamic_string_buffer<Elem, Traits, Allocator>(data);
}

/// Create a new dynamic buffer that represents the given string.
/**
 * @returns <tt>dynamic_string_buffer<Elem, Traits, Allocator>(data,
 * max_size)</tt>.
 */
template <typename Elem, typename Traits, typename Allocator>
inline dynamic_string_buffer<Elem, Traits, Allocator> dynamic_buffer(
    std::basic_string<Elem, Traits, Allocator>& data,
    std::size_t max_size) BOOST_ASIO_NOEXCEPT
{
  return dynamic_string_buffer<Elem, Traits, Allocator>(data, max_size);
}

/// Create a new dynamic buffer that represents the given vector.
/**
 * @returns <tt>dynamic_vector_buffer<Elem, Allocator>(data)</tt>.
 */
template <typename Elem, typename Allocator>
inline dynamic_vector_buffer<Elem, Allocator> dynamic_buffer(
    std::vector<Elem, Allocator>& data) BOOST_ASIO_NOEXCEPT
{
  return dynamic_vector_buffer<Elem, Allocator>(data);
}

/// Create a new dynamic buffer that represents the given vector.
/**
 * @returns <tt>dynamic_vector_buffer<Elem, Allocator>(data, max_size)</tt>.
 */
template <typename Elem, typename Allocator>
inline dynamic_vector_buffer<Elem, Allocator> dynamic_buffer(
    std::vector<Elem, Allocator>& data,
    std::size_t max_size) BOOST_ASIO_NOEXCEPT
{
  return dynamic_vector_buffer<Elem, Allocator>(data, max_size);
}

/*@}*/

/** @defgroup buffer_copy boost::asio::buffer_copy
 *
 * @brief The boost::asio::buffer_copy function is used to copy bytes from a
 * source buffer (or buffer sequence) to a target buffer (or buffer sequence).
 *
 * The @c buffer_copy function is available in two forms:
 *
 * @li A 2-argument form: @c buffer_copy(target, source)
 *
 * @li A 3-argument form: @c buffer_copy(target, source, max_bytes_to_copy)
 *
 * Both forms return the number of bytes actually copied. The number of bytes
 * copied is the lesser of:
 *
 * @li @c buffer_size(target)
 *
 * @li @c buffer_size(source)
 *
 * @li @c If specified, @c max_bytes_to_copy.
 *
 * This prevents buffer overflow, regardless of the buffer sizes used in the
 * copy operation.
 *
 * Note that @ref buffer_copy is implemented in terms of @c memcpy, and
 * consequently it cannot be used to copy between overlapping memory regions.
 */
/*@{*/

namespace detail {

inline std::size_t buffer_copy_1(const mutable_buffer& target,
    const const_buffer& source)
{
  using namespace std; // For memcpy.
  std::size_t target_size = target.size();
  std::size_t source_size = source.size();
  std::size_t n = target_size < source_size ? target_size : source_size;
  if (n > 0)
    memcpy(target.data(), source.data(), n);
  return n;
}

template <typename TargetIterator, typename SourceIterator>
inline std::size_t buffer_copy(one_buffer, one_buffer,
    TargetIterator target_begin, TargetIterator,
    SourceIterator source_begin, SourceIterator) BOOST_ASIO_NOEXCEPT
{
  return (buffer_copy_1)(*target_begin, *source_begin);
}

template <typename TargetIterator, typename SourceIterator>
inline std::size_t buffer_copy(one_buffer, one_buffer,
    TargetIterator target_begin, TargetIterator,
    SourceIterator source_begin, SourceIterator,
    std::size_t max_bytes_to_copy) BOOST_ASIO_NOEXCEPT
{
  return (buffer_copy_1)(*target_begin,
      boost::asio::buffer(*source_begin, max_bytes_to_copy));
}

template <typename TargetIterator, typename SourceIterator>
std::size_t buffer_copy(one_buffer, multiple_buffers,
    TargetIterator target_begin, TargetIterator,
    SourceIterator source_begin, SourceIterator source_end,
    std::size_t max_bytes_to_copy
      = (std::numeric_limits<std::size_t>::max)()) BOOST_ASIO_NOEXCEPT
{
  std::size_t total_bytes_copied = 0;
  SourceIterator source_iter = source_begin;

  for (mutable_buffer target_buffer(
        boost::asio::buffer(*target_begin, max_bytes_to_copy));
      target_buffer.size() && source_iter != source_end; ++source_iter)
  {
    const_buffer source_buffer(*source_iter);
    std::size_t bytes_copied = (buffer_copy_1)(target_buffer, source_buffer);
    total_bytes_copied += bytes_copied;
    target_buffer += bytes_copied;
  }

  return total_bytes_copied;
}

template <typename TargetIterator, typename SourceIterator>
std::size_t buffer_copy(multiple_buffers, one_buffer,
    TargetIterator target_begin, TargetIterator target_end,
    SourceIterator source_begin, SourceIterator,
    std::size_t max_bytes_to_copy
      = (std::numeric_limits<std::size_t>::max)()) BOOST_ASIO_NOEXCEPT
{
  std::size_t total_bytes_copied = 0;
  TargetIterator target_iter = target_begin;

  for (const_buffer source_buffer(
        boost::asio::buffer(*source_begin, max_bytes_to_copy));
      source_buffer.size() && target_iter != target_end; ++target_iter)
  {
    mutable_buffer target_buffer(*target_iter);
    std::size_t bytes_copied = (buffer_copy_1)(target_buffer, source_buffer);
    total_bytes_copied += bytes_copied;
    source_buffer += bytes_copied;
  }

  return total_bytes_copied;
}

template <typename TargetIterator, typename SourceIterator>
std::size_t buffer_copy(multiple_buffers, multiple_buffers,
    TargetIterator target_begin, TargetIterator target_end,
    SourceIterator source_begin, SourceIterator source_end) BOOST_ASIO_NOEXCEPT
{
  std::size_t total_bytes_copied = 0;

  TargetIterator target_iter = target_begin;
  std::size_t target_buffer_offset = 0;

  SourceIterator source_iter = source_begin;
  std::size_t source_buffer_offset = 0;

  while (target_iter != target_end && source_iter != source_end)
  {
    mutable_buffer target_buffer =
      mutable_buffer(*target_iter) + target_buffer_offset;

    const_buffer source_buffer =
      const_buffer(*source_iter) + source_buffer_offset;

    std::size_t bytes_copied = (buffer_copy_1)(target_buffer, source_buffer);
    total_bytes_copied += bytes_copied;

    if (bytes_copied == target_buffer.size())
    {
      ++target_iter;
      target_buffer_offset = 0;
    }
    else
      target_buffer_offset += bytes_copied;

    if (bytes_copied == source_buffer.size())
    {
      ++source_iter;
      source_buffer_offset = 0;
    }
    else
      source_buffer_offset += bytes_copied;
  }

  return total_bytes_copied;
}

template <typename TargetIterator, typename SourceIterator>
std::size_t buffer_copy(multiple_buffers, multiple_buffers,
    TargetIterator target_begin, TargetIterator target_end,
    SourceIterator source_begin, SourceIterator source_end,
    std::size_t max_bytes_to_copy) BOOST_ASIO_NOEXCEPT
{
  std::size_t total_bytes_copied = 0;

  TargetIterator target_iter = target_begin;
  std::size_t target_buffer_offset = 0;

  SourceIterator source_iter = source_begin;
  std::size_t source_buffer_offset = 0;

  while (total_bytes_copied != max_bytes_to_copy
      && target_iter != target_end && source_iter != source_end)
  {
    mutable_buffer target_buffer =
      mutable_buffer(*target_iter) + target_buffer_offset;

    const_buffer source_buffer =
      const_buffer(*source_iter) + source_buffer_offset;

    std::size_t bytes_copied = (buffer_copy_1)(
        target_buffer, boost::asio::buffer(source_buffer,
          max_bytes_to_copy - total_bytes_copied));
    total_bytes_copied += bytes_copied;

    if (bytes_copied == target_buffer.size())
    {
      ++target_iter;
      target_buffer_offset = 0;
    }
    else
      target_buffer_offset += bytes_copied;

    if (bytes_copied == source_buffer.size())
    {
      ++source_iter;
      source_buffer_offset = 0;
    }
    else
      source_buffer_offset += bytes_copied;
  }

  return total_bytes_copied;
}

} // namespace detail

/// Copies bytes from a source buffer sequence to a target buffer sequence.
/**
 * @param target A modifiable buffer sequence representing the memory regions to
 * which the bytes will be copied.
 *
 * @param source A non-modifiable buffer sequence representing the memory
 * regions from which the bytes will be copied.
 *
 * @returns The number of bytes copied.
 *
 * @note The number of bytes copied is the lesser of:
 *
 * @li @c buffer_size(target)
 *
 * @li @c buffer_size(source)
 *
 * This function is implemented in terms of @c memcpy, and consequently it
 * cannot be used to copy between overlapping memory regions.
 */
template <typename MutableBufferSequence, typename ConstBufferSequence>
inline std::size_t buffer_copy(const MutableBufferSequence& target,
    const ConstBufferSequence& source) BOOST_ASIO_NOEXCEPT
{
  return detail::buffer_copy(
      detail::buffer_sequence_cardinality<MutableBufferSequence>(),
      detail::buffer_sequence_cardinality<ConstBufferSequence>(),
      boost::asio::buffer_sequence_begin(target),
      boost::asio::buffer_sequence_end(target),
      boost::asio::buffer_sequence_begin(source),
      boost::asio::buffer_sequence_end(source));
}

/// Copies a limited number of bytes from a source buffer sequence to a target
/// buffer sequence.
/**
 * @param target A modifiable buffer sequence representing the memory regions to
 * which the bytes will be copied.
 *
 * @param source A non-modifiable buffer sequence representing the memory
 * regions from which the bytes will be copied.
 *
 * @param max_bytes_to_copy The maximum number of bytes to be copied.
 *
 * @returns The number of bytes copied.
 *
 * @note The number of bytes copied is the lesser of:
 *
 * @li @c buffer_size(target)
 *
 * @li @c buffer_size(source)
 *
 * @li @c max_bytes_to_copy
 *
 * This function is implemented in terms of @c memcpy, and consequently it
 * cannot be used to copy between overlapping memory regions.
 */
template <typename MutableBufferSequence, typename ConstBufferSequence>
inline std::size_t buffer_copy(const MutableBufferSequence& target,
    const ConstBufferSequence& source,
    std::size_t max_bytes_to_copy) BOOST_ASIO_NOEXCEPT
{
  return detail::buffer_copy(
      detail::buffer_sequence_cardinality<MutableBufferSequence>(),
      detail::buffer_sequence_cardinality<ConstBufferSequence>(),
      boost::asio::buffer_sequence_begin(target),
      boost::asio::buffer_sequence_end(target),
      boost::asio::buffer_sequence_begin(source),
      boost::asio::buffer_sequence_end(source), max_bytes_to_copy);
}

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>
#include <boost/asio/detail/is_buffer_sequence.hpp>
#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Trait to determine whether a type satisfies the MutableBufferSequence
/// requirements.
template <typename T>
struct is_mutable_buffer_sequence
#if defined(GENERATING_DOCUMENTATION)
  : integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  : boost::asio::detail::is_buffer_sequence<T, mutable_buffer>
#endif // defined(GENERATING_DOCUMENTATION)
{
};

/// Trait to determine whether a type satisfies the ConstBufferSequence
/// requirements.
template <typename T>
struct is_const_buffer_sequence
#if defined(GENERATING_DOCUMENTATION)
  : integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  : boost::asio::detail::is_buffer_sequence<T, const_buffer>
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
/// Trait to determine whether a type satisfies the DynamicBuffer_v1
/// requirements.
template <typename T>
struct is_dynamic_buffer_v1
#if defined(GENERATING_DOCUMENTATION)
  : integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  : boost::asio::detail::is_dynamic_buffer_v1<T>
#endif // defined(GENERATING_DOCUMENTATION)
{
};
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

/// Trait to determine whether a type satisfies the DynamicBuffer_v2
/// requirements.
template <typename T>
struct is_dynamic_buffer_v2
#if defined(GENERATING_DOCUMENTATION)
  : integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  : boost::asio::detail::is_dynamic_buffer_v2<T>
#endif // defined(GENERATING_DOCUMENTATION)
{
};

/// Trait to determine whether a type satisfies the DynamicBuffer requirements.
/**
 * If @c BOOST_ASIO_NO_DYNAMIC_BUFFER_V1 is not defined, determines whether the
 * type satisfies the DynamicBuffer_v1 requirements. Otherwise, if @c
 * BOOST_ASIO_NO_DYNAMIC_BUFFER_V1 is defined, determines whether the type
 * satisfies the DynamicBuffer_v2 requirements.
 */
template <typename T>
struct is_dynamic_buffer
#if defined(GENERATING_DOCUMENTATION)
  : integral_constant<bool, automatically_determined>
#elif defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  : boost::asio::is_dynamic_buffer_v2<T>
#else // defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
  : boost::asio::is_dynamic_buffer_v1<T>
#endif // defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)
{
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BUFFER_HPP
