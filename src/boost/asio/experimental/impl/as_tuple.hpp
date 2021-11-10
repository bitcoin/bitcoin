//
// experimental/impl/as_tuple.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_EXPERIMENTAL_AS_TUPLE_HPP
#define BOOST_ASIO_IMPL_EXPERIMENTAL_AS_TUPLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <tuple>

#include <boost/asio/associator.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/detail/variadic_templates.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {
namespace detail {

// Class to adapt a as_tuple_t as a completion handler.
template <typename Handler>
class as_tuple_handler
{
public:
  typedef void result_type;

  template <typename CompletionToken>
  as_tuple_handler(as_tuple_t<CompletionToken> e)
    : handler_(BOOST_ASIO_MOVE_CAST(CompletionToken)(e.token_))
  {
  }

  template <typename RedirectedHandler>
  as_tuple_handler(BOOST_ASIO_MOVE_ARG(RedirectedHandler) h)
    : handler_(BOOST_ASIO_MOVE_CAST(RedirectedHandler)(h))
  {
  }

  template <typename... Args>
  void operator()(BOOST_ASIO_MOVE_ARG(Args)... args)
  {
    BOOST_ASIO_MOVE_OR_LVALUE(Handler)(handler_)(
        std::make_tuple(BOOST_ASIO_MOVE_CAST(Args)(args)...));
  }

//private:
  Handler handler_;
};

template <typename Handler>
inline asio_handler_allocate_is_deprecated
asio_handler_allocate(std::size_t size,
    as_tuple_handler<Handler>* this_handler)
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
    as_tuple_handler<Handler>* this_handler)
{
  boost_asio_handler_alloc_helpers::deallocate(
      pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Handler>
inline bool asio_handler_is_continuation(
    as_tuple_handler<Handler>* this_handler)
{
  return boost_asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
}

template <typename Function, typename Handler>
inline asio_handler_invoke_is_deprecated
asio_handler_invoke(Function& function,
    as_tuple_handler<Handler>* this_handler)
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
    as_tuple_handler<Handler>* this_handler)
{
  boost_asio_handler_invoke_helpers::invoke(
      function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
  return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
}

template <typename Signature>
struct as_tuple_signature;

template <typename R, typename... Args>
struct as_tuple_signature<R(Args...)>
{
  typedef R type(std::tuple<typename decay<Args>::type...>);
};

#if defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

template <typename R, typename... Args>
struct as_tuple_signature<R(Args...) &>
{
  typedef R type(std::tuple<typename decay<Args>::type...>) &;
};

template <typename R, typename... Args>
struct as_tuple_signature<R(Args...) &&>
{
  typedef R type(std::tuple<typename decay<Args>::type...>) &&;
};

# if defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)

template <typename R, typename... Args>
struct as_tuple_signature<R(Args...) noexcept>
{
  typedef R type(std::tuple<typename decay<Args>::type...>) noexcept;
};

template <typename R, typename... Args>
struct as_tuple_signature<R(Args...) & noexcept>
{
  typedef R type(std::tuple<typename decay<Args>::type...>) & noexcept;
};

template <typename R, typename... Args>
struct as_tuple_signature<R(Args...) && noexcept>
{
  typedef R type(std::tuple<typename decay<Args>::type...>) && noexcept;
};

# endif // defined(BOOST_ASIO_HAS_NOEXCEPT_FUNCTION_TYPE)
#endif // defined(BOOST_ASIO_HAS_REF_QUALIFIED_FUNCTIONS)

} // namespace detail
} // namespace experimental

#if !defined(GENERATING_DOCUMENTATION)

template <typename CompletionToken, typename... Signatures>
struct async_result<experimental::as_tuple_t<CompletionToken>, Signatures...>
  : async_result<CompletionToken,
      typename experimental::detail::as_tuple_signature<Signatures>::type...>
{
  typedef async_result<CompletionToken,
    typename experimental::detail::as_tuple_signature<Signatures>::type...>
      base_async_result;

  template <typename Initiation>
  struct init_wrapper
  {
    init_wrapper(Initiation init)
      : initiation_(BOOST_ASIO_MOVE_CAST(Initiation)(init))
    {
    }

    template <typename Handler, typename... Args>
    void operator()(
        BOOST_ASIO_MOVE_ARG(Handler) handler,
        BOOST_ASIO_MOVE_ARG(Args)... args)
    {
      BOOST_ASIO_MOVE_CAST(Initiation)(initiation_)(
          experimental::detail::as_tuple_handler<
            typename decay<Handler>::type>(
              BOOST_ASIO_MOVE_CAST(Handler)(handler)),
          BOOST_ASIO_MOVE_CAST(Args)(args)...);
    }

    Initiation initiation_;
  };

  template <typename Initiation, typename RawCompletionToken, typename... Args>
  static BOOST_ASIO_INITFN_DEDUCED_RESULT_TYPE(CompletionToken,
      typename experimental::detail::as_tuple_signature<Signatures>::type...,
      (base_async_result::initiate(
        declval<init_wrapper<typename decay<Initiation>::type> >(),
        declval<CompletionToken>(),
        declval<BOOST_ASIO_MOVE_ARG(Args)>()...)))
  initiate(
      BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      BOOST_ASIO_MOVE_ARG(RawCompletionToken) token,
      BOOST_ASIO_MOVE_ARG(Args)... args)
  {
    return base_async_result::initiate(
        init_wrapper<typename decay<Initiation>::type>(
          BOOST_ASIO_MOVE_CAST(Initiation)(initiation)),
        token.token_, BOOST_ASIO_MOVE_CAST(Args)(args)...);
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename DefaultCandidate>
struct associator<Associator,
    experimental::detail::as_tuple_handler<Handler>, DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const experimental::detail::as_tuple_handler<Handler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_EXPERIMENTAL_AS_TUPLE_HPP
