//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_NTH_ELEMENT_HPP
#define BOOST_COMPUTE_ALGORITHM_NTH_ELEMENT_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/fill_n.hpp>
#include <boost/compute/algorithm/find.hpp>
#include <boost/compute/algorithm/partition.hpp>
#include <boost/compute/algorithm/sort.hpp>
#include <boost/compute/functional/bind.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Rearranges the elements in the range [\p first, \p last) such that
/// the \p nth element would be in that position in a sorted sequence.
///
/// Space complexity: \Omega(3n)
template<class Iterator, class Compare>
inline void nth_element(Iterator first,
                        Iterator nth,
                        Iterator last,
                        Compare compare,
                        command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator>::value);
    if(nth == last) return;

    typedef typename std::iterator_traits<Iterator>::value_type value_type;

    while(1)
    {
        value_type value = nth.read(queue);

        using boost::compute::placeholders::_1;
        Iterator new_nth = partition(
            first, last, ::boost::compute::bind(compare, _1, value), queue
        );

        Iterator old_nth = find(new_nth, last, value, queue);

        value_type new_value = new_nth.read(queue);

        fill_n(new_nth, 1, value, queue);
        fill_n(old_nth, 1, new_value, queue);

        new_value = nth.read(queue);

        if(value == new_value) break;

        if(std::distance(first, nth) < std::distance(first, new_nth))
        {
            last = new_nth;
        }
        else
        {
            first = new_nth;
        }
    }
}

/// \overload
template<class Iterator>
inline void nth_element(Iterator first,
                        Iterator nth,
                        Iterator last,
                        command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator>::value);
    if(nth == last) return;

    typedef typename std::iterator_traits<Iterator>::value_type value_type;

    less<value_type> less_than;

    return nth_element(first, nth, last, less_than, queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_NTH_ELEMENT_HPP
