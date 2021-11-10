//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_COUNT_IF_HPP
#define BOOST_COMPUTE_ALGORITHM_COUNT_IF_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/device.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/detail/count_if_with_ballot.hpp>
#include <boost/compute/algorithm/detail/count_if_with_reduce.hpp>
#include <boost/compute/algorithm/detail/count_if_with_threads.hpp>
#include <boost/compute/algorithm/detail/serial_count_if.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Returns the number of elements in the range [\p first, \p last)
/// for which \p predicate returns \c true.
///
/// Space complexity on CPUs: \Omega(1)<br>
/// Space complexity on GPUs: \Omega(n)
template<class InputIterator, class Predicate>
inline size_t count_if(InputIterator first,
                       InputIterator last,
                       Predicate predicate,
                       command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    const device &device = queue.get_device();

    size_t input_size = detail::iterator_range_size(first, last);
    if(input_size == 0){
        return 0;
    }

    if(device.type() & device::cpu){
        if(input_size < 1024){
            return detail::serial_count_if(first, last, predicate, queue);
        }
        else {
            return detail::count_if_with_threads(first, last, predicate, queue);
        }
    }
    else {
        if(input_size < 32){
            return detail::serial_count_if(first, last, predicate, queue);
        }
        else {
            return detail::count_if_with_reduce(first, last, predicate, queue);
        }
    }
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_COUNT_IF_HPP
