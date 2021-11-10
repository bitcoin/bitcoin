//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_OPENGL_CONTEXT_HPP
#define BOOST_COMPUTE_INTEROP_OPENGL_CONTEXT_HPP

#include <boost/throw_exception.hpp>

#include <boost/compute/device.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/exception/unsupported_extension_error.hpp>
#include <boost/compute/interop/opengl/cl_gl.hpp>

#ifdef __APPLE__
#include <OpenCL/cl_gl_ext.h>
#include <OpenGL/OpenGL.h>
#endif

#ifdef __linux__
#include <GL/glx.h>
#endif

namespace boost {
namespace compute {

/// Creates a shared OpenCL/OpenGL context for the currently active
/// OpenGL context.
///
/// Once created, the shared context can be used to create OpenCL memory
/// objects which can interact with OpenGL memory objects (e.g. VBOs).
///
/// \throws unsupported_extension_error if no CL-GL sharing capable devices
///         are found.
inline context opengl_create_shared_context()
{
    // name of the OpenGL sharing extension for the system
#if defined(__APPLE__)
    const char *cl_gl_sharing_extension = "cl_APPLE_gl_sharing";
#else
    const char *cl_gl_sharing_extension = "cl_khr_gl_sharing";
#endif

#if defined(__APPLE__)
    // get OpenGL share group
    CGLContextObj cgl_current_context = CGLGetCurrentContext();
    CGLShareGroupObj cgl_share_group = CGLGetShareGroup(cgl_current_context);

    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties) cgl_share_group,
        0
    };

    cl_int error = 0;
    cl_context cl_gl_context = clCreateContext(properties, 0, 0, 0, 0, &error);
    if(!cl_gl_context){
        BOOST_THROW_EXCEPTION(opencl_error(error));
    }

    return context(cl_gl_context, false);
#else
    typedef cl_int(*GetGLContextInfoKHRFunction)(
        const cl_context_properties*, cl_gl_context_info, size_t, void *, size_t *
    );

    std::vector<platform> platforms = system::platforms();
    for(size_t i = 0; i < platforms.size(); i++){
        const platform &platform = platforms[i];

        // check whether this platform supports OpenCL/OpenGL sharing
        if (!platform.supports_extension(cl_gl_sharing_extension))
          continue;

        // load clGetGLContextInfoKHR() extension function
        GetGLContextInfoKHRFunction GetGLContextInfoKHR =
            reinterpret_cast<GetGLContextInfoKHRFunction>(
                reinterpret_cast<size_t>(
                    platform.get_extension_function_address("clGetGLContextInfoKHR")
                )
            );
        if(!GetGLContextInfoKHR){
            continue;
        }

        // create context properties listing the platform and current OpenGL display
        cl_context_properties properties[] = {
            CL_CONTEXT_PLATFORM, (cl_context_properties) platform.id(),
        #if defined(__linux__)
            CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
            CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
        #elif defined(_WIN32)
            CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
            CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        #endif
            0
        };

        // lookup current OpenCL device for current OpenGL context
        cl_device_id gpu_id;
        cl_int ret = GetGLContextInfoKHR(
            properties,
            CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR,
            sizeof(cl_device_id),
            &gpu_id,
            0
        );
        if(ret != CL_SUCCESS){
            continue;
        }

        // create device object for the GPU and ensure it supports CL-GL sharing
        device gpu(gpu_id, false);
        if(!gpu.supports_extension(cl_gl_sharing_extension)){
            continue;
        }

        // return CL-GL sharing context
        return context(gpu, properties);
    }
#endif

    // no CL-GL sharing capable devices found
    BOOST_THROW_EXCEPTION(
        unsupported_extension_error(cl_gl_sharing_extension)
    );
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_OPENGL_CONTEXT_HPP
