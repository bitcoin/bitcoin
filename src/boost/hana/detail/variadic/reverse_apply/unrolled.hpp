/*!
@file
Defines `boost::hana::detail::variadic::reverse_apply_unrolled`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_VARIADIC_REVERSE_APPLY_UNROLLED_HPP
#define BOOST_HANA_DETAIL_VARIADIC_REVERSE_APPLY_UNROLLED_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/functional/reverse_partial.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail { namespace variadic {
    struct reverse_apply_unrolled_impl {
        template <typename F>
        constexpr decltype(auto) operator()(F&& f) const {
            return static_cast<F&&>(f)();
        }

        template <typename F, typename X1>
        constexpr decltype(auto) operator()(F&& f, X1&& x1) const {
            return static_cast<F&&>(f)(
                static_cast<X1&&>(x1)
            );
        }

        template <typename F, typename X1, typename X2>
        constexpr decltype(auto) operator()(F&& f, X1&& x1, X2&& x2) const {
            return static_cast<F&&>(f)(
                static_cast<X2&&>(x2),
                static_cast<X1&&>(x1)
            );
        }

        template <typename F, typename X1, typename X2, typename X3>
        constexpr decltype(auto) operator()(F&& f, X1&& x1, X2&& x2, X3&& x3) const {
            return static_cast<F&&>(f)(
                static_cast<X3&&>(x3),
                static_cast<X2&&>(x2),
                static_cast<X1&&>(x1)
            );
        }

        template <typename F, typename X1, typename X2, typename X3, typename X4>
        constexpr decltype(auto) operator()(F&& f, X1&& x1, X2&& x2, X3&& x3, X4&& x4) const {
            return static_cast<F&&>(f)(
                static_cast<X4&&>(x4),
                static_cast<X3&&>(x3),
                static_cast<X2&&>(x2),
                static_cast<X1&&>(x1)
            );
        }

        template <typename F, typename X1, typename X2, typename X3, typename X4, typename X5>
        constexpr decltype(auto) operator()(F&& f, X1&& x1, X2&& x2, X3&& x3, X4&& x4, X5&& x5) const {
            return static_cast<F&&>(f)(
                static_cast<X5&&>(x5),
                static_cast<X4&&>(x4),
                static_cast<X3&&>(x3),
                static_cast<X2&&>(x2),
                static_cast<X1&&>(x1)
            );
        }

        template <typename F, typename X1, typename X2, typename X3, typename X4, typename X5, typename X6, typename ...Xn>
        constexpr decltype(auto) operator()(F&& f, X1&& x1, X2&& x2, X3&& x3, X4&& x4, X5&& x5, X6&& x6, Xn&& ...xn) const {
            return (*this)(hana::reverse_partial(
                  static_cast<F&&>(f)
                , static_cast<X6&&>(x6)
                , static_cast<X5&&>(x5)
                , static_cast<X4&&>(x4)
                , static_cast<X3&&>(x3)
                , static_cast<X2&&>(x2)
                , static_cast<X1&&>(x1)
            ), static_cast<Xn&&>(xn)...);
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr reverse_apply_unrolled_impl reverse_apply_unrolled{};
}} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_VARIADIC_REVERSE_APPLY_UNROLLED_HPP
