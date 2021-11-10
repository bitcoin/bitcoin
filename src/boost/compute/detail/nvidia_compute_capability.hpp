//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_NVIDIA_COMPUTE_CAPABILITY_HPP
#define BOOST_COMPUTE_DETAIL_NVIDIA_COMPUTE_CAPABILITY_HPP

#include <boost/compute/device.hpp>

#ifdef BOOST_COMPUTE_HAVE_HDR_CL_EXT    
  #include <boost/compute/detail/cl_versions.hpp>
  #include <CL/cl_ext.h>
#endif

namespace boost {
namespace compute {
namespace detail {

#ifdef CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV
    #define BOOST_COMPUTE_CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV
#else
    #define BOOST_COMPUTE_CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV 0x4000
#endif

#ifdef CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV
    #define BOOST_COMPUTE_CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV
#else
    #define BOOST_COMPUTE_CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV 0x4001
#endif

inline void get_nvidia_compute_capability(const device &device, int &major, int &minor)
{
    if(!device.supports_extension("cl_nv_device_attribute_query")){
        major = minor = 0;
        return;
    }

    major = device.get_info<uint_>(BOOST_COMPUTE_CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV);
    minor = device.get_info<uint_>(BOOST_COMPUTE_CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV);
}

inline bool check_nvidia_compute_capability(const device &device, int major, int minor)
{
    int actual_major, actual_minor;
    get_nvidia_compute_capability(device, actual_major, actual_minor);

    return actual_major > major ||
           (actual_major == major && actual_minor >= minor);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_NVIDIA_COMPUTE_CAPABILITY_HPP
