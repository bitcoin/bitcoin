/*!
@file
Defines `boost::hana::detail::first_unsatisfied_index`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_FIRST_UNSATISFIED_INDEX_HPP
#define BOOST_HANA_DETAIL_FIRST_UNSATISFIED_INDEX_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/value.hpp>

#include <utility>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <bool, typename Pred, typename ...Xs>
    struct find_tail_size;

    template <typename Pred, typename X, typename ...Xs>
    struct find_tail_size<true, Pred, X, Xs...> {
        static constexpr int value = find_tail_size<
            static_cast<bool>(hana::value<decltype(std::declval<Pred>()(std::declval<X>()))>()),
            Pred, Xs...
        >::value;
    };

    template <typename Pred>
    struct find_tail_size<true, Pred> {
        static constexpr int value = -1;
    };

    template <typename Pred, typename ...Xs>
    struct find_tail_size<false, Pred, Xs...> {
        static constexpr int value = sizeof...(Xs);
    };

    //! @ingroup group-details
    //! Returns the index of the first element which does not satisfy `Pred`,
    //! or `sizeof...(Xs)` if no such element exists.
    template <typename Pred>
    struct first_unsatisfied_index {
        template <typename ...Xs>
        constexpr auto operator()(Xs&& ...) const {
            return hana::size_c<
                sizeof...(Xs) - 1 - find_tail_size<true, Pred, Xs&&...>::value
            >;
        }
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_FIRST_UNSATISFIED_INDEX_HPP
