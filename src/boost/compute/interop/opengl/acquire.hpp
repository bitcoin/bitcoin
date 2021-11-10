//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_OPENGL_ACQUIRE_HPP
#define BOOST_COMPUTE_INTEROP_OPENGL_ACQUIRE_HPP

#include <boost/compute/command_queue.hpp>
#include <boost/compute/interop/opengl/cl_gl.hpp>
#include <boost/compute/interop/opengl/opengl_buffer.hpp>
#include <boost/compute/types/fundamental.hpp>
#include <boost/compute/utility/wait_list.hpp>

namespace boost {
namespace compute {

/// Enqueues a command to acquire the specified OpenGL memory objects.
///
/// \see_opencl_ref{clEnqueueAcquireGLObjects}
inline event opengl_enqueue_acquire_gl_objects(const uint_ num_objects,
                                              const cl_mem *mem_objects,
                                              command_queue &queue,
                                              const wait_list &events = wait_list())
{
    BOOST_ASSERT(queue != 0);

    event event_;

    cl_int ret = clEnqueueAcquireGLObjects(queue.get(),
                                           num_objects,
                                           mem_objects,
                                           events.size(),
                                           events.get_event_ptr(),
                                           &event_.get());
    if(ret != CL_SUCCESS){
        BOOST_THROW_EXCEPTION(opencl_error(ret));
    }

    return event_;
}

/// Enqueues a command to release the specified OpenGL memory objects.
///
/// \see_opencl_ref{clEnqueueReleaseGLObjects}
inline event opengl_enqueue_release_gl_objects(const uint_ num_objects,
                                              const cl_mem *mem_objects,
                                              command_queue &queue,
                                              const wait_list &events = wait_list())
{
    BOOST_ASSERT(queue != 0);

    event event_;

    cl_int ret = clEnqueueReleaseGLObjects(queue.get(),
                                           num_objects,
                                           mem_objects,
                                           events.size(),
                                           events.get_event_ptr(),
                                           &event_.get());
    if(ret != CL_SUCCESS){
        BOOST_THROW_EXCEPTION(opencl_error(ret));
    }

    return event_;
}

/// Enqueues a command to acquire the specified OpenGL buffer.
///
/// \see_opencl_ref{clEnqueueAcquireGLObjects}
inline event opengl_enqueue_acquire_buffer(const opengl_buffer &buffer,
                                          command_queue &queue,
                                          const wait_list &events = wait_list())
{
    BOOST_ASSERT(buffer.get_context() == queue.get_context());

    return opengl_enqueue_acquire_gl_objects(1, &buffer.get(), queue, events);
}

/// Enqueues a command to release the specified OpenGL buffer.
///
/// \see_opencl_ref{clEnqueueReleaseGLObjects}
inline event opengl_enqueue_release_buffer(const opengl_buffer &buffer,
                                          command_queue &queue,
                                          const wait_list &events = wait_list())
{
    BOOST_ASSERT(buffer.get_context() == queue.get_context());

    return opengl_enqueue_release_gl_objects(1, &buffer.get(), queue, events);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_OPENGL_ACQUIRE_HPP
