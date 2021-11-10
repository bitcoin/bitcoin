//---------------------------------------------------------------------------//
// Copyright (c) 2015 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_REDUCE_BY_KEY_WITH_SCAN_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_REDUCE_BY_KEY_WITH_SCAN_HPP

#include <algorithm>
#include <iterator>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/algorithm/inclusive_scan.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/container/detail/scalar.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/read_write_single_value.hpp>
#include <boost/compute/type_traits.hpp>
#include <boost/compute/utility/program_cache.hpp>

namespace boost {
namespace compute {
namespace detail {

/// \internal_
///
/// Fills \p new_keys_first with unsigned integer keys generated from vector
/// of original keys \p keys_first. New keys can be distinguish by simple equality
/// predicate.
///
/// \param keys_first iterator pointing to the first key
/// \param number_of_keys number of keys
/// \param predicate binary predicate for key comparison
/// \param new_keys_first iterator pointing to the new keys vector
/// \param preferred_work_group_size preferred work group size
/// \param queue command queue to perform the operation
///
/// Binary function \p predicate must take two keys as arguments and
/// return true only if they are considered the same.
///
/// The first new key equals zero and the last equals number of unique keys
/// minus one.
///
/// No local memory usage.
template<class InputKeyIterator, class BinaryPredicate>
inline void generate_uint_keys(InputKeyIterator keys_first,
                               size_t number_of_keys,
                               BinaryPredicate predicate,
                               vector<uint_>::iterator new_keys_first,
                               size_t preferred_work_group_size,
                               command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputKeyIterator>::value_type key_type;

    detail::meta_kernel k("reduce_by_key_new_key_flags");
    k.add_set_arg<const uint_>("count", uint_(number_of_keys));

    k <<
        k.decl<const uint_>("gid") << " = get_global_id(0);\n" <<
        k.decl<uint_>("value") << " = 0;\n" <<
        "if(gid >= count){\n    return;\n}\n" <<
        "if(gid > 0){ \n" <<
        k.decl<key_type>("key") << " = " <<
                                keys_first[k.var<const uint_>("gid")] << ";\n" <<
        k.decl<key_type>("previous_key") << " = " <<
                                keys_first[k.var<const uint_>("gid - 1")] << ";\n" <<
        "    value = " << predicate(k.var<key_type>("previous_key"),
                                    k.var<key_type>("key")) <<
                          " ? 0 : 1;\n" <<
        "}\n else {\n" <<
        "    value = 0;\n" <<
        "}\n" <<
        new_keys_first[k.var<const uint_>("gid")] << " = value;\n";

    const context &context = queue.get_context();
    kernel kernel = k.compile(context);

    size_t work_group_size = preferred_work_group_size;
    size_t work_groups_no = static_cast<size_t>(
        std::ceil(float(number_of_keys) / work_group_size)
    );

    queue.enqueue_1d_range_kernel(kernel,
                                  0,
                                  work_groups_no * work_group_size,
                                  work_group_size);

    inclusive_scan(new_keys_first, new_keys_first + number_of_keys,
                   new_keys_first, queue);
}

/// \internal_
/// Calculate carry-out for each work group.
/// Carry-out is a pair of the last key processed by a work group and sum of all
/// values under this key in this work group.
template<class InputValueIterator, class OutputValueIterator, class BinaryFunction>
inline void carry_outs(vector<uint_>::iterator keys_first,
                       InputValueIterator values_first,
                       size_t count,
                       vector<uint_>::iterator carry_out_keys_first,
                       OutputValueIterator carry_out_values_first,
                       BinaryFunction function,
                       size_t work_group_size,
                       command_queue &queue)
{
    typedef typename
        std::iterator_traits<OutputValueIterator>::value_type value_out_type;

    detail::meta_kernel k("reduce_by_key_with_scan_carry_outs");
    k.add_set_arg<const uint_>("count", uint_(count));
    size_t local_keys_arg = k.add_arg<uint_ *>(memory_object::local_memory, "lkeys");
    size_t local_vals_arg = k.add_arg<value_out_type *>(memory_object::local_memory, "lvals");

    k <<
        k.decl<const uint_>("gid") << " = get_global_id(0);\n" <<
        k.decl<const uint_>("wg_size") << " = get_local_size(0);\n" <<
        k.decl<const uint_>("lid") << " = get_local_id(0);\n" <<
        k.decl<const uint_>("group_id") << " = get_group_id(0);\n" <<

        k.decl<uint_>("key") << ";\n" <<
        k.decl<value_out_type>("value") << ";\n" <<
        "if(gid < count){\n" <<
            k.var<uint_>("key") << " = " <<
                keys_first[k.var<const uint_>("gid")] << ";\n" <<
            k.var<value_out_type>("value") << " = " <<
                values_first[k.var<const uint_>("gid")] << ";\n" <<
            "lkeys[lid] = key;\n" <<
            "lvals[lid] = value;\n" <<
        "}\n" <<

        // Calculate carry out for each work group by performing Hillis/Steele scan
        // where only last element (key-value pair) is saved
        k.decl<value_out_type>("result") << " = value;\n" <<
        k.decl<uint_>("other_key") << ";\n" <<
        k.decl<value_out_type>("other_value") << ";\n" <<

        "for(" << k.decl<uint_>("offset") << " = 1; " <<
                  "offset < wg_size; offset *= 2){\n"
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    if(lid >= offset){\n"
        "        other_key = lkeys[lid - offset];\n" <<
        "        if(other_key == key){\n" <<
        "            other_value = lvals[lid - offset];\n" <<
        "            result = " << function(k.var<value_out_type>("result"),
                                            k.var<value_out_type>("other_value")) << ";\n" <<
        "        }\n" <<
        "    }\n" <<
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    lvals[lid] = result;\n" <<
        "}\n" <<

        // save carry out
        "if(lid == (wg_size - 1)){\n" <<
        carry_out_keys_first[k.var<const uint_>("group_id")] << " = key;\n" <<
        carry_out_values_first[k.var<const uint_>("group_id")] << " = result;\n" <<
        "}\n";

    size_t work_groups_no = static_cast<size_t>(
        std::ceil(float(count) / work_group_size)
    );

    const context &context = queue.get_context();
    kernel kernel = k.compile(context);
    kernel.set_arg(local_keys_arg, local_buffer<uint_>(work_group_size));
    kernel.set_arg(local_vals_arg, local_buffer<value_out_type>(work_group_size));

    queue.enqueue_1d_range_kernel(kernel,
                                  0,
                                  work_groups_no * work_group_size,
                                  work_group_size);
}

/// \internal_
/// Calculate carry-in by performing inclusive scan by key on carry-outs vector.
template<class OutputValueIterator, class BinaryFunction>
inline void carry_ins(vector<uint_>::iterator carry_out_keys_first,
                      OutputValueIterator carry_out_values_first,
                      OutputValueIterator carry_in_values_first,
                      size_t carry_out_size,
                      BinaryFunction function,
                      size_t work_group_size,
                      command_queue &queue)
{
    typedef typename
        std::iterator_traits<OutputValueIterator>::value_type value_out_type;

    uint_ values_pre_work_item = static_cast<uint_>(
        std::ceil(float(carry_out_size) / work_group_size)
    );

    detail::meta_kernel k("reduce_by_key_with_scan_carry_ins");
    k.add_set_arg<const uint_>("carry_out_size", uint_(carry_out_size));
    k.add_set_arg<const uint_>("values_per_work_item", values_pre_work_item);
    size_t local_keys_arg = k.add_arg<uint_ *>(memory_object::local_memory, "lkeys");
    size_t local_vals_arg = k.add_arg<value_out_type *>(memory_object::local_memory, "lvals");

    k <<
        k.decl<uint_>("id") << " = get_global_id(0) * values_per_work_item;\n" <<
        k.decl<uint_>("idx") << " = id;\n" <<
        k.decl<const uint_>("wg_size") << " = get_local_size(0);\n" <<
        k.decl<const uint_>("lid") << " = get_local_id(0);\n" <<
        k.decl<const uint_>("group_id") << " = get_group_id(0);\n" <<

        k.decl<uint_>("key") << ";\n" <<
        k.decl<value_out_type>("value") << ";\n" <<
        k.decl<uint_>("previous_key") << ";\n" <<
        k.decl<value_out_type>("result") << ";\n" <<

        "if(id < carry_out_size){\n" <<
            k.var<uint_>("previous_key") << " = " <<
                carry_out_keys_first[k.var<const uint_>("id")] << ";\n" <<
            k.var<value_out_type>("result") << " = " <<
                carry_out_values_first[k.var<const uint_>("id")] << ";\n" <<
            carry_in_values_first[k.var<const uint_>("id")] << " = result;\n" <<
        "}\n" <<

        k.decl<const uint_>("end") << " = (id + values_per_work_item) <= carry_out_size" <<
                                      " ? (values_per_work_item + id) :  carry_out_size;\n" <<

        "for(idx = idx + 1; idx < end; idx += 1){\n" <<
        "    key = " << carry_out_keys_first[k.var<const uint_>("idx")] << ";\n" <<
        "    value = " << carry_out_values_first[k.var<const uint_>("idx")] << ";\n" <<
        "    if(previous_key == key){\n" <<
        "        result = " << function(k.var<value_out_type>("result"),
                                        k.var<value_out_type>("value")) << ";\n" <<
        "    }\n else { \n" <<
        "        result = value;\n"
        "    }\n" <<
        "    " << carry_in_values_first[k.var<const uint_>("idx")] << " = result;\n" <<
        "    previous_key = key;\n"
        "}\n" <<

        // save the last key and result to local memory
        "lkeys[lid] = previous_key;\n" <<
        "lvals[lid] = result;\n" <<

        // Hillis/Steele scan
        "for(" << k.decl<uint_>("offset") << " = 1; " <<
                  "offset < wg_size; offset *= 2){\n"
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    if(lid >= offset){\n"
        "        key = lkeys[lid - offset];\n" <<
        "        if(previous_key == key){\n" <<
        "            value = lvals[lid - offset];\n" <<
        "            result = " << function(k.var<value_out_type>("result"),
                                            k.var<value_out_type>("value")) << ";\n" <<
        "        }\n" <<
        "    }\n" <<
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    lvals[lid] = result;\n" <<
        "}\n" <<
        "barrier(CLK_LOCAL_MEM_FENCE);\n" <<

        "if(lid > 0){\n" <<
        // load key-value reduced by previous work item
        "    previous_key = lkeys[lid - 1];\n" <<
        "    result       = lvals[lid - 1];\n" <<
        "}\n" <<

        // add key-value reduced by previous work item
        "for(idx = id; idx < id + values_per_work_item; idx += 1){\n" <<
        // make sure all carry-ins are saved in global memory
        "    barrier( CLK_GLOBAL_MEM_FENCE );\n" <<
        "    if(lid > 0 && idx < carry_out_size) {\n"
        "        key = " << carry_out_keys_first[k.var<const uint_>("idx")] << ";\n" <<
        "        value = " << carry_in_values_first[k.var<const uint_>("idx")] << ";\n" <<
        "        if(previous_key == key){\n" <<
        "            value = " << function(k.var<value_out_type>("result"),
                                           k.var<value_out_type>("value")) << ";\n" <<
        "        }\n" <<
        "        " << carry_in_values_first[k.var<const uint_>("idx")] << " = value;\n" <<
        "    }\n" <<
        "}\n";


    const context &context = queue.get_context();
    kernel kernel = k.compile(context);
    kernel.set_arg(local_keys_arg, local_buffer<uint_>(work_group_size));
    kernel.set_arg(local_vals_arg, local_buffer<value_out_type>(work_group_size));

    queue.enqueue_1d_range_kernel(kernel,
                                  0,
                                  work_group_size,
                                  work_group_size);
}

/// \internal_
///
/// Perform final reduction by key. Each work item:
/// 1. Perform local work-group reduction (Hillis/Steele scan)
/// 2. Add carry-in (if keys are right)
/// 3. Save reduced value if next key is different than processed one
template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator,
         class BinaryFunction>
inline void final_reduction(InputKeyIterator keys_first,
                            InputValueIterator values_first,
                            OutputKeyIterator keys_result,
                            OutputValueIterator values_result,
                            size_t count,
                            BinaryFunction function,
                            vector<uint_>::iterator new_keys_first,
                            vector<uint_>::iterator carry_in_keys_first,
                            OutputValueIterator carry_in_values_first,
                            size_t carry_in_size,
                            size_t work_group_size,
                            command_queue &queue)
{
    typedef typename
        std::iterator_traits<OutputValueIterator>::value_type value_out_type;

    detail::meta_kernel k("reduce_by_key_with_scan_final_reduction");
    k.add_set_arg<const uint_>("count", uint_(count));
    size_t local_keys_arg = k.add_arg<uint_ *>(memory_object::local_memory, "lkeys");
    size_t local_vals_arg = k.add_arg<value_out_type *>(memory_object::local_memory, "lvals");

    k <<
        k.decl<const uint_>("gid") << " = get_global_id(0);\n" <<
        k.decl<const uint_>("wg_size") << " = get_local_size(0);\n" <<
        k.decl<const uint_>("lid") << " = get_local_id(0);\n" <<
        k.decl<const uint_>("group_id") << " = get_group_id(0);\n" <<

        k.decl<uint_>("key") << ";\n" <<
        k.decl<value_out_type>("value") << ";\n"

        "if(gid < count){\n" <<
            k.var<uint_>("key") << " = " <<
                new_keys_first[k.var<const uint_>("gid")] << ";\n" <<
            k.var<value_out_type>("value") << " = " <<
                values_first[k.var<const uint_>("gid")] << ";\n" <<
            "lkeys[lid] = key;\n" <<
            "lvals[lid] = value;\n" <<
        "}\n" <<

        // Hillis/Steele scan
        k.decl<value_out_type>("result") << " = value;\n" <<
        k.decl<uint_>("other_key") << ";\n" <<
        k.decl<value_out_type>("other_value") << ";\n" <<

        "for(" << k.decl<uint_>("offset") << " = 1; " <<
                 "offset < wg_size ; offset *= 2){\n"
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    if(lid >= offset) {\n" <<
        "        other_key = lkeys[lid - offset];\n" <<
        "        if(other_key == key){\n" <<
        "            other_value = lvals[lid - offset];\n" <<
        "            result = " << function(k.var<value_out_type>("result"),
                                            k.var<value_out_type>("other_value")) << ";\n" <<
        "        }\n" <<
        "    }\n" <<
        "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "    lvals[lid] = result;\n" <<
        "}\n" <<

        "if(gid >= count) {\n return;\n};\n" <<

        k.decl<const bool>("save") << " = (gid < (count - 1)) ?"
                                   << new_keys_first[k.var<const uint_>("gid + 1")] << " != key" <<
                                   ": true;\n" <<

        // Add carry in
        k.decl<uint_>("carry_in_key") << ";\n" <<
        "if(group_id > 0 && save) {\n" <<
        "    carry_in_key = " << carry_in_keys_first[k.var<const uint_>("group_id - 1")] << ";\n" <<
        "    if(key == carry_in_key){\n" <<
        "        other_value = " << carry_in_values_first[k.var<const uint_>("group_id - 1")] << ";\n" <<
        "        result = " << function(k.var<value_out_type>("result"),
                                        k.var<value_out_type>("other_value")) << ";\n" <<
        "    }\n" <<
        "}\n" <<

        // Save result only if the next key is different or it's the last element.
        "if(save){\n" <<
        keys_result[k.var<uint_>("key")] << " = " << keys_first[k.var<const uint_>("gid")] << ";\n" <<
        values_result[k.var<uint_>("key")] << " = result;\n" <<
        "}\n"
        ;

    size_t work_groups_no = static_cast<size_t>(
        std::ceil(float(count) / work_group_size)
    );

    const context &context = queue.get_context();
    kernel kernel = k.compile(context);
    kernel.set_arg(local_keys_arg, local_buffer<uint_>(work_group_size));
    kernel.set_arg(local_vals_arg, local_buffer<value_out_type>(work_group_size));

    queue.enqueue_1d_range_kernel(kernel,
                                  0,
                                  work_groups_no * work_group_size,
                                  work_group_size);
}

/// \internal_
/// Returns preferred work group size for reduce by key with scan algorithm.
template<class KeyType, class ValueType>
inline size_t get_work_group_size(const device& device)
{
    std::string cache_key = std::string("__boost_reduce_by_key_with_scan")
        + "k_" + type_name<KeyType>() + "_v_" + type_name<ValueType>();

    // load parameters
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    return (std::max)(
        static_cast<size_t>(parameters->get(cache_key, "wgsize", 256)),
        static_cast<size_t>(device.get_info<CL_DEVICE_MAX_WORK_GROUP_SIZE>())
    );
}

/// \internal_
///
/// 1. For each work group carry-out value is calculated (it's done by key-oriented
/// Hillis/Steele scan). Carry-out is a pair of the last key processed by work
/// group and sum of all values under this key in work group.
/// 2. From every carry-out carry-in is calculated by performing inclusive scan
/// by key.
/// 3. Final reduction by key is performed (key-oriented Hillis/Steele scan),
/// carry-in values are added where needed.
template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator,
         class BinaryFunction, class BinaryPredicate>
inline size_t reduce_by_key_with_scan(InputKeyIterator keys_first,
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
        std::iterator_traits<OutputValueIterator>::value_type value_out_type;

    const context &context = queue.get_context();
    size_t count = detail::iterator_range_size(keys_first, keys_last);

    if(count == 0){
        return size_t(0);
    }

    const device &device = queue.get_device();
    size_t work_group_size = get_work_group_size<value_type, key_type>(device);

    // Replace original key with unsigned integer keys generated based on given
    // predicate. New key is also an index for keys_result and values_result vectors,
    // which points to place where reduced value should be saved.
    vector<uint_> new_keys(count, context);
    vector<uint_>::iterator new_keys_first = new_keys.begin();
    generate_uint_keys(keys_first, count, predicate, new_keys_first,
                       work_group_size, queue);

    // Calculate carry-out and carry-in vectors size
    const size_t carry_out_size = static_cast<size_t>(
           std::ceil(float(count) / work_group_size)
    );
    vector<uint_> carry_out_keys(carry_out_size, context);
    vector<value_out_type> carry_out_values(carry_out_size, context);
    carry_outs(new_keys_first, values_first, count, carry_out_keys.begin(),
               carry_out_values.begin(), function, work_group_size, queue);

    vector<value_out_type> carry_in_values(carry_out_size, context);
    carry_ins(carry_out_keys.begin(), carry_out_values.begin(),
              carry_in_values.begin(), carry_out_size, function, work_group_size,
              queue);

    final_reduction(keys_first, values_first, keys_result, values_result,
                    count, function, new_keys_first, carry_out_keys.begin(),
                    carry_in_values.begin(), carry_out_size, work_group_size,
                    queue);

    const size_t result = read_single_value<uint_>(new_keys.get_buffer(),
                                                   count - 1, queue);
    return result + 1;
}

/// \internal_
/// Return true if requirements for running reduce by key with scan on given
/// device are met (at least one work group of preferred size can be run).
template<class InputKeyIterator, class InputValueIterator,
         class OutputKeyIterator, class OutputValueIterator>
bool reduce_by_key_with_scan_requirements_met(InputKeyIterator keys_first,
                                              InputValueIterator values_first,
                                              OutputKeyIterator keys_result,
                                              OutputValueIterator values_result,
                                              const size_t count,
                                              command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputValueIterator>::value_type value_type;
    typedef typename
        std::iterator_traits<InputKeyIterator>::value_type key_type;
    typedef typename
        std::iterator_traits<OutputValueIterator>::value_type value_out_type;

    (void) keys_first;
    (void) values_first;
    (void) keys_result;
    (void) values_result;

    const device &device = queue.get_device();
    // device must have dedicated local memory storage
    if(device.get_info<CL_DEVICE_LOCAL_MEM_TYPE>() != CL_LOCAL)
    {
        return false;
    }

    // local memory size in bytes (per compute unit)
    const size_t local_mem_size = device.get_info<CL_DEVICE_LOCAL_MEM_SIZE>();

    // preferred work group size
    size_t work_group_size = get_work_group_size<key_type, value_type>(device);

    // local memory size needed to perform parallel reduction
    size_t required_local_mem_size = 0;
    // keys size
    required_local_mem_size += sizeof(uint_) * work_group_size;
    // reduced values size
    required_local_mem_size += sizeof(value_out_type) * work_group_size;

    return (required_local_mem_size <= local_mem_size);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_REDUCE_BY_KEY_WITH_SCAN_HPP
