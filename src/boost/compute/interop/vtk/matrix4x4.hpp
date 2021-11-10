//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_VTK_MATRIX4X4_HPP
#define BOOST_COMPUTE_INTEROP_VTK_MATRIX4X4_HPP

#include <vtkMatrix4x4.h>

#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// Converts a \c vtkMatrix4x4 to a \c float16_.
inline float16_ vtk_matrix4x4_to_float16(const vtkMatrix4x4 *matrix)
{
    float16_ result;

    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            result[i*4+j] = matrix->GetElement(i, j);
        }
    }

    return result;
}

/// Converts a \c vtkMatrix4x4 to a \c double16_;
inline double16_ vtk_matrix4x4_to_double16(const vtkMatrix4x4 *matrix)
{
    double16_ result;
    std::memcpy(&result, matrix->Element, 16 * sizeof(double));
    return result;
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_VTK_MATRIX4X4_HPP
