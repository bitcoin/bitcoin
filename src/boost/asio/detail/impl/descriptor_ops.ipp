//
// detail/impl/descriptor_ops.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_DESCRIPTOR_OPS_IPP
#define BOOST_ASIO_DETAIL_IMPL_DESCRIPTOR_OPS_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cerrno>
#include <boost/asio/detail/descriptor_ops.hpp>
#include <boost/asio/error.hpp>

#if !defined(BOOST_ASIO_WINDOWS) \
  && !defined(BOOST_ASIO_WINDOWS_RUNTIME) \
  && !defined(__CYGWIN__)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {
namespace descriptor_ops {

int open(const char* path, int flags, boost::system::error_code& ec)
{
  int result = ::open(path, flags);
  get_last_error(ec, result < 0);
  return result;
}

int close(int d, state_type& state, boost::system::error_code& ec)
{
  int result = 0;
  if (d != -1)
  {
    result = ::close(d);
    get_last_error(ec, result < 0);

    if (result != 0
        && (ec == boost::asio::error::would_block
          || ec == boost::asio::error::try_again))
    {
      // According to UNIX Network Programming Vol. 1, it is possible for
      // close() to fail with EWOULDBLOCK under certain circumstances. What
      // isn't clear is the state of the descriptor after this error. The one
      // current OS where this behaviour is seen, Windows, says that the socket
      // remains open. Therefore we'll put the descriptor back into blocking
      // mode and have another attempt at closing it.
#if defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
      int flags = ::fcntl(d, F_GETFL, 0);
      if (flags >= 0)
        ::fcntl(d, F_SETFL, flags & ~O_NONBLOCK);
#else // defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
      ioctl_arg_type arg = 0;
      ::ioctl(d, FIONBIO, &arg);
#endif // defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
      state &= ~non_blocking;

      result = ::close(d);
      get_last_error(ec, result < 0);
    }
  }

  return result;
}

bool set_user_non_blocking(int d, state_type& state,
    bool value, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return false;
  }

#if defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
  int result = ::fcntl(d, F_GETFL, 0);
  get_last_error(ec, result < 0);
  if (result >= 0)
  {
    int flag = (value ? (result | O_NONBLOCK) : (result & ~O_NONBLOCK));
    result = ::fcntl(d, F_SETFL, flag);
    get_last_error(ec, result < 0);
  }
#else // defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
  ioctl_arg_type arg = (value ? 1 : 0);
  int result = ::ioctl(d, FIONBIO, &arg);
  get_last_error(ec, result < 0);
#endif // defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)

  if (result >= 0)
  {
    if (value)
      state |= user_set_non_blocking;
    else
    {
      // Clearing the user-set non-blocking mode always overrides any
      // internally-set non-blocking flag. Any subsequent asynchronous
      // operations will need to re-enable non-blocking I/O.
      state &= ~(user_set_non_blocking | internal_non_blocking);
    }
    return true;
  }

  return false;
}

bool set_internal_non_blocking(int d, state_type& state,
    bool value, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return false;
  }

  if (!value && (state & user_set_non_blocking))
  {
    // It does not make sense to clear the internal non-blocking flag if the
    // user still wants non-blocking behaviour. Return an error and let the
    // caller figure out whether to update the user-set non-blocking flag.
    ec = boost::asio::error::invalid_argument;
    return false;
  }

#if defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
  int result = ::fcntl(d, F_GETFL, 0);
  get_last_error(ec, result < 0);
  if (result >= 0)
  {
    int flag = (value ? (result | O_NONBLOCK) : (result & ~O_NONBLOCK));
    result = ::fcntl(d, F_SETFL, flag);
    get_last_error(ec, result < 0);
  }
#else // defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)
  ioctl_arg_type arg = (value ? 1 : 0);
  int result = ::ioctl(d, FIONBIO, &arg);
  get_last_error(ec, result < 0);
#endif // defined(__SYMBIAN32__) || defined(__EMSCRIPTEN__)

  if (result >= 0)
  {
    if (value)
      state |= internal_non_blocking;
    else
      state &= ~internal_non_blocking;
    return true;
  }

  return false;
}

std::size_t sync_read(int d, state_type state, buf* bufs,
    std::size_t count, bool all_empty, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  // A request to read 0 bytes on a stream is a no-op.
  if (all_empty)
  {
    ec.assign(0, ec.category());
    return 0;
  }

  // Read some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = ::readv(d, bufs, static_cast<int>(count));
    get_last_error(ec, bytes < 0);

    // Check if operation succeeded.
    if (bytes > 0)
      return bytes;

    // Check for EOF.
    if (bytes == 0)
    {
      ec = boost::asio::error::eof;
      return 0;
    }

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != boost::asio::error::would_block
          && ec != boost::asio::error::try_again))
      return 0;

    // Wait for descriptor to become ready.
    if (descriptor_ops::poll_read(d, 0, ec) < 0)
      return 0;
  }
}

std::size_t sync_read1(int d, state_type state, void* data,
    std::size_t size, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  // A request to read 0 bytes on a stream is a no-op.
  if (size == 0)
  {
    ec.assign(0, ec.category());
    return 0;
  }

  // Read some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = ::read(d, data, size);
    get_last_error(ec, bytes < 0);

    // Check if operation succeeded.
    if (bytes > 0)
      return bytes;

    // Check for EOF.
    if (bytes == 0)
    {
      ec = boost::asio::error::eof;
      return 0;
    }

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != boost::asio::error::would_block
          && ec != boost::asio::error::try_again))
      return 0;

    // Wait for descriptor to become ready.
    if (descriptor_ops::poll_read(d, 0, ec) < 0)
      return 0;
  }
}

bool non_blocking_read(int d, buf* bufs, std::size_t count,
    boost::system::error_code& ec, std::size_t& bytes_transferred)
{
  for (;;)
  {
    // Read some data.
    signed_size_type bytes = ::readv(d, bufs, static_cast<int>(count));
    get_last_error(ec, bytes < 0);

    // Check for end of stream.
    if (bytes == 0)
    {
      ec = boost::asio::error::eof;
      return true;
    }

    // Check if operation succeeded.
    if (bytes > 0)
    {
      bytes_transferred = bytes;
      return true;
    }

    // Retry operation if interrupted by signal.
    if (ec == boost::asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == boost::asio::error::would_block
        || ec == boost::asio::error::try_again)
      return false;

    // Operation failed.
    bytes_transferred = 0;
    return true;
  }
}

bool non_blocking_read1(int d, void* data, std::size_t size,
    boost::system::error_code& ec, std::size_t& bytes_transferred)
{
  for (;;)
  {
    // Read some data.
    signed_size_type bytes = ::read(d, data, size);
    get_last_error(ec, bytes < 0);

    // Check for end of stream.
    if (bytes == 0)
    {
      ec = boost::asio::error::eof;
      return true;
    }

    // Check if operation succeeded.
    if (bytes > 0)
    {
      bytes_transferred = bytes;
      return true;
    }

    // Retry operation if interrupted by signal.
    if (ec == boost::asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == boost::asio::error::would_block
        || ec == boost::asio::error::try_again)
      return false;

    // Operation failed.
    bytes_transferred = 0;
    return true;
  }
}

std::size_t sync_write(int d, state_type state, const buf* bufs,
    std::size_t count, bool all_empty, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  // A request to write 0 bytes on a stream is a no-op.
  if (all_empty)
  {
    ec.assign(0, ec.category());
    return 0;
  }

  // Write some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = ::writev(d, bufs, static_cast<int>(count));
    get_last_error(ec, bytes < 0);

    // Check if operation succeeded.
    if (bytes > 0)
      return bytes;

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != boost::asio::error::would_block
          && ec != boost::asio::error::try_again))
      return 0;

    // Wait for descriptor to become ready.
    if (descriptor_ops::poll_write(d, 0, ec) < 0)
      return 0;
  }
}

std::size_t sync_write1(int d, state_type state, const void* data,
    std::size_t size, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  // A request to write 0 bytes on a stream is a no-op.
  if (size == 0)
  {
    ec.assign(0, ec.category());
    return 0;
  }

  // Write some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = ::write(d, data, size);
    get_last_error(ec, bytes < 0);

    // Check if operation succeeded.
    if (bytes > 0)
      return bytes;

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != boost::asio::error::would_block
          && ec != boost::asio::error::try_again))
      return 0;

    // Wait for descriptor to become ready.
    if (descriptor_ops::poll_write(d, 0, ec) < 0)
      return 0;
  }
}

bool non_blocking_write(int d, const buf* bufs, std::size_t count,
    boost::system::error_code& ec, std::size_t& bytes_transferred)
{
  for (;;)
  {
    // Write some data.
    signed_size_type bytes = ::writev(d, bufs, static_cast<int>(count));
    get_last_error(ec, bytes < 0);

    // Check if operation succeeded.
    if (bytes >= 0)
    {
      bytes_transferred = bytes;
      return true;
    }

    // Retry operation if interrupted by signal.
    if (ec == boost::asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == boost::asio::error::would_block
        || ec == boost::asio::error::try_again)
      return false;

    // Operation failed.
    bytes_transferred = 0;
    return true;
  }
}

bool non_blocking_write1(int d, const void* data, std::size_t size,
    boost::system::error_code& ec, std::size_t& bytes_transferred)
{
  for (;;)
  {
    // Write some data.
    signed_size_type bytes = ::write(d, data, size);
    get_last_error(ec, bytes < 0);

    // Check if operation succeeded.
    if (bytes >= 0)
    {
      bytes_transferred = bytes;
      return true;
    }

    // Retry operation if interrupted by signal.
    if (ec == boost::asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == boost::asio::error::would_block
        || ec == boost::asio::error::try_again)
      return false;

    // Operation failed.
    bytes_transferred = 0;
    return true;
  }
}

int ioctl(int d, state_type& state, long cmd,
    ioctl_arg_type* arg, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return -1;
  }

  int result = ::ioctl(d, cmd, arg);
  get_last_error(ec, result < 0);

  if (result >= 0)
  {
    // When updating the non-blocking mode we always perform the ioctl syscall,
    // even if the flags would otherwise indicate that the descriptor is
    // already in the correct state. This ensures that the underlying
    // descriptor is put into the state that has been requested by the user. If
    // the ioctl syscall was successful then we need to update the flags to
    // match.
    if (cmd == static_cast<long>(FIONBIO))
    {
      if (*arg)
      {
        state |= user_set_non_blocking;
      }
      else
      {
        // Clearing the non-blocking mode always overrides any internally-set
        // non-blocking flag. Any subsequent asynchronous operations will need
        // to re-enable non-blocking I/O.
        state &= ~(user_set_non_blocking | internal_non_blocking);
      }
    }
  }

  return result;
}

int fcntl(int d, int cmd, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return -1;
  }

  int result = ::fcntl(d, cmd);
  get_last_error(ec, result < 0);
  return result;
}

int fcntl(int d, int cmd, long arg, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return -1;
  }

  int result = ::fcntl(d, cmd, arg);
  get_last_error(ec, result < 0);
  return result;
}

int poll_read(int d, state_type state, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return -1;
  }

  pollfd fds;
  fds.fd = d;
  fds.events = POLLIN;
  fds.revents = 0;
  int timeout = (state & user_set_non_blocking) ? 0 : -1;
  int result = ::poll(&fds, 1, timeout);
  get_last_error(ec, result < 0);
  if (result == 0)
    if (state & user_set_non_blocking)
      ec = boost::asio::error::would_block;
  return result;
}

int poll_write(int d, state_type state, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return -1;
  }

  pollfd fds;
  fds.fd = d;
  fds.events = POLLOUT;
  fds.revents = 0;
  int timeout = (state & user_set_non_blocking) ? 0 : -1;
  int result = ::poll(&fds, 1, timeout);
  get_last_error(ec, result < 0);
  if (result == 0)
    if (state & user_set_non_blocking)
      ec = boost::asio::error::would_block;
  return result;
}

int poll_error(int d, state_type state, boost::system::error_code& ec)
{
  if (d == -1)
  {
    ec = boost::asio::error::bad_descriptor;
    return -1;
  }

  pollfd fds;
  fds.fd = d;
  fds.events = POLLPRI | POLLERR | POLLHUP;
  fds.revents = 0;
  int timeout = (state & user_set_non_blocking) ? 0 : -1;
  int result = ::poll(&fds, 1, timeout);
  get_last_error(ec, result < 0);
  if (result == 0)
    if (state & user_set_non_blocking)
      ec = boost::asio::error::would_block;
  return result;
}

} // namespace descriptor_ops
} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // !defined(BOOST_ASIO_WINDOWS)
       //   && !defined(BOOST_ASIO_WINDOWS_RUNTIME)
       //   && !defined(__CYGWIN__)

#endif // BOOST_ASIO_DETAIL_IMPL_DESCRIPTOR_OPS_IPP
