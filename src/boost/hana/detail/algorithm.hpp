/*!
@file
Defines several `constexpr` algorithms.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_ALGORITHM_HPP
#define BOOST_HANA_DETAIL_ALGORITHM_HPP

#include <boost/hana/functional/placeholder.hpp>

#include <boost/hana/config.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    // Do not call this swap, otherwise it can get picked up by ADL and conflict
    // with std::swap (see https://github.com/boostorg/hana/issues/297).
    template <typename T>
    constexpr void constexpr_swap(T& x, T& y) {
        auto tmp = x;
        x = y;
        y = std::move(tmp);
    }

    template <typename BidirIter>
    constexpr void reverse(BidirIter first, BidirIter last) {
        while (first != last) {
            if (first == --last)
                break;
            detail::constexpr_swap(*first, *last);
            ++first;
        }
    }

    template <typename BidirIter, typename BinaryPred>
    constexpr bool next_permutation(BidirIter first, BidirIter last,
                                    BinaryPred pred)
    {
        BidirIter i = last;
        if (first == last || first == --i)
            return false;
        while (true) {
            BidirIter ip1 = i;
            if (pred(*--i, *ip1)) {
                BidirIter j = last;
                while (!pred(*i, *--j))
                    ;
                detail::constexpr_swap(*i, *j);
                detail::reverse(ip1, last);
                return true;
            }
            if (i == first) {
                detail::reverse(first, last);
                return false;
            }
        }
    }

    template <typename BidirIter>
    constexpr bool next_permutation(BidirIter first, BidirIter last)
    { return detail::next_permutation(first, last, hana::_ < hana::_); }


    template <typename InputIter1, typename InputIter2, typename BinaryPred>
    constexpr bool lexicographical_compare(InputIter1 first1, InputIter1 last1,
                                           InputIter2 first2, InputIter2 last2,
                                           BinaryPred pred)
    {
        for (; first2 != last2; ++first1, ++first2) {
            if (first1 == last1 || pred(*first1, *first2))
                return true;
            else if (pred(*first2, *first1))
                return false;
        }
        return false;
    }

    template <typename InputIter1, typename InputIter2>
    constexpr bool lexicographical_compare(InputIter1 first1, InputIter1 last1,
                                           InputIter2 first2, InputIter2 last2)
    { return detail::lexicographical_compare(first1, last1, first2, last2, hana::_ < hana::_); }


    template <typename InputIter1, typename InputIter2, typename BinaryPred>
    constexpr bool equal(InputIter1 first1, InputIter1 last1,
                         InputIter2 first2, InputIter2 last2,
                         BinaryPred pred)
    {
        for (; first1 != last1 && first2 != last2; ++first1, ++first2)
            if (!pred(*first1, *first2))
                return false;
        return first1 == last1 && first2 == last2;
    }

    template <typename InputIter1, typename InputIter2>
    constexpr bool equal(InputIter1 first1, InputIter1 last1,
                         InputIter2 first2, InputIter2 last2)
    { return detail::equal(first1, last1, first2, last2, hana::_ == hana::_); }


    template <typename BidirIter, typename BinaryPred>
    constexpr void sort(BidirIter first, BidirIter last, BinaryPred pred) {
        if (first == last) return;

        BidirIter i = first;
        for (++i; i != last; ++i) {
            BidirIter j = i;
            auto t = *j;
            for (BidirIter k = i; k != first && pred(t,  *--k); --j)
                *j = *k;
            *j = t;
        }
    }

    template <typename BidirIter>
    constexpr void sort(BidirIter first, BidirIter last)
    { detail::sort(first, last, hana::_ < hana::_); }


    template <typename InputIter, typename T>
    constexpr InputIter find(InputIter first, InputIter last, T const& value) {
        for (; first != last; ++first)
            if (*first == value)
                return first;
        return last;
    }

    template <typename InputIter, typename UnaryPred>
    constexpr InputIter find_if(InputIter first, InputIter last, UnaryPred pred) {
        for (; first != last; ++first)
            if (pred(*first))
                return first;
        return last;
    }

    template <typename ForwardIter, typename T>
    constexpr void iota(ForwardIter first, ForwardIter last, T value) {
        while (first != last) {
            *first++ = value;
            ++value;
        }
    }

    template <typename InputIt, typename T>
    constexpr std::size_t
    count(InputIt first, InputIt last, T const& value) {
        std::size_t n = 0;
        for (; first != last; ++first)
            if (*first == value)
                ++n;
        return n;
    }

    template <typename InputIt, typename T, typename F>
    constexpr T accumulate(InputIt first, InputIt last, T init, F f) {
        for (; first != last; ++first)
            init = f(init, *first);
        return init;
    }

    template <typename InputIt, typename T>
    constexpr T accumulate(InputIt first, InputIt last, T init) {
        return detail::accumulate(first, last, init, hana::_ + hana::_);
    }

    template <typename ForwardIt>
    constexpr ForwardIt min_element(ForwardIt first, ForwardIt last) {
        if (first == last)
            return last;

        ForwardIt smallest = first;
        ++first;
        for (; first != last; ++first)
            if (*first < *smallest)
                smallest = first;
        return smallest;
    }
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_ALGORITHM_HPP
