//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_VTK_BOUNDS_HPP
#define BOOST_COMPUTE_INTEROP_VTK_BOUNDS_HPP

#include <vector>
#include <iterator>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy_n.hpp>
#include <boost/compute/algorithm/reduce.hpp>
#include <boost/compute/container/array.hpp>

namespace boost {
namespace compute {

/// Calculates the bounds for the points in the range [\p first, \p last) and
/// stores the result in \p bounds.
///
/// For example, this can be used to implement the GetBounds() method for a
/// vtkMapper subclass.
template<class PointIterator>
inline void vtk_compute_bounds(PointIterator first,
                               PointIterator last,
                               double bounds[6],
                               command_queue &queue = system::default_queue())
{
    typedef typename std::iterator_traits<PointIterator>::value_type T;

    const context &context = queue.get_context();

    // compute min and max point
    array<T, 2> extrema(context);
    reduce(first, last, extrema.begin() + 0, min<T>(), queue);
    reduce(first, last, extrema.begin() + 1, max<T>(), queue);

    // copy results to host buffer
    std::vector<T> buffer(2);
    copy_n(extrema.begin(), 2, buffer.begin(), queue);

    // copy to vtk-style bounds
    bounds[0] = buffer[0][0]; bounds[1] = buffer[1][0];
    bounds[2] = buffer[0][1]; bounds[3] = buffer[1][1];
    bounds[4] = buffer[0][2]; bounds[5] = buffer[1][2];
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_VTK_BOUNDS_HPP
