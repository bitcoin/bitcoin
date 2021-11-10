//
// detail/win_iocp_overlapped_ptr.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_WIN_IOCP_OVERLAPPED_PTR_HPP
#define BOOST_ASIO_DETAIL_WIN_IOCP_OVERLAPPED_PTR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_IOCP)

#include <boost/asio/io_context.hpp>
#include <boost/asio/query.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/detail/win_iocp_overlapped_op.hpp>
#include <boost/asio/detail/win_iocp_io_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Wraps a handler to create an OVERLAPPED object for use with overlapped I/O.
class win_iocp_overlapped_ptr
  : private noncopyable
{
public:
  // Construct an empty win_iocp_overlapped_ptr.
  win_iocp_overlapped_ptr()
    : ptr_(0),
      iocp_service_(0)
  {
  }

  // Construct an win_iocp_overlapped_ptr to contain the specified handler.
  template <typename Executor, typename Handler>
  explicit win_iocp_overlapped_ptr(const Executor& ex,
      BOOST_ASIO_MOVE_ARG(Handler) handler)
    : ptr_(0),
      iocp_service_(0)
  {
    this->reset(ex, BOOST_ASIO_MOVE_CAST(Handler)(handler));
  }

  // Destructor automatically frees the OVERLAPPED object unless released.
  ~win_iocp_overlapped_ptr()
  {
    reset();
  }

  // Reset to empty.
  void reset()
  {
    if (ptr_)
    {
      ptr_->destroy();
      ptr_ = 0;
      iocp_service_->work_finished();
      iocp_service_ = 0;
    }
  }

  // Reset to contain the specified handler, freeing any current OVERLAPPED
  // object.
  template <typename Executor, typename Handler>
  void reset(const Executor& ex, Handler handler)
  {
    win_iocp_io_context* iocp_service = this->get_iocp_service(ex);

    typedef win_iocp_overlapped_op<Handler, Executor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(handler, ex);

    BOOST_ASIO_HANDLER_CREATION((ex.context(), *p.p,
          "iocp_service", iocp_service, 0, "overlapped"));

    iocp_service->work_started();
    reset();
    ptr_ = p.p;
    p.v = p.p = 0;
    iocp_service_ = iocp_service;
  }

  // Get the contained OVERLAPPED object.
  OVERLAPPED* get()
  {
    return ptr_;
  }

  // Get the contained OVERLAPPED object.
  const OVERLAPPED* get() const
  {
    return ptr_;
  }

  // Release ownership of the OVERLAPPED object.
  OVERLAPPED* release()
  {
    if (ptr_)
      iocp_service_->on_pending(ptr_);

    OVERLAPPED* tmp = ptr_;
    ptr_ = 0;
    iocp_service_ = 0;
    return tmp;
  }

  // Post completion notification for overlapped operation. Releases ownership.
  void complete(const boost::system::error_code& ec,
      std::size_t bytes_transferred)
  {
    if (ptr_)
    {
      iocp_service_->on_completion(ptr_, ec,
          static_cast<DWORD>(bytes_transferred));
      ptr_ = 0;
      iocp_service_ = 0;
    }
  }

private:
  template <typename Executor>
  static win_iocp_io_context* get_iocp_service(const Executor& ex,
      typename enable_if<
        can_query<const Executor&, execution::context_t>::value
      >::type* = 0)
  {
    return &use_service<win_iocp_io_context>(
        boost::asio::query(ex, execution::context));
  }

  template <typename Executor>
  static win_iocp_io_context* get_iocp_service(const Executor& ex,
      typename enable_if<
        !can_query<const Executor&, execution::context_t>::value
      >::type* = 0)
  {
    return &use_service<win_iocp_io_context>(ex.context());
  }

  static win_iocp_io_context* get_iocp_service(
      const io_context::executor_type& ex)
  {
    return &boost::asio::query(ex, execution::context).impl_;
  }

  win_iocp_operation* ptr_;
  win_iocp_io_context* iocp_service_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_IOCP)

#endif // BOOST_ASIO_DETAIL_WIN_IOCP_OVERLAPPED_PTR_HPP
