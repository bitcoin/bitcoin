//
// buffers_iterator.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BUFFERS_ITERATOR_HPP
#define BOOST_ASIO_BUFFERS_ITERATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <iterator>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/assert.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

namespace detail
{
  template <bool IsMutable>
  struct buffers_iterator_types_helper;

  template <>
  struct buffers_iterator_types_helper<false>
  {
    typedef const_buffer buffer_type;
    template <typename ByteType>
    struct byte_type
    {
      typedef typename add_const<ByteType>::type type;
    };
  };

  template <>
  struct buffers_iterator_types_helper<true>
  {
    typedef mutable_buffer buffer_type;
    template <typename ByteType>
    struct byte_type
    {
      typedef ByteType type;
    };
  };

  template <typename BufferSequence, typename ByteType>
  struct buffers_iterator_types
  {
    enum
    {
      is_mutable = is_convertible<
          typename BufferSequence::value_type,
          mutable_buffer>::value
    };
    typedef buffers_iterator_types_helper<is_mutable> helper;
    typedef typename helper::buffer_type buffer_type;
    typedef typename helper::template byte_type<ByteType>::type byte_type;
    typedef typename BufferSequence::const_iterator const_iterator;
  };

  template <typename ByteType>
  struct buffers_iterator_types<mutable_buffer, ByteType>
  {
    typedef mutable_buffer buffer_type;
    typedef ByteType byte_type;
    typedef const mutable_buffer* const_iterator;
  };

  template <typename ByteType>
  struct buffers_iterator_types<const_buffer, ByteType>
  {
    typedef const_buffer buffer_type;
    typedef typename add_const<ByteType>::type byte_type;
    typedef const const_buffer* const_iterator;
  };

#if !defined(BOOST_ASIO_NO_DEPRECATED)

  template <typename ByteType>
  struct buffers_iterator_types<mutable_buffers_1, ByteType>
  {
    typedef mutable_buffer buffer_type;
    typedef ByteType byte_type;
    typedef const mutable_buffer* const_iterator;
  };

  template <typename ByteType>
  struct buffers_iterator_types<const_buffers_1, ByteType>
  {
    typedef const_buffer buffer_type;
    typedef typename add_const<ByteType>::type byte_type;
    typedef const const_buffer* const_iterator;
  };

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)
}

/// A random access iterator over the bytes in a buffer sequence.
template <typename BufferSequence, typename ByteType = char>
class buffers_iterator
{
private:
  typedef typename detail::buffers_iterator_types<
      BufferSequence, ByteType>::buffer_type buffer_type;

  typedef typename detail::buffers_iterator_types<BufferSequence,
          ByteType>::const_iterator buffer_sequence_iterator_type;

public:
  /// The type used for the distance between two iterators.
  typedef std::ptrdiff_t difference_type;

  /// The type of the value pointed to by the iterator.
  typedef ByteType value_type;

#if defined(GENERATING_DOCUMENTATION)
  /// The type of the result of applying operator->() to the iterator.
  /**
   * If the buffer sequence stores buffer objects that are convertible to
   * mutable_buffer, this is a pointer to a non-const ByteType. Otherwise, a
   * pointer to a const ByteType.
   */
  typedef const_or_non_const_ByteType* pointer;
#else // defined(GENERATING_DOCUMENTATION)
  typedef typename detail::buffers_iterator_types<
      BufferSequence, ByteType>::byte_type* pointer;
#endif // defined(GENERATING_DOCUMENTATION)

#if defined(GENERATING_DOCUMENTATION)
  /// The type of the result of applying operator*() to the iterator.
  /**
   * If the buffer sequence stores buffer objects that are convertible to
   * mutable_buffer, this is a reference to a non-const ByteType. Otherwise, a
   * reference to a const ByteType.
   */
  typedef const_or_non_const_ByteType& reference;
#else // defined(GENERATING_DOCUMENTATION)
  typedef typename detail::buffers_iterator_types<
      BufferSequence, ByteType>::byte_type& reference;
#endif // defined(GENERATING_DOCUMENTATION)

  /// The iterator category.
  typedef std::random_access_iterator_tag iterator_category;

  /// Default constructor. Creates an iterator in an undefined state.
  buffers_iterator()
    : current_buffer_(),
      current_buffer_position_(0),
      begin_(),
      current_(),
      end_(),
      position_(0)
  {
  }

  /// Construct an iterator representing the beginning of the buffers' data.
  static buffers_iterator begin(const BufferSequence& buffers)
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 3)
    __attribute__ ((__noinline__))
#endif // defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 3)
  {
    buffers_iterator new_iter;
    new_iter.begin_ = boost::asio::buffer_sequence_begin(buffers);
    new_iter.current_ = boost::asio::buffer_sequence_begin(buffers);
    new_iter.end_ = boost::asio::buffer_sequence_end(buffers);
    while (new_iter.current_ != new_iter.end_)
    {
      new_iter.current_buffer_ = *new_iter.current_;
      if (new_iter.current_buffer_.size() > 0)
        break;
      ++new_iter.current_;
    }
    return new_iter;
  }

  /// Construct an iterator representing the end of the buffers' data.
  static buffers_iterator end(const BufferSequence& buffers)
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 3)
    __attribute__ ((__noinline__))
#endif // defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 3)
  {
    buffers_iterator new_iter;
    new_iter.begin_ = boost::asio::buffer_sequence_begin(buffers);
    new_iter.current_ = boost::asio::buffer_sequence_begin(buffers);
    new_iter.end_ = boost::asio::buffer_sequence_end(buffers);
    while (new_iter.current_ != new_iter.end_)
    {
      buffer_type buffer = *new_iter.current_;
      new_iter.position_ += buffer.size();
      ++new_iter.current_;
    }
    return new_iter;
  }

  /// Dereference an iterator.
  reference operator*() const
  {
    return dereference();
  }

  /// Dereference an iterator.
  pointer operator->() const
  {
    return &dereference();
  }

  /// Access an individual element.
  reference operator[](std::ptrdiff_t difference) const
  {
    buffers_iterator tmp(*this);
    tmp.advance(difference);
    return *tmp;
  }

  /// Increment operator (prefix).
  buffers_iterator& operator++()
  {
    increment();
    return *this;
  }

  /// Increment operator (postfix).
  buffers_iterator operator++(int)
  {
    buffers_iterator tmp(*this);
    ++*this;
    return tmp;
  }

  /// Decrement operator (prefix).
  buffers_iterator& operator--()
  {
    decrement();
    return *this;
  }

  /// Decrement operator (postfix).
  buffers_iterator operator--(int)
  {
    buffers_iterator tmp(*this);
    --*this;
    return tmp;
  }

  /// Addition operator.
  buffers_iterator& operator+=(std::ptrdiff_t difference)
  {
    advance(difference);
    return *this;
  }

  /// Subtraction operator.
  buffers_iterator& operator-=(std::ptrdiff_t difference)
  {
    advance(-difference);
    return *this;
  }

  /// Addition operator.
  friend buffers_iterator operator+(const buffers_iterator& iter,
      std::ptrdiff_t difference)
  {
    buffers_iterator tmp(iter);
    tmp.advance(difference);
    return tmp;
  }

  /// Addition operator.
  friend buffers_iterator operator+(std::ptrdiff_t difference,
      const buffers_iterator& iter)
  {
    buffers_iterator tmp(iter);
    tmp.advance(difference);
    return tmp;
  }

  /// Subtraction operator.
  friend buffers_iterator operator-(const buffers_iterator& iter,
      std::ptrdiff_t difference)
  {
    buffers_iterator tmp(iter);
    tmp.advance(-difference);
    return tmp;
  }

  /// Subtraction operator.
  friend std::ptrdiff_t operator-(const buffers_iterator& a,
      const buffers_iterator& b)
  {
    return b.distance_to(a);
  }

  /// Test two iterators for equality.
  friend bool operator==(const buffers_iterator& a, const buffers_iterator& b)
  {
    return a.equal(b);
  }

  /// Test two iterators for inequality.
  friend bool operator!=(const buffers_iterator& a, const buffers_iterator& b)
  {
    return !a.equal(b);
  }

  /// Compare two iterators.
  friend bool operator<(const buffers_iterator& a, const buffers_iterator& b)
  {
    return a.distance_to(b) > 0;
  }

  /// Compare two iterators.
  friend bool operator<=(const buffers_iterator& a, const buffers_iterator& b)
  {
    return !(b < a);
  }

  /// Compare two iterators.
  friend bool operator>(const buffers_iterator& a, const buffers_iterator& b)
  {
    return b < a;
  }

  /// Compare two iterators.
  friend bool operator>=(const buffers_iterator& a, const buffers_iterator& b)
  {
    return !(a < b);
  }

private:
  // Dereference the iterator.
  reference dereference() const
  {
    return static_cast<pointer>(
        current_buffer_.data())[current_buffer_position_];
  }

  // Compare two iterators for equality.
  bool equal(const buffers_iterator& other) const
  {
    return position_ == other.position_;
  }

  // Increment the iterator.
  void increment()
  {
    BOOST_ASIO_ASSERT(current_ != end_ && "iterator out of bounds");
    ++position_;

    // Check if the increment can be satisfied by the current buffer.
    ++current_buffer_position_;
    if (current_buffer_position_ != current_buffer_.size())
      return;

    // Find the next non-empty buffer.
    ++current_;
    current_buffer_position_ = 0;
    while (current_ != end_)
    {
      current_buffer_ = *current_;
      if (current_buffer_.size() > 0)
        return;
      ++current_;
    }
  }

  // Decrement the iterator.
  void decrement()
  {
    BOOST_ASIO_ASSERT(position_ > 0 && "iterator out of bounds");
    --position_;

    // Check if the decrement can be satisfied by the current buffer.
    if (current_buffer_position_ != 0)
    {
      --current_buffer_position_;
      return;
    }

    // Find the previous non-empty buffer.
    buffer_sequence_iterator_type iter = current_;
    while (iter != begin_)
    {
      --iter;
      buffer_type buffer = *iter;
      std::size_t buffer_size = buffer.size();
      if (buffer_size > 0)
      {
        current_ = iter;
        current_buffer_ = buffer;
        current_buffer_position_ = buffer_size - 1;
        return;
      }
    }
  }

  // Advance the iterator by the specified distance.
  void advance(std::ptrdiff_t n)
  {
    if (n > 0)
    {
      BOOST_ASIO_ASSERT(current_ != end_ && "iterator out of bounds");
      for (;;)
      {
        std::ptrdiff_t current_buffer_balance
          = current_buffer_.size() - current_buffer_position_;

        // Check if the advance can be satisfied by the current buffer.
        if (current_buffer_balance > n)
        {
          position_ += n;
          current_buffer_position_ += n;
          return;
        }

        // Update position.
        n -= current_buffer_balance;
        position_ += current_buffer_balance;

        // Move to next buffer. If it is empty then it will be skipped on the
        // next iteration of this loop.
        if (++current_ == end_)
        {
          BOOST_ASIO_ASSERT(n == 0 && "iterator out of bounds");
          current_buffer_ = buffer_type();
          current_buffer_position_ = 0;
          return;
        }
        current_buffer_ = *current_;
        current_buffer_position_ = 0;
      }
    }
    else if (n < 0)
    {
      std::size_t abs_n = -n;
      BOOST_ASIO_ASSERT(position_ >= abs_n && "iterator out of bounds");
      for (;;)
      {
        // Check if the advance can be satisfied by the current buffer.
        if (current_buffer_position_ >= abs_n)
        {
          position_ -= abs_n;
          current_buffer_position_ -= abs_n;
          return;
        }

        // Update position.
        abs_n -= current_buffer_position_;
        position_ -= current_buffer_position_;

        // Check if we've reached the beginning of the buffers.
        if (current_ == begin_)
        {
          BOOST_ASIO_ASSERT(abs_n == 0 && "iterator out of bounds");
          current_buffer_position_ = 0;
          return;
        }

        // Find the previous non-empty buffer.
        buffer_sequence_iterator_type iter = current_;
        while (iter != begin_)
        {
          --iter;
          buffer_type buffer = *iter;
          std::size_t buffer_size = buffer.size();
          if (buffer_size > 0)
          {
            current_ = iter;
            current_buffer_ = buffer;
            current_buffer_position_ = buffer_size;
            break;
          }
        }
      }
    }
  }

  // Determine the distance between two iterators.
  std::ptrdiff_t distance_to(const buffers_iterator& other) const
  {
    return other.position_ - position_;
  }

  buffer_type current_buffer_;
  std::size_t current_buffer_position_;
  buffer_sequence_iterator_type begin_;
  buffer_sequence_iterator_type current_;
  buffer_sequence_iterator_type end_;
  std::size_t position_;
};

/// Construct an iterator representing the beginning of the buffers' data.
template <typename BufferSequence>
inline buffers_iterator<BufferSequence> buffers_begin(
    const BufferSequence& buffers)
{
  return buffers_iterator<BufferSequence>::begin(buffers);
}

/// Construct an iterator representing the end of the buffers' data.
template <typename BufferSequence>
inline buffers_iterator<BufferSequence> buffers_end(
    const BufferSequence& buffers)
{
  return buffers_iterator<BufferSequence>::end(buffers);
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_BUFFERS_ITERATOR_HPP
