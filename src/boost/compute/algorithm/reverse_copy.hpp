//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_REVERSE_COPY_HPP
#define BOOST_COMPUTE_ALGORITHM_REVERSE_COPY_HPP

#include <iterator>

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/reverse.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Iterator, class OutputIterator>
struct reverse_copy_kernel : public meta_kernel
{
    reverse_copy_kernel(Iterator first, Iterator last, OutputIterator result)
        : meta_kernel("reverse_copy")
    {
        // store size of the range
        m_size = detail::iterator_range_size(first, last);
        add_set_arg<const cl_uint>("size", static_cast<const cl_uint>(m_size));

        *this <<
            decl<cl_uint>("i") << " = get_global_id(0);\n" <<
            decl<cl_uint>("j") << " = size - get_global_id(0) - 1;\n" <<
            result[var<cl_uint>("j")] << "=" << first[var<cl_uint>("i")] << ";\n";
    }

    void exec(command_queue &queue)
    {
        exec_1d(queue, 0, m_size);
    }

    size_t m_size;
};

} // end detail namespace

/// Copies the elements in the range [\p first, \p last) in reversed
/// order to the range beginning at \p result.
///
/// Space complexity: \Omega(1)
///
/// \see reverse()
template<class InputIterator, class OutputIterator>
inline OutputIterator
reverse_copy(InputIterator first,
             InputIterator last,
             OutputIterator result,
             command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);

    typedef typename std::iterator_traits<OutputIterator>::difference_type difference_type;

    difference_type count = std::distance(first, last);

    detail::reverse_copy_kernel<InputIterator, OutputIterator>
        kernel(first, last, result);

    // run kernel
    kernel.exec(queue);

    // return iterator to the end of result
    return result + count;
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_REVERSE_COPY_HPP
