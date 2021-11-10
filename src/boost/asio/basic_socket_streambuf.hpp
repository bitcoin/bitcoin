//
// basic_socket_streambuf.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_SOCKET_STREAMBUF_HPP
#define BOOST_ASIO_BASIC_SOCKET_STREAMBUF_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_NO_IOSTREAM)

#include <streambuf>
#include <vector>
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/io_context.hpp>

#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME) \
  && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
# include <boost/asio/detail/deadline_timer_service.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
      // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
# include <boost/asio/steady_timer.hpp>
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
       // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)

#if !defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

# include <boost/asio/detail/variadic_templates.hpp>

// A macro that should expand to:
//   template <typename T1, ..., typename Tn>
//   basic_socket_streambuf* connect(T1 x1, ..., Tn xn)
//   {
//     init_buffers();
//     typedef typename Protocol::resolver resolver_type;
//     resolver_type resolver(socket().get_executor());
//     connect_to_endpoints(
//         resolver.resolve(x1, ..., xn, ec_));
//     return !ec_ ? this : 0;
//   }
// This macro should only persist within this file.

# define BOOST_ASIO_PRIVATE_CONNECT_DEF(n) \
  template <BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  basic_socket_streambuf* connect(BOOST_ASIO_VARIADIC_BYVAL_PARAMS(n)) \
  { \
    init_buffers(); \
    typedef typename Protocol::resolver resolver_type; \
    resolver_type resolver(socket().get_executor()); \
    connect_to_endpoints( \
        resolver.resolve(BOOST_ASIO_VARIADIC_BYVAL_ARGS(n), ec_)); \
    return !ec_ ? this : 0; \
  } \
  /**/

#endif // !defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// A separate base class is used to ensure that the io_context member is
// initialised prior to the basic_socket_streambuf's basic_socket base class.
class socket_streambuf_io_context
{
protected:
  socket_streambuf_io_context(io_context* ctx)
    : default_io_context_(ctx)
  {
  }

  shared_ptr<io_context> default_io_context_;
};

// A separate base class is used to ensure that the dynamically allocated
// buffers are constructed prior to the basic_socket_streambuf's basic_socket
// base class. This makes moving the socket is the last potentially throwing
// step in the streambuf's move constructor, giving the constructor a strong
// exception safety guarantee.
class socket_streambuf_buffers
{
protected:
  socket_streambuf_buffers()
    : get_buffer_(buffer_size),
      put_buffer_(buffer_size)
  {
  }

  enum { buffer_size = 512 };
  std::vector<char> get_buffer_;
  std::vector<char> put_buffer_;
};

} // namespace detail

#if !defined(BOOST_ASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL)
#define BOOST_ASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol,
#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME) \
  && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
    typename Clock = boost::posix_time::ptime,
    typename WaitTraits = time_traits<Clock> >
#else // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
      // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
    typename Clock = chrono::steady_clock,
    typename WaitTraits = wait_traits<Clock> >
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
       // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
class basic_socket_streambuf;

#endif // !defined(BOOST_ASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL)

/// Iostream streambuf for a socket.
#if defined(GENERATING_DOCUMENTATION)
template <typename Protocol,
    typename Clock = chrono::steady_clock,
    typename WaitTraits = wait_traits<Clock> >
#else // defined(GENERATING_DOCUMENTATION)
template <typename Protocol, typename Clock, typename WaitTraits>
#endif // defined(GENERATING_DOCUMENTATION)
class basic_socket_streambuf
  : public std::streambuf,
    private detail::socket_streambuf_io_context,
    private detail::socket_streambuf_buffers,
#if defined(BOOST_ASIO_NO_DEPRECATED) || defined(GENERATING_DOCUMENTATION)
    private basic_socket<Protocol>
#else // defined(BOOST_ASIO_NO_DEPRECATED) || defined(GENERATING_DOCUMENTATION)
    public basic_socket<Protocol>
#endif // defined(BOOST_ASIO_NO_DEPRECATED) || defined(GENERATING_DOCUMENTATION)
{
private:
  // These typedefs are intended keep this class's implementation independent
  // of whether it's using Boost.DateClock, Boost.Chrono or std::chrono.
#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME) \
  && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
  typedef WaitTraits traits_helper;
#else // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
      // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
  typedef detail::chrono_time_traits<Clock, WaitTraits> traits_helper;
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
       // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)

public:
  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// The clock type.
  typedef Clock clock_type;

#if defined(GENERATING_DOCUMENTATION)
  /// (Deprecated: Use time_point.) The time type.
  typedef typename WaitTraits::time_type time_type;

  /// The time type.
  typedef typename WaitTraits::time_point time_point;

  /// (Deprecated: Use duration.) The duration type.
  typedef typename WaitTraits::duration_type duration_type;

  /// The duration type.
  typedef typename WaitTraits::duration duration;
#else
# if !defined(BOOST_ASIO_NO_DEPRECATED)
  typedef typename traits_helper::time_type time_type;
  typedef typename traits_helper::duration_type duration_type;
# endif // !defined(BOOST_ASIO_NO_DEPRECATED)
  typedef typename traits_helper::time_type time_point;
  typedef typename traits_helper::duration_type duration;
#endif

  /// Construct a basic_socket_streambuf without establishing a connection.
  basic_socket_streambuf()
    : detail::socket_streambuf_io_context(new io_context),
      basic_socket<Protocol>(*default_io_context_),
      expiry_time_(max_expiry_time())
  {
    init_buffers();
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Construct a basic_socket_streambuf from the supplied socket.
  explicit basic_socket_streambuf(basic_stream_socket<protocol_type> s)
    : detail::socket_streambuf_io_context(0),
      basic_socket<Protocol>(std::move(s)),
      expiry_time_(max_expiry_time())
  {
    init_buffers();
  }

  /// Move-construct a basic_socket_streambuf from another.
  basic_socket_streambuf(basic_socket_streambuf&& other)
    : detail::socket_streambuf_io_context(other),
      basic_socket<Protocol>(std::move(other.socket())),
      ec_(other.ec_),
      expiry_time_(other.expiry_time_)
  {
    get_buffer_.swap(other.get_buffer_);
    put_buffer_.swap(other.put_buffer_);
    setg(other.eback(), other.gptr(), other.egptr());
    setp(other.pptr(), other.epptr());
    other.ec_ = boost::system::error_code();
    other.expiry_time_ = max_expiry_time();
    other.init_buffers();
  }

  /// Move-assign a basic_socket_streambuf from another.
  basic_socket_streambuf& operator=(basic_socket_streambuf&& other)
  {
    this->close();
    socket() = std::move(other.socket());
    detail::socket_streambuf_io_context::operator=(other);
    ec_ = other.ec_;
    expiry_time_ = other.expiry_time_;
    get_buffer_.swap(other.get_buffer_);
    put_buffer_.swap(other.put_buffer_);
    setg(other.eback(), other.gptr(), other.egptr());
    setp(other.pptr(), other.epptr());
    other.ec_ = boost::system::error_code();
    other.expiry_time_ = max_expiry_time();
    other.put_buffer_.resize(buffer_size);
    other.init_buffers();
    return *this;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Destructor flushes buffered data.
  virtual ~basic_socket_streambuf()
  {
    if (pptr() != pbase())
      overflow(traits_type::eof());
  }

  /// Establish a connection.
  /**
   * This function establishes a connection to the specified endpoint.
   *
   * @return \c this if a connection was successfully established, a null
   * pointer otherwise.
   */
  basic_socket_streambuf* connect(const endpoint_type& endpoint)
  {
    init_buffers();
    ec_ = boost::system::error_code();
    this->connect_to_endpoints(&endpoint, &endpoint + 1);
    return !ec_ ? this : 0;
  }

#if defined(GENERATING_DOCUMENTATION)
  /// Establish a connection.
  /**
   * This function automatically establishes a connection based on the supplied
   * resolver query parameters. The arguments are used to construct a resolver
   * query object.
   *
   * @return \c this if a connection was successfully established, a null
   * pointer otherwise.
   */
  template <typename T1, ..., typename TN>
  basic_socket_streambuf* connect(T1 t1, ..., TN tn);
#elif defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
  template <typename... T>
  basic_socket_streambuf* connect(T... x)
  {
    init_buffers();
    typedef typename Protocol::resolver resolver_type;
    resolver_type resolver(socket().get_executor());
    connect_to_endpoints(resolver.resolve(x..., ec_));
    return !ec_ ? this : 0;
  }
#else
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_CONNECT_DEF)
#endif

  /// Close the connection.
  /**
   * @return \c this if a connection was successfully established, a null
   * pointer otherwise.
   */
  basic_socket_streambuf* close()
  {
    sync();
    socket().close(ec_);
    if (!ec_)
      init_buffers();
    return !ec_ ? this : 0;
  }

  /// Get a reference to the underlying socket.
  basic_socket<Protocol>& socket()
  {
    return *this;
  }

  /// Get the last error associated with the stream buffer.
  /**
   * @return An \c error_code corresponding to the last error from the stream
   * buffer.
   */
  const boost::system::error_code& error() const
  {
    return ec_;
  }

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use error().) Get the last error associated with the stream
  /// buffer.
  /**
   * @return An \c error_code corresponding to the last error from the stream
   * buffer.
   */
  const boost::system::error_code& puberror() const
  {
    return error();
  }

  /// (Deprecated: Use expiry().) Get the stream buffer's expiry time as an
  /// absolute time.
  /**
   * @return An absolute time value representing the stream buffer's expiry
   * time.
   */
  time_point expires_at() const
  {
    return expiry_time_;
  }
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

  /// Get the stream buffer's expiry time as an absolute time.
  /**
   * @return An absolute time value representing the stream buffer's expiry
   * time.
   */
  time_point expiry() const
  {
    return expiry_time_;
  }

  /// Set the stream buffer's expiry time as an absolute time.
  /**
   * This function sets the expiry time associated with the stream. Stream
   * operations performed after this time (where the operations cannot be
   * completed using the internal buffers) will fail with the error
   * boost::asio::error::operation_aborted.
   *
   * @param expiry_time The expiry time to be used for the stream.
   */
  void expires_at(const time_point& expiry_time)
  {
    expiry_time_ = expiry_time;
  }

  /// Set the stream buffer's expiry time relative to now.
  /**
   * This function sets the expiry time associated with the stream. Stream
   * operations performed after this time (where the operations cannot be
   * completed using the internal buffers) will fail with the error
   * boost::asio::error::operation_aborted.
   *
   * @param expiry_time The expiry time to be used for the timer.
   */
  void expires_after(const duration& expiry_time)
  {
    expiry_time_ = traits_helper::add(traits_helper::now(), expiry_time);
  }

#if !defined(BOOST_ASIO_NO_DEPRECATED)
  /// (Deprecated: Use expiry().) Get the stream buffer's expiry time relative
  /// to now.
  /**
   * @return A relative time value representing the stream buffer's expiry time.
   */
  duration expires_from_now() const
  {
    return traits_helper::subtract(expires_at(), traits_helper::now());
  }

  /// (Deprecated: Use expires_after().) Set the stream buffer's expiry time
  /// relative to now.
  /**
   * This function sets the expiry time associated with the stream. Stream
   * operations performed after this time (where the operations cannot be
   * completed using the internal buffers) will fail with the error
   * boost::asio::error::operation_aborted.
   *
   * @param expiry_time The expiry time to be used for the timer.
   */
  void expires_from_now(const duration& expiry_time)
  {
    expiry_time_ = traits_helper::add(traits_helper::now(), expiry_time);
  }
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

protected:
  int_type underflow()
  {
#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
    ec_ = boost::asio::error::operation_not_supported;
    return traits_type::eof();
#else // defined(BOOST_ASIO_WINDOWS_RUNTIME)
    if (gptr() != egptr())
      return traits_type::eof();

    for (;;)
    {
      // Check if we are past the expiry time.
      if (traits_helper::less_than(expiry_time_, traits_helper::now()))
      {
        ec_ = boost::asio::error::timed_out;
        return traits_type::eof();
      }

      // Try to complete the operation without blocking.
      if (!socket().native_non_blocking())
        socket().native_non_blocking(true, ec_);
      detail::buffer_sequence_adapter<mutable_buffer, mutable_buffer>
        bufs(boost::asio::buffer(get_buffer_) + putback_max);
      detail::signed_size_type bytes = detail::socket_ops::recv(
          socket().native_handle(), bufs.buffers(), bufs.count(), 0, ec_);

      // Check if operation succeeded.
      if (bytes > 0)
      {
        setg(&get_buffer_[0], &get_buffer_[0] + putback_max,
            &get_buffer_[0] + putback_max + bytes);
        return traits_type::to_int_type(*gptr());
      }

      // Check for EOF.
      if (bytes == 0)
      {
        ec_ = boost::asio::error::eof;
        return traits_type::eof();
      }

      // Operation failed.
      if (ec_ != boost::asio::error::would_block
          && ec_ != boost::asio::error::try_again)
        return traits_type::eof();

      // Wait for socket to become ready.
      if (detail::socket_ops::poll_read(
            socket().native_handle(), 0, timeout(), ec_) < 0)
        return traits_type::eof();
    }
#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)
  }

  int_type overflow(int_type c)
  {
#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
    ec_ = boost::asio::error::operation_not_supported;
    return traits_type::eof();
#else // defined(BOOST_ASIO_WINDOWS_RUNTIME)
    char_type ch = traits_type::to_char_type(c);

    // Determine what needs to be sent.
    const_buffer output_buffer;
    if (put_buffer_.empty())
    {
      if (traits_type::eq_int_type(c, traits_type::eof()))
        return traits_type::not_eof(c); // Nothing to do.
      output_buffer = boost::asio::buffer(&ch, sizeof(char_type));
    }
    else
    {
      output_buffer = boost::asio::buffer(pbase(),
          (pptr() - pbase()) * sizeof(char_type));
    }

    while (output_buffer.size() > 0)
    {
      // Check if we are past the expiry time.
      if (traits_helper::less_than(expiry_time_, traits_helper::now()))
      {
        ec_ = boost::asio::error::timed_out;
        return traits_type::eof();
      }

      // Try to complete the operation without blocking.
      if (!socket().native_non_blocking())
        socket().native_non_blocking(true, ec_);
      detail::buffer_sequence_adapter<
        const_buffer, const_buffer> bufs(output_buffer);
      detail::signed_size_type bytes = detail::socket_ops::send(
          socket().native_handle(), bufs.buffers(), bufs.count(), 0, ec_);

      // Check if operation succeeded.
      if (bytes > 0)
      {
        output_buffer += static_cast<std::size_t>(bytes);
        continue;
      }

      // Operation failed.
      if (ec_ != boost::asio::error::would_block
          && ec_ != boost::asio::error::try_again)
        return traits_type::eof();

      // Wait for socket to become ready.
      if (detail::socket_ops::poll_write(
            socket().native_handle(), 0, timeout(), ec_) < 0)
        return traits_type::eof();
    }

    if (!put_buffer_.empty())
    {
      setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());

      // If the new character is eof then our work here is done.
      if (traits_type::eq_int_type(c, traits_type::eof()))
        return traits_type::not_eof(c);

      // Add the new character to the output buffer.
      *pptr() = ch;
      pbump(1);
    }

    return c;
#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)
  }

  int sync()
  {
    return overflow(traits_type::eof());
  }

  std::streambuf* setbuf(char_type* s, std::streamsize n)
  {
    if (pptr() == pbase() && s == 0 && n == 0)
    {
      put_buffer_.clear();
      setp(0, 0);
      sync();
      return this;
    }

    return 0;
  }

private:
  // Disallow copying and assignment.
  basic_socket_streambuf(const basic_socket_streambuf&) BOOST_ASIO_DELETED;
  basic_socket_streambuf& operator=(
      const basic_socket_streambuf&) BOOST_ASIO_DELETED;

  void init_buffers()
  {
    setg(&get_buffer_[0],
        &get_buffer_[0] + putback_max,
        &get_buffer_[0] + putback_max);

    if (put_buffer_.empty())
      setp(0, 0);
    else
      setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
  }

  int timeout() const
  {
    int64_t msec = traits_helper::to_posix_duration(
        traits_helper::subtract(expiry_time_,
          traits_helper::now())).total_milliseconds();
    if (msec > (std::numeric_limits<int>::max)())
      msec = (std::numeric_limits<int>::max)();
    else if (msec < 0)
      msec = 0;
    return static_cast<int>(msec);
  }

  template <typename EndpointSequence>
  void connect_to_endpoints(const EndpointSequence& endpoints)
  {
    this->connect_to_endpoints(endpoints.begin(), endpoints.end());
  }

  template <typename EndpointIterator>
  void connect_to_endpoints(EndpointIterator begin, EndpointIterator end)
  {
#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
    ec_ = boost::asio::error::operation_not_supported;
#else // defined(BOOST_ASIO_WINDOWS_RUNTIME)
    if (ec_)
      return;

    ec_ = boost::asio::error::not_found;
    for (EndpointIterator i = begin; i != end; ++i)
    {
      // Check if we are past the expiry time.
      if (traits_helper::less_than(expiry_time_, traits_helper::now()))
      {
        ec_ = boost::asio::error::timed_out;
        return;
      }

      // Close and reopen the socket.
      typename Protocol::endpoint ep(*i);
      socket().close(ec_);
      socket().open(ep.protocol(), ec_);
      if (ec_)
        continue;

      // Try to complete the operation without blocking.
      if (!socket().native_non_blocking())
        socket().native_non_blocking(true, ec_);
      detail::socket_ops::connect(socket().native_handle(),
          ep.data(), ep.size(), ec_);

      // Check if operation succeeded.
      if (!ec_)
        return;

      // Operation failed.
      if (ec_ != boost::asio::error::in_progress
          && ec_ != boost::asio::error::would_block)
        continue;

      // Wait for socket to become ready.
      if (detail::socket_ops::poll_connect(
            socket().native_handle(), timeout(), ec_) < 0)
        continue;

      // Get the error code from the connect operation.
      int connect_error = 0;
      size_t connect_error_len = sizeof(connect_error);
      if (detail::socket_ops::getsockopt(socket().native_handle(), 0,
            SOL_SOCKET, SO_ERROR, &connect_error, &connect_error_len, ec_)
          == detail::socket_error_retval)
        return;

      // Check the result of the connect operation.
      ec_ = boost::system::error_code(connect_error,
          boost::asio::error::get_system_category());
      if (!ec_)
        return;
    }
#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)
  }

  // Helper function to get the maximum expiry time.
  static time_point max_expiry_time()
  {
#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME) \
  && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
    return boost::posix_time::pos_infin;
#else // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
      // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
    return (time_point::max)();
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
       // && defined(BOOST_ASIO_USE_BOOST_DATE_TIME_FOR_SOCKET_IOSTREAM)
  }

  enum { putback_max = 8 };
  boost::system::error_code ec_;
  time_point expiry_time_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if !defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
# undef BOOST_ASIO_PRIVATE_CONNECT_DEF
#endif // !defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

#endif // BOOST_ASIO_BASIC_SOCKET_STREAMBUF_HPP
