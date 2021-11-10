//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_IMAGE_IMAGE_OBJECT_HPP
#define BOOST_COMPUTE_IMAGE_IMAGE_OBJECT_HPP

#include <algorithm>
#include <vector>

#include <boost/compute/config.hpp>
#include <boost/compute/memory_object.hpp>
#include <boost/compute/detail/get_object_info.hpp>
#include <boost/compute/image/image_format.hpp>

namespace boost {
namespace compute {

/// \class image_object
/// \brief Base-class for image objects.
///
/// The image_object class is the base-class for image objects on compute
/// devices.
///
/// \see image1d, image2d, image3d
class image_object : public memory_object
{
public:
    image_object()
        : memory_object()
    {
    }

    explicit image_object(cl_mem mem, bool retain = true)
        : memory_object(mem, retain)
    {
    }

    image_object(const image_object &other)
        : memory_object(other)
    {
    }

    image_object& operator=(const image_object &other)
    {
        if(this != &other){
            memory_object::operator=(other);
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    image_object(image_object&& other) BOOST_NOEXCEPT
        : memory_object(std::move(other))
    {
    }

    /// \internal_
    image_object& operator=(image_object&& other) BOOST_NOEXCEPT
    {
        memory_object::operator=(std::move(other));

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the image object.
    ~image_object()
    {
    }

    /// Returns information about the image object.
    ///
    /// \see_opencl_ref{clGetImageInfo}
    template<class T>
    T get_image_info(cl_mem_info info) const
    {
        return detail::get_object_info<T>(clGetImageInfo, m_mem, info);
    }

    /// Returns the format for the image.
    image_format format() const
    {
        return image_format(get_image_info<cl_image_format>(CL_IMAGE_FORMAT));
    }

    /// \internal_ (deprecated)
    image_format get_format() const
    {
        return format();
    }

    /// Returns the width of the image.
    size_t width() const
    {
        return get_image_info<size_t>(CL_IMAGE_WIDTH);
    }

    /// Returns the height of the image.
    ///
    /// For 1D images, this function will return \c 1.
    size_t height() const
    {
        return get_image_info<size_t>(CL_IMAGE_HEIGHT);
    }

    /// Returns the depth of the image.
    ///
    /// For 1D and 2D images, this function will return \c 1.
    size_t depth() const
    {
        return get_image_info<size_t>(CL_IMAGE_DEPTH);
    }

    /// Returns the supported image formats for the \p type in \p context.
    ///
    /// \see_opencl_ref{clGetSupportedImageFormats}
    static std::vector<image_format>
    get_supported_formats(const context &context,
                          cl_mem_object_type type,
                          cl_mem_flags flags = read_write)
    {
        cl_uint count = 0;
        clGetSupportedImageFormats(context, flags, type, 0, 0, &count);

        std::vector<cl_image_format> cl_formats(count);
        clGetSupportedImageFormats(context, flags, type, count, &cl_formats[0], 0);

        std::vector<image_format> formats;
        formats.reserve(count);

        for(cl_uint i = 0; i < count; i++){
            formats.push_back(image_format(cl_formats[i]));
        }

        return formats;
    }

    /// Returns \c true if \p format is a supported image format for
    /// \p type in \p context with \p flags.
    static bool is_supported_format(const image_format &format,
                                    const context &context,
                                    cl_mem_object_type type,
                                    cl_mem_flags flags = read_write)
    {
        const std::vector<image_format> formats =
            get_supported_formats(context, type, flags);

        return std::find(formats.begin(), formats.end(), format) != formats.end();
    }
};

namespace detail {

// set_kernel_arg() specialization for image_object
template<>
struct set_kernel_arg<image_object> : public set_kernel_arg<memory_object> { };

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_IMAGE_IMAGE_OBJECT_HPP
