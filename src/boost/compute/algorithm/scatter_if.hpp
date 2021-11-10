//---------------------------------------------------------------------------//
// Copyright (c) 2015 Jakub Pola <jakub.pola@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_SCATTER_IF_HPP
#define BOOST_COMPUTE_ALGORITHM_SCATTER_IF_HPP

#include <boost/static_assert.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class MapIterator, class StencilIterator, class OutputIterator, class Predicate>
class scatter_if_kernel : meta_kernel
{
public:
    scatter_if_kernel() : meta_kernel("scatter_if")
    {}

    void set_range(InputIterator first,
                   InputIterator last,
                   MapIterator map,
                   StencilIterator stencil,
                   OutputIterator result,
                   Predicate predicate)
    {
        m_count = iterator_range_size(first, last);
        m_input_offset = first.get_index();
        m_output_offset = result.get_index();

        m_input_offset_arg = add_arg<uint_>("input_offset");
        m_output_offset_arg = add_arg<uint_>("output_offset");

        *this <<
        "const uint i = get_global_id(0);\n" <<
        "uint i1 = " << map[expr<uint_>("i")] <<
        " + output_offset;\n" <<
        "uint i2 = i + input_offset;\n" <<
        if_(predicate(stencil[expr<uint_>("i")])) << "\n" <<
            result[expr<uint_>("i1")] << "=" <<
            first[expr<uint_>("i2")] << ";\n";
    }

    event exec(command_queue &queue)
    {
        if(m_count == 0) {
            return event();
        }

        set_arg(m_input_offset_arg, uint_(m_input_offset));
        set_arg(m_output_offset_arg, uint_(m_output_offset));

        return exec_1d(queue, 0, m_count);
    }

private:
    size_t m_count;
    size_t m_input_offset;
    size_t m_input_offset_arg;
    size_t m_output_offset;
    size_t m_output_offset_arg;
};

} // end detail namespace

/// Copies the elements from the range [\p first, \p last) to the range
/// beginning at \p result using the output indices from the range beginning
/// at \p map if stencil is resolved to true. By default the predicate is
/// an identity
///
/// Space complexity: \Omega(1)
template<class InputIterator, class MapIterator, class StencilIterator, class OutputIterator,
         class Predicate>
inline void scatter_if(InputIterator first,
                       InputIterator last,
                       MapIterator map,
                       StencilIterator stencil,
                       OutputIterator result,
                       Predicate predicate,
                       command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<MapIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<StencilIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);

    detail::scatter_if_kernel<InputIterator, MapIterator, StencilIterator, OutputIterator, Predicate> kernel;

    kernel.set_range(first, last, map, stencil, result, predicate);
    kernel.exec(queue);
}

template<class InputIterator, class MapIterator, class StencilIterator, class OutputIterator>
inline void scatter_if(InputIterator first,
                       InputIterator last,
                       MapIterator map,
                       StencilIterator stencil,
                       OutputIterator result,
                       command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<MapIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<StencilIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);

    typedef typename std::iterator_traits<StencilIterator>::value_type T;

    scatter_if(first, last, map, stencil, result, identity<T>(), queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_SCATTER_IF_HPP
