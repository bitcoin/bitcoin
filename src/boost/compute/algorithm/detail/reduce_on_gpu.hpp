//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_REDUCE_ON_GPU_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_REDUCE_ON_GPU_HPP

#include <iterator>

#include <boost/compute/utility/source.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/vendor.hpp>
#include <boost/compute/detail/parameter_cache.hpp>
#include <boost/compute/detail/work_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/utility/program_cache.hpp>

namespace boost {
namespace compute {
namespace detail {

/// \internal
/// body reduction inside a warp
template<typename T,bool isNvidiaDevice>
struct ReduceBody
{
    static std::string body()
    {
        std::stringstream k;
        // local reduction
        k << "for(int i = 1; i < TPB; i <<= 1){\n" <<
             "   barrier(CLK_LOCAL_MEM_FENCE);\n"  <<
             "   uint mask = (i << 1) - 1;\n"      <<
             "   if((lid & mask) == 0){\n"         <<
             "       scratch[lid] += scratch[lid+i];\n" <<
             "   }\n" <<
            "}\n";
        return k.str();
    }
};

/// \internal
/// body reduction inside a warp
/// for nvidia device we can use the "unsafe"
/// memory optimisation
template<typename T>
struct ReduceBody<T,true>
{
    static std::string body()
    {
        std::stringstream k;
        // local reduction
        // we use TPB to compile only useful instruction
        // local reduction when size is greater than warp size
        k << "barrier(CLK_LOCAL_MEM_FENCE);\n" <<
        "if(TPB >= 1024){\n" <<
            "if(lid < 512) { sum += scratch[lid + 512]; scratch[lid] = sum;} barrier(CLK_LOCAL_MEM_FENCE);}\n" <<
         "if(TPB >= 512){\n" <<
            "if(lid < 256) { sum += scratch[lid + 256]; scratch[lid] = sum;} barrier(CLK_LOCAL_MEM_FENCE);}\n" <<
         "if(TPB >= 256){\n" <<
            "if(lid < 128) { sum += scratch[lid + 128]; scratch[lid] = sum;} barrier(CLK_LOCAL_MEM_FENCE);}\n" <<
         "if(TPB >= 128){\n" <<
            "if(lid < 64) { sum += scratch[lid + 64]; scratch[lid] = sum;} barrier(CLK_LOCAL_MEM_FENCE);} \n" <<

        // warp reduction
        "if(lid < 32){\n" <<
            // volatile this way we don't need any barrier
            "volatile __local " << type_name<T>() << " *lmem = scratch;\n" <<
            "if(TPB >= 64) { lmem[lid] = sum = sum + lmem[lid+32];} \n" <<
            "if(TPB >= 32) { lmem[lid] = sum = sum + lmem[lid+16];} \n" <<
            "if(TPB >= 16) { lmem[lid] = sum = sum + lmem[lid+ 8];} \n" <<
            "if(TPB >=  8) { lmem[lid] = sum = sum + lmem[lid+ 4];} \n" <<
            "if(TPB >=  4) { lmem[lid] = sum = sum + lmem[lid+ 2];} \n" <<
            "if(TPB >=  2) { lmem[lid] = sum = sum + lmem[lid+ 1];} \n" <<
        "}\n";
        return k.str();
    }
};

template<class InputIterator, class Function>
inline void initial_reduce(InputIterator first,
                           InputIterator last,
                           buffer result,
                           const Function &function,
                           kernel &reduce_kernel,
                           const uint_ vpt,
                           const uint_ tpb,
                           command_queue &queue)
{
    (void) function;
    (void) reduce_kernel;

    typedef typename std::iterator_traits<InputIterator>::value_type Arg;
    typedef typename boost::tr1_result_of<Function(Arg, Arg)>::type T;

    size_t count = std::distance(first, last);
    detail::meta_kernel k("initial_reduce");
    k.add_set_arg<const uint_>("count", uint_(count));
    size_t output_arg = k.add_arg<T *>(memory_object::global_memory, "output");

    k <<
        k.decl<const uint_>("offset") << " = get_group_id(0) * VPT * TPB;\n" <<
        k.decl<const uint_>("lid") << " = get_local_id(0);\n" <<

        "__local " << type_name<T>() << " scratch[TPB];\n" <<

        // private reduction
        k.decl<T>("sum") << " = 0;\n" <<
        "for(uint i = 0; i < VPT; i++){\n" <<
        "    if(offset + lid + i*TPB < count){\n" <<
        "        sum = sum + " << first[k.var<uint_>("offset+lid+i*TPB")] << ";\n" <<
        "    }\n" <<
        "}\n" <<

        "scratch[lid] = sum;\n" <<

        // local reduction
        ReduceBody<T,false>::body() <<

        // write sum to output
        "if(lid == 0){\n" <<
        "    output[get_group_id(0)] = scratch[0];\n" <<
        "}\n";

    const context &context = queue.get_context();
    std::stringstream options;
    options << "-DVPT=" << vpt << " -DTPB=" << tpb;
    kernel generic_reduce_kernel = k.compile(context, options.str());
    generic_reduce_kernel.set_arg(output_arg, result);

    size_t work_size = calculate_work_size(count, vpt, tpb);

    queue.enqueue_1d_range_kernel(generic_reduce_kernel, 0, work_size, tpb);
}

template<class T>
inline void initial_reduce(const buffer_iterator<T> &first,
                           const buffer_iterator<T> &last,
                           const buffer &result,
                           const plus<T> &function,
                           kernel &reduce_kernel,
                           const uint_ vpt,
                           const uint_ tpb,
                           command_queue &queue)
{
    (void) function;

    size_t count = std::distance(first, last);

    reduce_kernel.set_arg(0, first.get_buffer());
    reduce_kernel.set_arg(1, uint_(first.get_index()));
    reduce_kernel.set_arg(2, uint_(count));
    reduce_kernel.set_arg(3, result);
    reduce_kernel.set_arg(4, uint_(0));

    size_t work_size = calculate_work_size(count, vpt, tpb);

    queue.enqueue_1d_range_kernel(reduce_kernel, 0, work_size, tpb);
}

template<class InputIterator, class T, class Function>
inline void reduce_on_gpu(InputIterator first,
                          InputIterator last,
                          buffer_iterator<T> result,
                          Function function,
                          command_queue &queue)
{
    const device &device = queue.get_device();
    const context &context = queue.get_context();

    detail::meta_kernel k("reduce");
    k.add_arg<const T*>(memory_object::global_memory, "input");
    k.add_arg<const uint_>("offset");
    k.add_arg<const uint_>("count");
    k.add_arg<T*>(memory_object::global_memory, "output");
    k.add_arg<const uint_>("output_offset");

    k <<
        k.decl<const uint_>("block_offset") << " = get_group_id(0) * VPT * TPB;\n" <<
        "__global const " << type_name<T>() << " *block = input + offset + block_offset;\n" <<
        k.decl<const uint_>("lid") << " = get_local_id(0);\n" <<

        "__local " << type_name<T>() << " scratch[TPB];\n" <<
        // private reduction
        k.decl<T>("sum") << " = 0;\n" <<
        "for(uint i = 0; i < VPT; i++){\n" <<
        "    if(block_offset + lid + i*TPB < count){\n" <<
        "        sum = sum + block[lid+i*TPB]; \n" <<
        "    }\n" <<
        "}\n" <<

        "scratch[lid] = sum;\n";

    // discrimination on vendor name
    if(is_nvidia_device(device))
        k << ReduceBody<T,true>::body();
    else
        k << ReduceBody<T,false>::body();

    k <<
        // write sum to output
         "if(lid == 0){\n" <<
         "    output[output_offset + get_group_id(0)] = scratch[0];\n" <<
         "}\n";

    std::string cache_key = std::string("__boost_reduce_on_gpu_") + type_name<T>();

    // load parameters
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    uint_ vpt = parameters->get(cache_key, "vpt", 8);
    uint_ tpb = parameters->get(cache_key, "tpb", 128);

    // reduce program compiler flags
    std::stringstream options;
    options << "-DT=" << type_name<T>()
            << " -DVPT=" << vpt
            << " -DTPB=" << tpb;

    // load program
    boost::shared_ptr<program_cache> cache =
        program_cache::get_global_cache(context);

    program reduce_program = cache->get_or_build(
        cache_key, options.str(), k.source(), context
    );

    // create reduce kernel
    kernel reduce_kernel(reduce_program, "reduce");

    size_t count = std::distance(first, last);

    // first pass, reduce from input to ping
    buffer ping(context, std::ceil(float(count) / vpt / tpb) * sizeof(T));
    initial_reduce(first, last, ping, function, reduce_kernel, vpt, tpb, queue);

    // update count after initial reduce
    count = static_cast<size_t>(std::ceil(float(count) / vpt / tpb));

    // middle pass(es), reduce between ping and pong
    const buffer *input_buffer = &ping;
    buffer pong(context, static_cast<size_t>(count / vpt / tpb * sizeof(T)));
    const buffer *output_buffer = &pong;
    if(count > vpt * tpb){
        while(count > vpt * tpb){
            reduce_kernel.set_arg(0, *input_buffer);
            reduce_kernel.set_arg(1, uint_(0));
            reduce_kernel.set_arg(2, uint_(count));
            reduce_kernel.set_arg(3, *output_buffer);
            reduce_kernel.set_arg(4, uint_(0));

            size_t work_size = static_cast<size_t>(std::ceil(float(count) / vpt));
            if(work_size % tpb != 0){
                work_size += tpb - work_size % tpb;
            }
            queue.enqueue_1d_range_kernel(reduce_kernel, 0, work_size, tpb);

            std::swap(input_buffer, output_buffer);
            count = static_cast<size_t>(std::ceil(float(count) / vpt / tpb));
        }
    }

    // final pass, reduce from ping/pong to result
    reduce_kernel.set_arg(0, *input_buffer);
    reduce_kernel.set_arg(1, uint_(0));
    reduce_kernel.set_arg(2, uint_(count));
    reduce_kernel.set_arg(3, result.get_buffer());
    reduce_kernel.set_arg(4, uint_(result.get_index()));

    queue.enqueue_1d_range_kernel(reduce_kernel, 0, tpb, tpb);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_REDUCE_ON_GPU_HPP
