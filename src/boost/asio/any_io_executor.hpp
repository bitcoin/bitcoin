//
// any_io_executor.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_ANY_IO_EXECUTOR_HPP
#define BOOST_ASIO_ANY_IO_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#if defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
# include <boost/asio/executor.hpp>
#else // defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
# include <boost/asio/execution.hpp>
# include <boost/asio/execution_context.hpp>
#endif // defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

typedef executor any_io_executor;

#else // defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

/// Polymorphic executor type for use with I/O objects.
/**
 * The @c any_io_executor type is a polymorphic executor that supports the set
 * of properties required by I/O objects. It is defined as the
 * execution::any_executor class template parameterised as follows:
 * @code execution::any_executor<
 *   execution::context_as_t<execution_context&>,
 *   execution::blocking_t::never_t,
 *   execution::prefer_only<execution::blocking_t::possibly_t>,
 *   execution::prefer_only<execution::outstanding_work_t::tracked_t>,
 *   execution::prefer_only<execution::outstanding_work_t::untracked_t>,
 *   execution::prefer_only<execution::relationship_t::fork_t>,
 *   execution::prefer_only<execution::relationship_t::continuation_t>
 * > @endcode
 */
class any_io_executor :
#if defined(GENERATING_DOCUMENTATION)
  public execution::any_executor<...>
#else // defined(GENERATING_DOCUMENTATION)
  public execution::any_executor<
      execution::context_as_t<execution_context&>,
      execution::blocking_t::never_t,
      execution::prefer_only<execution::blocking_t::possibly_t>,
      execution::prefer_only<execution::outstanding_work_t::tracked_t>,
      execution::prefer_only<execution::outstanding_work_t::untracked_t>,
      execution::prefer_only<execution::relationship_t::fork_t>,
      execution::prefer_only<execution::relationship_t::continuation_t>
    >
#endif // defined(GENERATING_DOCUMENTATION)
{
public:
#if !defined(GENERATING_DOCUMENTATION)
  typedef execution::any_executor<
      execution::context_as_t<execution_context&>,
      execution::blocking_t::never_t,
      execution::prefer_only<execution::blocking_t::possibly_t>,
      execution::prefer_only<execution::outstanding_work_t::tracked_t>,
      execution::prefer_only<execution::outstanding_work_t::untracked_t>,
      execution::prefer_only<execution::relationship_t::fork_t>,
      execution::prefer_only<execution::relationship_t::continuation_t>
    > base_type;

  typedef void supportable_properties_type(
      execution::context_as_t<execution_context&>,
      execution::blocking_t::never_t,
      execution::prefer_only<execution::blocking_t::possibly_t>,
      execution::prefer_only<execution::outstanding_work_t::tracked_t>,
      execution::prefer_only<execution::outstanding_work_t::untracked_t>,
      execution::prefer_only<execution::relationship_t::fork_t>,
      execution::prefer_only<execution::relationship_t::continuation_t>
    );
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Default constructor.
  any_io_executor() BOOST_ASIO_NOEXCEPT
    : base_type()
  {
  }

  /// Construct in an empty state. Equivalent effects to default constructor.
  any_io_executor(nullptr_t) BOOST_ASIO_NOEXCEPT
    : base_type(nullptr_t())
  {
  }

  /// Copy constructor.
  any_io_executor(const any_io_executor& e) BOOST_ASIO_NOEXCEPT
    : base_type(static_cast<const base_type&>(e))
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move constructor.
  any_io_executor(any_io_executor&& e) BOOST_ASIO_NOEXCEPT
    : base_type(static_cast<base_type&&>(e))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Construct to point to the same target as another any_executor.
#if defined(GENERATING_DOCUMENTATION)
  template <class... OtherSupportableProperties>
    any_io_executor(execution::any_executor<OtherSupportableProperties...> e);
#else // defined(GENERATING_DOCUMENTATION)
  template <typename OtherAnyExecutor>
  any_io_executor(OtherAnyExecutor e,
      typename constraint<
        conditional<
          !is_same<OtherAnyExecutor, any_io_executor>::value
            && is_base_of<execution::detail::any_executor_base,
              OtherAnyExecutor>::value,
          typename execution::detail::supportable_properties<
            0, supportable_properties_type>::template
              is_valid_target<OtherAnyExecutor>,
          false_type
        >::type::value
      >::type = 0)
    : base_type(BOOST_ASIO_MOVE_CAST(OtherAnyExecutor)(e))
  {
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Construct a polymorphic wrapper for the specified executor.
#if defined(GENERATING_DOCUMENTATION)
  template <BOOST_ASIO_EXECUTION_EXECUTOR Executor>
  any_io_executor(Executor e);
#else // defined(GENERATING_DOCUMENTATION)
  template <BOOST_ASIO_EXECUTION_EXECUTOR Executor>
  any_io_executor(Executor e,
      typename constraint<
        conditional<
          !is_same<Executor, any_io_executor>::value
            && !is_base_of<execution::detail::any_executor_base,
              Executor>::value,
          execution::detail::is_valid_target_executor<
            Executor, supportable_properties_type>,
          false_type
        >::type::value
      >::type = 0)
    : base_type(BOOST_ASIO_MOVE_CAST(Executor)(e))
  {
  }
#endif // defined(GENERATING_DOCUMENTATION)

  /// Assignment operator.
  any_io_executor& operator=(const any_io_executor& e) BOOST_ASIO_NOEXCEPT
  {
    base_type::operator=(static_cast<const base_type&>(e));
    return *this;
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move assignment operator.
  any_io_executor& operator=(any_io_executor&& e) BOOST_ASIO_NOEXCEPT
  {
    base_type::operator=(static_cast<base_type&&>(e));
    return *this;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Assignment operator that sets the polymorphic wrapper to the empty state.
  any_io_executor& operator=(nullptr_t)
  {
    base_type::operator=(nullptr_t());
    return *this;
  }

  /// Destructor.
  ~any_io_executor()
  {
  }

  /// Swap targets with another polymorphic wrapper.
  void swap(any_io_executor& other) BOOST_ASIO_NOEXCEPT
  {
    static_cast<base_type&>(*this).swap(static_cast<base_type&>(other));
  }

  /// Obtain a polymorphic wrapper with the specified property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require and boost::asio::prefer customisation points.
   *
   * For example:
   * @code any_io_executor ex = ...;
   * auto ex2 = boost::asio::require(ex, execution::blocking.possibly); @endcode
   */
  template <typename Property>
  any_io_executor require(const Property& p,
      typename constraint<
        traits::require_member<const base_type&, const Property&>::is_valid
      >::type = 0) const
  {
    return static_cast<const base_type&>(*this).require(p);
  }

  /// Obtain a polymorphic wrapper with the specified property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::prefer customisation point.
   *
   * For example:
   * @code any_io_executor ex = ...;
   * auto ex2 = boost::asio::prefer(ex, execution::blocking.possibly); @endcode
   */
  template <typename Property>
  any_io_executor prefer(const Property& p,
      typename constraint<
        traits::prefer_member<const base_type&, const Property&>::is_valid
      >::type = 0) const
  {
    return static_cast<const base_type&>(*this).prefer(p);
  }
};

#if !defined(GENERATING_DOCUMENTATION)

namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <>
struct equality_comparable<any_io_executor>
{
  static const bool is_valid = true;
  static const bool is_noexcept = true;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename F>
struct execute_member<any_io_executor, F>
{
  static const bool is_valid = true;
  static const bool is_noexcept = false;
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Prop>
struct query_member<any_io_executor, Prop> :
  query_member<any_io_executor::base_type, Prop>
{
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Prop>
struct require_member<any_io_executor, Prop> :
  require_member<any_io_executor::base_type, Prop>
{
  typedef any_io_executor result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

template <typename Prop>
struct prefer_member<any_io_executor, Prop> :
  prefer_member<any_io_executor::base_type, Prop>
{
  typedef any_io_executor result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_PREFER_MEMBER_TRAIT)

} // namespace traits

#endif // !defined(GENERATING_DOCUMENTATION)

#endif // defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_ANY_IO_EXECUTOR_HPP
