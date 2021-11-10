//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_IMAGE_IMAGE_FORMAT_HPP
#define BOOST_COMPUTE_IMAGE_IMAGE_FORMAT_HPP

#include <boost/compute/cl.hpp>

namespace boost {
namespace compute {

/// \class image_format
/// \brief A OpenCL image format
///
/// For example, to create a format for a 8-bit RGBA image:
/// \code
/// boost::compute::image_format rgba8(CL_RGBA, CL_UNSIGNED_INT8);
/// \endcode
///
/// After being constructed, image_format objects are usually passed to the
/// constructor of the various image classes (e.g. \ref image2d, \ref image3d)
/// to create an image object on a compute device.
///
/// Image formats supported by a context can be queried with the static
/// get_supported_formats() in each image class. For example:
/// \code
/// std::vector<image_format> formats = image2d::get_supported_formats(ctx);
/// \endcode
///
/// \see image2d
class image_format
{
public:
    enum channel_order {
        r = CL_R,
        a = CL_A,
        intensity = CL_INTENSITY,
        luminance = CL_LUMINANCE,
        rg = CL_RG,
        ra = CL_RA,
        rgb = CL_RGB,
        rgba = CL_RGBA,
        argb = CL_ARGB,
        bgra = CL_BGRA
    };

    enum channel_data_type {
        snorm_int8 = CL_SNORM_INT8,
        snorm_int16 = CL_SNORM_INT16,
        unorm_int8 = CL_UNORM_INT8,
        unorm_int16 = CL_UNORM_INT16,
        unorm_short_565 = CL_UNORM_SHORT_565,
        unorm_short_555 = CL_UNORM_SHORT_555,
        unorm_int_101010 = CL_UNORM_INT_101010,
        signed_int8 = CL_SIGNED_INT8,
        signed_int16 = CL_SIGNED_INT16,
        signed_int32 = CL_SIGNED_INT32,
        unsigned_int8 = CL_UNSIGNED_INT8,
        unsigned_int16 = CL_UNSIGNED_INT16,
        unsigned_int32 = CL_UNSIGNED_INT32,
        float16 = CL_HALF_FLOAT,
        float32 = CL_FLOAT
    };

    /// Creates a new image format object with \p order and \p type.
    explicit image_format(cl_channel_order order, cl_channel_type type)
    {
        m_format.image_channel_order = order;
        m_format.image_channel_data_type = type;
    }

    /// Creates a new image format object from \p format.
    explicit image_format(const cl_image_format &format)
    {
        m_format.image_channel_order = format.image_channel_order;
        m_format.image_channel_data_type = format.image_channel_data_type;
    }

    /// Creates a new image format object as a copy of \p other.
    image_format(const image_format &other)
        : m_format(other.m_format)
    {
    }

    /// Copies the format from \p other to \c *this.
    image_format& operator=(const image_format &other)
    {
        if(this != &other){
            m_format = other.m_format;
        }

        return *this;
    }

    /// Destroys the image format object.
    ~image_format()
    {
    }

    /// Returns a pointer to the \c cl_image_format object.
    const cl_image_format* get_format_ptr() const
    {
        return &m_format;
    }

    /// Returns \c true if \c *this is the same as \p other.
    bool operator==(const image_format &other) const
    {
        return m_format.image_channel_order ==
                   other.m_format.image_channel_order &&
               m_format.image_channel_data_type ==
                   other.m_format.image_channel_data_type;
    }

    /// Returns \c true if \c *this is not the same as \p other.
    bool operator!=(const image_format &other) const
    {
        return !(*this == other);
    }

private:
    cl_image_format m_format;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_IMAGE_IMAGE_FORMAT_HPP
