/*!
@file
Defines `boost::hana::index_if`.

@copyright Louis Dionne 2013-2017
@copyright Jason Rice 2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_INDEX_IF_HPP
#define BOOST_HANA_INDEX_IF_HPP

#include <boost/hana/concept/foldable.hpp>
#include <boost/hana/concept/iterable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/detail/index_if.hpp>
#include <boost/hana/fwd/at.hpp>
#include <boost/hana/fwd/basic_tuple.hpp>
#include <boost/hana/fwd/index_if.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/length.hpp>
#include <boost/hana/optional.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Pred>
    constexpr auto index_if_t::operator()(Xs&& xs, Pred&& pred) const {
        using S = typename hana::tag_of<Xs>::type;
        using IndexIf = BOOST_HANA_DISPATCH_IF(index_if_impl<S>,
            hana::Iterable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Iterable<S>::value,
        "hana::index_if(xs, pred) requires 'xs' to be a Iterable");
    #endif

        return IndexIf::apply(static_cast<Xs&&>(xs), static_cast<Pred&&>(pred));
    }
    //! @endcond

    namespace detail {
        template <std::size_t i, std::size_t N, bool Done>
        struct iterate_while;

        template <std::size_t i, std::size_t N>
        struct iterate_while<i, N, false> {
            template <typename Xs, typename Pred>
            using f = typename iterate_while<i + 1, N,
                static_cast<bool>(detail::decay<decltype(
                    std::declval<Pred>()(
                      hana::at(std::declval<Xs>(), hana::size_c<i>)))>::type::value)
            >::template f<Xs, Pred>;
        };

        template <std::size_t N>
        struct iterate_while<N, N, false> {
            template <typename Xs, typename Pred>
            using f = hana::optional<>;
        };

        template <std::size_t i, std::size_t N>
        struct iterate_while<i, N, true> {
            template <typename Xs, typename Pred>
            using f = hana::optional<hana::size_t<i - 1>>;
        };
    }

    template <typename Tag>
    struct index_if_impl<Tag, when<Foldable<Tag>::value>> {
        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs const& xs, Pred const&)
            -> typename detail::iterate_while<0,
                decltype(hana::length(xs))::value, false>
                    ::template f<Xs, Pred>
        { return {}; }
    };

    template <typename It>
    struct index_if_impl<It, when<!Foldable<It>::value>> {
        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs const&, Pred const&)
            -> typename detail::iterate_while<0,
                static_cast<std::size_t>(-1), false>
                    ::template f<Xs, Pred>
        { return {}; }
    };

    // basic_tuple is implemented here to solve circular dependency issues.
    template <>
    struct index_if_impl<basic_tuple_tag> {
        template <typename ...Xs, typename Pred>
        static constexpr auto apply(basic_tuple<Xs...> const&, Pred const&)
            -> typename detail::index_if<Pred, Xs...>::type
        { return {}; }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_INDEX_IF_HPP
