//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_TRANSFORM_IF_HPP
#define BOOST_COMPUTE_ALGORITHM_TRANSFORM_IF_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/cl.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/count.hpp>
#include <boost/compute/algorithm/count_if.hpp>
#include <boost/compute/algorithm/exclusive_scan.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/iterator/discard_iterator.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

// Space complexity: O(2n)
template<class InputIterator, class OutputIterator, class UnaryFunction, class Predicate>
inline OutputIterator transform_if_impl(InputIterator first,
                                        InputIterator last,
                                        OutputIterator result,
                                        UnaryFunction function,
                                        Predicate predicate,
                                        bool copyIndex,
                                        command_queue &queue)
{
    typedef typename std::iterator_traits<OutputIterator>::difference_type difference_type;

    size_t count = detail::iterator_range_size(first, last);
    if(count == 0){
        return result;
    }

    const context &context = queue.get_context();

    // storage for destination indices
    ::boost::compute::vector<cl_uint> indices(count, context);

    // write counts
    ::boost::compute::detail::meta_kernel k1("transform_if_write_counts");
    k1 << indices.begin()[k1.get_global_id(0)] << " = "
           << predicate(first[k1.get_global_id(0)]) << " ? 1 : 0;\n";
    k1.exec_1d(queue, 0, count);

    // scan indices
    size_t copied_element_count = (indices.cend() - 1).read(queue);
    ::boost::compute::exclusive_scan(
        indices.begin(), indices.end(), indices.begin(), queue
    );
    copied_element_count += (indices.cend() - 1).read(queue); // last scan element plus last mask element

    // copy values
    ::boost::compute::detail::meta_kernel k2("transform_if_do_copy");
    k2 << "if(" << predicate(first[k2.get_global_id(0)]) << ")" <<
          "    " << result[indices.begin()[k2.get_global_id(0)]] << "=";

    if(copyIndex){
        k2 << k2.get_global_id(0) << ";\n";
    }
    else {
        k2 << function(first[k2.get_global_id(0)]) << ";\n";
    }

    k2.exec_1d(queue, 0, count);

    return result + static_cast<difference_type>(copied_element_count);
}

template<class InputIterator, class UnaryFunction, class Predicate>
inline discard_iterator transform_if_impl(InputIterator first,
                                          InputIterator last,
                                          discard_iterator result,
                                          UnaryFunction function,
                                          Predicate predicate,
                                          bool copyIndex,
                                          command_queue &queue)
{
    (void) function;
    (void) copyIndex;

    return result + count_if(first, last, predicate, queue);
}

} // end detail namespace

/// Copies each element in the range [\p first, \p last) for which
/// \p predicate returns \c true to the range beginning at \p result.
///
/// Space complexity: O(2n)
template<class InputIterator, class OutputIterator, class UnaryFunction, class Predicate>
inline OutputIterator transform_if(InputIterator first,
                                   InputIterator last,
                                   OutputIterator result,
                                   UnaryFunction function,
                                   Predicate predicate,
                                   command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);
    return detail::transform_if_impl(
        first, last, result, function, predicate, false, queue
    );
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_TRANSFORM_IF_HPP
