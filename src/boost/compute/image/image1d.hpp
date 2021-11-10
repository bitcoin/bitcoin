//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_IMAGE_IMAGE1D_HPP
#define BOOST_COMPUTE_IMAGE_IMAGE1D_HPP

#include <boost/throw_exception.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/exception/opencl_error.hpp>
#include <boost/compute/image/image_format.hpp>
#include <boost/compute/image/image_object.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/utility/extents.hpp>

namespace boost {
namespace compute {

// forward declarations
class command_queue;

/// \class image1d
/// \brief An OpenCL 1D image object
///
/// \opencl_version_warning{1,2}
///
/// \see image_format, image2d
class image1d : public image_object
{
public:
    /// Creates a null image1d object.
    image1d()
        : image_object()
    {
    }

    /// Creates a new image1d object.
    ///
    /// \see_opencl_ref{clCreateImage}
    image1d(const context &context,
            size_t image_width,
            const image_format &format,
            cl_mem_flags flags = read_write,
            void *host_ptr = 0)
    {
    #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        cl_image_desc desc;
        desc.image_type = CL_MEM_OBJECT_IMAGE1D;
        desc.image_width = image_width;
        desc.image_height = 1;
        desc.image_depth = 1;
        desc.image_array_size = 0;
        desc.image_row_pitch = 0;
        desc.image_slice_pitch = 0;
        desc.num_mip_levels = 0;
        desc.num_samples = 0;
    #ifdef BOOST_COMPUTE_CL_VERSION_2_0
        desc.mem_object = 0;
    #else
        desc.buffer = 0;
    #endif

        cl_int error = 0;

        m_mem = clCreateImage(
            context, flags, format.get_format_ptr(), &desc, host_ptr, &error
        );

        if(!m_mem){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    #else
        // image1d objects are only supported in OpenCL 1.2 and later
        BOOST_THROW_EXCEPTION(opencl_error(CL_IMAGE_FORMAT_NOT_SUPPORTED));
    #endif
    }

    /// Creates a new image1d as a copy of \p other.
    image1d(const image1d &other)
      : image_object(other)
    {
    }

    /// Copies the image1d from \p other.
    image1d& operator=(const image1d &other)
    {
        image_object::operator=(other);

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new image object from \p other.
    image1d(image1d&& other) BOOST_NOEXCEPT
        : image_object(std::move(other))
    {
    }

    /// Move-assigns the image from \p other to \c *this.
    image1d& operator=(image1d&& other) BOOST_NOEXCEPT
    {
        image_object::operator=(std::move(other));

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the image1d object.
    ~image1d()
    {
    }

    /// Returns the size (width) of the image.
    extents<1> size() const
    {
        extents<1> size;
        size[0] = get_info<size_t>(CL_IMAGE_WIDTH);
        return size;
    }

    /// Returns the origin of the image (\c 0).
    extents<1> origin() const
    {
        return extents<1>();
    }

    /// Returns information about the image.
    ///
    /// \see_opencl_ref{clGetImageInfo}
    template<class T>
    T get_info(cl_image_info info) const
    {
        return get_image_info<T>(info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<image1d, Enum>::type
    get_info() const;

    /// Returns the supported image formats for the context.
    ///
    /// \see_opencl_ref{clGetSupportedImageFormats}
    static std::vector<image_format>
    get_supported_formats(const context &context, cl_mem_flags flags = read_write)
    {
    #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        return image_object::get_supported_formats(context, CL_MEM_OBJECT_IMAGE1D, flags);
    #else
        return std::vector<image_format>();
    #endif
    }

    /// Returns \c true if \p format is a supported 1D image format for
    /// \p context.
    static bool is_supported_format(const image_format &format,
                                    const context &context,
                                    cl_mem_flags flags = read_write)
    {
    #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        return image_object::is_supported_format(
            format, context, CL_MEM_OBJECT_IMAGE1D, flags
        );
    #else
        return false;
    #endif
    }

    /// Creates a new image with a copy of the data in \c *this. Uses \p queue
    /// to perform the copy operation.
    image1d clone(command_queue &queue) const;
};

/// \internal_ define get_info() specializations for image1d
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(image1d,
    ((cl_image_format, CL_IMAGE_FORMAT))
    ((size_t, CL_IMAGE_ELEMENT_SIZE))
    ((size_t, CL_IMAGE_ROW_PITCH))
    ((size_t, CL_IMAGE_SLICE_PITCH))
    ((size_t, CL_IMAGE_WIDTH))
    ((size_t, CL_IMAGE_HEIGHT))
    ((size_t, CL_IMAGE_DEPTH))
)

namespace detail {

// set_kernel_arg() specialization for image1d
template<>
struct set_kernel_arg<image1d> : public set_kernel_arg<image_object> { };

} // end detail namespace
} // end compute namespace
} // end boost namespace

BOOST_COMPUTE_TYPE_NAME(boost::compute::image1d, image1d_t)

#endif // BOOST_COMPUTE_IMAGE_IMAGE1D_HPP
