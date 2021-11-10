//
// execution/scheduler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_SCHEDULER_HPP
#define BOOST_ASIO_EXECUTION_SCHEDULER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/schedule.hpp>
#include <boost/asio/traits/equality_comparable.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {
namespace detail {

template <typename T>
struct is_scheduler_base :
  integral_constant<bool,
    is_copy_constructible<typename remove_cvref<T>::type>::value
      && traits::equality_comparable<typename remove_cvref<T>::type>::is_valid
  >
{
};

} // namespace detail

/// The is_scheduler trait detects whether a type T satisfies the
/// execution::scheduler concept.
/**
 * Class template @c is_scheduler is a type trait that is derived from @c
 * true_type if the type @c T meets the concept definition for a scheduler for
 * error type @c E, otherwise @c false_type.
 */
template <typename T>
struct is_scheduler :
#if defined(GENERATING_DOCUMENTATION)
  integral_constant<bool, automatically_determined>
#else // defined(GENERATING_DOCUMENTATION)
  conditional<
    can_schedule<T>::value,
    detail::is_scheduler_base<T>,
    false_type
  >::type
#endif // defined(GENERATING_DOCUMENTATION)
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T>
BOOST_ASIO_CONSTEXPR const bool is_scheduler_v = is_scheduler<T>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

#if defined(BOOST_ASIO_HAS_CONCEPTS)

template <typename T>
BOOST_ASIO_CONCEPT scheduler = is_scheduler<T>::value;

#define BOOST_ASIO_EXECUTION_SCHEDULER ::boost::asio::execution::scheduler

#else // defined(BOOST_ASIO_HAS_CONCEPTS)

#define BOOST_ASIO_EXECUTION_SCHEDULER typename

#endif // defined(BOOST_ASIO_HAS_CONCEPTS)

} // namespace execution
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_SCHEDULER_HPP
