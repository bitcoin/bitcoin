//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_SWAP_RANGES_HPP
#define BOOST_COMPUTE_ALGORITHM_SWAP_RANGES_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Swaps the elements in the range [\p first1, \p last1) with the
/// elements in the range beginning at \p first2.
///
/// Space complexity: \Omega(distance(\p first1, \p last1))
template<class Iterator1, class Iterator2>
inline Iterator2 swap_ranges(Iterator1 first1,
                             Iterator1 last1,
                             Iterator2 first2,
                             command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator2>::value);

    typedef typename std::iterator_traits<Iterator1>::value_type value_type;

    Iterator2 last2 = first2 + std::distance(first1, last1);

    ::boost::compute::vector<value_type> tmp(first1, last1, queue);
    ::boost::compute::copy(first2, last2, first1, queue);
    ::boost::compute::copy(tmp.begin(), tmp.end(), first2, queue);

    return last2;
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_SWAP_RANGES_HPP
