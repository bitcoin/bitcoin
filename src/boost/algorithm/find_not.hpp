/*
   Copyright (c) T. Zachary Laine 2018.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALGORITHM_FIND_NOT_HPP
#define BOOST_ALGORITHM_FIND_NOT_HPP

#include <utility>

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm {

template<typename InputIter, typename Sentinel, typename T>        
BOOST_CXX14_CONSTEXPR
InputIter find_not(InputIter first, Sentinel last, const T & x)
{
    for (; first != last; ++first) {
        if (*first != x)
            break;
    }
    return first;
}

template<typename Range, typename T>
BOOST_CXX14_CONSTEXPR
typename boost::range_iterator<Range>::type find_not(Range & r, const T & x)
{
    return ::boost::algorithm::find_not(boost::begin(r), boost::end(r), x);
}

}} // namespace boost and algorithm

#endif // BOOST_ALGORITHM_FIND_NOT_HPP
