//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_SCAN_ON_GPU_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_SCAN_ON_GPU_HPP

#include <boost/compute/kernel.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/memory/local_buffer.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class OutputIterator, class BinaryOperator>
class local_scan_kernel : public meta_kernel
{
public:
    local_scan_kernel(InputIterator first,
                      InputIterator last,
                      OutputIterator result,
                      bool exclusive,
                      BinaryOperator op)
        : meta_kernel("local_scan")
    {
        typedef typename std::iterator_traits<InputIterator>::value_type T;

        (void) last;

        bool checked = true;

        m_block_sums_arg = add_arg<T *>(memory_object::global_memory, "block_sums");
        m_scratch_arg = add_arg<T *>(memory_object::local_memory, "scratch");
        m_block_size_arg = add_arg<const cl_uint>("block_size");
        m_count_arg = add_arg<const cl_uint>("count");
        m_init_value_arg = add_arg<const T>("init");

        // work-item parameters
        *this <<
            "const uint gid = get_global_id(0);\n" <<
            "const uint lid = get_local_id(0);\n";

        // check against data size
        if(checked){
            *this <<
                "if(gid < count){\n";
        }

        // copy values from input to local memory
        if(exclusive){
            *this <<
                decl<const T>("local_init") << "= (gid == 0) ? init : 0;\n" <<
                "if(lid == 0){ scratch[lid] = local_init; }\n" <<
                "else { scratch[lid] = " << first[expr<cl_uint>("gid-1")] << "; }\n";
        }
        else{
            *this <<
                "scratch[lid] = " << first[expr<cl_uint>("gid")] << ";\n";
        }

        if(checked){
            *this <<
                "}\n"
                "else {\n" <<
                "    scratch[lid] = 0;\n" <<
                "}\n";
        }

        // wait for all threads to read from input
        *this <<
            "barrier(CLK_LOCAL_MEM_FENCE);\n";

        // perform scan
        *this <<
            "for(uint i = 1; i < block_size; i <<= 1){\n" <<
            "    " << decl<const T>("x") << " = lid >= i ? scratch[lid-i] : 0;\n" <<
            "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
            "    if(lid >= i){\n" <<
            "        scratch[lid] = " << op(var<T>("scratch[lid]"), var<T>("x")) << ";\n" <<
            "    }\n" <<
            "    barrier(CLK_LOCAL_MEM_FENCE);\n" <<
            "}\n";

        // copy results to output
        if(checked){
            *this <<
                "if(gid < count){\n";
        }

        *this <<
            result[expr<cl_uint>("gid")] << " = scratch[lid];\n";

        if(checked){
            *this << "}\n";
        }

        // store sum for the block
        if(exclusive){
            *this <<
                "if(lid == block_size - 1 && gid < count) {\n" <<
                "    block_sums[get_group_id(0)] = " <<
                       op(first[expr<cl_uint>("gid")], var<T>("scratch[lid]")) <<
                       ";\n" <<
                "}\n";
        }
        else {
            *this <<
                "if(lid == block_size - 1){\n" <<
                "    block_sums[get_group_id(0)] = scratch[lid];\n" <<
                "}\n";
        }
    }

    size_t m_block_sums_arg;
    size_t m_scratch_arg;
    size_t m_block_size_arg;
    size_t m_count_arg;
    size_t m_init_value_arg;
};

template<class T, class BinaryOperator>
class write_scanned_output_kernel : public meta_kernel
{
public:
    write_scanned_output_kernel(BinaryOperator op)
        : meta_kernel("write_scanned_output")
    {
        bool checked = true;

        m_output_arg = add_arg<T *>(memory_object::global_memory, "output");
        m_block_sums_arg = add_arg<const T *>(memory_object::global_memory, "block_sums");
        m_count_arg = add_arg<const cl_uint>("count");

        // work-item parameters
        *this <<
            "const uint gid = get_global_id(0);\n" <<
            "const uint block_id = get_group_id(0);\n";

        // check against data size
        if(checked){
            *this << "if(gid < count){\n";
        }

        // write output
        *this <<
            "output[gid] = " <<
                op(var<T>("block_sums[block_id]"), var<T>("output[gid] ")) << ";\n";

        if(checked){
            *this << "}\n";
        }
    }

    size_t m_output_arg;
    size_t m_block_sums_arg;
    size_t m_count_arg;
};

template<class InputIterator>
inline size_t pick_scan_block_size(InputIterator first, InputIterator last)
{
    size_t count = iterator_range_size(first, last);

    if(count == 0)        { return 0; }
    else if(count <= 1)   { return 1; }
    else if(count <= 2)   { return 2; }
    else if(count <= 4)   { return 4; }
    else if(count <= 8)   { return 8; }
    else if(count <= 16)  { return 16; }
    else if(count <= 32)  { return 32; }
    else if(count <= 64)  { return 64; }
    else if(count <= 128) { return 128; }
    else                  { return 256; }
}

template<class InputIterator, class OutputIterator, class T, class BinaryOperator>
inline OutputIterator scan_impl(InputIterator first,
                                InputIterator last,
                                OutputIterator result,
                                bool exclusive,
                                T init,
                                BinaryOperator op,
                                command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputIterator>::value_type
        input_type;
    typedef typename
        std::iterator_traits<InputIterator>::difference_type
        difference_type;
    typedef typename
        std::iterator_traits<OutputIterator>::value_type
        output_type;

    const context &context = queue.get_context();
    const size_t count = detail::iterator_range_size(first, last);

    size_t block_size = pick_scan_block_size(first, last);
    size_t block_count = count / block_size;

    if(block_count * block_size < count){
        block_count++;
    }

    ::boost::compute::vector<input_type> block_sums(block_count, context);

    // zero block sums
    input_type zero;
    std::memset(&zero, 0, sizeof(input_type));
    ::boost::compute::fill(block_sums.begin(), block_sums.end(), zero, queue);

    // local scan
    local_scan_kernel<InputIterator, OutputIterator, BinaryOperator>
        local_scan_kernel(first, last, result, exclusive, op);

    ::boost::compute::kernel kernel = local_scan_kernel.compile(context);
    kernel.set_arg(local_scan_kernel.m_scratch_arg, local_buffer<input_type>(block_size));
    kernel.set_arg(local_scan_kernel.m_block_sums_arg, block_sums);
    kernel.set_arg(local_scan_kernel.m_block_size_arg, static_cast<cl_uint>(block_size));
    kernel.set_arg(local_scan_kernel.m_count_arg, static_cast<cl_uint>(count));
    kernel.set_arg(local_scan_kernel.m_init_value_arg, static_cast<output_type>(init));

    queue.enqueue_1d_range_kernel(kernel,
                                  0,
                                  block_count * block_size,
                                  block_size);

    // inclusive scan block sums
    if(block_count > 1){
        scan_impl(block_sums.begin(),
                  block_sums.end(),
                  block_sums.begin(),
                  false,
                  init,
                  op,
                  queue
        );
    }

    // add block sums to each block
    if(block_count > 1){
        write_scanned_output_kernel<input_type, BinaryOperator>
            write_output_kernel(op);
        kernel = write_output_kernel.compile(context);
        kernel.set_arg(write_output_kernel.m_output_arg, result.get_buffer());
        kernel.set_arg(write_output_kernel.m_block_sums_arg, block_sums);
        kernel.set_arg(write_output_kernel.m_count_arg, static_cast<cl_uint>(count));

        queue.enqueue_1d_range_kernel(kernel,
                                      block_size,
                                      block_count * block_size,
                                      block_size);
    }

    return result + static_cast<difference_type>(count);
}

template<class InputIterator, class OutputIterator, class T, class BinaryOperator>
inline OutputIterator dispatch_scan(InputIterator first,
                                    InputIterator last,
                                    OutputIterator result,
                                    bool exclusive,
                                    T init,
                                    BinaryOperator op,
                                    command_queue &queue)
{
    return scan_impl(first, last, result, exclusive, init, op, queue);
}

template<class InputIterator, class T, class BinaryOperator>
inline InputIterator dispatch_scan(InputIterator first,
                                   InputIterator last,
                                   InputIterator result,
                                   bool exclusive,
                                   T init,
                                   BinaryOperator op,
                                   command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;

    if(first == result){
        // scan input in-place
        const context &context = queue.get_context();

        // make a temporary copy the input
        size_t count = iterator_range_size(first, last);
        vector<value_type> tmp(count, context);
        copy(first, last, tmp.begin(), queue);

        // scan from temporary values
        return scan_impl(tmp.begin(), tmp.end(), first, exclusive, init, op, queue);
    }
    else {
        // scan input to output
        return scan_impl(first, last, result, exclusive, init, op, queue);
    }
}

template<class InputIterator, class OutputIterator, class T, class BinaryOperator>
inline OutputIterator scan_on_gpu(InputIterator first,
                                  InputIterator last,
                                  OutputIterator result,
                                  bool exclusive,
                                  T init,
                                  BinaryOperator op,
                                  command_queue &queue)
{
    if(first == last){
        return result;
    }

    return dispatch_scan(first, last, result, exclusive, init, op, queue);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_SCAN_ON_GPU_HPP
