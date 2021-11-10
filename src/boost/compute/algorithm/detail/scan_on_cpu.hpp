//---------------------------------------------------------------------------//
// Copyright (c) 2016 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_SCAN_ON_CPU_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_SCAN_ON_CPU_HPP

#include <iterator>

#include <boost/compute/device.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/detail/serial_scan.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/parameter_cache.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class OutputIterator, class T, class BinaryOperator>
inline OutputIterator scan_on_cpu(InputIterator first,
                                  InputIterator last,
                                  OutputIterator result,
                                  bool exclusive,
                                  T init,
                                  BinaryOperator op,
                                  command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputIterator>::value_type input_type;
    typedef typename
        std::iterator_traits<OutputIterator>::value_type output_type;

    const context &context = queue.get_context();
    const device &device = queue.get_device();
    const size_t compute_units = queue.get_device().compute_units();

    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    std::string cache_key =
        "__boost_scan_cpu_" + boost::lexical_cast<std::string>(sizeof(T));

    // for inputs smaller than serial_scan_threshold
    // serial_scan algorithm is used
    uint_ serial_scan_threshold =
        parameters->get(cache_key, "serial_scan_threshold", 16384 * sizeof(T));
    serial_scan_threshold =
        (std::max)(serial_scan_threshold, uint_(compute_units));

    size_t count = detail::iterator_range_size(first, last);
    if(count == 0){
        return result;
    }
    else if(count < serial_scan_threshold) {
        return serial_scan(first, last, result, exclusive, init, op, queue);
    }

    buffer block_partial_sums(context, sizeof(output_type) * compute_units );

    // create scan kernel
    meta_kernel k("scan_on_cpu_block_scan");

    // Arguments
    size_t count_arg = k.add_arg<uint_>("count");
    size_t init_arg = k.add_arg<output_type>("initial_value");
    size_t block_partial_sums_arg =
        k.add_arg<output_type *>(memory_object::global_memory, "block_partial_sums");

    k <<
        "uint block = (count + get_global_size(0))/(get_global_size(0) + 1);\n" <<
        "uint index = get_global_id(0) * block;\n" <<
        "uint end = min(count, index + block);\n" <<
        "if(index >= end) return;\n";

    if(!exclusive){
        k <<
            k.decl<output_type>("sum") << " = " <<
                first[k.var<uint_>("index")] << ";\n" <<
            result[k.var<uint_>("index")] << " = sum;\n" <<
            "index++;\n";
    }
    else {
        k <<
            k.decl<output_type>("sum") << ";\n" <<
            "if(index == 0){\n" <<
                "sum = initial_value;\n" <<
            "}\n" <<
            "else {\n" <<
                "sum = " << first[k.var<uint_>("index")] << ";\n" <<
                "index++;\n" <<
            "}\n";
    }

    k <<
        "while(index < end){\n" <<
            // load next value
            k.decl<const input_type>("value") << " = "
                << first[k.var<uint_>("index")] << ";\n";

    if(exclusive){
        k <<
            "if(get_global_id(0) == 0){\n" <<
                result[k.var<uint_>("index")] << " = sum;\n" <<
            "}\n";
    }
    k <<
            "sum = " << op(k.var<output_type>("sum"),
                           k.var<output_type>("value")) << ";\n";

    if(!exclusive){
        k <<
            "if(get_global_id(0) == 0){\n" <<
                result[k.var<uint_>("index")] << " = sum;\n" <<
            "}\n";
    }

    k <<
            "index++;\n" <<
        "}\n" << // end while
        "block_partial_sums[get_global_id(0)] = sum;\n";

    // compile scan kernel
    kernel block_scan_kernel = k.compile(context);

    // setup kernel arguments
    block_scan_kernel.set_arg(count_arg, static_cast<uint_>(count));
    block_scan_kernel.set_arg(init_arg, static_cast<output_type>(init));
    block_scan_kernel.set_arg(block_partial_sums_arg, block_partial_sums);

    // execute the kernel
    size_t global_work_size = compute_units;
    queue.enqueue_1d_range_kernel(block_scan_kernel, 0, global_work_size, 0);

    // scan is done
    if(compute_units < 2) {
        return result + count;
    }

    // final scan kernel
    meta_kernel l("scan_on_cpu_final_scan");

    // Arguments
    count_arg = l.add_arg<uint_>("count");
    block_partial_sums_arg =
        l.add_arg<output_type *>(memory_object::global_memory, "block_partial_sums");

    l <<
        "uint block = (count + get_global_size(0))/(get_global_size(0) + 1);\n" <<
        "uint index = block + get_global_id(0) * block;\n" <<
        "uint end = min(count, index + block);\n" <<
        k.decl<output_type>("sum") << " = block_partial_sums[0];\n" <<
        "for(uint i = 0; i < get_global_id(0); i++) {\n" <<
            "sum = " << op(k.var<output_type>("sum"),
                           k.var<output_type>("block_partial_sums[i + 1]")) << ";\n" <<
        "}\n" <<

        "while(index < end){\n";
    if(exclusive){
        l <<
            l.decl<output_type>("value") << " = "
                << first[k.var<uint_>("index")] << ";\n" <<
            result[k.var<uint_>("index")] << " = sum;\n" <<
            "sum = " << op(k.var<output_type>("sum"),
                           k.var<output_type>("value")) << ";\n";
    }
    else {
        l <<
            "sum = " << op(k.var<output_type>("sum"),
                           first[k.var<uint_>("index")]) << ";\n" <<
            result[k.var<uint_>("index")] << " = sum;\n";
    }
    l <<
            "index++;\n" <<
        "}\n";


    // compile scan kernel
    kernel final_scan_kernel = l.compile(context);

    // setup kernel arguments
    final_scan_kernel.set_arg(count_arg, static_cast<uint_>(count));
    final_scan_kernel.set_arg(block_partial_sums_arg, block_partial_sums);

    // execute the kernel
    global_work_size = compute_units;
    queue.enqueue_1d_range_kernel(final_scan_kernel, 0, global_work_size, 0);

    // return iterator pointing to the end of the result range
    return result + count;
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_SCAN_ON_CPU_HPP
