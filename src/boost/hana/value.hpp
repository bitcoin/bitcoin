/*!
@file
Defines `boost::hana::value`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_VALUE_HPP
#define BOOST_HANA_VALUE_HPP

#include <boost/hana/fwd/value.hpp>

#include <boost/hana/concept/constant.hpp>
#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    template <typename C, bool condition>
    struct value_impl<C, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) = delete;
    };

    template <typename T>
    constexpr decltype(auto) value() {
        using RawT = typename std::remove_cv<
            typename std::remove_reference<T>::type
        >::type;
        using C = typename hana::tag_of<RawT>::type;
        using Value = BOOST_HANA_DISPATCH_IF(
            value_impl<C>, hana::Constant<C>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Constant<C>::value,
        "hana::value<T>() requires 'T' to be a Constant");
    #endif

        return Value::template apply<RawT>();
    }

    template <typename I>
    struct value_impl<I, when<hana::IntegralConstant<I>::value>> {
        template <typename C>
        static constexpr auto apply()
        { return C::value; }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_VALUE_HPP
