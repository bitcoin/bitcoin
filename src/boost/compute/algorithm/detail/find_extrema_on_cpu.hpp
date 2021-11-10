//---------------------------------------------------------------------------//
// Copyright (c) 2016 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_FIND_EXTREMA_ON_CPU_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_FIND_EXTREMA_ON_CPU_HPP

#include <algorithm>

#include <boost/compute/algorithm/detail/find_extrema_with_reduce.hpp>
#include <boost/compute/algorithm/detail/find_extrema_with_atomics.hpp>
#include <boost/compute/algorithm/detail/serial_find_extrema.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class Compare>
inline InputIterator find_extrema_on_cpu(InputIterator first,
                                         InputIterator last,
                                         Compare compare,
                                         const bool find_minimum,
                                         command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type input_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;
    size_t count = iterator_range_size(first, last);

    const device &device = queue.get_device();
    const uint_ compute_units = queue.get_device().compute_units();

    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);
    std::string cache_key =
        "__boost_find_extrema_cpu_"
            + boost::lexical_cast<std::string>(sizeof(input_type));

    // for inputs smaller than serial_find_extrema_threshold
    // serial_find_extrema algorithm is used
    uint_ serial_find_extrema_threshold = parameters->get(
        cache_key,
        "serial_find_extrema_threshold",
        16384 * sizeof(input_type)
    );
    serial_find_extrema_threshold =
        (std::max)(serial_find_extrema_threshold, uint_(2 * compute_units));

    const context &context = queue.get_context();
    if(count < serial_find_extrema_threshold) {
        return serial_find_extrema(first, last, compare, find_minimum, queue);
    }

    meta_kernel k("find_extrema_on_cpu");
    buffer output(context, sizeof(input_type) * compute_units);
    buffer output_idx(
        context, sizeof(uint_) * compute_units,
        buffer::read_write | buffer::alloc_host_ptr
    );

    size_t count_arg = k.add_arg<uint_>("count");
    size_t output_arg =
        k.add_arg<input_type *>(memory_object::global_memory, "output");
    size_t output_idx_arg =
        k.add_arg<uint_ *>(memory_object::global_memory, "output_idx");

    k <<
        "uint block = " <<
            "(uint)ceil(((float)count)/get_global_size(0));\n" <<
        "uint index = get_global_id(0) * block;\n" <<
        "uint end = min(count, index + block);\n" <<

        "uint value_index = index;\n" <<
        k.decl<input_type>("value") << " = " << first[k.var<uint_>("index")] << ";\n" <<

        "index++;\n" <<
        "while(index < end){\n" <<
            k.decl<input_type>("candidate") <<
                " = " << first[k.var<uint_>("index")] << ";\n" <<
        "#ifndef BOOST_COMPUTE_FIND_MAXIMUM\n" <<
            "bool compare = " << compare(k.var<input_type>("candidate"),
                                         k.var<input_type>("value")) << ";\n" <<
        "#else\n" <<
            "bool compare = " << compare(k.var<input_type>("value"),
                                         k.var<input_type>("candidate")) << ";\n" <<
        "#endif\n" <<
            "value = compare ? candidate : value;\n" <<
            "value_index = compare ? index : value_index;\n" <<
            "index++;\n" <<
        "}\n" <<
        "output[get_global_id(0)] = value;\n" <<
        "output_idx[get_global_id(0)] = value_index;\n";

    size_t global_work_size = compute_units;
    std::string options;
    if(!find_minimum){
        options = "-DBOOST_COMPUTE_FIND_MAXIMUM";
    }
    kernel kernel = k.compile(context, options);

    kernel.set_arg(count_arg, static_cast<uint_>(count));
    kernel.set_arg(output_arg, output);
    kernel.set_arg(output_idx_arg, output_idx);
    queue.enqueue_1d_range_kernel(kernel, 0, global_work_size, 0);
    
    buffer_iterator<input_type> result = serial_find_extrema(
        make_buffer_iterator<input_type>(output),
        make_buffer_iterator<input_type>(output, global_work_size),
        compare,
        find_minimum,
        queue
    );

    uint_* output_idx_host_ptr =
        static_cast<uint_*>(
            queue.enqueue_map_buffer(
                output_idx, command_queue::map_read,
                0, global_work_size * sizeof(uint_)
            )
        );

    difference_type extremum_idx =
        static_cast<difference_type>(*(output_idx_host_ptr + result.get_index()));
    return first + extremum_idx;
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_FIND_EXTREMA_ON_CPU_HPP
