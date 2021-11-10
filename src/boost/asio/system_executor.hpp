//
// system_executor.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SYSTEM_EXECUTOR_HPP
#define BOOST_ASIO_SYSTEM_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/execution.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

class system_context;

/// An executor that uses arbitrary threads.
/**
 * The system executor represents an execution context where functions are
 * permitted to run on arbitrary threads. When the blocking.never property is
 * established, the system executor will schedule the function to run on an
 * unspecified system thread pool. When either blocking.possibly or
 * blocking.always is established, the executor invokes the function
 * immediately.
 */
template <typename Blocking, typename Relationship, typename Allocator>
class basic_system_executor
{
public:
  /// Default constructor.
  basic_system_executor() BOOST_ASIO_NOEXCEPT
    : allocator_(Allocator())
  {
  }

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct boost_asio_require_fn::impl;
  friend struct boost_asio_prefer_fn::impl;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Obtain an executor with the @c blocking.possibly property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::blocking.possibly); @endcode
   */
  basic_system_executor<execution::blocking_t::possibly_t,
      Relationship, Allocator>
  require(execution::blocking_t::possibly_t) const
  {
    return basic_system_executor<execution::blocking_t::possibly_t,
        Relationship, Allocator>(allocator_);
  }

  /// Obtain an executor with the @c blocking.always property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::blocking.always); @endcode
   */
  basic_system_executor<execution::blocking_t::always_t,
      Relationship, Allocator>
  require(execution::blocking_t::always_t) const
  {
    return basic_system_executor<execution::blocking_t::always_t,
        Relationship, Allocator>(allocator_);
  }

  /// Obtain an executor with the @c blocking.never property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::blocking.never); @endcode
   */
  basic_system_executor<execution::blocking_t::never_t,
      Relationship, Allocator>
  require(execution::blocking_t::never_t) const
  {
    return basic_system_executor<execution::blocking_t::never_t,
        Relationship, Allocator>(allocator_);
  }

  /// Obtain an executor with the @c relationship.continuation property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::relationship.continuation); @endcode
   */
  basic_system_executor<Blocking,
      execution::relationship_t::continuation_t, Allocator>
  require(execution::relationship_t::continuation_t) const
  {
    return basic_system_executor<Blocking,
        execution::relationship_t::continuation_t, Allocator>(allocator_);
  }

  /// Obtain an executor with the @c relationship.fork property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::relationship.fork); @endcode
   */
  basic_system_executor<Blocking,
      execution::relationship_t::fork_t, Allocator>
  require(execution::relationship_t::fork_t) const
  {
    return basic_system_executor<Blocking,
        execution::relationship_t::fork_t, Allocator>(allocator_);
  }

  /// Obtain an executor with the specified @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::allocator(my_allocator)); @endcode
   */
  template <typename OtherAllocator>
  basic_system_executor<Blocking, Relationship, OtherAllocator>
  require(execution::allocator_t<OtherAllocator> a) const
  {
    return basic_system_executor<Blocking,
        Relationship, OtherAllocator>(a.value());
  }

  /// Obtain an executor with the default @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::require customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex1;
   * auto ex2 = boost::asio::require(ex1,
   *     boost::asio::execution::allocator); @endcode
   */
  basic_system_executor<Blocking, Relationship, std::allocator<void> >
  require(execution::allocator_t<void>) const
  {
    return basic_system_executor<Blocking,
        Relationship, std::allocator<void> >();
  }

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct boost_asio_query_fn::impl;
  friend struct boost::asio::execution::detail::blocking_t<0>;
  friend struct boost::asio::execution::detail::mapping_t<0>;
  friend struct boost::asio::execution::detail::outstanding_work_t<0>;
  friend struct boost::asio::execution::detail::relationship_t<0>;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Query the current value of the @c mapping property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * if (boost::asio::query(ex, boost::asio::execution::mapping)
   *       == boost::asio::execution::mapping.thread)
   *   ... @endcode
   */
  static BOOST_ASIO_CONSTEXPR execution::mapping_t query(
      execution::mapping_t) BOOST_ASIO_NOEXCEPT
  {
    return execution::mapping.thread;
  }

  /// Query the current value of the @c context property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * boost::asio::system_context& pool = boost::asio::query(
   *     ex, boost::asio::execution::context); @endcode
   */
  static system_context& query(execution::context_t) BOOST_ASIO_NOEXCEPT;

  /// Query the current value of the @c blocking property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * if (boost::asio::query(ex, boost::asio::execution::blocking)
   *       == boost::asio::execution::blocking.always)
   *   ... @endcode
   */
  static BOOST_ASIO_CONSTEXPR execution::blocking_t query(
      execution::blocking_t) BOOST_ASIO_NOEXCEPT
  {
    return Blocking();
  }

  /// Query the current value of the @c relationship property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * if (boost::asio::query(ex, boost::asio::execution::relationship)
   *       == boost::asio::execution::relationship.continuation)
   *   ... @endcode
   */
  static BOOST_ASIO_CONSTEXPR execution::relationship_t query(
      execution::relationship_t) BOOST_ASIO_NOEXCEPT
  {
    return Relationship();
  }

  /// Query the current value of the @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * auto alloc = boost::asio::query(ex,
   *     boost::asio::execution::allocator); @endcode
   */
  template <typename OtherAllocator>
  BOOST_ASIO_CONSTEXPR Allocator query(
      execution::allocator_t<OtherAllocator>) const BOOST_ASIO_NOEXCEPT
  {
    return allocator_;
  }

  /// Query the current value of the @c allocator property.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * auto alloc = boost::asio::query(ex,
   *     boost::asio::execution::allocator); @endcode
   */
  BOOST_ASIO_CONSTEXPR Allocator query(
      execution::allocator_t<void>) const BOOST_ASIO_NOEXCEPT
  {
    return allocator_;
  }

  /// Query the occupancy (recommended number of work items) for the system
  /// context.
  /**
   * Do not call this function directly. It is intended for use with the
   * boost::asio::query customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * std::size_t occupancy = boost::asio::query(
   *     ex, boost::asio::execution::occupancy); @endcode
   */
  std::size_t query(execution::occupancy_t) const BOOST_ASIO_NOEXCEPT;

public:
  /// Compare two executors for equality.
  /**
   * Two executors are equal if they refer to the same underlying io_context.
   */
  friend bool operator==(const basic_system_executor&,
      const basic_system_executor&) BOOST_ASIO_NOEXCEPT
  {
    return true;
  }

  /// Compare two executors for inequality.
  /**
   * Two executors are equal if they refer to the same underlying io_context.
   */
  friend bool operator!=(const basic_system_executor&,
      const basic_system_executor&) BOOST_ASIO_NOEXCEPT
  {
    return false;
  }

#if !defined(GENERATING_DOCUMENTATION)
private:
  friend struct boost_asio_execution_execute_fn::impl;
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Execution function.
  /**
   * Do not call this function directly. It is intended for use with the
   * execution::execute customisation point.
   *
   * For example:
   * @code boost::asio::system_executor ex;
   * execution::execute(ex, my_function_object); @endcode
   */
  template <typename Function>
  void execute(BOOST_ASIO_MOVE_ARG(Function) f) const
  {
    this->do_execute(BOOST_ASIO_MOVE_CAST(Function)(f), Blocking());
  }

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)
public:
  /// Obtain the underlying execution context.
  system_context& context() const BOOST_ASIO_NOEXCEPT;

  /// Inform the executor that it has some outstanding work to do.
  /**
   * For the system executor, this is a no-op.
   */
  void on_work_started() const BOOST_ASIO_NOEXCEPT
  {
  }

  /// Inform the executor that some work is no longer outstanding.
  /**
   * For the system executor, this is a no-op.
   */
  void on_work_finished() const BOOST_ASIO_NOEXCEPT
  {
  }

  /// Request the system executor to invoke the given function object.
  /**
   * This function is used to ask the executor to execute the given function
   * object. The function object will always be executed inside this function.
   *
   * @param f The function object to be called. The executor will make
   * a copy of the handler object as required. The function signature of the
   * function object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename OtherAllocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const;

  /// Request the system executor to invoke the given function object.
  /**
   * This function is used to ask the executor to execute the given function
   * object. The function object will never be executed inside this function.
   * Instead, it will be scheduled to run on an unspecified system thread pool.
   *
   * @param f The function object to be called. The executor will make
   * a copy of the handler object as required. The function signature of the
   * function object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename OtherAllocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const;

  /// Request the system executor to invoke the given function object.
  /**
   * This function is used to ask the executor to execute the given function
   * object. The function object will never be executed inside this function.
   * Instead, it will be scheduled to run on an unspecified system thread pool.
   *
   * @param f The function object to be called. The executor will make
   * a copy of the handler object as required. The function signature of the
   * function object must be: @code void function(); @endcode
   *
   * @param a An allocator that may be used by the executor to allocate the
   * internal storage needed for function invocation.
   */
  template <typename Function, typename OtherAllocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f, const OtherAllocator& a) const;
#endif // !defined(BOOST_ASIO_NO_TS_EXECUTORS)

private:
  template <typename, typename, typename> friend class basic_system_executor;

  // Constructor used by require().
  basic_system_executor(const Allocator& a)
    : allocator_(a)
  {
  }

  /// Execution helper implementation for the possibly blocking property.
  template <typename Function>
  void do_execute(BOOST_ASIO_MOVE_ARG(Function) f,
      execution::blocking_t::possibly_t) const;

  /// Execution helper implementation for the always blocking property.
  template <typename Function>
  void do_execute(BOOST_ASIO_MOVE_ARG(Function) f,
      execution::blocking_t::always_t) const;

  /// Execution helper implementation for the never blocking property.
  template <typename Function>
  void do_execute(BOOST_ASIO_MOVE_ARG(Function) f,
      execution::blocking_t::never_t) const;

  // The allocator used for execution functions.
  Allocator allocator_;
};

/// An executor that uses arbitrary threads.
/**
 * The system executor represents an execution context where functions are
 * permitted to run on arbitrary threads. When the blocking.never property is
 * established, the system executor will schedule the function to run on an
 * unspecified system thread pool. When either blocking.possibly or
 * blocking.always is established, the executor invokes the function
 * immediately.
 */
typedef basic_system_executor<execution::blocking_t::possibly_t,
    execution::relationship_t::fork_t, std::allocator<void> >
  system_executor;

#if !defined(GENERATING_DOCUMENTATION)

namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <typename Blocking, typename Relationship, typename Allocator>
struct equality_comparable<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename Blocking, typename Relationship,
    typename Allocator, typename Function>
struct execute_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    Function
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

template <typename Blocking, typename Relationship, typename Allocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::blocking_t::possibly_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<
      boost::asio::execution::blocking_t::possibly_t,
      Relationship, Allocator> result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::blocking_t::always_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<
      boost::asio::execution::blocking_t::always_t,
      Relationship, Allocator> result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::blocking_t::never_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<
      boost::asio::execution::blocking_t::never_t,
      Relationship, Allocator> result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::relationship_t::fork_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<Blocking,
      boost::asio::execution::relationship_t::fork_t,
      Allocator> result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::relationship_t::continuation_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<Blocking,
      boost::asio::execution::relationship_t::continuation_t,
      Allocator> result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::allocator_t<void>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<Blocking,
      Relationship, std::allocator<void> > result_type;
};

template <typename Blocking, typename Relationship,
    typename Allocator, typename OtherAllocator>
struct require_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::allocator_t<OtherAllocator>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef boost::asio::basic_system_executor<Blocking,
      Relationship, OtherAllocator> result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Blocking, typename Relationship,
    typename Allocator, typename Property>
struct query_static_constexpr_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::mapping_t
      >::value
    >::type
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::mapping_t::thread_t result_type;

  static BOOST_ASIO_CONSTEXPR result_type value() BOOST_ASIO_NOEXCEPT
  {
    return result_type();
  }
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <typename Blocking, typename Relationship,
    typename Allocator, typename Property>
struct query_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::blocking_t
      >::value
    >::type
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::blocking_t result_type;
};

template <typename Blocking, typename Relationship,
    typename Allocator, typename Property>
struct query_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    Property,
    typename boost::asio::enable_if<
      boost::asio::is_convertible<
        Property,
        boost::asio::execution::relationship_t
      >::value
    >::type
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::execution::relationship_t result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct query_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::context_t
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef boost::asio::system_context& result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct query_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::allocator_t<void>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef Allocator result_type;
};

template <typename Blocking, typename Relationship, typename Allocator>
struct query_member<
    boost::asio::basic_system_executor<Blocking, Relationship, Allocator>,
    boost::asio::execution::allocator_t<Allocator>
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef Allocator result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

} // namespace traits

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/system_executor.hpp>

#endif // BOOST_ASIO_SYSTEM_EXECUTOR_HPP
