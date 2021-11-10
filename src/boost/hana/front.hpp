/*!
@file
Defines `boost::hana::front`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FRONT_HPP
#define BOOST_HANA_FRONT_HPP

#include <boost/hana/fwd/front.hpp>

#include <boost/hana/at.hpp>
#include <boost/hana/concept/iterable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs>
    constexpr decltype(auto) front_t::operator()(Xs&& xs) const {
        using It = typename hana::tag_of<Xs>::type;
        using Front = BOOST_HANA_DISPATCH_IF(front_impl<It>,
            hana::Iterable<It>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Iterable<It>::value,
        "hana::front(xs) requires 'xs' to be an Iterable");
    #endif

        return Front::apply(static_cast<Xs&&>(xs));
    }
    //! @endcond

    template <typename It, bool condition>
    struct front_impl<It, when<condition>> : default_ {
        template <typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs)
        { return hana::at_c<0>(static_cast<Xs&&>(xs)); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FRONT_HPP
