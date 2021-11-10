/*!
@file
Defines `boost::hana::sort`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_SORT_HPP
#define BOOST_HANA_SORT_HPP

#include <boost/hana/fwd/sort.hpp>

#include <boost/hana/at.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/detail/nested_by.hpp> // required by fwd decl
#include <boost/hana/length.hpp>
#include <boost/hana/less.hpp>

#include <utility> // std::declval, std::index_sequence


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Predicate>
    constexpr auto sort_t::operator()(Xs&& xs, Predicate&& pred) const {
        using S = typename hana::tag_of<Xs>::type;
        using Sort = BOOST_HANA_DISPATCH_IF(sort_impl<S>,
            hana::Sequence<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Sequence<S>::value,
        "hana::sort(xs, predicate) requires 'xs' to be a Sequence");
    #endif

        return Sort::apply(static_cast<Xs&&>(xs),
                           static_cast<Predicate&&>(pred));
    }

    template <typename Xs>
    constexpr auto sort_t::operator()(Xs&& xs) const {
        using S = typename hana::tag_of<Xs>::type;
        using Sort = BOOST_HANA_DISPATCH_IF(sort_impl<S>,
            hana::Sequence<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Sequence<S>::value,
        "hana::sort(xs) requires 'xs' to be a Sequence");
    #endif

        return Sort::apply(static_cast<Xs&&>(xs));
    }
    //! @endcond

    namespace detail {
        template <typename Xs, typename Pred>
        struct sort_predicate {
            template <std::size_t I, std::size_t J>
            using apply = decltype(std::declval<Pred>()(
                hana::at_c<I>(std::declval<Xs>()),
                hana::at_c<J>(std::declval<Xs>())
            ));
        };

        template <typename Left, typename Right>
        struct concat;

        template <std::size_t ...l, std::size_t ...r>
        struct concat<std::index_sequence<l...>, std::index_sequence<r...>> {
            using type = std::index_sequence<l..., r...>;
        };

        template <typename Pred, bool PickRight, typename Left, typename Right>
        struct merge;

        template <
            typename Pred,
            std::size_t l0,
            std::size_t l1,
            std::size_t ...l,
            std::size_t r0,
            std::size_t ...r>
        struct merge<
            Pred,
            false,
            std::index_sequence<l0, l1, l...>,
            std::index_sequence<r0, r...>
        > {
            using type = typename concat<
                std::index_sequence<l0>,
                typename merge<
                    Pred,
                    (bool)Pred::template apply<r0, l1>::value,
                    std::index_sequence<l1, l...>,
                    std::index_sequence<r0, r...>
                >::type
            >::type;
        };

        template <
            typename Pred,
            std::size_t l0,
            std::size_t r0,
            std::size_t ...r>
        struct merge<
            Pred,
            false,
            std::index_sequence<l0>,
            std::index_sequence<r0, r...>
        > {
            using type = std::index_sequence<l0, r0, r...>;
        };

        template <
            typename Pred,
            std::size_t l0,
            std::size_t ...l,
            std::size_t r0,
            std::size_t r1,
            std::size_t ...r>
        struct merge<
            Pred,
            true,
            std::index_sequence<l0, l...>,
            std::index_sequence<r0, r1, r...>
        > {
            using type = typename concat<
                std::index_sequence<r0>,
                typename merge<
                    Pred,
                    (bool)Pred::template apply<r1, l0>::value,
                    std::index_sequence<l0, l...>,
                    std::index_sequence<r1, r...>
                >::type
            >::type;
        };

        template <
            typename Pred,
            std::size_t l0,
            std::size_t ...l,
            std::size_t r0>
        struct merge<
            Pred,
            true,
            std::index_sequence<l0, l...>,
            std::index_sequence<r0>
        > {
            using type = std::index_sequence<r0, l0, l...>;
        };

        template <typename Pred, typename Left, typename Right>
        struct merge_helper;

        template <
            typename Pred,
            std::size_t l0,
            std::size_t ...l,
            std::size_t r0,
            std::size_t ...r>
        struct merge_helper<
            Pred,
            std::index_sequence<l0, l...>,
            std::index_sequence<r0, r...>
        > {
            using type = typename merge<
                Pred,
                (bool)Pred::template apply<r0, l0>::value,
                std::index_sequence<l0, l...>,
                std::index_sequence<r0, r...>
            >::type;
        };

        // split templated structure, Nr represents the number of elements
        // from Right to move to Left
        // There are two specializations:
        // The first handles the generic case (Nr > 0)
        // The second handles the stop condition (Nr == 0)
        // These two specializations are not strictly ordered as
        //   the first cannot match Nr==0 && empty Right
        //   the second cannot match Nr!=0
        // std::enable_if<Nr!=0> is therefore required to make sure these two
        // specializations will never both be candidates during an overload
        // resolution (otherwise ambiguity occurs for Nr==0 and non-empty Right)
        template <std::size_t Nr, typename Left, typename Right, typename=void>
        struct split;

        template <
            std::size_t Nr,
            std::size_t ...l,
            std::size_t ...r,
            std::size_t r0>
        struct split<
            Nr,
            std::index_sequence<l...>,
            std::index_sequence<r0, r...>,
            typename std::enable_if<Nr!=0>::type
        > {
            using sp = split<
                Nr-1,
                std::index_sequence<l..., r0>,
                std::index_sequence<r...>
            >;
            using left = typename sp::left;
            using right = typename sp::right;
        };

        template <std::size_t ...l, std::size_t ...r>
        struct split<0, std::index_sequence<l...>, std::index_sequence<r...>> {
            using left = std::index_sequence<l...>;
            using right = std::index_sequence<r...>;
        };

        template <typename Pred, typename Sequence>
        struct merge_sort_impl;

        template <typename Pred, std::size_t ...seq>
        struct merge_sort_impl<Pred, std::index_sequence<seq...>> {
            using sequence = std::index_sequence<seq...>;
            using sp = split<
                sequence::size() / 2,
                std::index_sequence<>,
                sequence
            >;
            using type = typename merge_helper<
                Pred,
                typename merge_sort_impl<Pred, typename sp::left>::type,
                typename merge_sort_impl<Pred, typename sp::right>::type
            >::type;
        };

        template <typename Pred, std::size_t x>
        struct merge_sort_impl<Pred, std::index_sequence<x>> {
            using type = std::index_sequence<x>;
        };

        template <typename Pred>
        struct merge_sort_impl<Pred, std::index_sequence<>> {
            using type = std::index_sequence<>;
        };
    } // end namespace detail

    template <typename S, bool condition>
    struct sort_impl<S, when<condition>> : default_ {
        template <typename Xs, std::size_t ...i>
        static constexpr auto apply_impl(Xs&& xs, std::index_sequence<i...>) {
            return hana::make<S>(hana::at_c<i>(static_cast<Xs&&>(xs))...);
        }

        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred const&) {
            constexpr std::size_t Len = decltype(hana::length(xs))::value;
            using Indices = typename detail::merge_sort_impl<
                detail::sort_predicate<Xs&&, Pred>,
                std::make_index_sequence<Len>
            >::type;

            return apply_impl(static_cast<Xs&&>(xs), Indices{});
        }

        template <typename Xs>
        static constexpr auto apply(Xs&& xs)
        { return sort_impl::apply(static_cast<Xs&&>(xs), hana::less); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_SORT_HPP
