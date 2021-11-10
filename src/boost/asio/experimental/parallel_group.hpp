//
// experimental/parallel_group.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_PARALLEL_GROUP_HPP
#define BOOST_ASIO_EXPERIMENTAL_PARALLEL_GROUP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <utility>
#include <boost/asio/detail/array.hpp>
#include <boost/asio/experimental/cancellation_condition.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {
namespace detail {

// Helper trait for getting the completion signature from an async operation.

struct parallel_op_signature_probe {};

template <typename T>
struct parallel_op_signature_probe_result
{
  typedef T type;
};

template <typename Op>
struct parallel_op_signature
{
  typedef typename decltype(declval<Op>()(
    declval<parallel_op_signature_probe>()))::type type;
};

// Helper trait for getting a tuple from a completion signature.

template <typename Signature>
struct parallel_op_signature_as_tuple;

template <typename R, typename... Args>
struct parallel_op_signature_as_tuple<R(Args...)>
{
  typedef std::tuple<typename decay<Args>::type...> type;
};

// Helper trait for concatenating completion signatures.

template <std::size_t N, typename Offsets, typename... Signatures>
struct parallel_group_signature;

template <std::size_t N, typename R0, typename... Args0>
struct parallel_group_signature<N, R0(Args0...)>
{
  typedef boost::asio::detail::array<std::size_t, N> order_type;
  typedef R0 raw_type(Args0...);
  typedef R0 type(order_type, Args0...);
};

template <std::size_t N,
    typename R0, typename... Args0,
    typename R1, typename... Args1>
struct parallel_group_signature<N, R0(Args0...), R1(Args1...)>
{
  typedef boost::asio::detail::array<std::size_t, N> order_type;
  typedef R0 raw_type(Args0..., Args1...);
  typedef R0 type(order_type, Args0..., Args1...);
};

template <std::size_t N, typename Sig0,
    typename Sig1, typename... SigN>
struct parallel_group_signature<N, Sig0, Sig1, SigN...>
{
  typedef boost::asio::detail::array<std::size_t, N> order_type;
  typedef typename parallel_group_signature<N,
    typename parallel_group_signature<N, Sig0, Sig1>::raw_type,
      SigN...>::raw_type raw_type;
  typedef typename parallel_group_signature<N,
    typename parallel_group_signature<N, Sig0, Sig1>::raw_type,
      SigN...>::type type;
};

template <typename Condition, typename Handler,
    typename... Ops, std::size_t... I>
void parallel_group_launch(Condition cancellation_condition, Handler handler,
    std::tuple<Ops...>& ops, std::index_sequence<I...>);

} // namespace detail

/// A group of asynchronous operations that may be launched in parallel.
/**
 * See the documentation for boost::asio::experimental::make_parallel_group for
 * a usage example.
 */
template <typename... Ops>
class parallel_group
{
public:
  /// Constructor.
  explicit parallel_group(Ops... ops)
    : ops_(std::move(ops)...)
  {
  }

  /// The completion signature for the group of operations.
  typedef typename detail::parallel_group_signature<sizeof...(Ops),
      typename detail::parallel_op_signature<Ops>::type...>::type signature;

  /// Initiate an asynchronous wait for the group of operations.
  /**
   * Launches the group and asynchronously waits for completion.
   *
   * @param cancellation_condition A function object, called on completion of
   * an operation within the group, that is used to determine whether to cancel
   * the remaining operations. The function object is passed the arguments of
   * the completed operation's handler. To trigger cancellation of the remaining
   * operations, it must return a boost::asio::cancellation_type value other
   * than <tt>boost::asio::cancellation_type::none</tt>.
   *
   * @param token A completion token whose signature is comprised of
   * a @c std::array<std::size_t, N> indicating the completion order of the
   * operations, followed by all operations' completion handler arguments.
   *
   * The library provides the following @c cancellation_condition types:
   *
   * @li boost::asio::experimental::wait_for_all
   * @li boost::asio::experimental::wait_for_one
   * @li boost::asio::experimental::wait_for_one_error
   * @li boost::asio::experimental::wait_for_one_success
   */
  template <typename CancellationCondition,
      BOOST_ASIO_COMPLETION_TOKEN_FOR(signature) CompletionToken>
  auto async_wait(CancellationCondition cancellation_condition,
      CompletionToken&& token)
  {
    return boost::asio::async_initiate<CompletionToken, signature>(
        initiate_async_wait(), token,
        std::move(cancellation_condition), std::move(ops_));
  }

private:
  struct initiate_async_wait
  {
    template <typename Handler, typename Condition>
    void operator()(Handler&& h, Condition&& c, std::tuple<Ops...>&& ops) const
    {
      detail::parallel_group_launch(std::move(c), std::move(h),
          ops, std::make_index_sequence<sizeof...(Ops)>());
    }
  };

  std::tuple<Ops...> ops_;
};

/// Create a group of operations that may be launched in parallel.
/**
 * For example:
 * @code boost::asio::experimental::make_parallel_group(
 *    [&](auto token)
 *    {
 *      return in.async_read_some(boost::asio::buffer(data), token);
 *    },
 *    [&](auto token)
 *    {
 *      return timer.async_wait(token);
 *    }
 *  ).async_wait(
 *    boost::asio::experimental::wait_for_all(),
 *    [](
 *        std::array<std::size_t, 2> completion_order,
 *        boost::system::error_code ec1, std::size_t n1,
 *        boost::system::error_code ec2
 *    )
 *    {
 *      switch (completion_order[0])
 *      {
 *      case 0:
 *        {
 *          std::cout << "descriptor finished: " << ec1 << ", " << n1 << "\n";
 *        }
 *        break;
 *      case 1:
 *        {
 *          std::cout << "timer finished: " << ec2 << "\n";
 *        }
 *        break;
 *      }
 *    }
 *  );
 * @endcode
 */
template <typename... Ops>
inline parallel_group<Ops...> make_parallel_group(Ops... ops)
{
  return parallel_group<Ops...>(std::move(ops)...);
}

} // namespace experimental
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/experimental/impl/parallel_group.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_PARALLEL_GROUP_HPP
