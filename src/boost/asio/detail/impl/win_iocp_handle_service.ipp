//
// detail/impl/win_iocp_handle_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_WIN_IOCP_HANDLE_SERVICE_IPP
#define BOOST_ASIO_DETAIL_IMPL_WIN_IOCP_HANDLE_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_IOCP)

#include <boost/asio/detail/win_iocp_handle_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class win_iocp_handle_service::overlapped_wrapper
  : public OVERLAPPED
{
public:
  explicit overlapped_wrapper(boost::system::error_code& ec)
  {
    Internal = 0;
    InternalHigh = 0;
    Offset = 0;
    OffsetHigh = 0;

    // Create a non-signalled manual-reset event, for GetOverlappedResult.
    hEvent = ::CreateEventW(0, TRUE, FALSE, 0);
    if (hEvent)
    {
      // As documented in GetQueuedCompletionStatus, setting the low order
      // bit of this event prevents our synchronous writes from being treated
      // as completion port events.
      DWORD_PTR tmp = reinterpret_cast<DWORD_PTR>(hEvent);
      hEvent = reinterpret_cast<HANDLE>(tmp | 1);
    }
    else
    {
      DWORD last_error = ::GetLastError();
      ec = boost::system::error_code(last_error,
          boost::asio::error::get_system_category());
    }
  }

  ~overlapped_wrapper()
  {
    if (hEvent)
    {
      ::CloseHandle(hEvent);
    }
  }
};

win_iocp_handle_service::win_iocp_handle_service(execution_context& context)
  : execution_context_service_base<win_iocp_handle_service>(context),
    iocp_service_(boost::asio::use_service<win_iocp_io_context>(context)),
    mutex_(),
    impl_list_(0)
{
}

void win_iocp_handle_service::shutdown()
{
  // Close all implementations, causing all operations to complete.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  implementation_type* impl = impl_list_;
  while (impl)
  {
    close_for_destruction(*impl);
    impl = impl->next_;
  }
}

void win_iocp_handle_service::construct(
    win_iocp_handle_service::implementation_type& impl)
{
  impl.handle_ = INVALID_HANDLE_VALUE;
  impl.safe_cancellation_thread_id_ = 0;

  // Insert implementation into linked list of all implementations.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  impl.next_ = impl_list_;
  impl.prev_ = 0;
  if (impl_list_)
    impl_list_->prev_ = &impl;
  impl_list_ = &impl;
}

void win_iocp_handle_service::move_construct(
    win_iocp_handle_service::implementation_type& impl,
    win_iocp_handle_service::implementation_type& other_impl)
{
  impl.handle_ = other_impl.handle_;
  other_impl.handle_ = INVALID_HANDLE_VALUE;

  impl.safe_cancellation_thread_id_ = other_impl.safe_cancellation_thread_id_;
  other_impl.safe_cancellation_thread_id_ = 0;

  // Insert implementation into linked list of all implementations.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  impl.next_ = impl_list_;
  impl.prev_ = 0;
  if (impl_list_)
    impl_list_->prev_ = &impl;
  impl_list_ = &impl;
}

void win_iocp_handle_service::move_assign(
    win_iocp_handle_service::implementation_type& impl,
    win_iocp_handle_service& other_service,
    win_iocp_handle_service::implementation_type& other_impl)
{
  close_for_destruction(impl);

  if (this != &other_service)
  {
    // Remove implementation from linked list of all implementations.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    if (impl_list_ == &impl)
      impl_list_ = impl.next_;
    if (impl.prev_)
      impl.prev_->next_ = impl.next_;
    if (impl.next_)
      impl.next_->prev_= impl.prev_;
    impl.next_ = 0;
    impl.prev_ = 0;
  }

  impl.handle_ = other_impl.handle_;
  other_impl.handle_ = INVALID_HANDLE_VALUE;

  impl.safe_cancellation_thread_id_ = other_impl.safe_cancellation_thread_id_;
  other_impl.safe_cancellation_thread_id_ = 0;

  if (this != &other_service)
  {
    // Insert implementation into linked list of all implementations.
    boost::asio::detail::mutex::scoped_lock lock(other_service.mutex_);
    impl.next_ = other_service.impl_list_;
    impl.prev_ = 0;
    if (other_service.impl_list_)
      other_service.impl_list_->prev_ = &impl;
    other_service.impl_list_ = &impl;
  }
}

void win_iocp_handle_service::destroy(
    win_iocp_handle_service::implementation_type& impl)
{
  close_for_destruction(impl);
  
  // Remove implementation from linked list of all implementations.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  if (impl_list_ == &impl)
    impl_list_ = impl.next_;
  if (impl.prev_)
    impl.prev_->next_ = impl.next_;
  if (impl.next_)
    impl.next_->prev_= impl.prev_;
  impl.next_ = 0;
  impl.prev_ = 0;
}

boost::system::error_code win_iocp_handle_service::assign(
    win_iocp_handle_service::implementation_type& impl,
    const native_handle_type& handle, boost::system::error_code& ec)
{
  if (is_open(impl))
  {
    ec = boost::asio::error::already_open;
    return ec;
  }

  if (iocp_service_.register_handle(handle, ec))
    return ec;

  impl.handle_ = handle;
  ec = boost::system::error_code();
  return ec;
}

boost::system::error_code win_iocp_handle_service::close(
    win_iocp_handle_service::implementation_type& impl,
    boost::system::error_code& ec)
{
  if (is_open(impl))
  {
    BOOST_ASIO_HANDLER_OPERATION((iocp_service_.context(), "handle",
          &impl, reinterpret_cast<uintmax_t>(impl.handle_), "close"));

    if (!::CloseHandle(impl.handle_))
    {
      DWORD last_error = ::GetLastError();
      ec = boost::system::error_code(last_error,
          boost::asio::error::get_system_category());
    }
    else
    {
      ec = boost::system::error_code();
    }

    impl.handle_ = INVALID_HANDLE_VALUE;
    impl.safe_cancellation_thread_id_ = 0;
  }
  else
  {
    ec = boost::system::error_code();
  }

  return ec;
}

boost::system::error_code win_iocp_handle_service::cancel(
    win_iocp_handle_service::implementation_type& impl,
    boost::system::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return ec;
  }

  BOOST_ASIO_HANDLER_OPERATION((iocp_service_.context(), "handle",
        &impl, reinterpret_cast<uintmax_t>(impl.handle_), "cancel"));

  if (FARPROC cancel_io_ex_ptr = ::GetProcAddress(
        ::GetModuleHandleA("KERNEL32"), "CancelIoEx"))
  {
    // The version of Windows supports cancellation from any thread.
    typedef BOOL (WINAPI* cancel_io_ex_t)(HANDLE, LPOVERLAPPED);
    cancel_io_ex_t cancel_io_ex = reinterpret_cast<cancel_io_ex_t>(
        reinterpret_cast<void*>(cancel_io_ex_ptr));
    if (!cancel_io_ex(impl.handle_, 0))
    {
      DWORD last_error = ::GetLastError();
      if (last_error == ERROR_NOT_FOUND)
      {
        // ERROR_NOT_FOUND means that there were no operations to be
        // cancelled. We swallow this error to match the behaviour on other
        // platforms.
        ec = boost::system::error_code();
      }
      else
      {
        ec = boost::system::error_code(last_error,
            boost::asio::error::get_system_category());
      }
    }
    else
    {
      ec = boost::system::error_code();
    }
  }
  else if (impl.safe_cancellation_thread_id_ == 0)
  {
    // No operations have been started, so there's nothing to cancel.
    ec = boost::system::error_code();
  }
  else if (impl.safe_cancellation_thread_id_ == ::GetCurrentThreadId())
  {
    // Asynchronous operations have been started from the current thread only,
    // so it is safe to try to cancel them using CancelIo.
    if (!::CancelIo(impl.handle_))
    {
      DWORD last_error = ::GetLastError();
      ec = boost::system::error_code(last_error,
          boost::asio::error::get_system_category());
    }
    else
    {
      ec = boost::system::error_code();
    }
  }
  else
  {
    // Asynchronous operations have been started from more than one thread,
    // so cancellation is not safe.
    ec = boost::asio::error::operation_not_supported;
  }

  return ec;
}

size_t win_iocp_handle_service::do_write(
    win_iocp_handle_service::implementation_type& impl, uint64_t offset,
    const boost::asio::const_buffer& buffer, boost::system::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  // A request to write 0 bytes on a handle is a no-op.
  if (buffer.size() == 0)
  {
    ec = boost::system::error_code();
    return 0;
  }

  overlapped_wrapper overlapped(ec);
  if (ec)
  {
    return 0;
  }

  // Write the data. 
  overlapped.Offset = offset & 0xFFFFFFFF;
  overlapped.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
  BOOL ok = ::WriteFile(impl.handle_, buffer.data(),
      static_cast<DWORD>(buffer.size()), 0, &overlapped);
  if (!ok) 
  {
    DWORD last_error = ::GetLastError();
    if (last_error != ERROR_IO_PENDING)
    {
      ec = boost::system::error_code(last_error,
          boost::asio::error::get_system_category());
      return 0;
    }
  }

  // Wait for the operation to complete.
  DWORD bytes_transferred = 0;
  ok = ::GetOverlappedResult(impl.handle_,
      &overlapped, &bytes_transferred, TRUE);
  if (!ok)
  {
    DWORD last_error = ::GetLastError();
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return 0;
  }

  ec = boost::system::error_code();
  return bytes_transferred;
}

void win_iocp_handle_service::start_write_op(
    win_iocp_handle_service::implementation_type& impl, uint64_t offset,
    const boost::asio::const_buffer& buffer, operation* op)
{
  update_cancellation_thread_id(impl);
  iocp_service_.work_started();

  if (!is_open(impl))
  {
    iocp_service_.on_completion(op, boost::asio::error::bad_descriptor);
  }
  else if (buffer.size() == 0)
  {
    // A request to write 0 bytes on a handle is a no-op.
    iocp_service_.on_completion(op);
  }
  else
  {
    DWORD bytes_transferred = 0;
    op->Offset = offset & 0xFFFFFFFF;
    op->OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
    BOOL ok = ::WriteFile(impl.handle_, buffer.data(),
        static_cast<DWORD>(buffer.size()),
        &bytes_transferred, op);
    DWORD last_error = ::GetLastError();
    if (!ok && last_error != ERROR_IO_PENDING
        && last_error != ERROR_MORE_DATA)
    {
      iocp_service_.on_completion(op, last_error, bytes_transferred);
    }
    else
    {
      iocp_service_.on_pending(op);
    }
  }
}

size_t win_iocp_handle_service::do_read(
    win_iocp_handle_service::implementation_type& impl, uint64_t offset,
    const boost::asio::mutable_buffer& buffer, boost::system::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }
  
  // A request to read 0 bytes on a stream handle is a no-op.
  if (buffer.size() == 0)
  {
    ec = boost::system::error_code();
    return 0;
  }

  overlapped_wrapper overlapped(ec);
  if (ec)
  {
    return 0;
  }

  // Read some data.
  overlapped.Offset = offset & 0xFFFFFFFF;
  overlapped.OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
  BOOL ok = ::ReadFile(impl.handle_, buffer.data(),
      static_cast<DWORD>(buffer.size()), 0, &overlapped);
  if (!ok) 
  {
    DWORD last_error = ::GetLastError();
    if (last_error != ERROR_IO_PENDING && last_error != ERROR_MORE_DATA)
    {
      if (last_error == ERROR_HANDLE_EOF)
      {
        ec = boost::asio::error::eof;
      }
      else
      {
        ec = boost::system::error_code(last_error,
            boost::asio::error::get_system_category());
      }
      return 0;
    }
  }

  // Wait for the operation to complete.
  DWORD bytes_transferred = 0;
  ok = ::GetOverlappedResult(impl.handle_,
      &overlapped, &bytes_transferred, TRUE);
  if (!ok)
  {
    DWORD last_error = ::GetLastError();
    if (last_error == ERROR_HANDLE_EOF)
    {
      ec = boost::asio::error::eof;
    }
    else
    {
      ec = boost::system::error_code(last_error,
          boost::asio::error::get_system_category());
    }
    return (last_error == ERROR_MORE_DATA) ? bytes_transferred : 0;
  }

  ec = boost::system::error_code();
  return bytes_transferred;
}

void win_iocp_handle_service::start_read_op(
    win_iocp_handle_service::implementation_type& impl, uint64_t offset,
    const boost::asio::mutable_buffer& buffer, operation* op)
{
  update_cancellation_thread_id(impl);
  iocp_service_.work_started();

  if (!is_open(impl))
  {
    iocp_service_.on_completion(op, boost::asio::error::bad_descriptor);
  }
  else if (buffer.size() == 0)
  {
    // A request to read 0 bytes on a handle is a no-op.
    iocp_service_.on_completion(op);
  }
  else
  {
    DWORD bytes_transferred = 0;
    op->Offset = offset & 0xFFFFFFFF;
    op->OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
    BOOL ok = ::ReadFile(impl.handle_, buffer.data(),
        static_cast<DWORD>(buffer.size()),
        &bytes_transferred, op);
    DWORD last_error = ::GetLastError();
    if (!ok && last_error != ERROR_IO_PENDING
        && last_error != ERROR_MORE_DATA)
    {
      iocp_service_.on_completion(op, last_error, bytes_transferred);
    }
    else
    {
      iocp_service_.on_pending(op);
    }
  }
}

void win_iocp_handle_service::update_cancellation_thread_id(
    win_iocp_handle_service::implementation_type& impl)
{
  if (impl.safe_cancellation_thread_id_ == 0)
    impl.safe_cancellation_thread_id_ = ::GetCurrentThreadId();
  else if (impl.safe_cancellation_thread_id_ != ::GetCurrentThreadId())
    impl.safe_cancellation_thread_id_ = ~DWORD(0);
}

void win_iocp_handle_service::close_for_destruction(implementation_type& impl)
{
  if (is_open(impl))
  {
    BOOST_ASIO_HANDLER_OPERATION((iocp_service_.context(), "handle",
          &impl, reinterpret_cast<uintmax_t>(impl.handle_), "close"));

    ::CloseHandle(impl.handle_);
    impl.handle_ = INVALID_HANDLE_VALUE;
    impl.safe_cancellation_thread_id_ = 0;
  }
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_IOCP)

#endif // BOOST_ASIO_DETAIL_IMPL_WIN_IOCP_HANDLE_SERVICE_IPP
