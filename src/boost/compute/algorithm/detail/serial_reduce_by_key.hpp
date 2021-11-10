//---------------------------------------------------------------------------//
// Copyright (c) 2015 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_REDUCE_BY_KEY_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_REDUCE_BY_KEY_HPP

#include <iterator>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/container/detail/scalar.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/type_traits/result_of.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator,
         class BinaryFunction, class BinaryPredicate>
inline size_t serial_reduce_by_key(InputKeyIterator keys_first,
                                   InputKeyIterator keys_last,
                                   InputValueIterator values_first,
                                   OutputKeyIterator keys_result,
                                   OutputValueIterator values_result,
                                   BinaryFunction function,
                                   BinaryPredicate predicate,
                                   command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputValueIterator>::value_type value_type;
    typedef typename
        std::iterator_traits<InputKeyIterator>::value_type key_type;
    typedef typename
        ::boost::compute::result_of<BinaryFunction(value_type, value_type)>::type result_type;

    const context &context = queue.get_context();
    size_t count = detail::iterator_range_size(keys_first, keys_last);
    if(count < 1){
        return count;
    }

    meta_kernel k("serial_reduce_by_key");
    size_t count_arg = k.add_arg<uint_>("count");
    size_t result_size_arg = k.add_arg<uint_ *>(memory_object::global_memory,
                                                "result_size");

    k <<
        k.decl<result_type>("result") <<
            " = " << values_first[0] << ";\n" <<
        k.decl<key_type>("previous_key") << " = " << keys_first[0] << ";\n" <<
        k.decl<result_type>("value") << ";\n" <<
        k.decl<key_type>("key") << ";\n" <<

        k.decl<uint_>("size") << " = 1;\n" <<

        keys_result[0] << " = previous_key;\n" <<
        values_result[0] << " = result;\n" <<

        "for(ulong i = 1; i < count; i++) {\n" <<
        "    value = " << values_first[k.var<uint_>("i")] << ";\n" <<
        "    key = " << keys_first[k.var<uint_>("i")] << ";\n" <<
        "    if (" << predicate(k.var<key_type>("previous_key"),
                                k.var<key_type>("key")) << ") {\n" <<

        "        result = " << function(k.var<result_type>("result"),
                                        k.var<result_type>("value")) << ";\n" <<
        "    }\n " <<
        "    else { \n" <<
                 keys_result[k.var<uint_>("size - 1")] << " = previous_key;\n" <<
                 values_result[k.var<uint_>("size - 1")] << " = result;\n" <<
        "        result = value;\n" <<
        "        size++;\n" <<
        "    } \n" <<
        "    previous_key = key;\n" <<
        "}\n" <<
        keys_result[k.var<uint_>("size - 1")] << " = previous_key;\n" <<
        values_result[k.var<uint_>("size - 1")] << " = result;\n" <<
        "*result_size = size;";

    kernel kernel = k.compile(context);

    scalar<uint_> result_size(context);
    kernel.set_arg(result_size_arg, result_size.get_buffer());
    kernel.set_arg(count_arg, static_cast<uint_>(count));

    queue.enqueue_task(kernel);

    return static_cast<size_t>(result_size.read(queue));
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_REDUCE_BY_KEY_HPP
