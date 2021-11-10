/*!
@file
Defines concepts from the Standard library.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_CONCEPTS_HPP
#define BOOST_HANA_DETAIL_CONCEPTS_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/std_common_type.hpp>
#include <boost/hana/detail/void_t.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    //! @cond
    //////////////////////////////////////////////////////////////////////////
    // EqualityComparable
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename U = T, typename = void>
    struct EqualityComparable : std::false_type { };

    template <typename T>
    struct EqualityComparable<T, T, detail::void_t<
        decltype(static_cast<T&&>(*(T*)0) == static_cast<T&&>(*(T*)0) ? 0:0),
        decltype(static_cast<T&&>(*(T*)0) != static_cast<T&&>(*(T*)0) ? 0:0)
    >> : std::true_type { };

    template <typename T, typename U>
    struct EqualityComparable<T, U, typename std::enable_if<
        !std::is_same<T, U>::value, detail::void_t<
            decltype(static_cast<T&&>(*(T*)0) == static_cast<U&&>(*(U*)0) ? 0:0),
            decltype(static_cast<U&&>(*(U*)0) == static_cast<T&&>(*(T*)0) ? 0:0),
            decltype(static_cast<T&&>(*(T*)0) != static_cast<U&&>(*(U*)0) ? 0:0),
            decltype(static_cast<U&&>(*(U*)0) != static_cast<T&&>(*(T*)0) ? 0:0),
            typename detail::std_common_type<T, U>::type
    >>::type> : std::integral_constant<bool,
        EqualityComparable<T>::value &&
        EqualityComparable<U>::value &&
        EqualityComparable<typename detail::std_common_type<T, U>::type>::value
    > { };


    //////////////////////////////////////////////////////////////////////////
    // LessThanComparable
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename U = T, typename = void>
    struct LessThanComparable : std::false_type { };

    template <typename T>
    struct LessThanComparable<T, T, detail::void_t<
        decltype(static_cast<T&&>(*(T*)0) < static_cast<T&&>(*(T*)0) ? 0:0)
    >> : std::true_type { };

    template <typename T, typename U>
    struct LessThanComparable<T, U, std::enable_if_t<
        !std::is_same<T, U>::value,
        detail::void_t<
            decltype(static_cast<T&&>(*(T*)0) < static_cast<U&&>(*(U*)0) ? 0:0),
            decltype(static_cast<U&&>(*(U*)0) < static_cast<T&&>(*(T*)0) ? 0:0),
            typename detail::std_common_type<T, U>::type
        >
    >>
        : std::integral_constant<bool,
            LessThanComparable<T>::value &&
            LessThanComparable<U>::value &&
            LessThanComparable<typename detail::std_common_type<T, U>::type>::value
        >
    { };
    //! @endcond
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_CONCEPTS_HPP
