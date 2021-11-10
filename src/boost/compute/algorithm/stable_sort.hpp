//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_STABLE_SORT_HPP
#define BOOST_COMPUTE_ALGORITHM_STABLE_SORT_HPP

#include <iterator>

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/detail/merge_sort_on_cpu.hpp>
#include <boost/compute/algorithm/detail/merge_sort_on_gpu.hpp>
#include <boost/compute/algorithm/detail/radix_sort.hpp>
#include <boost/compute/algorithm/detail/insertion_sort.hpp>
#include <boost/compute/algorithm/reverse.hpp>
#include <boost/compute/functional/operator.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Iterator, class Compare>
inline void dispatch_gpu_stable_sort(Iterator first,
                                     Iterator last,
                                     Compare compare,
                                     command_queue &queue)
{
    size_t count = detail::iterator_range_size(first, last);

    if(count < 32){
        detail::serial_insertion_sort(
            first, last, compare, queue
        );
    } else {
        detail::merge_sort_on_gpu(
            first, last, compare, true /* stable */, queue
        );
    }
}

template<class T>
inline typename boost::enable_if_c<is_radix_sortable<T>::value>::type
dispatch_gpu_stable_sort(buffer_iterator<T> first,
                         buffer_iterator<T> last,
                         less<T>,
                         command_queue &queue)
{
    ::boost::compute::detail::radix_sort(first, last, queue);
}

template<class T>
inline typename boost::enable_if_c<is_radix_sortable<T>::value>::type
dispatch_gpu_stable_sort(buffer_iterator<T> first,
                         buffer_iterator<T> last,
                         greater<T>,
                         command_queue &queue)
{
    // radix sorts in descending order
    ::boost::compute::detail::radix_sort(first, last, false, queue);
}

} // end detail namespace

/// Sorts the values in the range [\p first, \p last) according to
/// \p compare. The relative order of identical values is preserved.
///
/// Space complexity: \Omega(n)
///
/// \see sort(), is_sorted()
template<class Iterator, class Compare>
inline void stable_sort(Iterator first,
                        Iterator last,
                        Compare compare,
                        command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator>::value);

    if(queue.get_device().type() & device::gpu) {
        ::boost::compute::detail::dispatch_gpu_stable_sort(
            first, last, compare, queue
        );
        return;
    }
    ::boost::compute::detail::merge_sort_on_cpu(first, last, compare, queue);
}

/// \overload
template<class Iterator>
inline void stable_sort(Iterator first,
                        Iterator last,
                        command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator>::value);
    typedef typename std::iterator_traits<Iterator>::value_type value_type;

    ::boost::compute::less<value_type> less;

    ::boost::compute::stable_sort(first, last, less, queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_STABLE_SORT_HPP
