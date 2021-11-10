//
// require_concept.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_REQUIRE_CONCEPT_HPP
#define BOOST_ASIO_REQUIRE_CONCEPT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/is_applicable_property.hpp>
#include <boost/asio/traits/require_concept_member.hpp>
#include <boost/asio/traits/require_concept_free.hpp>
#include <boost/asio/traits/static_require_concept.hpp>

#include <boost/asio/detail/push_options.hpp>

#if defined(GENERATING_DOCUMENTATION)

namespace boost {
namespace asio {

/// A customisation point that applies a concept-enforcing property to an
/// object.
/**
 * The name <tt>require_concept</tt> denotes a customization point object. The
 * expression <tt>boost::asio::require_concept(E, P)</tt> for some
 * subexpressions <tt>E</tt> and <tt>P</tt> (with types <tt>T =
 * decay_t<decltype(E)></tt> and <tt>Prop = decay_t<decltype(P)></tt>) is
 * expression-equivalent to:
 *
 * @li If <tt>is_applicable_property_v<T, Prop> &&
 *   Prop::is_requirable_concept</tt> is not a well-formed constant expression
 *   with value <tt>true</tt>, <tt>boost::asio::require_concept(E, P)</tt> is
 *   ill-formed.
 *
 * @li Otherwise, <tt>E</tt> if the expression <tt>Prop::template
 *   static_query_v<T> == Prop::value()</tt> is a well-formed constant
 *   expression with value <tt>true</tt>.
 *
 * @li Otherwise, <tt>(E).require_concept(P)</tt> if the expression
 *   <tt>(E).require_concept(P)</tt> is well-formed.
 *
 * @li Otherwise, <tt>require_concept(E, P)</tt> if the expression
 *   <tt>require_concept(E, P)</tt> is a valid expression with overload
 *   resolution performed in a context that does not include the declaration
 *   of the <tt>require_concept</tt> customization point object.
 *
 * @li Otherwise, <tt>boost::asio::require_concept(E, P)</tt> is ill-formed.
 */
inline constexpr unspecified require_concept = unspecified;

/// A type trait that determines whether a @c require_concept expression is
/// well-formed.
/**
 * Class template @c can_require_concept is a trait that is derived from
 * @c true_type if the expression
 * <tt>boost::asio::require_concept(std::declval<T>(),
 * std::declval<Property>())</tt> is well formed; otherwise @c false_type.
 */
template <typename T, typename Property>
struct can_require_concept :
  integral_constant<bool, automatically_determined>
{
};

/// A type trait that determines whether a @c require_concept expression will
/// not throw.
/**
 * Class template @c is_nothrow_require_concept is a trait that is derived from
 * @c true_type if the expression
 * <tt>boost::asio::require_concept(std::declval<T>(),
 * std::declval<Property>())</tt> is @c noexcept; otherwise @c false_type.
 */
template <typename T, typename Property>
struct is_nothrow_require_concept :
  integral_constant<bool, automatically_determined>
{
};

/// A type trait that determines the result type of a @c require_concept
/// expression.
/**
 * Class template @c require_concept_result is a trait that determines the
 * result type of the expression
 * <tt>boost::asio::require_concept(std::declval<T>(),
 * std::declval<Property>())</tt>.
 */
template <typename T, typename Property>
struct require_concept_result
{
  /// The result of the @c require_concept expression.
  typedef automatically_determined type;
};

} // namespace asio
} // namespace boost

#else // defined(GENERATING_DOCUMENTATION)

namespace boost_asio_require_concept_fn {

using boost::asio::conditional;
using boost::asio::decay;
using boost::asio::declval;
using boost::asio::enable_if;
using boost::asio::is_applicable_property;
using boost::asio::traits::require_concept_free;
using boost::asio::traits::require_concept_member;
using boost::asio::traits::static_require_concept;

void require_concept();

enum overload_type
{
  identity,
  call_member,
  call_free,
  ill_formed
};

template <typename Impl, typename T, typename Properties, typename = void,
    typename = void, typename = void, typename = void, typename = void>
struct call_traits
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = ill_formed);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = false);
  typedef void result_type;
};

template <typename Impl, typename T, typename Property>
struct call_traits<Impl, T, void(Property),
  typename enable_if<
    is_applicable_property<
      typename decay<T>::type,
      typename decay<Property>::type
    >::value
  >::type,
  typename enable_if<
    decay<Property>::type::is_requirable_concept
  >::type,
  typename enable_if<
    static_require_concept<T, Property>::is_valid
  >::type>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = identity);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef BOOST_ASIO_MOVE_ARG(T) result_type;
};

template <typename Impl, typename T, typename Property>
struct call_traits<Impl, T, void(Property),
  typename enable_if<
    is_applicable_property<
      typename decay<T>::type,
      typename decay<Property>::type
    >::value
  >::type,
  typename enable_if<
    decay<Property>::type::is_requirable_concept
  >::type,
  typename enable_if<
    !static_require_concept<T, Property>::is_valid
  >::type,
  typename enable_if<
    require_concept_member<
      typename Impl::template proxy<T>::type,
      Property
    >::is_valid
  >::type> :
  require_concept_member<
    typename Impl::template proxy<T>::type,
    Property
  >
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_member);
};

template <typename Impl, typename T, typename Property>
struct call_traits<Impl, T, void(Property),
  typename enable_if<
    is_applicable_property<
      typename decay<T>::type,
      typename decay<Property>::type
    >::value
  >::type,
  typename enable_if<
    decay<Property>::type::is_requirable_concept
  >::type,
  typename enable_if<
    !static_require_concept<T, Property>::is_valid
  >::type,
  typename enable_if<
    !require_concept_member<
      typename Impl::template proxy<T>::type,
      Property
    >::is_valid
  >::type,
  typename enable_if<
    require_concept_free<T, Property>::is_valid
  >::type> :
  require_concept_free<T, Property>
{
  BOOST_ASIO_STATIC_CONSTEXPR(overload_type, overload = call_free);
};

struct impl
{
  template <typename T>
  struct proxy
  {
#if defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_CONCEPT_MEMBER_TRAIT)
    struct type
    {
      template <typename P>
      auto require_concept(BOOST_ASIO_MOVE_ARG(P) p)
        noexcept(
          noexcept(
            declval<typename conditional<true, T, P>::type>().require_concept(
              BOOST_ASIO_MOVE_CAST(P)(p))
          )
        )
        -> decltype(
          declval<typename conditional<true, T, P>::type>().require_concept(
            BOOST_ASIO_MOVE_CAST(P)(p))
        );
    };
#else // defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_CONCEPT_MEMBER_TRAIT)
    typedef T type;
#endif // defined(BOOST_ASIO_HAS_DEDUCED_REQUIRE_CONCEPT_MEMBER_TRAIT)
  };

  template <typename T, typename Property>
  BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<impl, T, void(Property)>::overload == identity,
    typename call_traits<impl, T, void(Property)>::result_type
  >::type
  operator()(
      BOOST_ASIO_MOVE_ARG(T) t,
      BOOST_ASIO_MOVE_ARG(Property)) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<impl, T, void(Property)>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(T)(t);
  }

  template <typename T, typename Property>
  BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<impl, T, void(Property)>::overload == call_member,
    typename call_traits<impl, T, void(Property)>::result_type
  >::type
  operator()(
      BOOST_ASIO_MOVE_ARG(T) t,
      BOOST_ASIO_MOVE_ARG(Property) p) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<impl, T, void(Property)>::is_noexcept))
  {
    return BOOST_ASIO_MOVE_CAST(T)(t).require_concept(
        BOOST_ASIO_MOVE_CAST(Property)(p));
  }

  template <typename T, typename Property>
  BOOST_ASIO_NODISCARD BOOST_ASIO_CONSTEXPR typename enable_if<
    call_traits<impl, T, void(Property)>::overload == call_free,
    typename call_traits<impl, T, void(Property)>::result_type
  >::type
  operator()(
      BOOST_ASIO_MOVE_ARG(T) t,
      BOOST_ASIO_MOVE_ARG(Property) p) const
    BOOST_ASIO_NOEXCEPT_IF((
      call_traits<impl, T, void(Property)>::is_noexcept))
  {
    return require_concept(
        BOOST_ASIO_MOVE_CAST(T)(t),
        BOOST_ASIO_MOVE_CAST(Property)(p));
  }
};

template <typename T = impl>
struct static_instance
{
  static const T instance;
};

template <typename T>
const T static_instance<T>::instance = {};

} // namespace boost_asio_require_concept_fn
namespace boost {
namespace asio {
namespace {

static BOOST_ASIO_CONSTEXPR const boost_asio_require_concept_fn::impl&
  require_concept = boost_asio_require_concept_fn::static_instance<>::instance;

} // namespace

typedef boost_asio_require_concept_fn::impl require_concept_t;

template <typename T, typename Property>
struct can_require_concept :
  integral_constant<bool,
    boost_asio_require_concept_fn::call_traits<
      require_concept_t, T, void(Property)>::overload !=
        boost_asio_require_concept_fn::ill_formed>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename Property>
constexpr bool can_require_concept_v
  = can_require_concept<T, Property>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename Property>
struct is_nothrow_require_concept :
  integral_constant<bool,
    boost_asio_require_concept_fn::call_traits<
      require_concept_t, T, void(Property)>::is_noexcept>
{
};

#if defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename Property>
constexpr bool is_nothrow_require_concept_v
  = is_nothrow_require_concept<T, Property>::value;

#endif // defined(BOOST_ASIO_HAS_VARIABLE_TEMPLATES)

template <typename T, typename Property>
struct require_concept_result
{
  typedef typename boost_asio_require_concept_fn::call_traits<
      require_concept_t, T, void(Property)>::result_type type;
};

} // namespace asio
} // namespace boost

#endif // defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_REQUIRE_CONCEPT_HPP
