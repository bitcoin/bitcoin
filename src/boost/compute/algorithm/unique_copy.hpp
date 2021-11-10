//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_UNIQUE_COPY_HPP
#define BOOST_COMPUTE_ALGORITHM_UNIQUE_COPY_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/lambda.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/algorithm/copy_if.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/algorithm/gather.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/functional/operator.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class OutputIterator, class BinaryPredicate>
inline OutputIterator serial_unique_copy(InputIterator first,
                                         InputIterator last,
                                         OutputIterator result,
                                         BinaryPredicate op,
                                         command_queue &queue)
{
    if(first == last){
        return result;
    }

    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    const context &context = queue.get_context();

    size_t count = detail::iterator_range_size(first, last);

    detail::meta_kernel k("serial_unique_copy");

    vector<uint_> unique_count_vector(1, context);

    size_t size_arg = k.add_arg<const uint_>("size");
    size_t unique_count_arg = k.add_arg<uint_ *>(memory_object::global_memory, "unique_count");

    k << k.decl<uint_>("index") << " = 0;\n"
      << k.decl<value_type>("current") << " = " << first[k.var<uint_>("0")] << ";\n"
      << result[k.var<uint_>("0")] << " = current;\n"
      << "for(uint i = 1; i < size; i++){\n"
      << "    " << k.decl<value_type>("next") << " = " << first[k.var<uint_>("i")] << ";\n"
      << "    if(!" << op(k.var<value_type>("current"), k.var<value_type>("next")) << "){\n"
      << "        " << result[k.var<uint_>("++index")] << " = next;\n"
      << "        " << "current = next;\n"
      << "    }\n"
      << "}\n"
      << "*unique_count = index + 1;\n";

    k.set_arg<const uint_>(size_arg, count);
    k.set_arg(unique_count_arg, unique_count_vector.get_buffer());

    k.exec_1d(queue, 0, 1, 1);

    uint_ unique_count;
    copy_n(unique_count_vector.begin(), 1, &unique_count, queue);

    return result + unique_count;
}

template<class InputIterator, class OutputIterator, class BinaryPredicate>
inline OutputIterator unique_copy(InputIterator first,
                                  InputIterator last,
                                  OutputIterator result,
                                  BinaryPredicate op,
                                  command_queue &queue)
{
    if(first == last){
        return result;
    }

    const context &context = queue.get_context();
    size_t count = detail::iterator_range_size(first, last);

    // flags marking unique elements
    vector<uint_> flags(count, context);

    // find each unique element and mark it with a one
    transform(
        first, last - 1, first + 1, flags.begin() + 1, not2(op), queue
    );

    // first element is always unique
    fill_n(flags.begin(), 1, 1, queue);

    // storage for desination indices
    vector<uint_> indices(count, context);

    // copy indices for each unique element
    vector<uint_>::iterator last_index = detail::copy_index_if(
        flags.begin(), flags.end(), indices.begin(), lambda::_1 == 1, queue
    );

    // copy unique values from input to output using the computed indices
    gather(indices.begin(), last_index, first, result, queue);

    // return an iterator to the end of the unique output range
    return result + std::distance(indices.begin(), last_index);
}

} // end detail namespace

/// Makes a copy of the range [first, last) and removes all consecutive
/// duplicate elements (determined by \p op) from the copy. If \p op is not
/// provided, the equality operator is used.
///
/// \param first first element in the input range
/// \param last last element in the input range
/// \param result first element in the result range
/// \param op binary operator used to check for uniqueness
/// \param queue command queue to perform the operation
///
/// \return \c OutputIterator to the end of the result range
///
/// Space complexity: \Omega(4n)
///
/// \see unique()
template<class InputIterator, class OutputIterator, class BinaryPredicate>
inline OutputIterator unique_copy(InputIterator first,
                                  InputIterator last,
                                  OutputIterator result,
                                  BinaryPredicate op,
                                  command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);

    size_t count = detail::iterator_range_size(first, last);
    if(count < 32){
        return detail::serial_unique_copy(first, last, result, op, queue);
    }
    else {
        return detail::unique_copy(first, last, result, op, queue);
    }
}

/// \overload
template<class InputIterator, class OutputIterator>
inline OutputIterator unique_copy(InputIterator first,
                                  InputIterator last,
                                  OutputIterator result,
                                  command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<OutputIterator>::value);

    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    return ::boost::compute::unique_copy(
        first, last, result, ::boost::compute::equal_to<value_type>(), queue
    );
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_UNIQUE_COPY_HPP
