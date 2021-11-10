/*!
@file
Defines `boost::hana::contains` and `boost::hana::in`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CONTAINS_HPP
#define BOOST_HANA_CONTAINS_HPP

#include <boost/hana/fwd/contains.hpp>

#include <boost/hana/any_of.hpp>
#include <boost/hana/concept/searchable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/equal.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Key>
    constexpr auto contains_t::operator()(Xs&& xs, Key&& key) const {
        using S = typename hana::tag_of<Xs>::type;
        using Contains = BOOST_HANA_DISPATCH_IF(contains_impl<S>,
            hana::Searchable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Searchable<S>::value,
        "hana::contains(xs, key) requires 'xs' to be a Searchable");
    #endif

        return Contains::apply(static_cast<Xs&&>(xs),
                               static_cast<Key&&>(key));
    }
    //! @endcond

    template <typename S, bool condition>
    struct contains_impl<S, when<condition>> : default_ {
        template <typename Xs, typename X>
        static constexpr auto apply(Xs&& xs, X&& x) {
            return hana::any_of(static_cast<Xs&&>(xs),
                    hana::equal.to(static_cast<X&&>(x)));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_CONTAINS_HPP
