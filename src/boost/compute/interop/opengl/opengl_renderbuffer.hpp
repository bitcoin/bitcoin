//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_OPENGL_OPENGL_RENDERBUFFER_HPP
#define BOOST_COMPUTE_INTEROP_OPENGL_OPENGL_RENDERBUFFER_HPP

#include <boost/compute/image/image_object.hpp>
#include <boost/compute/interop/opengl/gl.hpp>
#include <boost/compute/interop/opengl/cl_gl.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/utility/extents.hpp>

namespace boost {
namespace compute {

/// \class opengl_renderbuffer
///
/// A OpenCL buffer for accessing an OpenGL renderbuffer object.
class opengl_renderbuffer : public image_object
{
public:
    /// Creates a null OpenGL renderbuffer object.
    opengl_renderbuffer()
        : image_object()
    {
    }

    /// Creates a new OpenGL renderbuffer object for \p mem.
    explicit opengl_renderbuffer(cl_mem mem, bool retain = true)
        : image_object(mem, retain)
    {
    }

    /// Creates a new OpenGL renderbuffer object in \p context for
    /// \p renderbuffer with \p flags.
    ///
    /// \see_opencl_ref{clCreateFromGLRenderbuffer}
    opengl_renderbuffer(const context &context,
                        GLuint renderbuffer,
                        cl_mem_flags flags = read_write)
    {
        cl_int error = 0;

        m_mem = clCreateFromGLRenderbuffer(
            context, flags, renderbuffer, &error
        );

        if(!m_mem){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new OpenGL renderbuffer object as a copy of \p other.
    opengl_renderbuffer(const opengl_renderbuffer &other)
        : image_object(other)
    {
    }

    /// Copies the OpenGL renderbuffer object from \p other.
    opengl_renderbuffer& operator=(const opengl_renderbuffer &other)
    {
        if(this != &other){
            image_object::operator=(other);
        }

        return *this;
    }

    /// Destroys the OpenGL buffer object.
    ~opengl_renderbuffer()
    {
    }

    /// Returns the size (width, height) of the renderbuffer.
    extents<2> size() const
    {
        extents<2> size;
        size[0] = get_image_info<size_t>(CL_IMAGE_WIDTH);
        size[1] = get_image_info<size_t>(CL_IMAGE_HEIGHT);
        return size;
    }

    /// Returns the origin of the renderbuffer (\c 0, \c 0).
    extents<2> origin() const
    {
        return extents<2>();
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

// set_kernel_arg() specialization for opengl_renderbuffer
template<>
struct set_kernel_arg<opengl_renderbuffer> : public set_kernel_arg<image_object> { };

} // end detail namespace
} // end compute namespace
} // end boost namespace

BOOST_COMPUTE_TYPE_NAME(boost::compute::opengl_renderbuffer, image2d_t)

#endif // BOOST_COMPUTE_INTEROP_OPENGL_OPENGL_RENDERBUFFER_HPP
