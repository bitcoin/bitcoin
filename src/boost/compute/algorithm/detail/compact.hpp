//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_COMPACT_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_COMPACT_HPP

#include <iterator>

#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/system.hpp>

namespace boost {
namespace compute {
namespace detail {

///
/// \brief Compact kernel class
///
/// Subclass of meta_kernel to compact the result of set kernels to
/// get actual sets
///
class compact_kernel : public meta_kernel
{
public:
    unsigned int tile_size;

    compact_kernel() : meta_kernel("compact")
    {
        tile_size = 4;
    }

    template<class InputIterator1, class InputIterator2, class OutputIterator>
    void set_range(InputIterator1 start,
                   InputIterator2 counts_begin,
                   InputIterator2 counts_end,
                   OutputIterator result)
    {
        m_count = iterator_range_size(counts_begin, counts_end) - 1;

        *this <<
            "uint i = get_global_id(0);\n" <<
            "uint count = i*" << tile_size << ";\n" <<
            "for(uint j = " << counts_begin[expr<uint_>("i")] << "; j<" <<
                counts_begin[expr<uint_>("i+1")] << "; j++, count++)\n" <<
            "{\n" <<
                result[expr<uint_>("j")] << " = " << start[expr<uint_>("count")]
                    << ";\n" <<
            "}\n";
    }

    event exec(command_queue &queue)
    {
        if(m_count == 0) {
            return event();
        }

        return exec_1d(queue, 0, m_count);
    }

private:
    size_t m_count;
};

} //end detail namespace
} //end compute namespace
} //end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_COMPACT_HPP
