/*!
@file
Defines `boost::hana::is_subset`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_IS_SUBSET_HPP
#define BOOST_HANA_IS_SUBSET_HPP

#include <boost/hana/fwd/is_subset.hpp>

#include <boost/hana/all_of.hpp>
#include <boost/hana/concept/searchable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/core/common.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/detail/has_common_embedding.hpp>
#include <boost/hana/functional/partial.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Ys>
    constexpr auto is_subset_t::operator()(Xs&& xs, Ys&& ys) const {
        using S1 = typename hana::tag_of<Xs>::type;
        using S2 = typename hana::tag_of<Ys>::type;
        using IsSubset = BOOST_HANA_DISPATCH_IF(
            decltype(is_subset_impl<S1, S2>{}),
            hana::Searchable<S1>::value &&
            hana::Searchable<S2>::value &&
            !is_default<is_subset_impl<S1, S2>>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Searchable<S1>::value,
        "hana::is_subset(xs, ys) requires 'xs' to be Searchable");

        static_assert(hana::Searchable<S2>::value,
        "hana::is_subset(xs, ys) requires 'ys' to be Searchable");

        static_assert(!is_default<is_subset_impl<S1, S2>>::value,
        "hana::is_subset(xs, ys) requires 'xs' and 'ys' to be embeddable "
        "in a common Searchable");
    #endif

        return IsSubset::apply(static_cast<Xs&&>(xs), static_cast<Ys&&>(ys));
    }
    //! @endcond

    template <typename S1, typename S2, bool condition>
    struct is_subset_impl<S1, S2, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };

    template <typename S, bool condition>
    struct is_subset_impl<S, S, when<condition>> {
        template <typename Xs, typename Ys>
        static constexpr decltype(auto) apply(Xs&& xs, Ys&& ys) {
            return hana::all_of(static_cast<Xs&&>(xs),
                    hana::partial(hana::contains, static_cast<Ys&&>(ys)));
        }
    };

    // Cross-type overload
    template <typename S1, typename S2>
    struct is_subset_impl<S1, S2, when<
        detail::has_nontrivial_common_embedding<Searchable, S1, S2>::value
    >> {
        using C = typename common<S1, S2>::type;
        template <typename Xs, typename Ys>
        static constexpr decltype(auto) apply(Xs&& xs, Ys&& ys) {
            return hana::is_subset(hana::to<C>(static_cast<Xs&&>(xs)),
                                   hana::to<C>(static_cast<Ys&&>(ys)));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_IS_SUBSET_HPP
