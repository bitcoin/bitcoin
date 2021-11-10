/*!
@file
Defines a SFINAE-friendly version of `std::common_type`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_STD_COMMON_TYPE_HPP
#define BOOST_HANA_DETAIL_STD_COMMON_TYPE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>

#include <utility>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    //! @ingroup group-details
    //! Equivalent to `std::common_type`, except it is SFINAE-friendly and
    //! does not support custom specializations.
    template <typename T, typename U, typename = void>
    struct std_common_type { };

    template <typename T, typename U>
    struct std_common_type<T, U, decltype((void)(
        true ? std::declval<T>() : std::declval<U>()
    ))> {
        using type = typename detail::decay<
            decltype(true ? std::declval<T>() : std::declval<U>())
        >::type;
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_STD_COMMON_TYPE_HPP
