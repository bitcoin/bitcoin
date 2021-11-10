/*!
@file
Defines `boost::hana::take_front` and `boost::hana::take_front_c`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_TAKE_FRONT_HPP
#define BOOST_HANA_TAKE_FRONT_HPP

#include <boost/hana/fwd/take_front.hpp>

#include <boost/hana/at.hpp>
#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/length.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename N>
    constexpr auto take_front_t::operator()(Xs&& xs, N const& n) const {
        using S = typename hana::tag_of<Xs>::type;
        using TakeFront = BOOST_HANA_DISPATCH_IF(take_front_impl<S>,
            hana::Sequence<S>::value &&
            hana::IntegralConstant<N>::value
        );

#ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Sequence<S>::value,
        "hana::take_front(xs, n) requires 'xs' to be a Sequence");

        static_assert(hana::IntegralConstant<N>::value,
        "hana::take_front(xs, n) requires 'n' to be an IntegralConstant");
#endif

        return TakeFront::apply(static_cast<Xs&&>(xs), n);
    }
    //! @endcond

    template <typename S, bool condition>
    struct take_front_impl<S, when<condition>> : default_ {
        template <typename Xs, std::size_t ...n>
        static constexpr auto take_front_helper(Xs&& xs, std::index_sequence<n...>) {
            return hana::make<S>(hana::at_c<n>(static_cast<Xs&&>(xs))...);
        }

        template <typename Xs, typename N>
        static constexpr auto apply(Xs&& xs, N const&) {
            constexpr std::size_t n = N::value;
            constexpr std::size_t size = decltype(hana::length(xs))::value;
            return take_front_helper(static_cast<Xs&&>(xs),
                        std::make_index_sequence<(n < size ? n : size)>{});
        }
    };

    template <std::size_t n>
    struct take_front_c_t {
        template <typename Xs>
        constexpr auto operator()(Xs&& xs) const {
            return hana::take_front(static_cast<Xs&&>(xs), hana::size_c<n>);
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_TAKE_FRONT_HPP
