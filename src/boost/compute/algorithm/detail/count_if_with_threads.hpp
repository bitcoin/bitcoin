//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_COUNT_IF_WITH_THREADS_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_COUNT_IF_WITH_THREADS_HPP

#include <numeric>

#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/container/vector.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class Predicate>
class count_if_with_threads_kernel : meta_kernel
{
public:
    typedef typename
        std::iterator_traits<InputIterator>::value_type
        value_type;

    count_if_with_threads_kernel()
        : meta_kernel("count_if_with_threads")
    {
    }

    void set_args(InputIterator first,
                  InputIterator last,
                  Predicate predicate)

    {
        typedef typename std::iterator_traits<InputIterator>::value_type T;

        m_size = detail::iterator_range_size(first, last);

        m_size_arg = add_arg<const ulong_>("size");
        m_counts_arg = add_arg<ulong_ *>(memory_object::global_memory, "counts");

        *this <<
            // thread parameters
            "const uint gid = get_global_id(0);\n" <<
            "const uint block_size = size / get_global_size(0);\n" <<
            "const uint start = block_size * gid;\n" <<
            "uint end = 0;\n" <<
            "if(gid == get_global_size(0) - 1)\n" <<
            "    end = size;\n" <<
            "else\n" <<
            "    end = block_size * gid + block_size;\n" <<

            // count values
            "uint count = 0;\n" <<
            "for(uint i = start; i < end; i++){\n" <<
                decl<const T>("value") << "="
                    << first[expr<uint_>("i")] << ";\n" <<
                if_(predicate(var<const T>("value"))) << "{\n" <<
                    "count++;\n" <<
                "}\n" <<
            "}\n" <<

            // write count
            "counts[gid] = count;\n";
    }

    size_t exec(command_queue &queue)
    {
        const device &device = queue.get_device();
        const context &context = queue.get_context();

        size_t threads = device.compute_units();

        const size_t minimum_block_size = 2048;
        if(m_size / threads < minimum_block_size){
            threads = static_cast<size_t>(
                          (std::max)(
                              std::ceil(float(m_size) / minimum_block_size),
                              1.0f
                          )
                      );
        }

        // storage for counts
        ::boost::compute::vector<ulong_> counts(threads, context);

        // exec kernel
        set_arg(m_size_arg, static_cast<ulong_>(m_size));
        set_arg(m_counts_arg, counts.get_buffer());
        exec_1d(queue, 0, threads, 1);

        // copy counts to the host
        std::vector<ulong_> host_counts(threads);
        ::boost::compute::copy(counts.begin(), counts.end(), host_counts.begin(), queue);

        // return sum of counts
        return std::accumulate(host_counts.begin(), host_counts.end(), size_t(0));
    }

private:
    size_t m_size;
    size_t m_size_arg;
    size_t m_counts_arg;
};

// counts values that match the predicate using one thread per block. this is
// optimized for cpu-type devices with a small number of compute units.
template<class InputIterator, class Predicate>
inline size_t count_if_with_threads(InputIterator first,
                                    InputIterator last,
                                    Predicate predicate,
                                    command_queue &queue)
{
    count_if_with_threads_kernel<InputIterator, Predicate> kernel;
    kernel.set_args(first, last, predicate);
    return kernel.exec(queue);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_COUNT_IF_WITH_THREADS_HPP
