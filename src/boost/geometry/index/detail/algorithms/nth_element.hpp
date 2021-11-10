// Boost.Geometry Index
//
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_NTH_ELEMENT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_NTH_ELEMENT_HPP

#include <algorithm>

namespace boost { namespace geometry { namespace index { namespace detail {

// See https://svn.boost.org/trac/boost/ticket/12861
//     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58800
//     https://gcc.gnu.org/develop.html#timeline
// 20120920 4.7.2 - no bug
// 20130322 4.8.0 - no bug
// 20130411 4.7.3 - no bug
// 20130531 4.8.1 - no bug
// 20131016 4.8.2 - bug
// 20140422 4.9.0 - fixed
// 20140522 4.8.3 - fixed
// 20140612 4.7.4 - fixed
// 20140716 4.9.1 - fixed
#if defined(__GLIBCXX__) && (__GLIBCXX__ == 20131016)

#warning "std::nth_element replaced with std::sort, libstdc++ bug workaround.";

template <typename RandomIt>
void nth_element(RandomIt first, RandomIt , RandomIt last)
{
    std::sort(first, last);
}

template <typename RandomIt, typename Compare>
void nth_element(RandomIt first, RandomIt , RandomIt last, Compare comp)
{
    std::sort(first, last, comp);
}

#else

template <typename RandomIt>
void nth_element(RandomIt first, RandomIt nth, RandomIt last)
{
    std::nth_element(first, nth, last);
}

template <typename RandomIt, typename Compare>
void nth_element(RandomIt first, RandomIt nth, RandomIt last, Compare comp)
{
    std::nth_element(first, nth, last, comp);
}

#endif

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ALGORITHMS_NTH_ELEMENT_HPP
