//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTEXT_HPP
#define BOOST_COMPUTE_CONTEXT_HPP

#include <vector>

#include <boost/throw_exception.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/exception/opencl_error.hpp>
#include <boost/compute/detail/assert_cl_success.hpp>

namespace boost {
namespace compute {

/// \class context
/// \brief A compute context.
///
/// The context class represents a compute context.
///
/// A context object manages a set of OpenCL resources including memory
/// buffers and program objects. Before allocating memory on the device or
/// executing kernels you must set up a context object.
///
/// To create a context for the default device on the system:
/// \code
/// // get the default compute device
/// boost::compute::device gpu = boost::compute::system::default_device();
///
/// // create a context for the device
/// boost::compute::context context(gpu);
/// \endcode
///
/// Once a context is created, memory can be allocated using the buffer class
/// and kernels can be executed using the command_queue class.
///
/// \see device, command_queue
class context
{
public:
    /// Create a null context object.
    context()
        : m_context(0)
    {
    }

    /// Creates a new context for \p device with \p properties.
    ///
    /// \see_opencl_ref{clCreateContext}
    explicit context(const device &device,
                     const cl_context_properties *properties = 0)
    {
        BOOST_ASSERT(device.id() != 0);

        cl_device_id device_id = device.id();

        cl_int error = 0;
        m_context = clCreateContext(properties, 1, &device_id, 0, 0, &error);

        if(!m_context){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new context for \p devices with \p properties.
    ///
    /// \see_opencl_ref{clCreateContext}
    explicit context(const std::vector<device> &devices,
                     const cl_context_properties *properties = 0)
    {
        BOOST_ASSERT(!devices.empty());

        cl_int error = 0;

        m_context = clCreateContext(
            properties,
            static_cast<cl_uint>(devices.size()),
            reinterpret_cast<const cl_device_id *>(&devices[0]),
            0,
            0,
            &error
        );

        if(!m_context){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new context object for \p context. If \p retain is
    /// \c true, the reference count for \p context will be incremented.
    explicit context(cl_context context, bool retain = true)
        : m_context(context)
    {
        if(m_context && retain){
            clRetainContext(m_context);
        }
    }

    /// Creates a new context object as a copy of \p other.
    context(const context &other)
        : m_context(other.m_context)
    {
        if(m_context){
            clRetainContext(m_context);
        }
    }

    /// Copies the context object from \p other to \c *this.
    context& operator=(const context &other)
    {
        if(this != &other){
            if(m_context){
                clReleaseContext(m_context);
            }

            m_context = other.m_context;

            if(m_context){
                clRetainContext(m_context);
            }
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new context object from \p other.
    context(context&& other) BOOST_NOEXCEPT
        : m_context(other.m_context)
    {
        other.m_context = 0;
    }

    /// Move-assigns the context from \p other to \c *this.
    context& operator=(context&& other) BOOST_NOEXCEPT
    {
        if(m_context){
            clReleaseContext(m_context);
        }

        m_context = other.m_context;
        other.m_context = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the context object.
    ~context()
    {
        if(m_context){
            BOOST_COMPUTE_ASSERT_CL_SUCCESS(
                clReleaseContext(m_context)
            );
        }
    }

    /// Returns the underlying OpenCL context.
    cl_context& get() const
    {
        return const_cast<cl_context &>(m_context);
    }

    /// Returns the device for the context. If the context contains multiple
    /// devices, the first is returned.
    device get_device() const
    {
        std::vector<device> devices = get_devices();

        if(devices.empty()) {
            return device();
        }

        return devices.front();
    }

    /// Returns a vector of devices for the context.
    std::vector<device> get_devices() const
    {
        return get_info<std::vector<device> >(CL_CONTEXT_DEVICES);
    }

    /// Returns information about the context.
    ///
    /// \see_opencl_ref{clGetContextInfo}
    template<class T>
    T get_info(cl_context_info info) const
    {
        return detail::get_object_info<T>(clGetContextInfo, m_context, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<context, Enum>::type
    get_info() const;

    /// Returns \c true if the context is the same as \p other.
    bool operator==(const context &other) const
    {
        return m_context == other.m_context;
    }

    /// Returns \c true if the context is different from \p other.
    bool operator!=(const context &other) const
    {
        return m_context != other.m_context;
    }

    /// \internal_
    operator cl_context() const
    {
        return m_context;
    }

private:
    cl_context m_context;
};

/// \internal_ define get_info() specializations for context
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(context,
    ((cl_uint, CL_CONTEXT_REFERENCE_COUNT))
    ((std::vector<cl_device_id>, CL_CONTEXT_DEVICES))
    ((std::vector<cl_context_properties>, CL_CONTEXT_PROPERTIES))
)

#ifdef BOOST_COMPUTE_CL_VERSION_1_1
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(context,
    ((cl_uint, CL_CONTEXT_NUM_DEVICES))
)
#endif // BOOST_COMPUTE_CL_VERSION_1_1

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTEXT_HPP
