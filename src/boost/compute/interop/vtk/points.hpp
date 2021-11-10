//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_VTK_POINTS_HPP
#define BOOST_COMPUTE_INTEROP_VTK_POINTS_HPP

#include <vector>

#include <vtkPoints.h>

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>

namespace boost {
namespace compute {

/// Copies \p points to \p buffer.
///
/// For example, to copy from a \c vtkPoints object to a \c vector<float4_>:
/// \code
/// vtkPoints *points = ...
/// vector<float4_> vector(points->GetNumberOfPoints(), context);
/// vtk_copy_points_to_buffer(points, vector.begin(), queue);
/// \endcode
template<class PointType>
inline void vtk_copy_points_to_buffer(const vtkPoints *points,
                                      buffer_iterator<PointType> buffer,
                                      command_queue &queue = system::default_queue())
{
    vtkPoints *points_ = const_cast<vtkPoints *>(points);

    // copy points to aligned buffer
    std::vector<PointType> tmp(points_->GetNumberOfPoints());
    for(vtkIdType i = 0; i < points_->GetNumberOfPoints(); i++){
        double *p = points_->GetPoint(i);
        tmp[i] = PointType(p[0], p[1], p[2], 1);
    }

    // copy data to device
    copy(tmp.begin(), tmp.end(), buffer, queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_VTK_POINTS_HPP
