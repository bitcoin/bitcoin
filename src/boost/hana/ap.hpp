/*!
@file
Defines `boost::hana::ap`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_AP_HPP
#define BOOST_HANA_AP_HPP

#include <boost/hana/fwd/ap.hpp>

#include <boost/hana/chain.hpp>
#include <boost/hana/concept/applicative.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/detail/variadic/foldl1.hpp>
#include <boost/hana/functional/curry.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/transform.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    template <typename A, bool condition>
    struct ap_impl<A, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) = delete;
    };

    //! @cond
    template <typename F, typename X>
    constexpr decltype(auto) ap_t::operator()(F&& f, X&& x) const {
        using Function = typename hana::tag_of<F>::type;
        using Value = typename hana::tag_of<X>::type;
        using Ap = BOOST_HANA_DISPATCH_IF(ap_impl<Function>,
            hana::Applicative<Function>::value && hana::Applicative<Value>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Applicative<Function>::value,
        "hana::ap(f, x) requires 'f' to be an Applicative");

        static_assert(hana::Applicative<Value>::value,
        "hana::ap(f, x) requires 'x' to be an Applicative");
    #endif

        return Ap::apply(static_cast<F&&>(f), static_cast<X&&>(x));
    }

    template <typename F, typename ...Xs>
    constexpr decltype(auto) ap_t::operator()(F&& f, Xs&& ...xs) const {
        static_assert(sizeof...(xs) >= 1,
        "hana::ap must be called with at least two arguments");

        return detail::variadic::foldl1(
            *this,
            hana::transform(static_cast<F&&>(f), hana::curry<sizeof...(xs)>),
            static_cast<Xs&&>(xs)...
        );
    }
    //! @endcond

    template <typename S>
    struct ap_impl<S, when<Sequence<S>::value>> {
        template <typename F, typename X>
        static constexpr decltype(auto) apply(F&& f, X&& x) {
            return hana::chain(
                static_cast<F&&>(f),
                hana::partial(hana::transform, static_cast<X&&>(x))
            );
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_AP_HPP
