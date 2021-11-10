/*!
@file
Defines `boost::hana::detail::index_if`.

@copyright Louis Dionne 2013-2017
@copyright Jason Rice 2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_INDEX_IF_HPP
#define BOOST_HANA_DETAIL_INDEX_IF_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/optional.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <std::size_t i, std::size_t N, bool Done>
    struct index_if_helper;

    template <std::size_t i, std::size_t N>
    struct index_if_helper<i, N, false> {
        template <typename Pred, typename X1, typename ...Xs>
        using f = typename index_if_helper<i + 1, N,
            static_cast<bool>(detail::decay<decltype(
                std::declval<Pred>()(std::declval<X1>()))>::type::value)
        >::template f<Pred, Xs...>;
    };

    template <std::size_t N>
    struct index_if_helper<N, N, false> {
        template <typename ...>
        using f = hana::optional<>;
    };

    template <std::size_t i, std::size_t N>
    struct index_if_helper<i, N, true> {
        template <typename ...>
        using f = hana::optional<hana::size_t<i - 1>>;
    };

    template <typename Pred, typename ...Xs>
    struct index_if {
        using type = typename index_if_helper<0, sizeof...(Xs), false>
            ::template f<Pred, Xs...>;
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_INDEX_IF_HPP
