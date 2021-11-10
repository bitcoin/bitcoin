//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_OPENCV_OCL_HPP
#define BOOST_COMPUTE_INTEROP_OPENCV_OCL_HPP

#include <opencv2/ocl/ocl.hpp>

#include <boost/compute/buffer.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>

namespace boost {
namespace compute {

context opencv_ocl_get_context()
{
    void *ocl_context = cv::ocl::getoclContext();
    if(!ocl_context){
        return context();
    }

    return context(*(static_cast<cl_context *>(ocl_context)));
}

command_queue opencv_ocl_get_command_queue()
{
    void *ocl_queue = cv::ocl::getoclCommandQueue();
    if(!ocl_queue){
        return command_queue();
    }

    return command_queue(*(static_cast<cl_command_queue *>(ocl_queue)));
}

buffer opencv_ocl_get_buffer(const cv::ocl::oclMat &mat)
{
    return buffer(reinterpret_cast<cl_mem>(mat.data));
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_OPENCV_OCL_HPP
