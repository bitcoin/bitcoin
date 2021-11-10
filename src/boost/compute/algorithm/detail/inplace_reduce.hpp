//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_INPLACE_REDUCE_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_INPLACE_REDUCE_HPP

#include <iterator>

#include <boost/utility/result_of.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/memory/local_buffer.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Iterator, class BinaryFunction>
inline void inplace_reduce(Iterator first,
                           Iterator last,
                           BinaryFunction function,
                           command_queue &queue)
{
    typedef typename
        std::iterator_traits<Iterator>::value_type
        value_type;

    size_t input_size = iterator_range_size(first, last);
    if(input_size < 2){
        return;
    }

    const context &context = queue.get_context();

    size_t block_size = 64;
    size_t values_per_thread = 8;
    size_t block_count = input_size / (block_size * values_per_thread);
    if(block_count * block_size * values_per_thread != input_size)
        block_count++;

    vector<value_type> output(block_count, context);

    meta_kernel k("inplace_reduce");
    size_t input_arg = k.add_arg<value_type *>(memory_object::global_memory, "input");
    size_t input_size_arg = k.add_arg<const uint_>("input_size");
    size_t output_arg = k.add_arg<value_type *>(memory_object::global_memory, "output");
    size_t scratch_arg = k.add_arg<value_type *>(memory_object::local_memory, "scratch");
    k <<
        "const uint gid = get_global_id(0);\n" <<
        "const uint lid = get_local_id(0);\n" <<
        "const uint values_per_thread =\n"
            << uint_(values_per_thread) << ";\n" <<

        // thread reduce
        "const uint index = gid * values_per_thread;\n" <<
        "if(index < input_size){\n" <<
            k.decl<value_type>("sum") << " = input[index];\n" <<
            "for(uint i = 1;\n" <<
                 "i < values_per_thread && (index + i) < input_size;\n" <<
                 "i++){\n" <<
            "    sum = " <<
                     function(k.var<value_type>("sum"),
                              k.var<value_type>("input[index+i]")) << ";\n" <<
            "}\n" <<
            "scratch[lid] = sum;\n" <<
        "}\n" <<

        // local reduce
        "for(uint i = 1; i < get_local_size(0); i <<= 1){\n" <<
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    uint mask = (i << 1) - 1;\n" <<
        "    uint next_index = (gid + i) * values_per_thread;\n"
        "    if((lid & mask) == 0 && next_index < input_size){\n" <<
        "        scratch[lid] = " <<
                     function(k.var<value_type>("scratch[lid]"),
                              k.var<value_type>("scratch[lid+i]")) << ";\n" <<
        "    }\n" <<
        "}\n" <<

        // write output for block
        "if(lid == 0){\n" <<
        "    output[get_group_id(0)] = scratch[0];\n" <<
        "}\n"
        ;

    const buffer *input_buffer = &first.get_buffer();
    const buffer *output_buffer = &output.get_buffer();

    kernel kernel = k.compile(context);

    while(input_size > 1){
        kernel.set_arg(input_arg, *input_buffer);
        kernel.set_arg(input_size_arg, static_cast<uint_>(input_size));
        kernel.set_arg(output_arg, *output_buffer);
        kernel.set_arg(scratch_arg, local_buffer<value_type>(block_size));

        queue.enqueue_1d_range_kernel(kernel,
                                      0,
                                      block_count * block_size,
                                      block_size);

        input_size =
            static_cast<size_t>(
                std::ceil(float(input_size) / (block_size * values_per_thread)
            )
        );

        block_count = input_size / (block_size * values_per_thread);
        if(block_count * block_size * values_per_thread != input_size)
            block_count++;

        std::swap(input_buffer, output_buffer);
    }

    if(input_buffer != &first.get_buffer()){
        ::boost::compute::copy(output.begin(),
                               output.begin() + 1,
                               first,
                               queue);
    }
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_INPLACE_REDUCE_HPP
