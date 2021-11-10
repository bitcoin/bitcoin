//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_MERGE_HPP
#define BOOST_COMPUTE_ALGORITHM_MERGE_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/detail/merge_with_merge_path.hpp>
#include <boost/compute/algorithm/detail/serial_merge.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/parameter_cache.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Merges the sorted values in the range [\p first1, \p last1) with the sorted
/// values in the range [\p first2, last2) and stores the result in the range
/// beginning at \p result. Values are compared using the \p comp function. If
/// no comparision function is given, \c less is used.
///
/// \param first1 first element in the first range to merge
/// \param last1 last element in the first range to merge
/// \param first2 first element in the second range to merge
/// \param last2 last element in the second range to merge
/// \param result first element in the result range
/// \param comp comparison function (by default \c less)
/// \param queue command queue to perform the operation
///
/// \return \c OutputIterator to the end of the result range
///
/// Space complexity: \Omega(distance(\p first1, \p last1) + distance(\p first2, \p last2))
///
/// \see inplace_merge()
template<class InputIterator1,
         class InputIterator2,
         class OutputIterator,
         class Compare>
inline OutputIterator merge(InputIterator1 first1,
                            InputIterator1 last1,
                            InputIterator2 first2,
                            InputIterator2 last2,
                            OutputIterator result,
                            Compare comp,
                            command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator2>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);
    typedef typename std::iterator_traits<InputIterator1>::value_type input1_type;
    typedef typename std::iterator_traits<InputIterator2>::value_type input2_type;
    typedef typename std::iterator_traits<OutputIterator>::value_type output_type;

    const device &device = queue.get_device();

    std::string cache_key =
        std::string("__boost_merge_") + type_name<input1_type>() + "_"
        + type_name<input2_type>() + "_" + type_name<output_type>();
    boost::shared_ptr<detail::parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    // default serial merge threshold depends on device type
    size_t default_serial_merge_threshold = 32768;
    if(device.type() & device::gpu) {
        default_serial_merge_threshold = 2048;
    }

    // loading serial merge threshold parameter
    const size_t serial_merge_threshold =
        parameters->get(cache_key, "serial_merge_threshold",
                        static_cast<uint_>(default_serial_merge_threshold));

    // choosing merge algorithm
    const size_t total_count =
        detail::iterator_range_size(first1, last1)
        + detail::iterator_range_size(first2, last2);
    // for small inputs serial merge turns out to outperform
    // merge with merge path algorithm
    if(total_count <= serial_merge_threshold){
       return detail::serial_merge(first1, last1, first2, last2, result, comp, queue);
    }
    return detail::merge_with_merge_path(first1, last1, first2, last2, result, comp, queue);
}

/// \overload
template<class InputIterator1, class InputIterator2, class OutputIterator>
inline OutputIterator merge(InputIterator1 first1,
                            InputIterator1 last1,
                            InputIterator2 first2,
                            InputIterator2 last2,
                            OutputIterator result,
                            command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator2>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);
    typedef typename std::iterator_traits<InputIterator1>::value_type value_type;
    less<value_type> less_than;
    return merge(first1, last1, first2, last2, result, less_than, queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_MERGE_HPP
