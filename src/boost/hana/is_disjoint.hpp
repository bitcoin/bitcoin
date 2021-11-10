/*!
@file
Defines `boost::hana::is_disjoint`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_IS_DISJOINT_HPP
#define BOOST_HANA_IS_DISJOINT_HPP

#include <boost/hana/fwd/is_disjoint.hpp>

#include <boost/hana/concept/searchable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/none_of.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Ys>
    constexpr auto is_disjoint_t::operator()(Xs&& xs, Ys&& ys) const {
        using S1 = typename hana::tag_of<Xs>::type;
        using S2 = typename hana::tag_of<Ys>::type;
        using IsDisjoint = BOOST_HANA_DISPATCH_IF(
            decltype(is_disjoint_impl<S1, S2>{}),
            hana::Searchable<S1>::value &&
            hana::Searchable<S2>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Searchable<S1>::value,
        "hana::is_disjoint(xs, ys) requires 'xs' to be Searchable");

        static_assert(hana::Searchable<S2>::value,
        "hana::is_disjoint(xs, ys) requires 'ys' to be Searchable");
    #endif

        return IsDisjoint::apply(static_cast<Xs&&>(xs), static_cast<Ys&&>(ys));
    }
    //! @endcond

    namespace detail {
        template <typename Ys>
        struct in_by_reference {
            Ys const& ys;
            template <typename X>
            constexpr auto operator()(X const& x) const
            { return hana::contains(ys, x); }
        };
    }

    template <typename S1, typename S2, bool condition>
    struct is_disjoint_impl<S1, S2, when<condition>> : default_ {
        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs const& xs, Ys const& ys) {
            return hana::none_of(xs, detail::in_by_reference<Ys>{ys});
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_IS_DISJOINT_HPP
