/*!
@file
Defines `boost::hana::concat`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CONCAT_HPP
#define BOOST_HANA_CONCAT_HPP

#include <boost/hana/fwd/concat.hpp>

#include <boost/hana/at.hpp>
#include <boost/hana/concept/monad_plus.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/length.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Ys>
    constexpr auto concat_t::operator()(Xs&& xs, Ys&& ys) const {
        using M = typename hana::tag_of<Xs>::type;
        using Concat = BOOST_HANA_DISPATCH_IF(concat_impl<M>,
            hana::MonadPlus<M>::value &&
            std::is_same<typename hana::tag_of<Ys>::type, M>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(std::is_same<typename hana::tag_of<Ys>::type, M>::value,
        "hana::concat(xs, ys) requires 'xs' and 'ys' to have the same tag");

        static_assert(hana::MonadPlus<M>::value,
        "hana::concat(xs, ys) requires 'xs' and 'ys' to be MonadPlus");
    #endif

        return Concat::apply(static_cast<Xs&&>(xs), static_cast<Ys&&>(ys));
    }
    //! @endcond

    template <typename M, bool condition>
    struct concat_impl<M, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };

    template <typename S>
    struct concat_impl<S, when<Sequence<S>::value>> {
        template <typename Xs, typename Ys, std::size_t ...xi, std::size_t ...yi>
        static constexpr auto
        concat_helper(Xs&& xs, Ys&& ys, std::index_sequence<xi...>,
                                        std::index_sequence<yi...>)
        {
            return hana::make<S>(
                hana::at_c<xi>(static_cast<Xs&&>(xs))...,
                hana::at_c<yi>(static_cast<Ys&&>(ys))...
            );
        }

        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs&& xs, Ys&& ys) {
            constexpr std::size_t xi = decltype(hana::length(xs))::value;
            constexpr std::size_t yi = decltype(hana::length(ys))::value;
            return concat_helper(static_cast<Xs&&>(xs), static_cast<Ys&&>(ys),
                                 std::make_index_sequence<xi>{},
                                 std::make_index_sequence<yi>{});
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_CONCAT_HPP
