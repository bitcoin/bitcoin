//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_MERGE_WIH_MERGE_PATH_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_MERGE_WIH_MERGE_PATH_HPP

#include <iterator>

#include <boost/compute/algorithm/detail/merge_path.hpp>
#include <boost/compute/algorithm/fill_n.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/system.hpp>

namespace boost {
namespace compute {
namespace detail {

///
/// \brief Serial merge kernel class
///
/// Subclass of meta_kernel to perform serial merge after tiling
///
class serial_merge_kernel : meta_kernel
{
public:
    unsigned int tile_size;

    serial_merge_kernel() : meta_kernel("merge")
    {
        tile_size = 4;
    }

    template<class InputIterator1, class InputIterator2,
             class InputIterator3, class InputIterator4,
             class OutputIterator, class Compare>
    void set_range(InputIterator1 first1,
                   InputIterator2 first2,
                   InputIterator3 tile_first1,
                   InputIterator3 tile_last1,
                   InputIterator4 tile_first2,
                   OutputIterator result,
                   Compare comp)
    {
        m_count = iterator_range_size(tile_first1, tile_last1) - 1;

        *this <<
        "uint i = get_global_id(0);\n" <<
        "uint start1 = " << tile_first1[expr<uint_>("i")] << ";\n" <<
        "uint end1 = " << tile_first1[expr<uint_>("i+1")] << ";\n" <<
        "uint start2 = " << tile_first2[expr<uint_>("i")] << ";\n" <<
        "uint end2 = " << tile_first2[expr<uint_>("i+1")] << ";\n" <<
        "uint index = i*" << tile_size << ";\n" <<
        "while(start1<end1 && start2<end2)\n" <<
        "{\n" <<
        "   if(!(" << comp(first2[expr<uint_>("start2")],
                            first1[expr<uint_>("start1")]) << "))\n" <<
        "   {\n" <<
                result[expr<uint_>("index")] <<
                    " = " << first1[expr<uint_>("start1")] << ";\n" <<
        "       index++;\n" <<
        "       start1++;\n" <<
        "   }\n" <<
        "   else\n" <<
        "   {\n" <<
                result[expr<uint_>("index")] <<
                    " = " << first2[expr<uint_>("start2")] << ";\n" <<
        "       index++;\n" <<
        "       start2++;\n" <<
        "   }\n" <<
        "}\n" <<
        "while(start1<end1)\n" <<
        "{\n" <<
            result[expr<uint_>("index")] <<
                " = " << first1[expr<uint_>("start1")] << ";\n" <<
        "   index++;\n" <<
        "   start1++;\n" <<
        "}\n" <<
        "while(start2<end2)\n" <<
        "{\n" <<
            result[expr<uint_>("index")] <<
                " = " << first2[expr<uint_>("start2")] << ";\n" <<
        "   index++;\n" <<
        "   start2++;\n" <<
        "}\n";
    }

    template<class InputIterator1, class InputIterator2,
             class InputIterator3, class InputIterator4,
             class OutputIterator>
    void set_range(InputIterator1 first1,
                   InputIterator2 first2,
                   InputIterator3 tile_first1,
                   InputIterator3 tile_last1,
                   InputIterator4 tile_first2,
                   OutputIterator result)
    {
        typedef typename std::iterator_traits<InputIterator1>::value_type value_type;
        ::boost::compute::less<value_type> less_than;
        set_range(first1, first2, tile_first1, tile_last1, tile_first2, result, less_than);
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

///
/// \brief Merge algorithm with merge path
///
/// Merges the sorted values in the range [\p first1, \p last1) with
/// the sorted values in the range [\p first2, last2) and stores the
/// result in the range beginning at \p result
///
/// \param first1 Iterator pointing to start of first set
/// \param last1 Iterator pointing to end of first set
/// \param first2 Iterator pointing to start of second set
/// \param last2 Iterator pointing to end of second set
/// \param result Iterator pointing to start of range in which the result
/// will be stored
/// \param comp Comparator which performs less than function
/// \param queue Queue on which to execute
///
template<class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
inline OutputIterator
merge_with_merge_path(InputIterator1 first1,
                      InputIterator1 last1,
                      InputIterator2 first2,
                      InputIterator2 last2,
                      OutputIterator result,
                      Compare comp,
                      command_queue &queue = system::default_queue())
{
    typedef typename
        std::iterator_traits<OutputIterator>::difference_type result_difference_type;

    size_t tile_size = 1024;

    size_t count1 = iterator_range_size(first1, last1);
    size_t count2 = iterator_range_size(first2, last2);

    vector<uint_> tile_a((count1+count2+tile_size-1)/tile_size+1, queue.get_context());
    vector<uint_> tile_b((count1+count2+tile_size-1)/tile_size+1, queue.get_context());

    // Tile the sets
    merge_path_kernel tiling_kernel;
    tiling_kernel.tile_size = static_cast<unsigned int>(tile_size);
    tiling_kernel.set_range(first1, last1, first2, last2,
                            tile_a.begin()+1, tile_b.begin()+1, comp);
    fill_n(tile_a.begin(), 1, uint_(0), queue);
    fill_n(tile_b.begin(), 1, uint_(0), queue);
    tiling_kernel.exec(queue);

    fill_n(tile_a.end()-1, 1, static_cast<uint_>(count1), queue);
    fill_n(tile_b.end()-1, 1, static_cast<uint_>(count2), queue);

    // Merge
    serial_merge_kernel merge_kernel;
    merge_kernel.tile_size = static_cast<unsigned int>(tile_size);
    merge_kernel.set_range(first1, first2, tile_a.begin(), tile_a.end(),
                           tile_b.begin(), result, comp);

    merge_kernel.exec(queue);

    return result + static_cast<result_difference_type>(count1 + count2);
}

/// \overload
template<class InputIterator1, class InputIterator2, class OutputIterator>
inline OutputIterator
merge_with_merge_path(InputIterator1 first1,
                      InputIterator1 last1,
                      InputIterator2 first2,
                      InputIterator2 last2,
                      OutputIterator result,
                      command_queue &queue = system::default_queue())
{
    typedef typename std::iterator_traits<InputIterator1>::value_type value_type;
    ::boost::compute::less<value_type> less_than;
    return merge_with_merge_path(first1, last1, first2, last2, result, less_than, queue);
}

} //end detail namespace
} //end compute namespace
} //end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_MERGE_WIH_MERGE_PATH_HPP
