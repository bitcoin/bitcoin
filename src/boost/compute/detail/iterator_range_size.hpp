//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_ITERATOR_RANGE_SIZE_H
#define BOOST_COMPUTE_DETAIL_ITERATOR_RANGE_SIZE_H

#include <cstddef>
#include <algorithm>
#include <iterator>

namespace boost {
namespace compute {
namespace detail {

// This is a convenience function which returns the size of a range
// bounded by two iterators. This function has two differences from
// the std::distance() function: 1) the return type (size_t) is
// unsigned, and 2) the return value is always positive.
template<class Iterator>
inline size_t iterator_range_size(Iterator first, Iterator last)
{
    typedef typename
        std::iterator_traits<Iterator>::difference_type
        difference_type;

    difference_type difference = std::distance(first, last);

    return static_cast<size_t>(
        (std::max)(difference, static_cast<difference_type>(0))
    );
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_ITERATOR_RANGE_SIZE_H
