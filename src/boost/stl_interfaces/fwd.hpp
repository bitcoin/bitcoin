// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_STL_INTERFACES_FWD_HPP
#define BOOST_STL_INTERFACES_FWD_HPP

#include <boost/stl_interfaces/config.hpp>

#if BOOST_STL_INTERFACES_USE_CONCEPTS
#include <ranges>
#endif
#if defined(__cpp_lib_three_way_comparison)
#include <compare>
#endif

#ifndef BOOST_STL_INTERFACES_DOXYGEN

#if defined(_MSC_VER) || defined(__GNUC__) && __GNUC__ < 8
#define BOOST_STL_INTERFACES_NO_HIDDEN_FRIEND_CONSTEXPR
#define BOOST_STL_INTERFACES_HIDDEN_FRIEND_CONSTEXPR
#else
#define BOOST_STL_INTERFACES_HIDDEN_FRIEND_CONSTEXPR constexpr
#endif

#if defined(__GNUC__) && __GNUC__ < 9
#define BOOST_STL_INTERFACES_CONCEPT concept bool
#else
#define BOOST_STL_INTERFACES_CONCEPT concept
#endif

#endif


namespace boost { namespace stl_interfaces {

    /** An enumeration used to indicate whether the underlying data have a
        contiguous or discontiguous layout when instantiating `view_interface`
        and `sequence_container_interface`. */
    enum class element_layout : bool {
        discontiguous = false,
        contiguous = true
    };

    BOOST_STL_INTERFACES_NAMESPACE_V1 {

        namespace v1_dtl {
            template<typename... T>
            using void_t = void;

            template<typename Iter>
            using iter_difference_t =
                typename std::iterator_traits<Iter>::difference_type;

            template<typename Range, typename = void>
            struct iterator;
            template<typename Range>
            struct iterator<
                Range,
                void_t<decltype(std::declval<Range &>().begin())>>
            {
                using type = decltype(std::declval<Range &>().begin());
            };
            template<typename Range>
            using iterator_t = typename iterator<Range>::type;

            template<typename Range, typename = void>
            struct sentinel;
            template<typename Range>
            struct sentinel<
                Range,
                void_t<decltype(std::declval<Range &>().end())>>
            {
                using type = decltype(std::declval<Range &>().end());
            };
            template<typename Range>
            using sentinel_t = typename sentinel<Range>::type;

            template<typename Range>
            using range_difference_t = iter_difference_t<iterator_t<Range>>;

            template<typename Range>
            using common_range =
                std::is_same<iterator_t<Range>, sentinel_t<Range>>;

            template<typename Range, typename = void>
            struct decrementable_sentinel : std::false_type
            {
            };
            template<typename Range>
            struct decrementable_sentinel<
                Range,
                void_t<decltype(--std::declval<sentinel_t<Range> &>())>>
                : std::true_type
            {
            };
        }

    }
}}

#endif
