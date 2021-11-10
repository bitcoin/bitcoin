//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_FIND_IF_WITH_ATOMICS_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_FIND_IF_WITH_ATOMICS_HPP

#include <iterator>

#include <boost/compute/types.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/detail/scalar.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/parameter_cache.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class UnaryPredicate>
inline InputIterator find_if_with_atomics_one_vpt(InputIterator first,
                                                  InputIterator last,
                                                  UnaryPredicate predicate,
                                                  const size_t count,
                                                  command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

    const context &context = queue.get_context();

    detail::meta_kernel k("find_if");
    size_t index_arg = k.add_arg<int *>(memory_object::global_memory, "index");
    atomic_min<uint_> atomic_min_uint;

    k << k.decl<const uint_>("i") << " = get_global_id(0);\n"
      << k.decl<const value_type>("value") << "="
      <<     first[k.var<const uint_>("i")] << ";\n"
      << "if(" << predicate(k.var<const value_type>("value")) << "){\n"
      << "    " << atomic_min_uint(k.var<uint_ *>("index"), k.var<uint_>("i")) << ";\n"
      << "}\n";

    kernel kernel = k.compile(context);

    scalar<uint_> index(context);
    kernel.set_arg(index_arg, index.get_buffer());

    // initialize index to the last iterator's index
    index.write(static_cast<uint_>(count), queue);
    queue.enqueue_1d_range_kernel(kernel, 0, count, 0);

    // read index and return iterator
    return first + static_cast<difference_type>(index.read(queue));
}

template<class InputIterator, class UnaryPredicate>
inline InputIterator find_if_with_atomics_multiple_vpt(InputIterator first,
                                                       InputIterator last,
                                                       UnaryPredicate predicate,
                                                       const size_t count,
                                                       const size_t vpt,
                                                       command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

    const context &context = queue.get_context();
    const device &device = queue.get_device();

    detail::meta_kernel k("find_if");
    size_t index_arg = k.add_arg<uint_ *>(memory_object::global_memory, "index");
    size_t count_arg = k.add_arg<const uint_>("count");
    size_t vpt_arg = k.add_arg<const uint_>("vpt");
    atomic_min<uint_> atomic_min_uint;

    // for GPUs reads from global memory are coalesced
    if(device.type() & device::gpu) {
        k <<
            k.decl<const uint_>("lsize") << " = get_local_size(0);\n" <<
            k.decl<uint_>("id") << " = get_local_id(0) + get_group_id(0) * lsize * vpt;\n" <<
            k.decl<const uint_>("end") << " = min(" <<
                    "id + (lsize *" << k.var<uint_>("vpt") << ")," <<
                    "count" <<
            ");\n" <<

            // checking if the index is already found
            "__local uint local_index;\n" <<
            "if(get_local_id(0) == 0){\n" <<
            "    local_index = *index;\n " <<
            "};\n" <<
            "barrier(CLK_LOCAL_MEM_FENCE);\n" <<
            "if(local_index < id){\n" <<
            "    return;\n" <<
            "}\n" <<

            "while(id < end){\n" <<
            "    " << k.decl<const value_type>("value") << " = " <<
                      first[k.var<const uint_>("id")] << ";\n"
            "    if(" << predicate(k.var<const value_type>("value")) << "){\n" <<
            "        " << atomic_min_uint(k.var<uint_ *>("index"),
                                          k.var<uint_>("id")) << ";\n" <<
            "        return;\n"
            "    }\n" <<
            "    id+=lsize;\n" <<
            "}\n";
    // for CPUs (and other devices) reads are ordered so the big cache is
    // efficiently used.
    } else {
        k <<
            k.decl<uint_>("id") << " = get_global_id(0) * " << k.var<uint_>("vpt") << ";\n" <<
            k.decl<const uint_>("end") << " = min(" <<
                    "id + " << k.var<uint_>("vpt") << "," <<
                    "count" <<
            ");\n" <<
            "while(id < end && (*index) > id){\n" <<
            "    " << k.decl<const value_type>("value") << " = " <<
                      first[k.var<const uint_>("id")] << ";\n"
            "    if(" << predicate(k.var<const value_type>("value")) << "){\n" <<
            "        " << atomic_min_uint(k.var<uint_ *>("index"),
                                          k.var<uint_>("id")) << ";\n" <<
            "        return;\n" <<
            "    }\n" <<
            "    id++;\n" <<
            "}\n";
    }

    kernel kernel = k.compile(context);

    scalar<uint_> index(context);
    kernel.set_arg(index_arg, index.get_buffer());
    kernel.set_arg(count_arg, static_cast<uint_>(count));
    kernel.set_arg(vpt_arg, static_cast<uint_>(vpt));

    // initialize index to the last iterator's index
    index.write(static_cast<uint_>(count), queue);

    const size_t global_wg_size = static_cast<size_t>(
        std::ceil(float(count) / vpt)
    );
    queue.enqueue_1d_range_kernel(kernel, 0, global_wg_size, 0);

    // read index and return iterator
    return first + static_cast<difference_type>(index.read(queue));
}

// Space complexity: O(1)
template<class InputIterator, class UnaryPredicate>
inline InputIterator find_if_with_atomics(InputIterator first,
                                          InputIterator last,
                                          UnaryPredicate predicate,
                                          command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    size_t count = detail::iterator_range_size(first, last);
    if(count == 0){
        return last;
    }

    const device &device = queue.get_device();

    // load cached parameters
    std::string cache_key = std::string("__boost_find_if_with_atomics_")
        + type_name<value_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    // for relatively small inputs on GPUs kernel checking one value per thread
    // (work-item) is more efficient than its multiple values per thread version
    if(device.type() & device::gpu){
        const size_t one_vpt_threshold =
            parameters->get(cache_key, "one_vpt_threshold", 1048576);
        if(count <= one_vpt_threshold){
            return find_if_with_atomics_one_vpt(
                first, last, predicate, count, queue
            );
        }
    }

    // values per thread
    size_t vpt;
    if(device.type() & device::gpu){
        // get vpt parameter
        vpt = parameters->get(cache_key, "vpt", 32);
    } else {
        // for CPUs work is split equally between compute units
        const size_t max_compute_units =
            device.get_info<CL_DEVICE_MAX_COMPUTE_UNITS>();
        vpt = static_cast<size_t>(
            std::ceil(float(count) / max_compute_units)
        );
    }

    return find_if_with_atomics_multiple_vpt(
        first, last, predicate, count, vpt, queue
    );
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_FIND_IF_WITH_ATOMICS_HPP
