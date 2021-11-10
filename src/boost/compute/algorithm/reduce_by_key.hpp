//---------------------------------------------------------------------------//
// Copyright (c) 2015 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_REDUCE_BY_KEY_HPP
#define BOOST_COMPUTE_ALGORITHM_REDUCE_BY_KEY_HPP

#include <iterator>
#include <utility>

#include <boost/static_assert.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/algorithm/detail/reduce_by_key.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// The \c reduce_by_key() algorithm performs reduction for each contiguous
/// subsequence of values determinate by equivalent keys.
///
/// Returns a pair of iterators at the end of the ranges [\p keys_result, keys_result_last)
/// and [\p values_result, \p values_result_last).
///
/// If no function is specified, \c plus will be used.
/// If no predicate is specified, \c equal_to will be used.
///
/// \param keys_first the first key
/// \param keys_last the last key
/// \param values_first the first input value
/// \param keys_result iterator pointing to the key output
/// \param values_result iterator pointing to the reduced value output
/// \param function binary reduction function
/// \param predicate binary predicate which returns true only if two keys are equal
/// \param queue command queue to perform the operation
///
/// The \c reduce_by_key() algorithm assumes that the binary reduction function
/// is associative. When used with non-associative functions the result may
/// be non-deterministic and vary in precision. Notably this affects the
/// \c plus<float>() function as floating-point addition is not associative
/// and may produce slightly different results than a serial algorithm.
///
/// For example, to calculate the sum of the values for each key:
///
/// \snippet test/test_reduce_by_key.cpp reduce_by_key_int
///
/// Space complexity on GPUs: \Omega(2n)<br>
/// Space complexity on CPUs: \Omega(1)
///
/// \see reduce()
template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator,
         class BinaryFunction, class BinaryPredicate>
inline std::pair<OutputKeyIterator, OutputValueIterator>
reduce_by_key(InputKeyIterator keys_first,
              InputKeyIterator keys_last,
              InputValueIterator values_first,
              OutputKeyIterator keys_result,
              OutputValueIterator values_result,
              BinaryFunction function,
              BinaryPredicate predicate,
              command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputKeyIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputValueIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputKeyIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputValueIterator>::value);

    return detail::dispatch_reduce_by_key(keys_first, keys_last, values_first,
                                          keys_result, values_result,
                                          function, predicate,
                                          queue);
}

/// \overload
template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator,
         class BinaryFunction>
inline std::pair<OutputKeyIterator, OutputValueIterator>
reduce_by_key(InputKeyIterator keys_first,
              InputKeyIterator keys_last,
              InputValueIterator values_first,
              OutputKeyIterator keys_result,
              OutputValueIterator values_result,
              BinaryFunction function,
              command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputKeyIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputValueIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputKeyIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputValueIterator>::value);
    typedef typename std::iterator_traits<InputKeyIterator>::value_type key_type;

    return reduce_by_key(keys_first, keys_last, values_first,
                         keys_result, values_result,
                         function, equal_to<key_type>(),
                         queue);
}

/// \overload
template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator>
inline std::pair<OutputKeyIterator, OutputValueIterator>
reduce_by_key(InputKeyIterator keys_first,
              InputKeyIterator keys_last,
              InputValueIterator values_first,
              OutputKeyIterator keys_result,
              OutputValueIterator values_result,
              command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputKeyIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputValueIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputKeyIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputValueIterator>::value);
    typedef typename std::iterator_traits<InputKeyIterator>::value_type key_type;
    typedef typename std::iterator_traits<InputValueIterator>::value_type value_type;

    return reduce_by_key(keys_first, keys_last, values_first,
                         keys_result, values_result,
                         plus<value_type>(), equal_to<key_type>(),
                         queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_REDUCE_BY_KEY_HPP
