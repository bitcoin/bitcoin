/*!
@file
Defines `boost::hana::range`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_RANGE_HPP
#define BOOST_HANA_RANGE_HPP

#include <boost/hana/fwd/range.hpp>

#include <boost/hana/bool.hpp>
#include <boost/hana/concept/integral_constant.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/common.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/core/tag_of.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/detail/operators/comparable.hpp>
#include <boost/hana/detail/operators/iterable.hpp>
#include <boost/hana/fwd/at.hpp>
#include <boost/hana/fwd/back.hpp>
#include <boost/hana/fwd/contains.hpp>
#include <boost/hana/fwd/drop_front.hpp>
#include <boost/hana/fwd/drop_front_exactly.hpp>
#include <boost/hana/fwd/equal.hpp>
#include <boost/hana/fwd/find.hpp>
#include <boost/hana/fwd/front.hpp>
#include <boost/hana/fwd/is_empty.hpp>
#include <boost/hana/fwd/length.hpp>
#include <boost/hana/fwd/maximum.hpp>
#include <boost/hana/fwd/minimum.hpp>
#include <boost/hana/fwd/product.hpp>
#include <boost/hana/fwd/sum.hpp>
#include <boost/hana/fwd/unpack.hpp>
#include <boost/hana/integral_constant.hpp> // required by fwd decl and below
#include <boost/hana/optional.hpp>
#include <boost/hana/value.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // range<>
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    template <typename T, T From, T To>
    struct range
        : detail::operators::adl<range<T, From, To>>
        , detail::iterable_operators<range<T, From, To>>
    {
        static_assert(From <= To,
        "hana::make_range(from, to) requires 'from <= to'");

        using value_type = T;
        static constexpr value_type from = From;
        static constexpr value_type to = To;
    };
    //! @endcond

    template <typename T, T From, T To>
    struct tag_of<range<T, From, To>> {
        using type = range_tag;
    };

    //////////////////////////////////////////////////////////////////////////
    // make<range_tag>
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct make_impl<range_tag> {
        template <typename From, typename To>
        static constexpr auto apply(From const&, To const&) {

        #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
            static_assert(hana::IntegralConstant<From>::value,
            "hana::make_range(from, to) requires 'from' to be an IntegralConstant");

            static_assert(hana::IntegralConstant<To>::value,
            "hana::make_range(from, to) requires 'to' to be an IntegralConstant");
        #endif

            using T = typename common<
                typename hana::tag_of<From>::type::value_type,
                typename hana::tag_of<To>::type::value_type
            >::type;
            constexpr T from = hana::to<T>(From::value);
            constexpr T to = hana::to<T>(To::value);
            return range<T, from, to>{};
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <>
        struct comparable_operators<range_tag> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<range_tag, range_tag> {
        template <typename R1, typename R2>
        static constexpr auto apply(R1 const&, R2 const&) {
            return hana::bool_c<
                (R1::from == R1::to && R2::from == R2::to) ||
                (R1::from == R2::from && R1::to == R2::to)
            >;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct unpack_impl<range_tag> {
        template <typename T, T from, typename F, T ...v>
        static constexpr decltype(auto)
        unpack_helper(F&& f, std::integer_sequence<T, v...>) {
            return static_cast<F&&>(f)(integral_constant<T, from + v>{}...);
        }

        template <typename T, T from, T to, typename F>
        static constexpr decltype(auto) apply(range<T, from, to> const&, F&& f) {
            return unpack_helper<T, from>(static_cast<F&&>(f),
                std::make_integer_sequence<T, to - from>{});
        }
    };

    template <>
    struct length_impl<range_tag> {
        template <typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return hana::size_c<static_cast<std::size_t>(to - from)>; }
    };

    template <>
    struct minimum_impl<range_tag> {
        template <typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return integral_c<T, from>; }
    };

    template <>
    struct maximum_impl<range_tag> {
        template <typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return integral_c<T, to-1>; }
    };

    template <>
    struct sum_impl<range_tag> {
        // Returns the sum of `[m, n]`, where `m <= n` always hold.
        template <typename I>
        static constexpr I sum_helper(I m, I n) {
            if (m == n)
                return m;

            // 0 == m < n
            else if (0 == m)
                return n * (n+1) / 2;

            // 0 < m < n
            else if (0 < m)
                return sum_helper(0, n) - sum_helper(0, m-1);

            // m < 0 <= n
            else if (0 <= n)
                return sum_helper(0, n) - sum_helper(0, -m);

            // m < n < 0
            else
                return -sum_helper(-n, -m);
        }

        template <typename, typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&) {
            return integral_c<T, from == to ? 0 : sum_helper(from, to-1)>;
        }
    };

    template <>
    struct product_impl<range_tag> {
        // Returns the product of `[m, n)`, where `m <= n` always hold.
        template <typename I>
        static constexpr I product_helper(I m, I n) {
            if (m <= 0 && 0 < n)
                return 0;
            else {
                I p = 1;
                for (; m != n; ++m)
                    p *= m;
                return p;
            }
        }

        template <typename, typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return integral_c<T, product_helper(from, to)>; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Searchable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct find_impl<range_tag> {
        template <typename T, T from, typename N>
        static constexpr auto find_helper(hana::true_) {
            constexpr T n = N::value;
            return hana::just(hana::integral_c<T, n>);
        }

        template <typename T, T from, typename N>
        static constexpr auto find_helper(hana::false_)
        { return hana::nothing; }

        template <typename T, T from, T to, typename N>
        static constexpr auto apply(range<T, from, to> const&, N const&) {
            constexpr auto n = N::value;
            return find_helper<T, from, N>(hana::bool_c<(n >= from && n < to)>);
        }
    };

    template <>
    struct contains_impl<range_tag> {
        template <typename T, T from, T to, typename N>
        static constexpr auto apply(range<T, from, to> const&, N const&) {
            constexpr auto n = N::value;
            return bool_c<(n >= from && n < to)>;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Iterable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct front_impl<range_tag> {
        template <typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return integral_c<T, from>; }
    };

    template <>
    struct is_empty_impl<range_tag> {
        template <typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return bool_c<from == to>; }
    };

    template <>
    struct at_impl<range_tag> {
        template <typename T, T from, T to, typename N>
        static constexpr auto apply(range<T, from, to> const&, N const&) {
            constexpr auto n = N::value;
            return integral_c<T, from + n>;
        }
    };

    template <>
    struct back_impl<range_tag> {
        template <typename T, T from, T to>
        static constexpr auto apply(range<T, from, to> const&)
        { return integral_c<T, to - 1>; }
    };

    template <>
    struct drop_front_impl<range_tag> {
        template <typename T, T from, T to, typename N>
        static constexpr auto apply(range<T, from, to> const&, N const&) {
            constexpr auto n = N::value;
            return range<T, (to < from + n ? to : from + n), to>{};
        }
    };

    template <>
    struct drop_front_exactly_impl<range_tag> {
        template <typename T, T from, T to, typename N>
        static constexpr auto apply(range<T, from, to> const&, N const&) {
            constexpr auto n = N::value;
            return range<T, from + n, to>{};
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_RANGE_HPP
