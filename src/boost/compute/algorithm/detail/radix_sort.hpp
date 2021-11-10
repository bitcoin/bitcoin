//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_RADIX_SORT_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_RADIX_SORT_HPP

#include <iterator>

#include <boost/assert.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/is_floating_point.hpp>

#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>

#include <boost/compute/kernel.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/exclusive_scan.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/parameter_cache.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/type_traits/is_fundamental.hpp>
#include <boost/compute/type_traits/is_vector_type.hpp>
#include <boost/compute/utility/program_cache.hpp>

namespace boost {
namespace compute {
namespace detail {

// meta-function returning true if type T is radix-sortable
template<class T>
struct is_radix_sortable :
    boost::mpl::and_<
        typename ::boost::compute::is_fundamental<T>::type,
        typename boost::mpl::not_<typename is_vector_type<T>::type>::type
    >
{
};

template<size_t N>
struct radix_sort_value_type
{
};

template<>
struct radix_sort_value_type<1>
{
    typedef uchar_ type;
};

template<>
struct radix_sort_value_type<2>
{
    typedef ushort_ type;
};

template<>
struct radix_sort_value_type<4>
{
    typedef uint_ type;
};

template<>
struct radix_sort_value_type<8>
{
    typedef ulong_ type;
};

template<typename T>
inline const char* enable_double()
{
    return " -DT2_double=0";
}

template<>
inline const char* enable_double<double>()
{
    return " -DT2_double=1";
}

const char radix_sort_source[] =
"#if T2_double\n"
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n"
"#endif\n"
"#define K2_BITS (1 << K_BITS)\n"
"#define RADIX_MASK ((((T)(1)) << K_BITS) - 1)\n"
"#define SIGN_BIT ((sizeof(T) * CHAR_BIT) - 1)\n"

"#if defined(ASC)\n" // asc order

"inline uint radix(const T x, const uint low_bit)\n"
"{\n"
"#if defined(IS_FLOATING_POINT)\n"
"    const T mask = -(x >> SIGN_BIT) | (((T)(1)) << SIGN_BIT);\n"
"    return ((x ^ mask) >> low_bit) & RADIX_MASK;\n"
"#elif defined(IS_SIGNED)\n"
"    return ((x ^ (((T)(1)) << SIGN_BIT)) >> low_bit) & RADIX_MASK;\n"
"#else\n"
"    return (x >> low_bit) & RADIX_MASK;\n"
"#endif\n"
"}\n"

"#else\n" // desc order

// For signed types we just negate the x and for unsigned types we
// subtract the x from max value of its type ((T)(-1) is a max value
// of type T when T is an unsigned type).
"inline uint radix(const T x, const uint low_bit)\n"
"{\n"
"#if defined(IS_FLOATING_POINT)\n"
"    const T mask = -(x >> SIGN_BIT) | (((T)(1)) << SIGN_BIT);\n"
"    return (((-x) ^ mask) >> low_bit) & RADIX_MASK;\n"
"#elif defined(IS_SIGNED)\n"
"    return (((-x) ^ (((T)(1)) << SIGN_BIT)) >> low_bit) & RADIX_MASK;\n"
"#else\n"
"    return (((T)(-1) - x) >> low_bit) & RADIX_MASK;\n"
"#endif\n"
"}\n"

"#endif\n" // #if defined(ASC)

"__kernel void count(__global const T *input,\n"
"                    const uint input_offset,\n"
"                    const uint input_size,\n"
"                    __global uint *global_counts,\n"
"                    __global uint *global_offsets,\n"
"                    __local uint *local_counts,\n"
"                    const uint low_bit)\n"
"{\n"
     // work-item parameters
"    const uint gid = get_global_id(0);\n"
"    const uint lid = get_local_id(0);\n"

     // zero local counts
"    if(lid < K2_BITS){\n"
"        local_counts[lid] = 0;\n"
"    }\n"
"    barrier(CLK_LOCAL_MEM_FENCE);\n"

     // reduce local counts
"    if(gid < input_size){\n"
"        T value = input[input_offset+gid];\n"
"        uint bucket = radix(value, low_bit);\n"
"        atomic_inc(local_counts + bucket);\n"
"    }\n"
"    barrier(CLK_LOCAL_MEM_FENCE);\n"

     // write block-relative offsets
"    if(lid < K2_BITS){\n"
"        global_counts[K2_BITS*get_group_id(0) + lid] = local_counts[lid];\n"

         // write global offsets
"        if(get_group_id(0) == (get_num_groups(0) - 1)){\n"
"            global_offsets[lid] = local_counts[lid];\n"
"        }\n"
"    }\n"
"}\n"

"__kernel void scan(__global const uint *block_offsets,\n"
"                   __global uint *global_offsets,\n"
"                   const uint block_count)\n"
"{\n"
"    __global const uint *last_block_offsets =\n"
"        block_offsets + K2_BITS * (block_count - 1);\n"

     // calculate and scan global_offsets
"    uint sum = 0;\n"
"    for(uint i = 0; i < K2_BITS; i++){\n"
"        uint x = global_offsets[i] + last_block_offsets[i];\n"
"        mem_fence(CLK_GLOBAL_MEM_FENCE);\n" // work around the RX 500/Vega bug, see #811
"        global_offsets[i] = sum;\n"
"        sum += x;\n"
"        mem_fence(CLK_GLOBAL_MEM_FENCE);\n" // work around the RX Vega bug, see #811
"    }\n"
"}\n"

"__kernel void scatter(__global const T *input,\n"
"                      const uint input_offset,\n"
"                      const uint input_size,\n"
"                      const uint low_bit,\n"
"                      __global const uint *counts,\n"
"                      __global const uint *global_offsets,\n"
"#ifndef SORT_BY_KEY\n"
"                      __global T *output,\n"
"                      const uint output_offset)\n"
"#else\n"
"                      __global T *keys_output,\n"
"                      const uint keys_output_offset,\n"
"                      __global T2 *values_input,\n"
"                      const uint values_input_offset,\n"
"                      __global T2 *values_output,\n"
"                      const uint values_output_offset)\n"
"#endif\n"
"{\n"
     // work-item parameters
"    const uint gid = get_global_id(0);\n"
"    const uint lid = get_local_id(0);\n"

     // copy input to local memory
"    T value;\n"
"    uint bucket;\n"
"    __local uint local_input[BLOCK_SIZE];\n"
"    if(gid < input_size){\n"
"        value = input[input_offset+gid];\n"
"        bucket = radix(value, low_bit);\n"
"        local_input[lid] = bucket;\n"
"    }\n"

     // copy block counts to local memory
"    __local uint local_counts[(1 << K_BITS)];\n"
"    if(lid < K2_BITS){\n"
"        local_counts[lid] = counts[get_group_id(0) * K2_BITS + lid];\n"
"    }\n"

     // wait until local memory is ready
"    barrier(CLK_LOCAL_MEM_FENCE);\n"

"    if(gid >= input_size){\n"
"        return;\n"
"    }\n"

     // get global offset
"    uint offset = global_offsets[bucket] + local_counts[bucket];\n"

     // calculate local offset
"    uint local_offset = 0;\n"
"    for(uint i = 0; i < lid; i++){\n"
"        if(local_input[i] == bucket)\n"
"            local_offset++;\n"
"    }\n"

"#ifndef SORT_BY_KEY\n"
     // write value to output
"    output[output_offset + offset + local_offset] = value;\n"
"#else\n"
     // write key and value if doing sort_by_key
"    keys_output[keys_output_offset+offset + local_offset] = value;\n"
"    values_output[values_output_offset+offset + local_offset] =\n"
"        values_input[values_input_offset+gid];\n"
"#endif\n"
"}\n";

template<class T, class T2>
inline void radix_sort_impl(const buffer_iterator<T> first,
                            const buffer_iterator<T> last,
                            const buffer_iterator<T2> values_first,
                            const bool ascending,
                            command_queue &queue)
{

    typedef T value_type;
    typedef typename radix_sort_value_type<sizeof(T)>::type sort_type;

    const device &device = queue.get_device();
    const context &context = queue.get_context();


    // if we have a valid values iterator then we are doing a
    // sort by key and have to set up the values buffer
    bool sort_by_key = (values_first.get_buffer().get() != 0);

    // load (or create) radix sort program
    std::string cache_key =
        std::string("__boost_radix_sort_") + type_name<value_type>();

    if(sort_by_key){
        cache_key += std::string("_with_") + type_name<T2>();
    }

    boost::shared_ptr<program_cache> cache =
        program_cache::get_global_cache(context);
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    // sort parameters
    const uint_ k = parameters->get(cache_key, "k", 4);
    const uint_ k2 = 1 << k;
    const uint_ block_size = parameters->get(cache_key, "tpb", 128);

    // sort program compiler options
    std::stringstream options;
    options << "-DK_BITS=" << k;
    options << " -DT=" << type_name<sort_type>();
    options << " -DBLOCK_SIZE=" << block_size;

    if(boost::is_floating_point<value_type>::value){
        options << " -DIS_FLOATING_POINT";
    }

    if(boost::is_signed<value_type>::value){
        options << " -DIS_SIGNED";
    }

    if(sort_by_key){
        options << " -DSORT_BY_KEY";
        options << " -DT2=" << type_name<T2>();
        options << enable_double<T2>();
    }

    if(ascending){
        options << " -DASC";
    }

    // get type definition if it is a custom struct
    std::string custom_type_def = boost::compute::type_definition<T2>() + "\n";

    // load radix sort program
    program radix_sort_program = cache->get_or_build(
       cache_key, options.str(), custom_type_def + radix_sort_source, context
    );

    kernel count_kernel(radix_sort_program, "count");
    kernel scan_kernel(radix_sort_program, "scan");
    kernel scatter_kernel(radix_sort_program, "scatter");

    size_t count = detail::iterator_range_size(first, last);

    uint_ block_count = static_cast<uint_>(count / block_size);
    if(block_count * block_size != count){
        block_count++;
    }

    // setup temporary buffers
    vector<value_type> output(count, context);
    vector<T2> values_output(sort_by_key ? count : 0, context);
    vector<uint_> offsets(k2, context);
    vector<uint_> counts(block_count * k2, context);

    const buffer *input_buffer = &first.get_buffer();
    uint_ input_offset = static_cast<uint_>(first.get_index());
    const buffer *output_buffer = &output.get_buffer();
    uint_ output_offset = 0;
    const buffer *values_input_buffer = &values_first.get_buffer();
    uint_ values_input_offset = static_cast<uint_>(values_first.get_index());
    const buffer *values_output_buffer = &values_output.get_buffer();
    uint_ values_output_offset = 0;

    for(uint_ i = 0; i < sizeof(sort_type) * CHAR_BIT / k; i++){
        // write counts
        count_kernel.set_arg(0, *input_buffer);
        count_kernel.set_arg(1, input_offset);
        count_kernel.set_arg(2, static_cast<uint_>(count));
        count_kernel.set_arg(3, counts);
        count_kernel.set_arg(4, offsets);
        count_kernel.set_arg(5, block_size * sizeof(uint_), 0);
        count_kernel.set_arg(6, i * k);
        queue.enqueue_1d_range_kernel(count_kernel,
                                      0,
                                      block_count * block_size,
                                      block_size);

        // scan counts
        if(k == 1){
            typedef uint2_ counter_type;
            ::boost::compute::exclusive_scan(
                make_buffer_iterator<counter_type>(counts.get_buffer(), 0),
                make_buffer_iterator<counter_type>(counts.get_buffer(), counts.size() / 2),
                make_buffer_iterator<counter_type>(counts.get_buffer()),
                queue
            );
        }
        else if(k == 2){
            typedef uint4_ counter_type;
            ::boost::compute::exclusive_scan(
                make_buffer_iterator<counter_type>(counts.get_buffer(), 0),
                make_buffer_iterator<counter_type>(counts.get_buffer(), counts.size() / 4),
                make_buffer_iterator<counter_type>(counts.get_buffer()),
                queue
            );
        }
        else if(k == 4){
            typedef uint16_ counter_type;
            ::boost::compute::exclusive_scan(
                make_buffer_iterator<counter_type>(counts.get_buffer(), 0),
                make_buffer_iterator<counter_type>(counts.get_buffer(), counts.size() / 16),
                make_buffer_iterator<counter_type>(counts.get_buffer()),
                queue
            );
        }
        else {
            BOOST_ASSERT(false && "unknown k");
            break;
        }

        // scan global offsets
        scan_kernel.set_arg(0, counts);
        scan_kernel.set_arg(1, offsets);
        scan_kernel.set_arg(2, block_count);
        queue.enqueue_task(scan_kernel);

        // scatter values
        scatter_kernel.set_arg(0, *input_buffer);
        scatter_kernel.set_arg(1, input_offset);
        scatter_kernel.set_arg(2, static_cast<uint_>(count));
        scatter_kernel.set_arg(3, i * k);
        scatter_kernel.set_arg(4, counts);
        scatter_kernel.set_arg(5, offsets);
        scatter_kernel.set_arg(6, *output_buffer);
        scatter_kernel.set_arg(7, output_offset);
        if(sort_by_key){
            scatter_kernel.set_arg(8, *values_input_buffer);
            scatter_kernel.set_arg(9, values_input_offset);
            scatter_kernel.set_arg(10, *values_output_buffer);
            scatter_kernel.set_arg(11, values_output_offset);
        }
        queue.enqueue_1d_range_kernel(scatter_kernel,
                                      0,
                                      block_count * block_size,
                                      block_size);

        // swap buffers
        std::swap(input_buffer, output_buffer);
        std::swap(values_input_buffer, values_output_buffer);
        std::swap(input_offset, output_offset);
        std::swap(values_input_offset, values_output_offset);
    }
}

template<class Iterator>
inline void radix_sort(Iterator first,
                       Iterator last,
                       command_queue &queue)
{
    radix_sort_impl(first, last, buffer_iterator<int>(), true, queue);
}

template<class KeyIterator, class ValueIterator>
inline void radix_sort_by_key(KeyIterator keys_first,
                              KeyIterator keys_last,
                              ValueIterator values_first,
                              command_queue &queue)
{
    radix_sort_impl(keys_first, keys_last, values_first, true, queue);
}

template<class Iterator>
inline void radix_sort(Iterator first,
                       Iterator last,
                       const bool ascending,
                       command_queue &queue)
{
    radix_sort_impl(first, last, buffer_iterator<int>(), ascending, queue);
}

template<class KeyIterator, class ValueIterator>
inline void radix_sort_by_key(KeyIterator keys_first,
                              KeyIterator keys_last,
                              ValueIterator values_first,
                              const bool ascending,
                              command_queue &queue)
{
    radix_sort_impl(keys_first, keys_last, values_first, ascending, queue);
}


} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_RADIX_SORT_HPP
