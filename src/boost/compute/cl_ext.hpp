//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CL_EXT_HPP
#define BOOST_COMPUTE_CL_EXT_HPP

#include "detail/cl_versions.hpp"

#if defined(__APPLE__)
#include <OpenCL/cl_ext.h>
#else
#include <CL/cl_ext.h>
#endif

#endif // BOOST_COMPUTE_CL_EXT_HPP
