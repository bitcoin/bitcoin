//
// impl/system_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_SYSTEM_EXECUTOR_HPP
#define BOOST_ASIO_IMPL_SYSTEM_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/executor_op.hpp>
#include <boost/asio/detail/global.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/system_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename Blocking, typename Relationship, typename Allocator>
inline system_context&
basic_system_executor<Blocking, Relationship, Allocator>::query(
    execution::context_t) BOOST_ASIO_NOEXCEPT
{
  return detail::global<system_context>();
}

template <typename Blocking, typename Relationship, typename Allocator>
inline std::size_t
basic_system_executor<Blocking, Relationship, Allocator>::query(
    execution::occupancy_t) const BOOST_ASIO_NOEXCEPT
{
  return detail::global<system_context>().num_threads_;
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function>
inline void
basic_system_executor<Blocking, Relationship, Allocator>::do_execute(
    BOOST_ASIO_MOVE_ARG(Function) f, execution::blocking_t::possibly_t) const
{
  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
  try
  {
#endif// !defined(BOOST_ASIO_NO_EXCEPTIONS)
    detail::fenced_block b(detail::fenced_block::full);
    boost_asio_handler_invoke_helpers::invoke(f2.value, f2.value);
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
  }
  catch (...)
  {
    std::terminate();
  }
#endif// !defined(BOOST_ASIO_NO_EXCEPTIONS)
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function>
inline void
basic_system_executor<Blocking, Relationship, Allocator>::do_execute(
    BOOST_ASIO_MOVE_ARG(Function) f, execution::blocking_t::always_t) const
{
  // Obtain a non-const instance of the function.
  detail::non_const_lvalue<Function> f2(f);

#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
  try
  {
#endif// !defined(BOOST_ASIO_NO_EXCEPTIONS)
    detail::fenced_block b(detail::fenced_block::full);
    boost_asio_handler_invoke_helpers::invoke(f2.value, f2.value);
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
  }
  catch (...)
  {
    std::terminate();
  }
#endif// !defined(BOOST_ASIO_NO_EXCEPTIONS)
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function>
void basic_system_executor<Blocking, Relationship, Allocator>::do_execute(
    BOOST_ASIO_MOVE_ARG(Function) f, execution::blocking_t::never_t) const
{
  system_context& ctx = detail::global<system_context>();

  // Allocate and construct an operation to wrap the function.
  typedef typename decay<Function>::type function_type;
  typedef detail::executor_op<function_type, Allocator> op;
  typename op::ptr p = { detail::addressof(allocator_),
      op::ptr::allocate(allocator_), 0 };
  p.p = new (p.v) op(BOOST_ASIO_MOVE_CAST(Function)(f), allocator_);

  if (is_same<Relationship, execution::relationship_t::continuation_t>::value)
  {
    BOOST_ASIO_HANDLER_CREATION((ctx, *p.p,
          "system_executor", &ctx, 0, "execute(blk=never,rel=cont)"));
  }
  else
  {
    BOOST_ASIO_HANDLER_CREATION((ctx, *p.p,
          "system_executor", &ctx, 0, "execute(blk=never,rel=fork)"));
  }

  ctx.scheduler_.post_immediate_completion(p.p,
      is_same<Relationship, execution::relationship_t::continuation_t>::value);
  p.v = p.p = 0;
}

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)
template <typename Blocking, typename Relationship, typename Allocator>
inline system_context& basic_system_executor<
    Blocking, Relationship, Allocator>::context() const BOOST_ASIO_NOEXCEPT
{
  return detail::global<system_context>();
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function, typename OtherAllocator>
void basic_system_executor<Blocking, Relationship, Allocator>::dispatch(
    BOOST_ASIO_MOVE_ARG(Function) f, const OtherAllocator&) const
{
  typename decay<Function>::type tmp(BOOST_ASIO_MOVE_CAST(Function)(f));
  boost_asio_handler_invoke_helpers::invoke(tmp, tmp);
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function, typename OtherAllocator>
void basic_system_executor<Blocking, Relationship, Allocator>::post(
    BOOST_ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  system_context& ctx = detail::global<system_context>();

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(BOOST_ASIO_MOVE_CAST(Function)(f), a);

  BOOST_ASIO_HANDLER_CREATION((ctx, *p.p,
        "system_executor", &this->context(), 0, "post"));

  ctx.scheduler_.post_immediate_completion(p.p, false);
  p.v = p.p = 0;
}

template <typename Blocking, typename Relationship, typename Allocator>
template <typename Function, typename OtherAllocator>
void basic_system_executor<Blocking, Relationship, Allocator>::defer(
    BOOST_ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const
{
  typedef typename decay<Function>::type function_type;

  system_context& ctx = detail::global<system_context>();

  // Allocate and construct an operation to wrap the function.
  typedef detail::executor_op<function_type, OtherAllocator> op;
  typename op::ptr p = { detail::addressof(a), op::ptr::allocate(a), 0 };
  p.p = new (p.v) op(BOOST_ASIO_MOVE_CAST(Function)(f), a);

  BOOST_ASIO_HANDLER_CREATION((ctx, *p.p,
        "system_executor", &this->context(), 0, "defer"));

  ctx.scheduler_.post_immediate_completion(p.p, true);
  p.v = p.p = 0;
}
#endif // !defined(BOOST_ASIO_NO_TS_EXECUTORS)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_SYSTEM_EXECUTOR_HPP
