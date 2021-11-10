//
// basic_streambuf.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_STREAMBUF_HPP
#define BOOST_ASIO_BASIC_STREAMBUF_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_NO_IOSTREAM)

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <streambuf>
#include <vector>
#include <boost/asio/basic_streambuf_fwd.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/limits.hpp>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/detail/throw_exception.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// Automatically resizable buffer class based on std::streambuf.
/**
 * The @c basic_streambuf class is derived from @c std::streambuf to associate
 * the streambuf's input and output sequences with one or more character
 * arrays. These character arrays are internal to the @c basic_streambuf
 * object, but direct access to the array elements is provided to permit them
 * to be used efficiently with I/O operations. Characters written to the output
 * sequence of a @c basic_streambuf object are appended to the input sequence
 * of the same object.
 *
 * The @c basic_streambuf class's public interface is intended to permit the
 * following implementation strategies:
 *
 * @li A single contiguous character array, which is reallocated as necessary
 * to accommodate changes in the size of the character sequence. This is the
 * implementation approach currently used in Asio.
 *
 * @li A sequence of one or more character arrays, where each array is of the
 * same size. Additional character array objects are appended to the sequence
 * to accommodate changes in the size of the character sequence.
 *
 * @li A sequence of one or more character arrays of varying sizes. Additional
 * character array objects are appended to the sequence to accommodate changes
 * in the size of the character sequence.
 *
 * The constructor for basic_streambuf accepts a @c size_t argument specifying
 * the maximum of the sum of the sizes of the input sequence and output
 * sequence. During the lifetime of the @c basic_streambuf object, the following
 * invariant holds:
 * @code size() <= max_size()@endcode
 * Any member function that would, if successful, cause the invariant to be
 * violated shall throw an exception of class @c std::length_error.
 *
 * The constructor for @c basic_streambuf takes an Allocator argument. A copy
 * of this argument is used for any memory allocation performed, by the
 * constructor and by all member functions, during the lifetime of each @c
 * basic_streambuf object.
 *
 * @par Examples
 * Writing directly from an streambuf to a socket:
 * @code
 * boost::asio::streambuf b;
 * std::ostream os(&b);
 * os << "Hello, World!\n";
 *
 * // try sending some data in input sequence
 * size_t n = sock.send(b.data());
 *
 * b.consume(n); // sent data is removed from input sequence
 * @endcode
 *
 * Reading from a socket directly into a streambuf:
 * @code
 * boost::asio::streambuf b;
 *
 * // reserve 512 bytes in output sequence
 * boost::asio::streambuf::mutable_buffers_type bufs = b.prepare(512);
 *
 * size_t n = sock.receive(bufs);
 *
 * // received data is "committed" from output sequence to input sequence
 * b.commit(n);
 *
 * std::istream is(&b);
 * std::string s;
 * is >> s;
 * @endcode
 */
#if defined(GENERATING_DOCUMENTATION)
template <typename Allocator = std::allocator<char> >
#else
template <typename Allocator>
#endif
class basic_streambuf
  : public std::streambuf,
    private noncopyable
{
public:
#if defined(GENERATING_DOCUMENTATION)
  /// The type used to represent the input sequence as a list of buffers.
  typedef implementation_defined const_buffers_type;

  /// The type used to represent the output sequence as a list of buffers.
  typedef implementation_defined mutable_buffers_type;
#else
  typedef BOOST_ASIO_CONST_BUFFER const_buffers_type;
  typedef BOOST_ASIO_MUTABLE_BUFFER mutable_buffers_type;
#endif

  /// Construct a basic_streambuf object.
  /**
   * Constructs a streambuf with the specified maximum size. The initial size
   * of the streambuf's input sequence is 0.
   */
  explicit basic_streambuf(
      std::size_t maximum_size = (std::numeric_limits<std::size_t>::max)(),
      const Allocator& allocator = Allocator())
    : max_size_(maximum_size),
      buffer_(allocator)
  {
    std::size_t pend = (std::min<std::size_t>)(max_size_, buffer_delta);
    buffer_.resize((std::max<std::size_t>)(pend, 1));
    setg(&buffer_[0], &buffer_[0], &buffer_[0]);
    setp(&buffer_[0], &buffer_[0] + pend);
  }

  /// Get the size of the input sequence.
  /**
   * @returns The size of the input sequence. The value is equal to that
   * calculated for @c s in the following code:
   * @code
   * size_t s = 0;
   * const_buffers_type bufs = data();
   * const_buffers_type::const_iterator i = bufs.begin();
   * while (i != bufs.end())
   * {
   *   const_buffer buf(*i++);
   *   s += buf.size();
   * }
   * @endcode
   */
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
    return pptr() - gptr();
  }

  /// Get the maximum size of the basic_streambuf.
  /**
   * @returns The allowed maximum of the sum of the sizes of the input sequence
   * and output sequence.
   */
  std::size_t max_size() const BOOST_ASIO_NOEXCEPT
  {
    return max_size_;
  }

  /// Get the current capacity of the basic_streambuf.
  /**
   * @returns The current total capacity of the streambuf, i.e. for both the
   * input sequence and output sequence.
   */
  std::size_t capacity() const BOOST_ASIO_NOEXCEPT
  {
    return buffer_.capacity();
  }

  /// Get a list of buffers that represents the input sequence.
  /**
   * @returns An object of type @c const_buffers_type that satisfies
   * ConstBufferSequence requirements, representing all character arrays in the
   * input sequence.
   *
   * @note The returned object is invalidated by any @c basic_streambuf member
   * function that modifies the input sequence or output sequence.
   */
  const_buffers_type data() const BOOST_ASIO_NOEXCEPT
  {
    return boost::asio::buffer(boost::asio::const_buffer(gptr(),
          (pptr() - gptr()) * sizeof(char_type)));
  }

  /// Get a list of buffers that represents the output sequence, with the given
  /// size.
  /**
   * Ensures that the output sequence can accommodate @c n characters,
   * reallocating character array objects as necessary.
   *
   * @returns An object of type @c mutable_buffers_type that satisfies
   * MutableBufferSequence requirements, representing character array objects
   * at the start of the output sequence such that the sum of the buffer sizes
   * is @c n.
   *
   * @throws std::length_error If <tt>size() + n > max_size()</tt>.
   *
   * @note The returned object is invalidated by any @c basic_streambuf member
   * function that modifies the input sequence or output sequence.
   */
  mutable_buffers_type prepare(std::size_t n)
  {
    reserve(n);
    return boost::asio::buffer(boost::asio::mutable_buffer(
          pptr(), n * sizeof(char_type)));
  }

  /// Move characters from the output sequence to the input sequence.
  /**
   * Appends @c n characters from the start of the output sequence to the input
   * sequence. The beginning of the output sequence is advanced by @c n
   * characters.
   *
   * Requires a preceding call <tt>prepare(x)</tt> where <tt>x >= n</tt>, and
   * no intervening operations that modify the input or output sequence.
   *
   * @note If @c n is greater than the size of the output sequence, the entire
   * output sequence is moved to the input sequence and no error is issued.
   */
  void commit(std::size_t n)
  {
    n = std::min<std::size_t>(n, epptr() - pptr());
    pbump(static_cast<int>(n));
    setg(eback(), gptr(), pptr());
  }

  /// Remove characters from the input sequence.
  /**
   * Removes @c n characters from the beginning of the input sequence.
   *
   * @note If @c n is greater than the size of the input sequence, the entire
   * input sequence is consumed and no error is issued.
   */
  void consume(std::size_t n)
  {
    if (egptr() < pptr())
      setg(&buffer_[0], gptr(), pptr());
    if (gptr() + n > pptr())
      n = pptr() - gptr();
    gbump(static_cast<int>(n));
  }

protected:
  enum { buffer_delta = 128 };

  /// Override std::streambuf behaviour.
  /**
   * Behaves according to the specification of @c std::streambuf::underflow().
   */
  int_type underflow()
  {
    if (gptr() < pptr())
    {
      setg(&buffer_[0], gptr(), pptr());
      return traits_type::to_int_type(*gptr());
    }
    else
    {
      return traits_type::eof();
    }
  }

  /// Override std::streambuf behaviour.
  /**
   * Behaves according to the specification of @c std::streambuf::overflow(),
   * with the specialisation that @c std::length_error is thrown if appending
   * the character to the input sequence would require the condition
   * <tt>size() > max_size()</tt> to be true.
   */
  int_type overflow(int_type c)
  {
    if (!traits_type::eq_int_type(c, traits_type::eof()))
    {
      if (pptr() == epptr())
      {
        std::size_t buffer_size = pptr() - gptr();
        if (buffer_size < max_size_ && max_size_ - buffer_size < buffer_delta)
        {
          reserve(max_size_ - buffer_size);
        }
        else
        {
          reserve(buffer_delta);
        }
      }

      *pptr() = traits_type::to_char_type(c);
      pbump(1);
      return c;
    }

    return traits_type::not_eof(c);
  }

  void reserve(std::size_t n)
  {
    // Get current stream positions as offsets.
    std::size_t gnext = gptr() - &buffer_[0];
    std::size_t pnext = pptr() - &buffer_[0];
    std::size_t pend = epptr() - &buffer_[0];

    // Check if there is already enough space in the put area.
    if (n <= pend - pnext)
    {
      return;
    }

    // Shift existing contents of get area to start of buffer.
    if (gnext > 0)
    {
      pnext -= gnext;
      std::memmove(&buffer_[0], &buffer_[0] + gnext, pnext);
    }

    // Ensure buffer is large enough to hold at least the specified size.
    if (n > pend - pnext)
    {
      if (n <= max_size_ && pnext <= max_size_ - n)
      {
        pend = pnext + n;
        buffer_.resize((std::max<std::size_t>)(pend, 1));
      }
      else
      {
        std::length_error ex("boost::asio::streambuf too long");
        boost::asio::detail::throw_exception(ex);
      }
    }

    // Update stream positions.
    setg(&buffer_[0], &buffer_[0], &buffer_[0] + pnext);
    setp(&buffer_[0] + pnext, &buffer_[0] + pend);
  }

private:
  std::size_t max_size_;
  std::vector<char_type, Allocator> buffer_;

  // Helper function to get the preferred size for reading data.
  friend std::size_t read_size_helper(
      basic_streambuf& sb, std::size_t max_size)
  {
    return std::min<std::size_t>(
        std::max<std::size_t>(512, sb.buffer_.capacity() - sb.size()),
        std::min<std::size_t>(max_size, sb.max_size() - sb.size()));
  }
};

/// Adapts basic_streambuf to the dynamic buffer sequence type requirements.
#if defined(GENERATING_DOCUMENTATION)
template <typename Allocator = std::allocator<char> >
#else
template <typename Allocator>
#endif
class basic_streambuf_ref
{
public:
  /// The type used to represent the input sequence as a list of buffers.
  typedef typename basic_streambuf<Allocator>::const_buffers_type
    const_buffers_type;

  /// The type used to represent the output sequence as a list of buffers.
  typedef typename basic_streambuf<Allocator>::mutable_buffers_type
    mutable_buffers_type;

  /// Construct a basic_streambuf_ref for the given basic_streambuf object.
  explicit basic_streambuf_ref(basic_streambuf<Allocator>& sb)
    : sb_(sb)
  {
  }

  /// Copy construct a basic_streambuf_ref.
  basic_streambuf_ref(const basic_streambuf_ref& other) BOOST_ASIO_NOEXCEPT
    : sb_(other.sb_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move construct a basic_streambuf_ref.
  basic_streambuf_ref(basic_streambuf_ref&& other) BOOST_ASIO_NOEXCEPT
    : sb_(other.sb_)
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Get the size of the input sequence.
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
    return sb_.size();
  }

  /// Get the maximum size of the dynamic buffer.
  std::size_t max_size() const BOOST_ASIO_NOEXCEPT
  {
    return sb_.max_size();
  }

  /// Get the current capacity of the dynamic buffer.
  std::size_t capacity() const BOOST_ASIO_NOEXCEPT
  {
    return sb_.capacity();
  }

  /// Get a list of buffers that represents the input sequence.
  const_buffers_type data() const BOOST_ASIO_NOEXCEPT
  {
    return sb_.data();
  }

  /// Get a list of buffers that represents the output sequence, with the given
  /// size.
  mutable_buffers_type prepare(std::size_t n)
  {
    return sb_.prepare(n);
  }

  /// Move bytes from the output sequence to the input sequence.
  void commit(std::size_t n)
  {
    return sb_.commit(n);
  }

  /// Remove characters from the input sequence.
  void consume(std::size_t n)
  {
    return sb_.consume(n);
  }

private:
  basic_streambuf<Allocator>& sb_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

#endif // BOOST_ASIO_BASIC_STREAMBUF_HPP
