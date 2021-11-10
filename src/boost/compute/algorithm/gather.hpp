//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_GATHER_HPP
#define BOOST_COMPUTE_ALGORITHM_GATHER_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class MapIterator, class OutputIterator>
class gather_kernel : public meta_kernel
{
public:
    gather_kernel() : meta_kernel("gather")
    {}

    void set_range(MapIterator first,
                   MapIterator last,
                   InputIterator input,
                   OutputIterator result)
    {
        m_count = iterator_range_size(first, last);

        *this <<
            "const uint i = get_global_id(0);\n" <<
            result[expr<uint_>("i")] << "=" << 
                input[first[expr<uint_>("i")]] << ";\n";
    }

    event exec(command_queue &queue)
    {
        if(m_count == 0) {
            return event();
        }

        return exec_1d(queue, 0, m_count);
    }

private:
    size_t m_count;
};

} // end detail namespace

/// Copies the elements using the indices from the range [\p first, \p last)
/// to the range beginning at \p result using the input values from the range
/// beginning at \p input.
///
/// Space complexity: \Omega(1)
///
/// \see scatter()
template<class InputIterator, class MapIterator, class OutputIterator>
inline void gather(MapIterator first,
                   MapIterator last,
                   InputIterator input,
                   OutputIterator result,
                   command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<MapIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);

    detail::gather_kernel<InputIterator, MapIterator, OutputIterator> kernel;
    
    kernel.set_range(first, last, input, result);
    kernel.exec(queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_GATHER_HPP
