//
// detail/base_from_cancellation_state.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_BASE_FROM_CANCELLATION_STATE_HPP
#define BOOST_ASIO_DETAIL_BASE_FROM_CANCELLATION_STATE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/cancellation_state.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Handler, typename = void>
class base_from_cancellation_state
{
public:
  typedef cancellation_slot cancellation_slot_type;

  cancellation_slot_type get_cancellation_slot() const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_state_.slot();
  }

  cancellation_state get_cancellation_state() const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_state_;
  }

protected:
  explicit base_from_cancellation_state(const Handler& handler)
    : cancellation_state_(
        boost::asio::get_associated_cancellation_slot(handler))
  {
  }

  template <typename Filter>
  base_from_cancellation_state(const Handler& handler, Filter filter)
    : cancellation_state_(
        boost::asio::get_associated_cancellation_slot(handler), filter, filter)
  {
  }

  template <typename InFilter, typename OutFilter>
  base_from_cancellation_state(const Handler& handler,
      BOOST_ASIO_MOVE_ARG(InFilter) in_filter,
      BOOST_ASIO_MOVE_ARG(OutFilter) out_filter)
    : cancellation_state_(
        boost::asio::get_associated_cancellation_slot(handler),
        BOOST_ASIO_MOVE_CAST(InFilter)(in_filter),
        BOOST_ASIO_MOVE_CAST(OutFilter)(out_filter))
  {
  }

  void reset_cancellation_state(const Handler& handler)
  {
    cancellation_state_ = cancellation_state(
        boost::asio::get_associated_cancellation_slot(handler));
  }

  template <typename Filter>
  void reset_cancellation_state(const Handler& handler, Filter filter)
  {
    cancellation_state_ = cancellation_state(
        boost::asio::get_associated_cancellation_slot(handler), filter, filter);
  }

  template <typename InFilter, typename OutFilter>
  void reset_cancellation_state(const Handler& handler,
      BOOST_ASIO_MOVE_ARG(InFilter) in_filter,
      BOOST_ASIO_MOVE_ARG(OutFilter) out_filter)
  {
    cancellation_state_ = cancellation_state(
        boost::asio::get_associated_cancellation_slot(handler),
        BOOST_ASIO_MOVE_CAST(InFilter)(in_filter),
        BOOST_ASIO_MOVE_CAST(OutFilter)(out_filter));
  }

  cancellation_type_t cancelled() const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_state_.cancelled();
  }

private:
  cancellation_state cancellation_state_;
};

template <typename Handler>
class base_from_cancellation_state<Handler,
    typename enable_if<
      is_same<
        typename associated_cancellation_slot<
          Handler, cancellation_slot
        >::asio_associated_cancellation_slot_is_unspecialised,
        void
      >::value
    >::type>
{
public:
  cancellation_state get_cancellation_state() const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_state();
  }

protected:
  explicit base_from_cancellation_state(const Handler&)
  {
  }

  template <typename Filter>
  base_from_cancellation_state(const Handler&, Filter)
  {
  }

  template <typename InFilter, typename OutFilter>
  base_from_cancellation_state(const Handler&,
      BOOST_ASIO_MOVE_ARG(InFilter),
      BOOST_ASIO_MOVE_ARG(OutFilter))
  {
  }

  void reset_cancellation_state(const Handler&)
  {
  }

  template <typename Filter>
  void reset_cancellation_state(const Handler&, Filter)
  {
  }

  template <typename InFilter, typename OutFilter>
  void reset_cancellation_state(const Handler&,
      BOOST_ASIO_MOVE_ARG(InFilter),
      BOOST_ASIO_MOVE_ARG(OutFilter))
  {
  }

  BOOST_ASIO_CONSTEXPR cancellation_type_t cancelled() const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_type::none;
  }
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_BASE_FROM_CANCELLATION_STATE_HPP
