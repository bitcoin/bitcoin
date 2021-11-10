//
// experimental/impl/deferred.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_IMPL_DEFERRED_HPP
#define BOOST_ASIO_EXPERIMENTAL_IMPL_DEFERRED_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if !defined(GENERATING_DOCUMENTATION)

template <typename R, typename... Args>
class async_result<
    experimental::detail::deferred_signature_probe, R(Args...)>
{
public:
  typedef experimental::detail::deferred_signature_probe_result<void(Args...)>
    return_type;

  template <typename Initiation, typename... InitArgs>
  static return_type initiate(
      BOOST_ASIO_MOVE_ARG(Initiation),
      experimental::detail::deferred_signature_probe,
      BOOST_ASIO_MOVE_ARG(InitArgs)...)
  {
    return return_type{};
  }
};

template <typename Signature>
class async_result<experimental::deferred_t, Signature>
{
public:
  template <typename Initiation, typename... InitArgs>
  static experimental::deferred_async_operation<
      Signature, Initiation, InitArgs...>
  initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      experimental::deferred_t, BOOST_ASIO_MOVE_ARG(InitArgs)... args)
  {
    return experimental::deferred_async_operation<
        Signature, Initiation, InitArgs...>(
          experimental::deferred_init_tag{},
          BOOST_ASIO_MOVE_CAST(Initiation)(initiation),
          BOOST_ASIO_MOVE_CAST(InitArgs)(args)...);
    }
};

template <typename Function, typename R, typename... Args>
class async_result<
    experimental::deferred_function<Function>, R(Args...)>
{
public:
  template <typename Initiation, typename... InitArgs>
  static auto initiate(BOOST_ASIO_MOVE_ARG(Initiation) initiation,
      experimental::deferred_function<Function> token,
      BOOST_ASIO_MOVE_ARG(InitArgs)... init_args)
  {
    return experimental::deferred_sequence<
        experimental::deferred_async_operation<
          R(Args...), Initiation, InitArgs...>,
        Function>(experimental::deferred_init_tag{},
          experimental::deferred_async_operation<
            R(Args...), Initiation, InitArgs...>(
              experimental::deferred_init_tag{},
              BOOST_ASIO_MOVE_CAST(Initiation)(initiation),
              BOOST_ASIO_MOVE_CAST(InitArgs)(init_args)...),
          BOOST_ASIO_MOVE_CAST(Function)(token.function_));
  }
};

template <template <typename, typename> class Associator,
    typename Handler, typename Tail, typename DefaultCandidate>
struct associator<Associator,
    experimental::detail::deferred_sequence_handler<Handler, Tail>,
    DefaultCandidate>
  : Associator<Handler, DefaultCandidate>
{
  static typename Associator<Handler, DefaultCandidate>::type get(
      const experimental::detail::deferred_sequence_handler<Handler, Tail>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<Handler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_IMPL_DEFERRED_HPP
