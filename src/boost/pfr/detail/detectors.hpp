// Copyright (c) 2016-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_DETECTORS_HPP
#define BOOST_PFR_DETAIL_DETECTORS_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <functional>
#include <type_traits>

namespace boost { namespace pfr { namespace detail {
///////////////////// `value` is true if Detector<Tleft, Tright> does not compile (SFINAE)
    struct can_not_apply{};

    template <template <class, class> class Detector, class Tleft, class Tright>
    struct not_appliable {
        static constexpr bool value = std::is_same<
            Detector<Tleft, Tright>,
            can_not_apply
        >::value;
    };

///////////////////// Detectors for different operators
    template <class S, class T> auto comp_eq_detector_msvc_helper(long) -> decltype(std::declval<S>() == std::declval<T>());
    template <class S, class T> can_not_apply comp_eq_detector_msvc_helper(int);
    template <class T1, class T2> using comp_eq_detector = decltype(comp_eq_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_ne_detector_msvc_helper(long) -> decltype(std::declval<S>() != std::declval<T>());
    template <class S, class T> can_not_apply comp_ne_detector_msvc_helper(int);
    template <class T1, class T2> using comp_ne_detector = decltype(comp_ne_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_lt_detector_msvc_helper(long) -> decltype(std::declval<S>() < std::declval<T>());
    template <class S, class T> can_not_apply comp_lt_detector_msvc_helper(int);
    template <class T1, class T2> using comp_lt_detector = decltype(comp_lt_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_le_detector_msvc_helper(long) -> decltype(std::declval<S>() <= std::declval<T>());
    template <class S, class T> can_not_apply comp_le_detector_msvc_helper(int);
    template <class T1, class T2> using comp_le_detector = decltype(comp_le_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_gt_detector_msvc_helper(long) -> decltype(std::declval<S>() > std::declval<T>());
    template <class S, class T> can_not_apply comp_gt_detector_msvc_helper(int);
    template <class T1, class T2> using comp_gt_detector = decltype(comp_gt_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_ge_detector_msvc_helper(long) -> decltype(std::declval<S>() >= std::declval<T>());
    template <class S, class T> can_not_apply comp_ge_detector_msvc_helper(int);
    template <class T1, class T2> using comp_ge_detector = decltype(comp_ge_detector_msvc_helper<T1,T2>(1L));


    template <class S> auto hash_detector_msvc_helper(long) -> decltype(std::hash<S>{}(std::declval<S>()));
    template <class S> can_not_apply hash_detector_msvc_helper(int);
    template <class T1, class T2> using hash_detector = decltype(hash_detector_msvc_helper<T1,T2>(1L));


    template <class S, class T> auto ostreamable_detector_msvc_helper(long) -> decltype(std::declval<S>() << std::declval<T>());
    template <class S, class T> can_not_apply ostreamable_detector_msvc_helper(int);
    template <class S, class T> using ostreamable_detector = decltype(ostreamable_detector_msvc_helper<S,T>(1L));

    template <class S, class T> auto istreamable_detector_msvc_helper(long) -> decltype(std::declval<S>() >> std::declval<T>());
    template <class S, class T> can_not_apply istreamable_detector_msvc_helper(int);
    template <class S, class T> using istreamable_detector = decltype(istreamable_detector_msvc_helper<S,T>(1L));

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_DETECTORS_HPP


