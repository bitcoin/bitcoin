//
// impl/use_awaitable.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_USE_AWAITABLE_HPP
#define BOOST_ASIO_IMPL_USE_AWAITABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/cancellation_signal.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Executor, typename T>
class awaitable_handler_base
  : public awaitable_thread<Executor>
{
public:
  typedef void result_type;
  typedef awaitable<T, Executor> awaitable_type;

  // Construct from the entry point of a new thread of execution.
  awaitable_handler_base(awaitable<awaitable_thread_entry_point, Executor> a,
      const Executor& ex, cancellation_slot pcs, cancellation_state cs)
    : awaitable_thread<Executor>(std::move(a), ex, pcs, cs)
  {
  }

  // Transfer ownership from another awaitable_thread.
  explicit awaitable_handler_base(awaitable_thread<Executor>* h)
    : awaitable_thread<Executor>(std::move(*h))
  {
  }

protected:
  awaitable_frame<T, Executor>* frame() noexcept
  {
    return static_cast<awaitable_frame<T, Executor>*>(
        this->entry_point()->top_of_stack_);
  }
};

template <typename, typename...>
class awaitable_handler;

template <typename Executor>
class awaitable_handler<Executor>
  : public awaitable_handler_base<Executor, void>
{
public:
  using awaitable_handler_base<Executor, void>::awaitable_handler_base;

  void operator()()
  {
    this->frame()->attach_thread(this);
    this->frame()->return_void();
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor>
class awaitable_handler<Executor, boost::system::error_code>
  : public awaitable_handler_base<Executor, void>
{
public:
  using awaitable_handler_base<Executor, void>::awaitable_handler_base;

  void operator()(const boost::system::error_code& ec)
  {
    this->frame()->attach_thread(this);
    if (ec)
      this->frame()->set_error(ec);
    else
      this->frame()->return_void();
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor>
class awaitable_handler<Executor, std::exception_ptr>
  : public awaitable_handler_base<Executor, void>
{
public:
  using awaitable_handler_base<Executor, void>::awaitable_handler_base;

  void operator()(std::exception_ptr ex)
  {
    this->frame()->attach_thread(this);
    if (ex)
      this->frame()->set_except(ex);
    else
      this->frame()->return_void();
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor, typename T>
class awaitable_handler<Executor, T>
  : public awaitable_handler_base<Executor, T>
{
public:
  using awaitable_handler_base<Executor, T>::awaitable_handler_base;

  template <typename Arg>
  void operator()(Arg&& arg)
  {
    this->frame()->attach_thread(this);
    this->frame()->return_value(std::forward<Arg>(arg));
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor, typename T>
class awaitable_handler<Executor, boost::system::error_code, T>
  : public awaitable_handler_base<Executor, T>
{
public:
  using awaitable_handler_base<Executor, T>::awaitable_handler_base;

  template <typename Arg>
  void operator()(const boost::system::error_code& ec, Arg&& arg)
  {
    this->frame()->attach_thread(this);
    if (ec)
      this->frame()->set_error(ec);
    else
      this->frame()->return_value(std::forward<Arg>(arg));
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor, typename T>
class awaitable_handler<Executor, std::exception_ptr, T>
  : public awaitable_handler_base<Executor, T>
{
public:
  using awaitable_handler_base<Executor, T>::awaitable_handler_base;

  template <typename Arg>
  void operator()(std::exception_ptr ex, Arg&& arg)
  {
    this->frame()->attach_thread(this);
    if (ex)
      this->frame()->set_except(ex);
    else
      this->frame()->return_value(std::forward<Arg>(arg));
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor, typename... Ts>
class awaitable_handler
  : public awaitable_handler_base<Executor, std::tuple<Ts...>>
{
public:
  using awaitable_handler_base<Executor,
    std::tuple<Ts...>>::awaitable_handler_base;

  template <typename... Args>
  void operator()(Args&&... args)
  {
    this->frame()->attach_thread(this);
    this->frame()->return_values(std::forward<Args>(args)...);
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor, typename... Ts>
class awaitable_handler<Executor, boost::system::error_code, Ts...>
  : public awaitable_handler_base<Executor, std::tuple<Ts...>>
{
public:
  using awaitable_handler_base<Executor,
    std::tuple<Ts...>>::awaitable_handler_base;

  template <typename... Args>
  void operator()(const boost::system::error_code& ec, Args&&... args)
  {
    this->frame()->attach_thread(this);
    if (ec)
      this->frame()->set_error(ec);
    else
      this->frame()->return_values(std::forward<Args>(args)...);
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

template <typename Executor, typename... Ts>
class awaitable_handler<Executor, std::exception_ptr, Ts...>
  : public awaitable_handler_base<Executor, std::tuple<Ts...>>
{
public:
  using awaitable_handler_base<Executor,
    std::tuple<Ts...>>::awaitable_handler_base;

  template <typename... Args>
  void operator()(std::exception_ptr ex, Args&&... args)
  {
    this->frame()->attach_thread(this);
    if (ex)
      this->frame()->set_except(ex);
    else
      this->frame()->return_values(std::forward<Args>(args)...);
    this->frame()->clear_cancellation_slot();
    this->frame()->pop_frame();
    this->pump();
  }
};

} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

#if defined(_MSC_VER)
template <typename T>
T dummy_return()
{
  return std::move(*static_cast<T*>(nullptr));
}

template <>
inline void dummy_return()
{
}
#endif // defined(_MSC_VER)

template <typename Executor, typename R, typename... Args>
class async_result<use_awaitable_t<Executor>, R(Args...)>
{
public:
  typedef typename detail::awaitable_handler<
      Executor, typename decay<Args>::type...> handler_type;
  typedef typename handler_type::awaitable_type return_type;

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(Initiation initiation,
      use_awaitable_t<Executor> u, InitArgs... args)
  {
    (void)u;

    co_await [&](auto* frame)
      {
        BOOST_ASIO_HANDLER_LOCATION((u.file_name_, u.line_, u.function_name_));
        handler_type handler(frame->detach_thread());
        std::move(initiation)(std::move(handler), std::move(args)...);
        return static_cast<handler_type*>(nullptr);
      };

    for (;;) {} // Never reached.
#if defined(_MSC_VER)
    co_return dummy_return<typename return_type::value_type>();
#endif // defined(_MSC_VER)
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_USE_AWAITABLE_HPP
