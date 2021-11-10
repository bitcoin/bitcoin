/*!
@file
Defines arithmetic operators.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_OPERATORS_ARITHMETIC_HPP
#define BOOST_HANA_DETAIL_OPERATORS_ARITHMETIC_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/tag_of.hpp>
#include <boost/hana/fwd/div.hpp>
#include <boost/hana/fwd/minus.hpp>
#include <boost/hana/fwd/mod.hpp>
#include <boost/hana/fwd/mult.hpp>
#include <boost/hana/fwd/negate.hpp>
#include <boost/hana/fwd/plus.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <typename Tag>
    struct arithmetic_operators {
        static constexpr bool value = false;
    };

    namespace operators {
        template <typename X, typename Y, typename = typename std::enable_if<
            detail::arithmetic_operators<typename hana::tag_of<X>::type>::value ||
            detail::arithmetic_operators<typename hana::tag_of<Y>::type>::value
        >::type>
        constexpr auto operator+(X&& x, Y&& y)
        { return hana::plus(static_cast<X&&>(x), static_cast<Y&&>(y)); }


        template <typename X, typename Y, typename = typename std::enable_if<
            detail::arithmetic_operators<typename hana::tag_of<X>::type>::value ||
            detail::arithmetic_operators<typename hana::tag_of<Y>::type>::value
        >::type>
        constexpr auto operator-(X&& x, Y&& y)
        { return hana::minus(static_cast<X&&>(x), static_cast<Y&&>(y)); }

        template <typename X, typename = typename std::enable_if<
            detail::arithmetic_operators<typename hana::tag_of<X>::type>::value
        >::type>
        constexpr auto operator-(X&& x)
        { return hana::negate(static_cast<X&&>(x)); }


        template <typename X, typename Y, typename = typename std::enable_if<
            detail::arithmetic_operators<typename hana::tag_of<X>::type>::value ||
            detail::arithmetic_operators<typename hana::tag_of<Y>::type>::value
        >::type>
        constexpr auto operator*(X&& x, Y&& y)
        { return hana::mult(static_cast<X&&>(x), static_cast<Y&&>(y)); }


        template <typename X, typename Y, typename = typename std::enable_if<
            detail::arithmetic_operators<typename hana::tag_of<X>::type>::value ||
            detail::arithmetic_operators<typename hana::tag_of<Y>::type>::value
        >::type>
        constexpr auto operator/(X&& x, Y&& y)
        { return hana::div(static_cast<X&&>(x), static_cast<Y&&>(y)); }

        template <typename X, typename Y, typename = typename std::enable_if<
            detail::arithmetic_operators<typename hana::tag_of<X>::type>::value ||
            detail::arithmetic_operators<typename hana::tag_of<Y>::type>::value
        >::type>
        constexpr auto operator%(X&& x, Y&& y)
        { return hana::mod(static_cast<X&&>(x), static_cast<Y&&>(y)); }
    } // end namespace operators
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_OPERATORS_ARITHMETIC_HPP
