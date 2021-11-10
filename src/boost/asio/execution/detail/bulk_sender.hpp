//
// execution/detail/bulk_sender.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_DETAIL_BULK_SENDER_HPP
#define BOOST_ASIO_EXECUTION_DETAIL_BULK_SENDER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/connect.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/set_done.hpp>
#include <boost/asio/execution/set_error.hpp>
#include <boost/asio/traits/connect_member.hpp>
#include <boost/asio/traits/set_done_member.hpp>
#include <boost/asio/traits/set_error_member.hpp>
#include <boost/asio/traits/set_value_member.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {
namespace detail {

template <typename Receiver, typename Function, typename Number, typename Index>
struct bulk_receiver
{
  typename remove_cvref<Receiver>::type receiver_;
  typename decay<Function>::type f_;
  typename decay<Number>::type n_;

  template <typename R, typename F, typename N>
  explicit bulk_receiver(BOOST_ASIO_MOVE_ARG(R) r,
      BOOST_ASIO_MOVE_ARG(F) f, BOOST_ASIO_MOVE_ARG(N) n)
    : receiver_(BOOST_ASIO_MOVE_CAST(R)(r)),
      f_(BOOST_ASIO_MOVE_CAST(F)(f)),
      n_(BOOST_ASIO_MOVE_CAST(N)(n))
  {
  }

  void set_value()
  {
    for (Index i = 0; i < n_; ++i)
      f_(i);

    execution::set_value(
        BOOST_ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(receiver_));
  }

  template <typename Error>
  void set_error(BOOST_ASIO_MOVE_ARG(Error) e) BOOST_ASIO_NOEXCEPT
  {
    execution::set_error(
        BOOST_ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(receiver_),
        BOOST_ASIO_MOVE_CAST(Error)(e));
  }

  void set_done() BOOST_ASIO_NOEXCEPT
  {
    execution::set_done(
        BOOST_ASIO_MOVE_OR_LVALUE(
          typename remove_cvref<Receiver>::type)(receiver_));
  }
};

template <typename Sender, typename Receiver,
  typename Function, typename Number>
struct bulk_receiver_traits
{
  typedef bulk_receiver<
      Receiver, Function, Number,
      typename execution::executor_index<
        typename remove_cvref<Sender>::type
      >::type
    > type;

#if defined(BOOST_ASIO_HAS_MOVE)
  typedef type arg_type;
#else // defined(BOOST_ASIO_HAS_MOVE)
  typedef const type& arg_type;
#endif // defined(BOOST_ASIO_HAS_MOVE)
};

template <typename Sender, typename Function, typename Number>
struct bulk_sender : sender_base
{
  typename remove_cvref<Sender>::type sender_;
  typename decay<Function>::type f_;
  typename decay<Number>::type n_;

  template <typename S, typename F, typename N>
  explicit bulk_sender(BOOST_ASIO_MOVE_ARG(S) s,
      BOOST_ASIO_MOVE_ARG(F) f, BOOST_ASIO_MOVE_ARG(N) n)
    : sender_(BOOST_ASIO_MOVE_CAST(S)(s)),
      f_(BOOST_ASIO_MOVE_CAST(F)(f)),
      n_(BOOST_ASIO_MOVE_CAST(N)(n))
  {
  }

  template <typename Receiver>
  typename connect_result<
      BOOST_ASIO_MOVE_OR_LVALUE_TYPE(typename remove_cvref<Sender>::type),
      typename bulk_receiver_traits<
        Sender, Receiver, Function, Number
      >::arg_type
  >::type connect(BOOST_ASIO_MOVE_ARG(Receiver) r,
      typename enable_if<
        can_connect<
          typename remove_cvref<Sender>::type,
          typename bulk_receiver_traits<
            Sender, Receiver, Function, Number
          >::arg_type
        >::value
      >::type* = 0) BOOST_ASIO_RVALUE_REF_QUAL BOOST_ASIO_NOEXCEPT
  {
    return execution::connect(
        BOOST_ASIO_MOVE_OR_LVALUE(typename remove_cvref<Sender>::type)(sender_),
        typename bulk_receiver_traits<Sender, Receiver, Function, Number>::type(
          BOOST_ASIO_MOVE_CAST(Receiver)(r),
          BOOST_ASIO_MOVE_CAST(typename decay<Function>::type)(f_),
          BOOST_ASIO_MOVE_CAST(typename decay<Number>::type)(n_)));
  }

  template <typename Receiver>
  typename connect_result<
      const typename remove_cvref<Sender>::type&,
      typename bulk_receiver_traits<
        Sender, Receiver, Function, Number
      >::arg_type
  >::type connect(BOOST_ASIO_MOVE_ARG(Receiver) r,
      typename enable_if<
        can_connect<
          const typename remove_cvref<Sender>::type&,
          typename bulk_receiver_traits<
            Sender, Receiver, Function, Number
          >::arg_type
        >::value
      >::type* = 0) const BOOST_ASIO_LVALUE_REF_QUAL BOOST_ASIO_NOEXCEPT
  {
    return execution::connect(sender_,
        typename bulk_receiver_traits<Sender, Receiver, Function, Number>::type(
          BOOST_ASIO_MOVE_CAST(Receiver)(r), f_, n_));
  }
};

} // namespace detail
} // namespace execution
namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

template <typename Receiver, typename Function, typename Number, typename Index>
struct set_value_member<
    execution::detail::bulk_receiver<Receiver, Function, Number, Index>,
    void()>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_SET_VALUE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

template <typename Receiver, typename Function,
    typename Number, typename Index, typename Error>
struct set_error_member<
    execution::detail::bulk_receiver<Receiver, Function, Number, Index>,
    Error>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_SET_ERROR_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_SET_DONE_MEMBER_TRAIT)

template <typename Receiver, typename Function, typename Number, typename Index>
struct set_done_member<
    execution::detail::bulk_receiver<Receiver, Function, Number, Index> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_SET_DONE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_CONNECT_MEMBER_TRAIT)

template <typename Sender, typename Function,
    typename Number, typename Receiver>
struct connect_member<
    execution::detail::bulk_sender<Sender, Function, Number>,
    Receiver,
    typename enable_if<
      execution::can_connect<
        BOOST_ASIO_MOVE_OR_LVALUE_TYPE(typename remove_cvref<Sender>::type),
        typename execution::detail::bulk_receiver_traits<
          Sender, Receiver, Function, Number
        >::arg_type
      >::value
    >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef typename execution::connect_result<
      BOOST_ASIO_MOVE_OR_LVALUE_TYPE(typename remove_cvref<Sender>::type),
      typename execution::detail::bulk_receiver_traits<
        Sender, Receiver, Function, Number
      >::arg_type
    >::type result_type;
};

template <typename Sender, typename Function,
    typename Number, typename Receiver>
struct connect_member<
    const execution::detail::bulk_sender<Sender, Function, Number>,
    Receiver,
    typename enable_if<
      execution::can_connect<
        const typename remove_cvref<Sender>::type&,
        typename execution::detail::bulk_receiver_traits<
          Sender, Receiver, Function, Number
        >::arg_type
      >::value
    >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef typename execution::connect_result<
      const typename remove_cvref<Sender>::type&,
      typename execution::detail::bulk_receiver_traits<
        Sender, Receiver, Function, Number
      >::arg_type
    >::type result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_CONNECT_MEMBER_TRAIT)

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_DETAIL_BULK_SENDER_HPP
