//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_IMAGE_IMAGE_SAMPLER_HPP
#define BOOST_COMPUTE_IMAGE_IMAGE_SAMPLER_HPP

#include <boost/throw_exception.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/detail/get_object_info.hpp>
#include <boost/compute/detail/assert_cl_success.hpp>
#include <boost/compute/exception/opencl_error.hpp>
#include <boost/compute/type_traits/type_name.hpp>

namespace boost {
namespace compute {

/// \class image_sampler
/// \brief An OpenCL image sampler object
///
/// \see image2d, image_format
class image_sampler
{
public:
    enum addressing_mode {
        none = CL_ADDRESS_NONE,
        clamp_to_edge = CL_ADDRESS_CLAMP_TO_EDGE,
        clamp = CL_ADDRESS_CLAMP,
        repeat = CL_ADDRESS_REPEAT
    };

    enum filter_mode {
        nearest = CL_FILTER_NEAREST,
        linear = CL_FILTER_LINEAR
    };

    image_sampler()
        : m_sampler(0)
    {
    }

    image_sampler(const context &context,
                  bool normalized_coords,
                  cl_addressing_mode addressing_mode,
                  cl_filter_mode filter_mode)
    {
        cl_int error = 0;

        #ifdef BOOST_COMPUTE_CL_VERSION_2_0
        std::vector<cl_sampler_properties> sampler_properties;
        sampler_properties.push_back(CL_SAMPLER_NORMALIZED_COORDS);
        sampler_properties.push_back(cl_sampler_properties(normalized_coords));
        sampler_properties.push_back(CL_SAMPLER_ADDRESSING_MODE);
        sampler_properties.push_back(cl_sampler_properties(addressing_mode));
        sampler_properties.push_back(CL_SAMPLER_FILTER_MODE);
        sampler_properties.push_back(cl_sampler_properties(filter_mode));
        sampler_properties.push_back(cl_sampler_properties(0));

        m_sampler = clCreateSamplerWithProperties(
            context, &sampler_properties[0], &error
        );
        #else
        m_sampler = clCreateSampler(
            context, normalized_coords, addressing_mode, filter_mode, &error
        );
        #endif

        if(!m_sampler){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    explicit image_sampler(cl_sampler sampler, bool retain = true)
        : m_sampler(sampler)
    {
        if(m_sampler && retain){
            clRetainSampler(m_sampler);
        }
    }

    /// Creates a new image sampler object as a copy of \p other.
    image_sampler(const image_sampler &other)
        : m_sampler(other.m_sampler)
    {
        if(m_sampler){
            clRetainSampler(m_sampler);
        }
    }

    /// Copies the image sampler object from \p other to \c *this.
    image_sampler& operator=(const image_sampler &other)
    {
        if(this != &other){
            if(m_sampler){
                clReleaseSampler(m_sampler);
            }

            m_sampler = other.m_sampler;

            if(m_sampler){
                clRetainSampler(m_sampler);
            }
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    image_sampler(image_sampler&& other) BOOST_NOEXCEPT
        : m_sampler(other.m_sampler)
    {
        other.m_sampler = 0;
    }

    image_sampler& operator=(image_sampler&& other) BOOST_NOEXCEPT
    {
        if(m_sampler){
            clReleaseSampler(m_sampler);
        }

        m_sampler = other.m_sampler;
        other.m_sampler = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the image sampler object.
    ~image_sampler()
    {
        if(m_sampler){
            BOOST_COMPUTE_ASSERT_CL_SUCCESS(
                clReleaseSampler(m_sampler)
            );
        }
    }

    /// Returns the underlying \c cl_sampler object.
    cl_sampler& get() const
    {
        return const_cast<cl_sampler &>(m_sampler);
    }

    /// Returns the context for the image sampler object.
    context get_context() const
    {
        return context(get_info<cl_context>(CL_SAMPLER_CONTEXT));
    }

    /// Returns information about the sampler.
    ///
    /// \see_opencl_ref{clGetSamplerInfo}
    template<class T>
    T get_info(cl_sampler_info info) const
    {
        return detail::get_object_info<T>(clGetSamplerInfo, m_sampler, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<image_sampler, Enum>::type
    get_info() const;

    /// Returns \c true if the sampler is the same at \p other.
    bool operator==(const image_sampler &other) const
    {
        return m_sampler == other.m_sampler;
    }

    /// Returns \c true if the sampler is different from \p other.
    bool operator!=(const image_sampler &other) const
    {
        return m_sampler != other.m_sampler;
    }

    operator cl_sampler() const
    {
        return m_sampler;
    }

private:
    cl_sampler m_sampler;
};

/// \internal_ define get_info() specializations for image_sampler
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(image_sampler,
    ((cl_uint, CL_SAMPLER_REFERENCE_COUNT))
    ((cl_context, CL_SAMPLER_CONTEXT))
    ((cl_addressing_mode, CL_SAMPLER_ADDRESSING_MODE))
    ((cl_filter_mode, CL_SAMPLER_FILTER_MODE))
    ((bool, CL_SAMPLER_NORMALIZED_COORDS))
)

namespace detail {

// set_kernel_arg specialization for image samplers
template<>
struct set_kernel_arg<image_sampler>
{
    void operator()(kernel &kernel_, size_t index, const image_sampler &sampler)
    {
        kernel_.set_arg(index, sampler.get());
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

BOOST_COMPUTE_TYPE_NAME(boost::compute::image_sampler, sampler_t)

#endif // BOOST_COMPUTE_IMAGE_IMAGE_SAMPLER_HPP
