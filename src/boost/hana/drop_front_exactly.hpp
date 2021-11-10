/*!
@file
Defines `boost::hana::drop_front_exactly`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DROP_FRONT_EXACTLY_HPP
#define BOOST_HANA_DROP_FRONT_EXACTLY_HPP

#include <boost/hana/fwd/drop_front_exactly.hpp>

#include <boost/hana/bool.hpp>
#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/concept/iterable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/drop_front.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/is_empty.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename N>
    constexpr auto drop_front_exactly_t::operator()(Xs&& xs, N const& n) const {
        using It = typename hana::tag_of<Xs>::type;
        using DropFrontExactly = BOOST_HANA_DISPATCH_IF(drop_front_exactly_impl<It>,
            hana::Iterable<It>::value &&
            hana::IntegralConstant<N>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Iterable<It>::value,
        "hana::drop_front_exactly(xs, n) requires 'xs' to be an Iterable");

        static_assert(hana::IntegralConstant<N>::value,
        "hana::drop_front_exactly(xs, n) requires 'n' to be an IntegralConstant");
    #endif

        static_assert(N::value >= 0,
        "hana::drop_front_exactly(xs, n) requires 'n' to be non-negative");

        return DropFrontExactly::apply(static_cast<Xs&&>(xs), n);
    }

    template <typename Xs>
    constexpr auto drop_front_exactly_t::operator()(Xs&& xs) const {
        return (*this)(static_cast<Xs&&>(xs), hana::size_c<1>);
    }
    //! @endcond

    namespace detail {
        template <typename Xs, typename N>
        constexpr void check_dfe_overflow(Xs const& xs, N const&, hana::true_) {
            constexpr bool n_overflew_length = decltype(
                hana::is_empty(hana::drop_front(xs, hana::size_c<N::value - 1>))
            )::value;
            static_assert(!n_overflew_length,
            "hana::drop_front_exactly(xs, n) requires 'n' to be less than or "
            "equal to the number of elements in 'xs'");
        }

        template <typename Xs, typename N>
        constexpr void check_dfe_overflow(Xs const&, N const&, hana::false_) { }
    }

    template <typename It, bool condition>
    struct drop_front_exactly_impl<It, when<condition>> : default_ {
        template <typename Xs, typename N>
        static constexpr auto apply(Xs&& xs, N const& n) {
            auto result = hana::drop_front(static_cast<Xs&&>(xs), n);
            constexpr bool check_for_overflow =
                decltype(hana::is_empty(result))::value && N::value != 0;

            detail::check_dfe_overflow(xs, n, hana::bool_c<check_for_overflow>);

            return result; // NRVO applied
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DROP_FRONT_EXACTLY_HPP
