//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_INCLUDES_HPP
#define BOOST_COMPUTE_ALGORITHM_INCLUDES_HPP

#include <iterator>

#include <boost/static_assert.hpp>

#include <boost/compute/algorithm/detail/balanced_path.hpp>
#include <boost/compute/algorithm/fill_n.hpp>
#include <boost/compute/algorithm/find.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/read_write_single_value.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

///
/// \brief Serial includes kernel class
///
/// Subclass of meta_kernel to perform includes operation after tiling
///
class serial_includes_kernel : meta_kernel
{
public:

    serial_includes_kernel() : meta_kernel("includes")
    {

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
        m_count = iterator_range_size(tile_first1, tile_last1) - 1;

        *this <<
        "uint i = get_global_id(0);\n" <<
        "uint start1 = " << tile_first1[expr<uint_>("i")] << ";\n" <<
        "uint end1 = " << tile_first1[expr<uint_>("i+1")] << ";\n" <<
        "uint start2 = " << tile_first2[expr<uint_>("i")] << ";\n" <<
        "uint end2 = " << tile_first2[expr<uint_>("i+1")] << ";\n" <<
        "uint includes = 1;\n" <<
        "while(start1<end1 && start2<end2)\n" <<
        "{\n" <<
        "   if(" << first1[expr<uint_>("start1")] << " == " <<
                    first2[expr<uint_>("start2")] << ")\n" <<
        "   {\n" <<
        "       start1++; start2++;\n" <<
        "   }\n" <<
        "   else if(" << first1[expr<uint_>("start1")] << " < " <<
                        first2[expr<uint_>("start2")] << ")\n" <<
        "       start1++;\n" <<
        "   else\n" <<
        "   {\n" <<
        "       includes = 0;\n" <<
        "       break;\n" <<
        "   }\n" <<
        "}\n" <<
        "if(start2<end2)\n" <<
        "   includes = 0;\n" <<
        result[expr<uint_>("i")] << " = includes;\n";
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

///
/// \brief Includes algorithm
///
/// Finds if the sorted range [first1, last1) includes the sorted
/// range [first2, last2). In other words, it checks if [first1, last1) is
/// a superset of [first2, last2).
///
/// \return True, if [first1, last1) includes [first2, last2). False otherwise.
///
/// \param first1 Iterator pointing to start of first set
/// \param last1 Iterator pointing to end of first set
/// \param first2 Iterator pointing to start of second set
/// \param last2 Iterator pointing to end of second set
/// \param queue Queue on which to execute
///
/// Space complexity: \Omega(distance(\p first1, \p last1) + distance(\p first2, \p last2))
template<class InputIterator1, class InputIterator2>
inline bool includes(InputIterator1 first1,
                    InputIterator1 last1,
                    InputIterator2 first2,
                    InputIterator2 last2,
                    command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator2>::value);

    size_t tile_size = 1024;

    size_t count1 = detail::iterator_range_size(first1, last1);
    size_t count2 = detail::iterator_range_size(first2, last2);

    vector<uint_> tile_a((count1+count2+tile_size-1)/tile_size+1, queue.get_context());
    vector<uint_> tile_b((count1+count2+tile_size-1)/tile_size+1, queue.get_context());

    // Tile the sets
    detail::balanced_path_kernel tiling_kernel;
    tiling_kernel.tile_size = static_cast<unsigned int>(tile_size);
    tiling_kernel.set_range(first1, last1, first2, last2,
                            tile_a.begin()+1, tile_b.begin()+1);
    fill_n(tile_a.begin(), 1, uint_(0), queue);
    fill_n(tile_b.begin(), 1, uint_(0), queue);
    tiling_kernel.exec(queue);

    fill_n(tile_a.end()-1, 1, static_cast<uint_>(count1), queue);
    fill_n(tile_b.end()-1, 1, static_cast<uint_>(count2), queue);

    vector<uint_> result((count1+count2+tile_size-1)/tile_size, queue.get_context());

    // Find individually
    detail::serial_includes_kernel includes_kernel;
    includes_kernel.set_range(first1, first2, tile_a.begin(), tile_a.end(),
                              tile_b.begin(), result.begin());

    includes_kernel.exec(queue);

    return find(result.begin(), result.end(), 0, queue) == result.end();
}

} //end compute namespace
} //end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_SET_UNION_HPP
