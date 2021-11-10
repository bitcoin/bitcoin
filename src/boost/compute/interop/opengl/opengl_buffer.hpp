//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_OPENGL_OPENGL_BUFFER_HPP
#define BOOST_COMPUTE_INTEROP_OPENGL_OPENGL_BUFFER_HPP

#include <boost/compute/buffer.hpp>
#include <boost/compute/interop/opengl/gl.hpp>
#include <boost/compute/interop/opengl/cl_gl.hpp>

namespace boost {
namespace compute {

/// \class opengl_buffer
///
/// A OpenCL buffer for accessing an OpenGL memory object.
class opengl_buffer : public buffer
{
public:
    /// Creates a null OpenGL buffer object.
    opengl_buffer()
        : buffer()
    {
    }

    /// Creates a new OpenGL buffer object for \p mem.
    explicit opengl_buffer(cl_mem mem, bool retain = true)
        : buffer(mem, retain)
    {
    }

    /// Creates a new OpenGL buffer object in \p context for \p bufobj
    /// with \p flags.
    ///
    /// \see_opencl_ref{clCreateFromGLBuffer}
    opengl_buffer(const context &context,
                  GLuint bufobj,
                  cl_mem_flags flags = read_write)
    {
        cl_int error = 0;
        m_mem = clCreateFromGLBuffer(context, flags, bufobj, &error);
        if(!m_mem){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new OpenGL buffer object as a copy of \p other.
    opengl_buffer(const opengl_buffer &other)
        : buffer(other)
    {
    }

    /// Copies the OpenGL buffer object from \p other.
    opengl_buffer& operator=(const opengl_buffer &other)
    {
        if(this != &other){
            buffer::operator=(other);
        }

        return *this;
    }

    /// Destroys the OpenGL buffer object.
    ~opengl_buffer()
    {
    }

    /// Returns the OpenGL memory object ID.
    ///
    /// \see_opencl_ref{clGetGLObjectInfo}
    GLuint get_opengl_object() const
    {
        GLuint object = 0;
        clGetGLObjectInfo(m_mem, 0, &object);
        return object;
    }

    /// Returns the OpenGL memory object type.
    ///
    /// \see_opencl_ref{clGetGLObjectInfo}
    cl_gl_object_type get_opengl_type() const
    {
        cl_gl_object_type type;
        clGetGLObjectInfo(m_mem, &type, 0);
        return type;
    }
};

namespace detail {

// set_kernel_arg specialization for opengl_buffer
template<>
struct set_kernel_arg<opengl_buffer> : set_kernel_arg<memory_object> { };

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_OPENGL_OPENGL_BUFFER_HPP
