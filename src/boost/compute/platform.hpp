//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_PLATFORM_HPP
#define BOOST_COMPUTE_PLATFORM_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <boost/compute/cl.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/detail/get_object_info.hpp>

namespace boost {
namespace compute {

/// \class platform
/// \brief A compute platform.
///
/// The platform class provides an interface to an OpenCL platform.
///
/// To obtain a list of all platforms on the system use the
/// system::platforms() method.
///
/// \see device, context
class platform
{
public:
    /// Creates a new platform object for \p id.
    explicit platform(cl_platform_id id)
        : m_platform(id)
    {
    }

    /// Creates a new platform as a copy of \p other.
    platform(const platform &other)
        : m_platform(other.m_platform)
    {
    }

    /// Copies the platform id from \p other.
    platform& operator=(const platform &other)
    {
        if(this != &other){
            m_platform = other.m_platform;
        }

        return *this;
    }

    /// Destroys the platform object.
    ~platform()
    {
    }

    /// Returns the ID of the platform.
    cl_platform_id id() const
    {
        return m_platform;
    }

    /// Returns the name of the platform.
    std::string name() const
    {
        return get_info<std::string>(CL_PLATFORM_NAME);
    }

    /// Returns the name of the vendor for the platform.
    std::string vendor() const
    {
        return get_info<std::string>(CL_PLATFORM_VENDOR);
    }

    /// Returns the profile string for the platform.
    std::string profile() const
    {
        return get_info<std::string>(CL_PLATFORM_PROFILE);
    }

    /// Returns the version string for the platform.
    std::string version() const
    {
        return get_info<std::string>(CL_PLATFORM_VERSION);
    }

    /// Returns a list of extensions supported by the platform.
    std::vector<std::string> extensions() const
    {
        std::string extensions_string =
            get_info<std::string>(CL_PLATFORM_EXTENSIONS);
        std::vector<std::string> extensions_vector;
        boost::split(extensions_vector,
                     extensions_string,
                     boost::is_any_of("\t "),
                     boost::token_compress_on);
        return extensions_vector;
    }

    /// Returns \c true if the platform supports the extension with
    /// \p name.
    bool supports_extension(const std::string &name) const
    {
        const std::vector<std::string> extensions = this->extensions();

        return std::find(
            extensions.begin(), extensions.end(), name) != extensions.end();
    }

    /// Returns a list of devices on the platform.
    std::vector<device> devices(cl_device_type type = CL_DEVICE_TYPE_ALL) const
    {
        size_t count = device_count(type);
        if(count == 0){
            // no devices for this platform
            return std::vector<device>();
        }

        std::vector<cl_device_id> device_ids(count);
        cl_int ret = clGetDeviceIDs(m_platform,
                                    type,
                                    static_cast<cl_uint>(count),
                                    &device_ids[0],
                                    0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        std::vector<device> devices;
        for(cl_uint i = 0; i < count; i++){
            devices.push_back(device(device_ids[i]));
        }

        return devices;
    }

    /// Returns the number of devices on the platform.
    size_t device_count(cl_device_type type = CL_DEVICE_TYPE_ALL) const
    {
        cl_uint count = 0;
        cl_int ret = clGetDeviceIDs(m_platform, type, 0, 0, &count);
        if(ret != CL_SUCCESS){
            if(ret == CL_DEVICE_NOT_FOUND){
                // no devices for this platform
                return 0;
            }
            else {
                // something else went wrong
                BOOST_THROW_EXCEPTION(opencl_error(ret));
            }
        }

        return count;
    }

    /// Returns information about the platform.
    ///
    /// \see_opencl_ref{clGetPlatformInfo}
    template<class T>
    T get_info(cl_platform_info info) const
    {
        return detail::get_object_info<T>(clGetPlatformInfo, m_platform, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<platform, Enum>::type
    get_info() const;

    /// Returns the address of the \p function_name extension
    /// function. Returns \c 0 if \p function_name is invalid.
    void* get_extension_function_address(const char *function_name) const
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        return clGetExtensionFunctionAddressForPlatform(m_platform,
                                                        function_name);
        #else
        return clGetExtensionFunctionAddress(function_name);
        #endif
    }

    /// Requests that the platform unload any compiler resources.
    void unload_compiler()
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        clUnloadPlatformCompiler(m_platform);
        #else
        clUnloadCompiler();
        #endif
    }

    /// Returns \c true if the platform is the same at \p other.
    bool operator==(const platform &other) const
    {
        return m_platform == other.m_platform;
    }

    /// Returns \c true if the platform is different from \p other.
    bool operator!=(const platform &other) const
    {
        return m_platform != other.m_platform;
    }

    /// Returns \c true if the platform OpenCL version is major.minor
    /// or newer; otherwise returns \c false.
    bool check_version(int major, int minor) const
    {
        std::stringstream stream;
        stream << version();

        int actual_major, actual_minor;
        stream.ignore(7); // 'OpenCL '
        stream >> actual_major;
        stream.ignore(1); // '.'
        stream >> actual_minor;

        return actual_major > major ||
               (actual_major == major && actual_minor >= minor);
    }

private:
    cl_platform_id m_platform;
};

/// \internal_ define get_info() specializations for platform
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(platform,
    ((std::string, CL_PLATFORM_PROFILE))
    ((std::string, CL_PLATFORM_VERSION))
    ((std::string, CL_PLATFORM_NAME))
    ((std::string, CL_PLATFORM_VENDOR))
    ((std::string, CL_PLATFORM_EXTENSIONS))
)

#ifdef BOOST_COMPUTE_CL_VERSION_2_1
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(platform,
    ((cl_ulong, CL_PLATFORM_HOST_TIMER_RESOLUTION))
)
#endif // BOOST_COMPUTE_CL_VERSION_2_1

inline boost::compute::platform device::platform() const
{
    return boost::compute::platform(get_info<CL_DEVICE_PLATFORM>());
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_PLATFORM_HPP
