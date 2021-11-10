//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_NEXT_PERMUTATION_HPP
#define BOOST_COMPUTE_ALGORITHM_NEXT_PERMUTATION_HPP

#include <iterator>

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/detail/scalar.hpp>
#include <boost/compute/algorithm/reverse.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

///
/// \brief Helper function for next_permutation
///
/// To find rightmost element which is smaller
/// than its next element
///
template<class InputIterator>
inline InputIterator next_permutation_helper(InputIterator first,
                                             InputIterator last,
                                             command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    size_t count = detail::iterator_range_size(first, last);
    if(count == 0 || count == 1){
        return last;
    }
    count = count - 1;
    const context &context = queue.get_context();

    detail::meta_kernel k("next_permutation");
    size_t index_arg = k.add_arg<int *>(memory_object::global_memory, "index");
    atomic_max<int_> atomic_max_int;

    k << k.decl<const int_>("i") << " = get_global_id(0);\n"
      << k.decl<const value_type>("cur_value") << "="
      <<     first[k.var<const int_>("i")] << ";\n"
      << k.decl<const value_type>("next_value") << "="
      <<     first[k.expr<const int_>("i+1")] << ";\n"
      << "if(cur_value < next_value){\n"
      << "    " << atomic_max_int(k.var<int_ *>("index"), k.var<int_>("i")) << ";\n"
      << "}\n";

    kernel kernel = k.compile(context);

    scalar<int_> index(context);
    kernel.set_arg(index_arg, index.get_buffer());

    index.write(static_cast<int_>(-1), queue);

    queue.enqueue_1d_range_kernel(kernel, 0, count, 0);

    int result = static_cast<int>(index.read(queue));
    if(result == -1) return last;
    else return first + result;
}

///
/// \brief Helper function for next_permutation
///
/// To find the smallest element to the right of the element found above
/// that is greater than it
///
template<class InputIterator, class ValueType>
inline InputIterator np_ceiling(InputIterator first,
                                InputIterator last,
                                ValueType value,
                                command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    size_t count = detail::iterator_range_size(first, last);
    if(count == 0){
        return last;
    }
    const context &context = queue.get_context();

    detail::meta_kernel k("np_ceiling");
    size_t index_arg = k.add_arg<int *>(memory_object::global_memory, "index");
    size_t value_arg = k.add_arg<value_type>(memory_object::private_memory, "value");
    atomic_max<int_> atomic_max_int;

    k << k.decl<const int_>("i") << " = get_global_id(0);\n"
      << k.decl<const value_type>("cur_value") << "="
      <<     first[k.var<const int_>("i")] << ";\n"
      << "if(cur_value <= " << first[k.expr<int_>("*index")]
      << "      && cur_value > value){\n"
      << "    " << atomic_max_int(k.var<int_ *>("index"), k.var<int_>("i")) << ";\n"
      << "}\n";

    kernel kernel = k.compile(context);

    scalar<int_> index(context);
    kernel.set_arg(index_arg, index.get_buffer());

    index.write(static_cast<int_>(0), queue);

    kernel.set_arg(value_arg, value);

    queue.enqueue_1d_range_kernel(kernel, 0, count, 0);

    int result = static_cast<int>(index.read(queue));
    return first + result;
}

} // end detail namespace

///
/// \brief Permutation generating algorithm
///
/// Transforms the range [first, last) into the next permutation from the
/// set of all permutations arranged in lexicographic order
/// \return Boolean value signifying if the last permutation was crossed
/// and the range was reset
///
/// \param first Iterator pointing to start of range
/// \param last Iterator pointing to end of range
/// \param queue Queue on which to execute
///
/// Space complexity: \Omega(1)
template<class InputIterator>
inline bool next_permutation(InputIterator first,
                             InputIterator last,
                             command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    if(first == last) return false;

    InputIterator first_element =
        detail::next_permutation_helper(first, last, queue);

    if(first_element == last)
    {
        reverse(first, last, queue);
        return false;
    }

    value_type first_value = first_element.read(queue);

    InputIterator ceiling_element =
        detail::np_ceiling(first_element + 1, last, first_value, queue);

    value_type ceiling_value = ceiling_element.read(queue);

    first_element.write(ceiling_value, queue);
    ceiling_element.write(first_value, queue);

    reverse(first_element + 1, last, queue);

    return true;
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_NEXT_PERMUTATION_HPP
