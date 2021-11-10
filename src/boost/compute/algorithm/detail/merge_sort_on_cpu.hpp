//---------------------------------------------------------------------------//
// Copyright (c) 2015 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_MERGE_SORT_ON_CPU_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_MERGE_SORT_ON_CPU_HPP

#include <boost/compute/kernel.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/detail/merge_with_merge_path.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class KeyIterator, class ValueIterator, class Compare>
inline void merge_blocks(KeyIterator keys_first,
                         ValueIterator values_first,
                         KeyIterator keys_result,
                         ValueIterator values_result,
                         Compare compare,
                         size_t count,
                         const size_t block_size,
                         const bool sort_by_key,
                         command_queue &queue)
{
    (void) values_result;
    (void) values_first;

    meta_kernel k("merge_sort_on_cpu_merge_blocks");
    size_t count_arg = k.add_arg<const uint_>("count");
    size_t block_size_arg = k.add_arg<uint_>("block_size");

    k <<
        k.decl<uint_>("b1_start") << " = get_global_id(0) * block_size * 2;\n" <<
        k.decl<uint_>("b1_end") << " = min(count, b1_start + block_size);\n" <<
        k.decl<uint_>("b2_start") << " = min(count, b1_start + block_size);\n" <<
        k.decl<uint_>("b2_end") << " = min(count, b2_start + block_size);\n" <<
        k.decl<uint_>("result_idx") << " = b1_start;\n" <<

        // merging block 1 and block 2 (stable)
        "while(b1_start < b1_end && b2_start < b2_end){\n" <<
        "    if( " << compare(keys_first[k.var<uint_>("b2_start")],
                              keys_first[k.var<uint_>("b1_start")]) << "){\n" <<
        "        " << keys_result[k.var<uint_>("result_idx")] <<  " = " <<
                      keys_first[k.var<uint_>("b2_start")] << ";\n";
    if(sort_by_key){
        k <<
        "        " << values_result[k.var<uint_>("result_idx")] <<  " = " <<
                      values_first[k.var<uint_>("b2_start")] << ";\n";
    }
    k <<
        "        b2_start++;\n" <<
        "    }\n" <<
        "    else {\n" <<
        "        " << keys_result[k.var<uint_>("result_idx")] <<  " = " <<
                      keys_first[k.var<uint_>("b1_start")] << ";\n";
    if(sort_by_key){
        k <<
        "        " << values_result[k.var<uint_>("result_idx")] <<  " = " <<
                      values_first[k.var<uint_>("b1_start")] << ";\n";
    }
    k <<
        "        b1_start++;\n" <<
        "    }\n" <<
        "    result_idx++;\n" <<
        "}\n" <<
        "while(b1_start < b1_end){\n" <<
        "    " << keys_result[k.var<uint_>("result_idx")] <<  " = " <<
                 keys_first[k.var<uint_>("b1_start")] << ";\n";
    if(sort_by_key){
        k <<
        "    " << values_result[k.var<uint_>("result_idx")] <<  " = " <<
                      values_first[k.var<uint_>("b1_start")] << ";\n";
    }
    k <<
        "    b1_start++;\n" <<
        "    result_idx++;\n" <<
        "}\n" <<
        "while(b2_start < b2_end){\n" <<
        "    " << keys_result[k.var<uint_>("result_idx")] <<  " = " <<
                 keys_first[k.var<uint_>("b2_start")] << ";\n";
    if(sort_by_key){
        k <<
        "    " << values_result[k.var<uint_>("result_idx")] <<  " = " <<
                      values_first[k.var<uint_>("b2_start")] << ";\n";
    }
    k <<
        "    b2_start++;\n" <<
        "    result_idx++;\n" <<
        "}\n";

    const context &context = queue.get_context();
    ::boost::compute::kernel kernel = k.compile(context);
    kernel.set_arg(count_arg, static_cast<const uint_>(count));
    kernel.set_arg(block_size_arg, static_cast<uint_>(block_size));

    const size_t global_size = static_cast<size_t>(
        std::ceil(float(count) / (2 * block_size))
    );
    queue.enqueue_1d_range_kernel(kernel, 0, global_size, 0);
}

template<class Iterator, class Compare>
inline void merge_blocks(Iterator first,
                         Iterator result,
                         Compare compare,
                         size_t count,
                         const size_t block_size,
                         const bool sort_by_key,
                         command_queue &queue)
{
    // dummy iterator as it's not sort by key
    Iterator dummy;
    merge_blocks(first, dummy, result, dummy, compare, count, block_size, false, queue);
}

template<class Iterator, class Compare>
inline void dispatch_merge_blocks(Iterator first,
                                  Iterator result,
                                  Compare compare,
                                  size_t count,
                                  const size_t block_size,
                                  const size_t input_size_threshold,
                                  const size_t blocks_no_threshold,
                                  command_queue &queue)
{
    const size_t blocks_no = static_cast<size_t>(
        std::ceil(float(count) / block_size)
    );
    // merge with merge path should used only for the large arrays and at the
    // end of merging part when there are only a few big blocks left to be merged
    if(blocks_no <= blocks_no_threshold && count >= input_size_threshold){
        Iterator last = first + count;
        for(size_t i = 0; i < count; i+= 2*block_size)
        {
            Iterator first1 = (std::min)(first + i, last);
            Iterator last1 = (std::min)(first1 + block_size, last);
            Iterator first2 = last1;
            Iterator last2 = (std::min)(first2 + block_size, last);
            Iterator block_result = (std::min)(result + i, result + count);
            merge_with_merge_path(first1, last1, first2, last2,
                                  block_result, compare, queue);
        }
    }
    else {
        merge_blocks(first, result, compare, count, block_size, false, queue);
    }
}

template<class KeyIterator, class ValueIterator, class Compare>
inline void block_insertion_sort(KeyIterator keys_first,
                                 ValueIterator values_first,
                                 Compare compare,
                                 const size_t count,
                                 const size_t block_size,
                                 const bool sort_by_key,
                                 command_queue &queue)
{
    (void) values_first;

    typedef typename std::iterator_traits<KeyIterator>::value_type K;
    typedef typename std::iterator_traits<ValueIterator>::value_type T;

    meta_kernel k("merge_sort_on_cpu_block_insertion_sort");
    size_t count_arg = k.add_arg<uint_>("count");
    size_t block_size_arg = k.add_arg<uint_>("block_size");

    k <<
        k.decl<uint_>("start") << " = get_global_id(0) * block_size;\n" <<
        k.decl<uint_>("end") << " = min(count, start + block_size);\n" <<

        // block insertion sort (stable)
        "for(uint i = start+1; i < end; i++){\n" <<
        "    " << k.decl<const K>("key") << " = " <<
                  keys_first[k.var<uint_>("i")] << ";\n";
    if(sort_by_key){
        k <<
        "    " << k.decl<const T>("value") << " = " <<
                  values_first[k.var<uint_>("i")] << ";\n";
    }
    k <<
        "    uint pos = i;\n" <<
        "    while(pos > start && " <<
                   compare(k.var<const K>("key"),
                           keys_first[k.var<uint_>("pos-1")]) << "){\n" <<
        "        " << keys_first[k.var<uint_>("pos")] << " = " <<
                      keys_first[k.var<uint_>("pos-1")] << ";\n";
    if(sort_by_key){
        k <<
        "        " << values_first[k.var<uint_>("pos")] << " = " <<
                      values_first[k.var<uint_>("pos-1")] << ";\n";
    }
    k <<
        "        pos--;\n" <<
        "    }\n" <<
        "    " << keys_first[k.var<uint_>("pos")] << " = key;\n";
    if(sort_by_key) {
        k <<
        "    " << values_first[k.var<uint_>("pos")] << " = value;\n";
    }
    k <<
        "}\n"; // block insertion sort

    const context &context = queue.get_context();
    ::boost::compute::kernel kernel = k.compile(context);
    kernel.set_arg(count_arg, static_cast<uint_>(count));
    kernel.set_arg(block_size_arg, static_cast<uint_>(block_size));

    const size_t global_size = static_cast<size_t>(std::ceil(float(count) / block_size));
    queue.enqueue_1d_range_kernel(kernel, 0, global_size, 0);
}

template<class Iterator, class Compare>
inline void block_insertion_sort(Iterator first,
                                 Compare compare,
                                 const size_t count,
                                 const size_t block_size,
                                 command_queue &queue)
{
    // dummy iterator as it's not sort by key
    Iterator dummy;
    block_insertion_sort(first, dummy, compare, count, block_size, false, queue);
}

// This sort is stable.
template<class Iterator, class Compare>
inline void merge_sort_on_cpu(Iterator first,
                              Iterator last,
                              Compare compare,
                              command_queue &queue)
{
    typedef typename std::iterator_traits<Iterator>::value_type value_type;

    size_t count = iterator_range_size(first, last);
    if(count < 2){
        return;
    }
    // for small input size only insertion sort is performed
    else if(count <= 512){
        block_insertion_sort(first, compare, count, count, queue);
        return;
    }

    const context &context = queue.get_context();
    const device &device = queue.get_device();

    // loading parameters
    std::string cache_key =
        std::string("__boost_merge_sort_on_cpu_") + type_name<value_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    // When there is merge_with_path_blocks_no_threshold or less blocks left to
    // merge AND input size is merge_with_merge_path_input_size_threshold or more
    // merge_with_merge_path() algorithm is used to merge sorted blocks;
    // otherwise merge_blocks() is used.
    const size_t merge_with_path_blocks_no_threshold =
        parameters->get(cache_key, "merge_with_merge_path_blocks_no_threshold", 8);
    const size_t merge_with_path_input_size_threshold =
        parameters->get(cache_key, "merge_with_merge_path_input_size_threshold", 2097152);

    const size_t block_size =
        parameters->get(cache_key, "insertion_sort_block_size", 64);
    block_insertion_sort(first, compare, count, block_size, queue);

    // temporary buffer for merge result
    vector<value_type> temp(count, context);
    bool result_in_temporary_buffer = false;

    for(size_t i = block_size; i < count; i *= 2){
        result_in_temporary_buffer = !result_in_temporary_buffer;
        if(result_in_temporary_buffer) {
            dispatch_merge_blocks(first, temp.begin(), compare, count, i,
                                  merge_with_path_input_size_threshold,
                                  merge_with_path_blocks_no_threshold,
                                  queue);
        } else {
            dispatch_merge_blocks(temp.begin(), first, compare, count, i,
                                  merge_with_path_input_size_threshold,
                                  merge_with_path_blocks_no_threshold,
                                  queue);
        }
    }

    if(result_in_temporary_buffer) {
        copy(temp.begin(), temp.end(), first, queue);
    }
}

// This sort is stable.
template<class KeyIterator, class ValueIterator, class Compare>
inline void merge_sort_by_key_on_cpu(KeyIterator keys_first,
                                     KeyIterator keys_last,
                                     ValueIterator values_first,
                                     Compare compare,
                                     command_queue &queue)
{
    typedef typename std::iterator_traits<KeyIterator>::value_type key_type;
    typedef typename std::iterator_traits<ValueIterator>::value_type value_type;

    size_t count = iterator_range_size(keys_first, keys_last);
    if(count < 2){
        return;
    }
    // for small input size only insertion sort is performed
    else if(count <= 512){
        block_insertion_sort(keys_first, values_first, compare,
                             count, count, true, queue);
        return;
    }

    const context &context = queue.get_context();
    const device &device = queue.get_device();

    // loading parameters
    std::string cache_key =
        std::string("__boost_merge_sort_by_key_on_cpu_") + type_name<value_type>()
        + "_with_" + type_name<key_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    const size_t block_size =
        parameters->get(cache_key, "insertion_sort_by_key_block_size", 64);
    block_insertion_sort(keys_first, values_first, compare,
                         count, block_size, true, queue);

    // temporary buffer for merge results
    vector<value_type> values_temp(count, context);
    vector<key_type> keys_temp(count, context);
    bool result_in_temporary_buffer = false;

    for(size_t i = block_size; i < count; i *= 2){
        result_in_temporary_buffer = !result_in_temporary_buffer;
        if(result_in_temporary_buffer) {
            merge_blocks(keys_first, values_first,
                         keys_temp.begin(), values_temp.begin(),
                         compare, count, i, true, queue);
        } else {
            merge_blocks(keys_temp.begin(), values_temp.begin(),
                         keys_first, values_first,
                         compare, count, i, true, queue);
        }
    }

    if(result_in_temporary_buffer) {
        copy(keys_temp.begin(), keys_temp.end(), keys_first, queue);
        copy(values_temp.begin(), values_temp.end(), values_first, queue);
    }
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_MERGE_SORT_ON_CPU_HPP
