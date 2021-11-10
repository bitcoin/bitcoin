//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_ACCUMULATE_HPP
#define BOOST_COMPUTE_ALGORITHM_ACCUMULATE_HPP

#include <boost/static_assert.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/reduce.hpp>
#include <boost/compute/algorithm/detail/serial_accumulate.hpp>
#include <boost/compute/container/array.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>

namespace boost {
namespace compute {
namespace detail {

// Space complexity O(1)
template<class InputIterator, class T, class BinaryFunction>
inline T generic_accumulate(InputIterator first,
                            InputIterator last,
                            T init,
                            BinaryFunction function,
                            command_queue &queue)
{
    const context &context = queue.get_context();

    size_t size = iterator_range_size(first, last);
    if(size == 0){
        return init;
    }

    // accumulate on device
    array<T, 1> device_result(context);
    detail::serial_accumulate(
        first, last, device_result.begin(), init, function, queue
    );

    // copy result to host
    T result;
    ::boost::compute::copy_n(device_result.begin(), 1, &result, queue);
    return result;
}

// returns true if we can use reduce() instead of accumulate() when
// accumulate() this is true when the function is commutative (such as
// addition of integers) and the initial value is the identity value
// for the operation (zero for addition, one for multiplication).
template<class T, class F>
inline bool can_accumulate_with_reduce(T init, F function)
{
    (void) init;
    (void) function;

    return false;
}

/// \internal_
#define BOOST_COMPUTE_DETAIL_DECLARE_CAN_ACCUMULATE_WITH_REDUCE(r, data, type) \
    inline bool can_accumulate_with_reduce(type init, plus<type>) \
    { \
        return init == type(0); \
    } \
    inline bool can_accumulate_with_reduce(type init, multiplies<type>) \
    { \
        return init == type(1); \
    }

BOOST_PP_SEQ_FOR_EACH(
    BOOST_COMPUTE_DETAIL_DECLARE_CAN_ACCUMULATE_WITH_REDUCE,
    _,
    (char_)(uchar_)(short_)(ushort_)(int_)(uint_)(long_)(ulong_)
)

template<class T>
inline bool can_accumulate_with_reduce(T init, min<T>)
{
    return init == (std::numeric_limits<T>::max)();
}

template<class T>
inline bool can_accumulate_with_reduce(T init, max<T>)
{
    return init == (std::numeric_limits<T>::min)();
}

#undef BOOST_COMPUTE_DETAIL_DECLARE_CAN_ACCUMULATE_WITH_REDUCE

template<class InputIterator, class T, class BinaryFunction>
inline T dispatch_accumulate(InputIterator first,
                             InputIterator last,
                             T init,
                             BinaryFunction function,
                             command_queue &queue)
{
    size_t size = iterator_range_size(first, last);
    if(size == 0){
        return init;
    }

    if(can_accumulate_with_reduce(init, function)){
        T result;
        reduce(first, last, &result, function, queue);
        return result;
    }
    else {
        return generic_accumulate(first, last, init, function, queue);
    }
}

} // end detail namespace

/// Returns the result of applying \p function to the elements in the
/// range [\p first, \p last) and \p init.
///
/// If no function is specified, \c plus will be used.
///
/// \param first first element in the input range
/// \param last last element in the input range
/// \param init initial value
/// \param function binary reduction function
/// \param queue command queue to perform the operation
///
/// \return the accumulated result value
///
/// In specific situations the call to \c accumulate() can be automatically
/// optimized to a call to the more efficient \c reduce() algorithm. This
/// occurs when the binary reduction function is recognized as associative
/// (such as the \c plus<int> function).
///
/// Note that because floating-point addition is not associative, calling
/// \c accumulate() with \c plus<float> results in a less efficient serial
/// reduction algorithm being executed. If a slight loss in precision is
/// acceptable, the more efficient parallel \c reduce() algorithm should be
/// used instead.
///
/// For example:
/// \code
/// // with vec = boost::compute::vector<int>
/// accumulate(vec.begin(), vec.end(), 0, plus<int>());   // fast
/// reduce(vec.begin(), vec.end(), &result, plus<int>()); // fast
///
/// // with vec = boost::compute::vector<float>
/// accumulate(vec.begin(), vec.end(), 0, plus<float>());   // slow
/// reduce(vec.begin(), vec.end(), &result, plus<float>()); // fast
/// \endcode
///
/// Space complexity: \Omega(1)<br>
/// Space complexity when optimized to \c reduce(): \Omega(n)
///
/// \see reduce()
template<class InputIterator, class T, class BinaryFunction>
inline T accumulate(InputIterator first,
                    InputIterator last,
                    T init,
                    BinaryFunction function,
                    command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);

    return detail::dispatch_accumulate(first, last, init, function, queue);
}

/// \overload
template<class InputIterator, class T>
inline T accumulate(InputIterator first,
                    InputIterator last,
                    T init,
                    command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    typedef typename std::iterator_traits<InputIterator>::value_type IT;

    return detail::dispatch_accumulate(first, last, init, plus<IT>(), queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_ACCUMULATE_HPP
