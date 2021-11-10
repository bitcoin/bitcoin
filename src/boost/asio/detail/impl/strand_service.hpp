//
// detail/impl/strand_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_HPP
#define BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/completion_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/memory.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

inline strand_service::strand_impl::strand_impl()
  : operation(&strand_service::do_complete),
    locked_(false)
{
}

template <typename Handler>
void strand_service::dispatch(strand_service::implementation_type& impl,
    Handler& handler)
{
  // If we are already in the strand then the handler can run immediately.
  if (running_in_this_thread(impl))
  {
    fenced_block b(fenced_block::full);
    boost_asio_handler_invoke_helpers::invoke(handler, handler);
    return;
  }

  // Allocate and construct an operation to wrap the handler.
  typedef completion_handler<Handler, io_context::executor_type> op;
  typename op::ptr p = { boost::asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(handler, io_context_.get_executor());

  BOOST_ASIO_HANDLER_CREATION((this->context(),
        *p.p, "strand", impl, 0, "dispatch"));

  operation* o = p.p;
  p.v = p.p = 0;
  do_dispatch(impl, o);
}

// Request the io_context to invoke the given handler and return immediately.
template <typename Handler>
void strand_service::post(strand_service::implementation_type& impl,
    Handler& handler)
{
  bool is_continuation =
    boost_asio_handler_cont_helpers::is_continuation(handler);

  // Allocate and construct an operation to wrap the handler.
  typedef completion_handler<Handler, io_context::executor_type> op;
  typename op::ptr p = { boost::asio::detail::addressof(handler),
    op::ptr::allocate(handler), 0 };
  p.p = new (p.v) op(handler, io_context_.get_executor());

  BOOST_ASIO_HANDLER_CREATION((this->context(),
        *p.p, "strand", impl, 0, "post"));

  do_post(impl, p.p, is_continuation);
  p.v = p.p = 0;
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_IMPL_STRAND_SERVICE_HPP
