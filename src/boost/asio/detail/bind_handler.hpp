//
// detail/bind_handler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_BIND_HANDLER_HPP
#define BOOST_ASIO_DETAIL_BIND_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/associator.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Handler>
class binder0
{
public:
  template <typename T>
  binder0(int, BOOST_ASIO_MOVE_ARG(T) handler)
    : handler_(BOOST_ASIO_MOVE_CAST(T)(handler))
  {
  }

  binder0(Handler& handler)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler))
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  binder0(const binder0& other)
    : handler_(other.handler_)
  {
  }

  binder0(binder0&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)();
  }

  void operator()() const
  {
    handler_();
  }

//private:
  Handler handler_;
};

template <typename Handler>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    binder0<Handler>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    binder0<Handler>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline bool asio_handler_is_continuation(
    binder0<Handler>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    binder0<Handler>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    binder0<Handler>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline binder0<typename decay<Handler>::type> bind_handler(
    BOOST_ASIO_MOVE_ARG(Handler) handler)
{
  return binder0<typename decay<Handler>::type>(
      0, BOOST_ASIO_MOVE_CAST(Handler)(handler));
}

template <typename Handler, typename Arg1>
class binder1
{
public:
  template <typename T>
  binder1(int, BOOST_ASIO_MOVE_ARG(T) handler, const Arg1& arg1)
    : handler_(BOOST_ASIO_MOVE_CAST(T)(handler)),
      arg1_(arg1)
  {
  }

  binder1(Handler& handler, const Arg1& arg1)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(arg1)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  binder1(const binder1& other)
    : handler_(other.handler_),
      arg1_(other.arg1_)
  {
  }

  binder1(binder1&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        static_cast<const Arg1&>(arg1_));
  }

  void operator()() const
  {
    handler_(arg1_);
  }

//private:
  Handler handler_;
  Arg1 arg1_;
};

template <typename Handler, typename Arg1>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    binder1<Handler, Arg1>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    binder1<Handler, Arg1>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1>
inline bool asio_handler_is_continuation(
    binder1<Handler, Arg1>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler, typename Arg1>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    binder1<Handler, Arg1>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler, typename Arg1>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    binder1<Handler, Arg1>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1>
inline binder1<typename decay<Handler>::type, Arg1> bind_handler(
    BOOST_ASIO_MOVE_ARG(Handler) handler, const Arg1& arg1)
{
  return binder1<typename decay<Handler>::type, Arg1>(0,
      BOOST_ASIO_MOVE_CAST(Handler)(handler), arg1);
}

template <typename Handler, typename Arg1, typename Arg2>
class binder2
{
public:
  template <typename T>
  binder2(int, BOOST_ASIO_MOVE_ARG(T) handler,
      const Arg1& arg1, const Arg2& arg2)
    : handler_(BOOST_ASIO_MOVE_CAST(T)(handler)),
      arg1_(arg1),
      arg2_(arg2)
  {
  }

  binder2(Handler& handler, const Arg1& arg1, const Arg2& arg2)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(arg1),
      arg2_(arg2)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  binder2(const binder2& other)
    : handler_(other.handler_),
      arg1_(other.arg1_),
      arg2_(other.arg2_)
  {
  }

  binder2(binder2&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_)),
      arg2_(BOOST_ASIO_MOVE_CAST(Arg2)(other.arg2_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        static_cast<const Arg1&>(arg1_),
        static_cast<const Arg2&>(arg2_));
  }

  void operator()() const
  {
    handler_(arg1_, arg2_);
  }

//private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
};

template <typename Handler, typename Arg1, typename Arg2>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    binder2<Handler, Arg1, Arg2>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    binder2<Handler, Arg1, Arg2>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2>
inline bool asio_handler_is_continuation(
    binder2<Handler, Arg1, Arg2>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler, typename Arg1, typename Arg2>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    binder2<Handler, Arg1, Arg2>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler, typename Arg1, typename Arg2>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    binder2<Handler, Arg1, Arg2>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2>
inline binder2<typename decay<Handler>::type, Arg1, Arg2> bind_handler(
    BOOST_ASIO_MOVE_ARG(Handler) handler, const Arg1& arg1, const Arg2& arg2)
{
  return binder2<typename decay<Handler>::type, Arg1, Arg2>(0,
      BOOST_ASIO_MOVE_CAST(Handler)(handler), arg1, arg2);
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
class binder3
{
public:
  template <typename T>
  binder3(int, BOOST_ASIO_MOVE_ARG(T) handler, const Arg1& arg1,
      const Arg2& arg2, const Arg3& arg3)
    : handler_(BOOST_ASIO_MOVE_CAST(T)(handler)),
      arg1_(arg1),
      arg2_(arg2),
      arg3_(arg3)
  {
  }

  binder3(Handler& handler, const Arg1& arg1,
      const Arg2& arg2, const Arg3& arg3)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(arg1),
      arg2_(arg2),
      arg3_(arg3)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  binder3(const binder3& other)
    : handler_(other.handler_),
      arg1_(other.arg1_),
      arg2_(other.arg2_),
      arg3_(other.arg3_)
  {
  }

  binder3(binder3&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_)),
      arg2_(BOOST_ASIO_MOVE_CAST(Arg2)(other.arg2_)),
      arg3_(BOOST_ASIO_MOVE_CAST(Arg3)(other.arg3_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        static_cast<const Arg1&>(arg1_),
        static_cast<const Arg2&>(arg2_),
        static_cast<const Arg3&>(arg3_));
  }

  void operator()() const
  {
    handler_(arg1_, arg2_, arg3_);
  }

//private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;
};

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    binder3<Handler, Arg1, Arg2, Arg3>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    binder3<Handler, Arg1, Arg2, Arg3>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
inline bool asio_handler_is_continuation(
    binder3<Handler, Arg1, Arg2, Arg3>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler,
    typename Arg1, typename Arg2, typename Arg3>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    binder3<Handler, Arg1, Arg2, Arg3>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler,
    typename Arg1, typename Arg2, typename Arg3>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    binder3<Handler, Arg1, Arg2, Arg3>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
inline binder3<typename decay<Handler>::type, Arg1, Arg2, Arg3> bind_handler(
    BOOST_ASIO_MOVE_ARG(Handler) handler, const Arg1& arg1, const Arg2& arg2,
    const Arg3& arg3)
{
  return binder3<typename decay<Handler>::type, Arg1, Arg2, Arg3>(0,
      BOOST_ASIO_MOVE_CAST(Handler)(handler), arg1, arg2, arg3);
}

template <typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
class binder4
{
public:
  template <typename T>
  binder4(int, BOOST_ASIO_MOVE_ARG(T) handler, const Arg1& arg1,
      const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    : handler_(BOOST_ASIO_MOVE_CAST(T)(handler)),
      arg1_(arg1),
      arg2_(arg2),
      arg3_(arg3),
      arg4_(arg4)
  {
  }

  binder4(Handler& handler, const Arg1& arg1,
      const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(arg1),
      arg2_(arg2),
      arg3_(arg3),
      arg4_(arg4)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  binder4(const binder4& other)
    : handler_(other.handler_),
      arg1_(other.arg1_),
      arg2_(other.arg2_),
      arg3_(other.arg3_),
      arg4_(other.arg4_)
  {
  }

  binder4(binder4&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_)),
      arg2_(BOOST_ASIO_MOVE_CAST(Arg2)(other.arg2_)),
      arg3_(BOOST_ASIO_MOVE_CAST(Arg3)(other.arg3_)),
      arg4_(BOOST_ASIO_MOVE_CAST(Arg4)(other.arg4_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        static_cast<const Arg1&>(arg1_),
        static_cast<const Arg2&>(arg2_),
        static_cast<const Arg3&>(arg3_),
        static_cast<const Arg4&>(arg4_));
  }

  void operator()() const
  {
    handler_(arg1_, arg2_, arg3_, arg4_);
  }

//private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;
  Arg4 arg4_;
};

template <typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    binder4<Handler, Arg1, Arg2, Arg3, Arg4>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    binder4<Handler, Arg1, Arg2, Arg3, Arg4>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
inline bool asio_handler_is_continuation(
    binder4<Handler, Arg1, Arg2, Arg3, Arg4>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    binder4<Handler, Arg1, Arg2, Arg3, Arg4>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    binder4<Handler, Arg1, Arg2, Arg3, Arg4>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4>
inline binder4<typename decay<Handler>::type, Arg1, Arg2, Arg3, Arg4>
bind_handler(BOOST_ASIO_MOVE_ARG(Handler) handler, const Arg1& arg1,
    const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
{
  return binder4<typename decay<Handler>::type, Arg1, Arg2, Arg3, Arg4>(0,
      BOOST_ASIO_MOVE_CAST(Handler)(handler), arg1, arg2, arg3, arg4);
}

template <typename Handler, typename Arg1, typename Arg2,
    typename Arg3, typename Arg4, typename Arg5>
class binder5
{
public:
  template <typename T>
  binder5(int, BOOST_ASIO_MOVE_ARG(T) handler, const Arg1& arg1,
      const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    : handler_(BOOST_ASIO_MOVE_CAST(T)(handler)),
      arg1_(arg1),
      arg2_(arg2),
      arg3_(arg3),
      arg4_(arg4),
      arg5_(arg5)
  {
  }

  binder5(Handler& handler, const Arg1& arg1, const Arg2& arg2,
      const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(arg1),
      arg2_(arg2),
      arg3_(arg3),
      arg4_(arg4),
      arg5_(arg5)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  binder5(const binder5& other)
    : handler_(other.handler_),
      arg1_(other.arg1_),
      arg2_(other.arg2_),
      arg3_(other.arg3_),
      arg4_(other.arg4_),
      arg5_(other.arg5_)
  {
  }

  binder5(binder5&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_)),
      arg2_(BOOST_ASIO_MOVE_CAST(Arg2)(other.arg2_)),
      arg3_(BOOST_ASIO_MOVE_CAST(Arg3)(other.arg3_)),
      arg4_(BOOST_ASIO_MOVE_CAST(Arg4)(other.arg4_)),
      arg5_(BOOST_ASIO_MOVE_CAST(Arg5)(other.arg5_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        static_cast<const Arg1&>(arg1_),
        static_cast<const Arg2&>(arg2_),
        static_cast<const Arg3&>(arg3_),
        static_cast<const Arg4&>(arg4_),
        static_cast<const Arg5&>(arg5_));
  }

  void operator()() const
  {
    handler_(arg1_, arg2_, arg3_, arg4_, arg5_);
  }

//private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;
  Arg4 arg4_;
  Arg5 arg5_;
};

template <typename Handler, typename Arg1, typename Arg2,
    typename Arg3, typename Arg4, typename Arg5>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2,
    typename Arg3, typename Arg4, typename Arg5>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2,
    typename Arg3, typename Arg4, typename Arg5>
inline bool asio_handler_is_continuation(
    binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4, typename Arg5>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Function, typename Handler, typename Arg1,
    typename Arg2, typename Arg3, typename Arg4, typename Arg5>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(const Function& function,
    binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2,
    typename Arg3, typename Arg4, typename Arg5>
inline binder5<typename decay<Handler>::type, Arg1, Arg2, Arg3, Arg4, Arg5>
bind_handler(BOOST_ASIO_MOVE_ARG(Handler) handler, const Arg1& arg1,
    const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
{
  return binder5<typename decay<Handler>::type, Arg1, Arg2, Arg3, Arg4, Arg5>(0,
      BOOST_ASIO_MOVE_CAST(Handler)(handler), arg1, arg2, arg3, arg4, arg5);
}

#if defined(BOOST_ASIO_HAS_MOVE)

template <typename Handler, typename Arg1>
class move_binder1
{
public:
  move_binder1(int, BOOST_ASIO_MOVE_ARG(Handler) handler,
      BOOST_ASIO_MOVE_ARG(Arg1) arg1)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(arg1))
  {
  }

  move_binder1(move_binder1&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_))
  {
  }

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        BOOST_ASIO_MOVE_CAST(Arg1)(arg1_));
  }

//private:
  Handler handler_;
  Arg1 arg1_;
};

template <typename Handler, typename Arg1>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    move_binder1<Handler, Arg1>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    move_binder1<Handler, Arg1>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1>
inline bool asio_handler_is_continuation(
    move_binder1<Handler, Arg1>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler, typename Arg1>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(BOOST_ASIO_MOVE_ARG(Function) function,
    move_binder1<Handler, Arg1>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      BOOST_ASIO_MOVE_CAST(Function)(function), this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2>
class move_binder2
{
public:
  move_binder2(int, BOOST_ASIO_MOVE_ARG(Handler) handler,
      const Arg1& arg1, BOOST_ASIO_MOVE_ARG(Arg2) arg2)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      arg1_(arg1),
      arg2_(BOOST_ASIO_MOVE_CAST(Arg2)(arg2))
  {
  }

  move_binder2(move_binder2&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      arg1_(BOOST_ASIO_MOVE_CAST(Arg1)(other.arg1_)),
      arg2_(BOOST_ASIO_MOVE_CAST(Arg2)(other.arg2_))
  {
  }

  void operator()()
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        static_cast<const Arg1&>(arg1_),
        BOOST_ASIO_MOVE_CAST(Arg2)(arg2_));
  }

//private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
};

template <typename Handler, typename Arg1, typename Arg2>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    move_binder2<Handler, Arg1, Arg2>* this_handler)
{
#if defined(BOOST_ASIO_NO_DEPRECATED)
  boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
  return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
  return boost_asio_handler_alloc_helpers::allocate(
      size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2>
inline asio_handler_deallocate_is_deprecated
asio_handler_deallocate(void* pointer, std::size_t size,
    move_binder2<Handler, Arg1, Arg2>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler, typename Arg1, typename Arg2>
inline bool asio_handler_is_continuation(
    move_binder2<Handler, Arg1, Arg2>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler_);
}

template <typename Function, typename Handler, typename Arg1, typename Arg2>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(BOOST_ASIO_MOVE_ARG(Function) function,
    move_binder2<Handler, Arg1, Arg2>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      BOOST_ASIO_MOVE_CAST(Function)(function), this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

#endif // defined(BOOST_ASIO_HAS_MOVE)

} // namespace detail

template <template <typename, typename> class Associator,
    typename Handler, typename DefaultCandidate>
struct associator<Associator,
    detail::binder0<Handler>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::binder0<Handler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename DefaultCandidate>
struct associator<Associator,
    detail::binder1<Handler, Arg1>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::binder1<Handler, Arg1>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename Arg2,
    typename DefaultCandidate>
struct associator<Associator,
    detail::binder2<Handler, Arg1, Arg2>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::binder2<Handler, Arg1, Arg2>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename DefaultCandidate>
struct associator<Associator,
    detail::binder3<Handler, Arg1, Arg2, Arg3>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::binder3<Handler, Arg1, Arg2, Arg3>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename Arg4, typename DefaultCandidate>
struct associator<Associator,
    detail::binder4<Handler, Arg1, Arg2, Arg3, Arg4>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::binder4<Handler, Arg1, Arg2, Arg3, Arg4>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename Arg4, typename Arg5, typename DefaultCandidate>
struct associator<Associator,
    detail::binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#if defined(BOOST_ASIO_HAS_MOVE)

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename DefaultCandidate>
struct associator<Associator,
    detail::move_binder1<Handler, Arg1>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::move_binder1<Handler, Arg1>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Arg1, typename Arg2,
    typename DefaultCandidate>
struct associator<Associator,
    detail::move_binder2<Handler, Arg1, Arg2>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const detail::move_binder2<Handler, Arg1, Arg2>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // defined(BOOST_ASIO_HAS_MOVE)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_BIND_HANDLER_HPP
