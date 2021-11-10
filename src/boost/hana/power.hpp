/*!
@file
Defines `boost::hana::power`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_POWER_HPP
#define BOOST_HANA_POWER_HPP

#include <boost/hana/fwd/power.hpp>

#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/concept/ring.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/functional/iterate.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/mult.hpp>
#include <boost/hana/one.hpp>

#include <cstddef>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename X, typename N>
    constexpr decltype(auto) power_t::operator()(X&& x, N const& n) const {
        using R = typename hana::tag_of<X>::type;
        using Power = BOOST_HANA_DISPATCH_IF(power_impl<R>,
            hana::Ring<R>::value &&
            hana::IntegralConstant<N>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Ring<R>::value,
        "hana::power(x, n) requires 'x' to be in a Ring");

        static_assert(hana::IntegralConstant<N>::value,
        "hana::power(x, n) requires 'n' to be an IntegralConstant");
    #endif

        static_assert(N::value >= 0,
        "hana::power(x, n) requires 'n' to be non-negative");

        return Power::apply(static_cast<X&&>(x), n);
    }
    //! @endcond

    template <typename R, bool condition>
    struct power_impl<R, when<condition>> : default_ {
        template <typename X, typename N>
        static constexpr decltype(auto) apply(X&& x, N const&) {
            constexpr std::size_t n = N::value;
            return hana::iterate<n>(
                hana::partial(hana::mult, static_cast<X&&>(x)),
                hana::one<R>()
            );
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_POWER_HPP
