/*!
@file
Defines `boost::hana::detail::variadic::take`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_VARIADIC_TAKE_HPP
#define BOOST_HANA_DETAIL_VARIADIC_TAKE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/variadic/split_at.hpp>
#include <boost/hana/functional/always.hpp>
#include <boost/hana/functional/reverse_partial.hpp>

#include <cstddef>


BOOST_HANA_NAMESPACE_BEGIN namespace detail { namespace variadic {
    struct take_impl2 {
        template <typename F, typename ...Xs>
        constexpr decltype(auto) operator()(F&& f, Xs&& ...xs) const {
            return static_cast<F&&>(f)(static_cast<Xs&&>(xs)...);
        }
    };

    struct take_impl1 {
        template <typename ...Xs>
        constexpr auto operator()(Xs&& ...xs) const {
            return hana::always(
                reverse_partial(take_impl2{},
                    static_cast<Xs&&>(xs)...)
            );
        }
    };

    template <std::size_t n>
    struct take_t {
        template <typename ...Xs>
        constexpr decltype(auto) operator()(Xs&& ...xs) const {
            return variadic::split_at<n>(static_cast<Xs&&>(xs)...)(take_impl1{});
        }
    };

    template <std::size_t n>
    BOOST_HANA_INLINE_VARIABLE constexpr take_t<n> take{};
}} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_VARIADIC_TAKE_HPP
