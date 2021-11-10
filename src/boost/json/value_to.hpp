//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_VALUE_TO_HPP
#define BOOST_JSON_VALUE_TO_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/value.hpp>
#include <boost/json/detail/value_to.hpp>

BOOST_JSON_NS_BEGIN

/** Customization point tag type.

    This tag type is used by the function
    @ref value_to to select overloads
    of `tag_invoke`.

    @note This type is empty; it has no members.

    @see @ref value_from, @ref value_from_tag, @ref value_to,
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1895r0.pdf">
        tag_invoke: A general pattern for supporting customisable functions</a>
*/
template<class T>
struct value_to_tag;

/** Convert a @ref value to an object of type `T`.

    This function attempts to convert a @ref value
    to `T` using

    @li one of @ref value's accessors, or

    @li a library-provided generic conversion, or

    @li a user-provided overload of `tag_invoke`.

    In all cases, the conversion is done by calling
    an overload of `tag_invoke` found by argument-dependent
    lookup. Its signature should be similar to:

    @code
    T tag_invoke( value_to_tag<T>, value );
    @endcode

    The object returned by the function call is
    returned by @ref value_to as the result of the
    conversion.

    @par Constraints
    @code
    ! std::is_reference< T >::value
    @endcode

    @par Exception Safety
    Strong guarantee.

    @tparam T The type to convert to.

    @returns `jv` converted to `T`.

    @param jv The @ref value to convert.

    @see @ref value_to_tag, @ref value_from,
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1895r0.pdf">
        tag_invoke: A general pattern for supporting customisable functions</a>
*/
#ifdef BOOST_JSON_DOCS
template<class T>
T
value_to(const value& jv);
#else
template<class T, class U
    , class = typename std::enable_if<
        ! std::is_reference<T>::value &&
    std::is_same<U, value>::value>::type
>
T
value_to(const U& jv)
{
    return detail::value_to_impl(
        value_to_tag<typename std::remove_cv<T>::type>(), jv);
}
#endif

/** Determine a @ref value can be converted to `T`.

    If @ref value can be converted to `T` via a
    call to @ref value_to, the static data member `value`
    is defined as `true`. Otherwise, `value` is
    defined as `false`.

    @see @ref value_to
*/
#ifdef BOOST_JSON_DOCS
template<class T>
using has_value_to = __see_below__;
#else
template<class T, class>
struct has_value_to
    : std::false_type { };

template<class T>
struct has_value_to<T, detail::void_t<decltype(
    detail::value_to_impl(
        value_to_tag<detail::remove_cvref<T>>(),
        std::declval<const value&>())),
    typename std::enable_if<
        ! std::is_reference<T>::value>::type
    > > : std::true_type { };
#endif

BOOST_JSON_NS_END

#endif
