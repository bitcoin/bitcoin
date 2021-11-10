//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_ADJACENT_FIND_HPP
#define BOOST_COMPUTE_ALGORITHM_ADJACENT_FIND_HPP

#include <iterator>

#include <boost/static_assert.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/lambda.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/container/detail/scalar.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/functional/operator.hpp>
#include <boost/compute/type_traits/vector_size.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class Compare>
inline InputIterator
serial_adjacent_find(InputIterator first,
                     InputIterator last,
                     Compare compare,
                     command_queue &queue)
{
    if(first == last){
        return last;
    }

    const context &context = queue.get_context();

    detail::scalar<uint_> output(context);

    detail::meta_kernel k("serial_adjacent_find");

    size_t size_arg = k.add_arg<const uint_>("size");
    size_t output_arg = k.add_arg<uint_ *>(memory_object::global_memory, "output");

    k << k.decl<uint_>("result") << " = size;\n"
      << "for(uint i = 0; i < size - 1; i++){\n"
      << "    if(" << compare(first[k.expr<uint_>("i")],
                              first[k.expr<uint_>("i+1")]) << "){\n"
      << "        result = i;\n"
      << "        break;\n"
      << "    }\n"
      << "}\n"
      << "*output = result;\n";

    k.set_arg<const uint_>(
        size_arg, static_cast<uint_>(detail::iterator_range_size(first, last))
    );
    k.set_arg(output_arg, output.get_buffer());

    k.exec_1d(queue, 0, 1, 1);

    return first + output.read(queue);
}

template<class InputIterator, class Compare>
inline InputIterator
adjacent_find_with_atomics(InputIterator first,
                           InputIterator last,
                           Compare compare,
                           command_queue &queue)
{
    if(first == last){
        return last;
    }

    const context &context = queue.get_context();
    size_t count = detail::iterator_range_size(first, last);

    // initialize output to the last index
    detail::scalar<uint_> output(context);
    output.write(static_cast<uint_>(count), queue);

    detail::meta_kernel k("adjacent_find_with_atomics");

    size_t output_arg = k.add_arg<uint_ *>(memory_object::global_memory, "output");

    k << "const uint i = get_global_id(0);\n"
      << "if(" << compare(first[k.expr<uint_>("i")],
                          first[k.expr<uint_>("i+1")]) << "){\n"
      << "    atomic_min(output, i);\n"
      << "}\n";

    k.set_arg(output_arg, output.get_buffer());

    k.exec_1d(queue, 0, count - 1, 1);

    return first + output.read(queue);
}

} // end detail namespace

/// Searches the range [\p first, \p last) for two identical adjacent
/// elements and returns an iterator pointing to the first.
///
/// \param first first element in the range to search
/// \param last last element in the range to search
/// \param compare binary comparison function
/// \param queue command queue to perform the operation
///
/// \return \c InputIteratorm to the first element which compares equal
///         to the following element. If none are equal, returns \c last.
///
/// Space complexity: \Omega(1)
///
/// \see find(), adjacent_difference()
template<class InputIterator, class Compare>
inline InputIterator
adjacent_find(InputIterator first,
              InputIterator last,
              Compare compare,
              command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    size_t count = detail::iterator_range_size(first, last);
    if(count < 32){
        return detail::serial_adjacent_find(first, last, compare, queue);
    }
    else {
        return detail::adjacent_find_with_atomics(first, last, compare, queue);
    }
}

/// \overload
template<class InputIterator>
inline InputIterator
adjacent_find(InputIterator first,
              InputIterator last,
              command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    using ::boost::compute::lambda::_1;
    using ::boost::compute::lambda::_2;
    using ::boost::compute::lambda::all;

    if(vector_size<value_type>::value == 1){
        return ::boost::compute::adjacent_find(
            first, last, _1 == _2, queue
        );
    }
    else {
        return ::boost::compute::adjacent_find(
            first, last, all(_1 == _2), queue
        );
    }
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_ADJACENT_FIND_HPP
