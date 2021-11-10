/*!
@file
Defines `boost::hana::detail::type_foldl1`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_TYPE_FOLDL1_HPP
#define BOOST_HANA_DETAIL_TYPE_FOLDL1_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <unsigned n>
    struct type_foldl1_t;

    template <>
    struct type_foldl1_t<0> {
        template <
            template <typename ...> class f,
            typename state
        >
        using result = state;
    };

    template <>
    struct type_foldl1_t<1> {
        template <
            template <typename ...> class f,
            typename state,
            typename x1
        >
        using result = typename f<state, x1>::type;
    };

    template <>
    struct type_foldl1_t<2> {
        template <
            template <typename ...> class f,
            typename state,
            typename x1, typename x2
        >
        using result = typename f<typename f<state, x1>::type, x2>::type;
    };

    template <>
    struct type_foldl1_t<3> {
        template <
            template <typename ...> class f,
            typename state,
            typename x1, typename x2, typename x3
        >
        using result = typename f<
            typename f<
                typename f<state, x1>::type,
                x2
            >::type,
            x3
        >::type;
    };

    template <>
    struct type_foldl1_t<4> {
        template <
            template <typename ...> class f,
            typename state,
            typename x1, typename x2, typename x3, typename x4
        >
        using result = typename f<
            typename f<
                typename f<
                    typename f<state, x1>::type,
                    x2
                >::type,
                x3
            >::type,
            x4
        >::type;
    };

    template <>
    struct type_foldl1_t<5> {
        template <
            template <typename ...> class f,
            typename state,
            typename x1, typename x2, typename x3, typename x4, typename x5
        >
        using result = typename f<
            typename f<
                typename f<
                    typename f<
                        typename f<state, x1>::type,
                        x2
                    >::type,
                    x3
                >::type,
                x4
            >::type,
            x5
        >::type;
    };

    template <>
    struct type_foldl1_t<6> {
        template <
            template <typename ...> class f,
            typename state,
            typename x1, typename x2, typename x3, typename x4, typename x5, typename x6,
            typename ...xs
        >
        using result =
            typename type_foldl1_t<(sizeof...(xs) > 6 ? 6 : sizeof...(xs))>::
            template result<
                f,
                typename f<
                    typename f<
                        typename f<
                            typename f<
                                typename f<
                                    typename f<state, x1>::type,
                                    x2
                                >::type,
                                x3
                            >::type,
                            x4
                        >::type,
                        x5
                    >::type,
                    x6
                >::type,
                xs...
            >;
    };

    template <template <typename ...> class f, typename x1, typename ...xn>
    struct type_foldl1 {
        using type = typename type_foldl1_t<(sizeof...(xn) > 6 ? 6 : sizeof...(xn))>
                     ::template result<f, x1, xn...>;
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_TYPE_FOLDL1_HPP
