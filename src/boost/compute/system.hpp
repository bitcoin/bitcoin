//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_SYSTEM_HPP
#define BOOST_COMPUTE_SYSTEM_HPP

#include <string>
#include <vector>
#include <cstdlib>

#include <boost/throw_exception.hpp>

#include <boost/compute/cl.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/platform.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/getenv.hpp>
#include <boost/compute/exception/no_device_found.hpp>

namespace boost {
namespace compute {

/// \class system
/// \brief Provides access to platforms and devices on the system.
///
/// The system class contains a set of static functions which provide access to
/// the OpenCL platforms and compute devices on the host system.
///
/// The default_device() convenience method automatically selects and returns
/// the "best" compute device for the system following a set of heuristics and
/// environment variables. This simplifies setup of the OpenCL enviornment.
///
/// \see platform, device, context
class system
{
public:
    /// Returns the default compute device for the system.
    ///
    /// The default device is selected based on a set of heuristics and can be
    /// influenced using one of the following environment variables:
    ///
    /// \li \c BOOST_COMPUTE_DEFAULT_DEVICE -
    ///        name of the compute device (e.g. "GTX TITAN")
    /// \li \c BOOST_COMPUTE_DEFAULT_DEVICE_TYPE
    ///        type of the compute device (e.g. "GPU" or "CPU")
    /// \li \c BOOST_COMPUTE_DEFAULT_PLATFORM -
    ///        name of the platform (e.g. "NVIDIA CUDA")
    /// \li \c BOOST_COMPUTE_DEFAULT_VENDOR -
    ///        name of the device vendor (e.g. "NVIDIA")
    /// \li \c BOOST_COMPUTE_DEFAULT_ENFORCE -
    ///        If this is set to "1", then throw a no_device_found() exception
    ///        if any of the above environment variables is set, but a matching
    ///        device was not found.
    ///
    /// The default device is determined once on the first time this function
    /// is called. Calling this function multiple times will always result in
    /// the same device being returned.
    ///
    /// If no OpenCL device is found on the system, a no_device_found exception
    /// is thrown.
    ///
    /// For example, to print the name of the default compute device on the
    /// system:
    /// \code
    /// // get the default compute device
    /// boost::compute::device device = boost::compute::system::default_device();
    ///
    /// // print the name of the device
    /// std::cout << "default device: " << device.name() << std::endl;
    /// \endcode
    static device default_device()
    {
        static device default_device = find_default_device();

        return default_device;
    }

    /// Returns the device with \p name.
    ///
    /// \throws no_device_found if no device with \p name is found.
    static device find_device(const std::string &name)
    {
        const std::vector<device> devices = system::devices();
        for(size_t i = 0; i < devices.size(); i++){
            const device& device = devices[i];

            if(device.name() == name){
                return device;
            }
        }

        BOOST_THROW_EXCEPTION(no_device_found());
    }

    /// Returns a vector containing all of the compute devices on
    /// the system.
    ///
    /// For example, to print out the name of each OpenCL-capable device
    /// available on the system:
    /// \code
    /// for(const auto &device : boost::compute::system::devices()){
    ///     std::cout << device.name() << std::endl;
    /// }
    /// \endcode
    static std::vector<device> devices()
    {
        std::vector<device> devices;

        const std::vector<platform> platforms = system::platforms();
        for(size_t i = 0; i < platforms.size(); i++){
            const std::vector<device> platform_devices = platforms[i].devices();

            devices.insert(
                devices.end(), platform_devices.begin(), platform_devices.end()
            );
        }

        return devices;
    }

    /// Returns the number of compute devices on the system.
    static size_t device_count()
    {
        size_t count = 0;

        const std::vector<platform> platforms = system::platforms();
        for(size_t i = 0; i < platforms.size(); i++){
            count += platforms[i].device_count();
        }

        return count;
    }

    /// Returns the default context for the system.
    ///
    /// The default context is created for the default device on the system
    /// (as returned by default_device()).
    ///
    /// The default context is created once on the first time this function is
    /// called. Calling this function multiple times will always result in the
    /// same context object being returned.
    static context default_context()
    {
        static context default_context(default_device());

        return default_context;
    }

    /// Returns the default command queue for the system.
    static command_queue& default_queue()
    {
        static command_queue queue(default_context(), default_device());

        return queue;
    }

    /// Blocks until all outstanding computations on the default
    /// command queue are complete.
    ///
    /// This is equivalent to:
    /// \code
    /// system::default_queue().finish();
    /// \endcode
    static void finish()
    {
        default_queue().finish();
    }

    /// Returns a vector containing each of the OpenCL platforms on the system.
    ///
    /// For example, to print out the name of each OpenCL platform present on
    /// the system:
    /// \code
    /// for(const auto &platform : boost::compute::system::platforms()){
    ///     std::cout << platform.name() << std::endl;
    /// }
    /// \endcode
    static std::vector<platform> platforms()
    {
        cl_uint count = 0;
        clGetPlatformIDs(0, 0, &count);

        std::vector<platform> platforms;
        if(count > 0)
        {
            std::vector<cl_platform_id> platform_ids(count);
            clGetPlatformIDs(count, &platform_ids[0], 0);

            for(size_t i = 0; i < platform_ids.size(); i++){
                platforms.push_back(platform(platform_ids[i]));
            }
        }
        return platforms;
    }

    /// Returns the number of compute platforms on the system.
    static size_t platform_count()
    {
        cl_uint count = 0;
        clGetPlatformIDs(0, 0, &count);
        return static_cast<size_t>(count);
    }

private:
    /// \internal_
    static device find_default_device()
    {
        // get a list of all devices on the system
        const std::vector<device> devices_ = devices();
        if(devices_.empty()){
            BOOST_THROW_EXCEPTION(no_device_found());
        }

        // check for device from environment variable
        const char *name     = detail::getenv("BOOST_COMPUTE_DEFAULT_DEVICE");
        const char *type     = detail::getenv("BOOST_COMPUTE_DEFAULT_DEVICE_TYPE");
        const char *platform = detail::getenv("BOOST_COMPUTE_DEFAULT_PLATFORM");
        const char *vendor   = detail::getenv("BOOST_COMPUTE_DEFAULT_VENDOR");
        const char *enforce  = detail::getenv("BOOST_COMPUTE_DEFAULT_ENFORCE");

        if(name || type || platform || vendor){
            for(size_t i = 0; i < devices_.size(); i++){
                const device& device = devices_[i];
                if (name && !matches(device.name(), name))
                    continue;

                if (type && matches(std::string("GPU"), type))
                    if (!(device.type() & device::gpu))
                        continue;

                if (type && matches(std::string("CPU"), type))
                    if (!(device.type() & device::cpu))
                        continue;

                if (platform && !matches(device.platform().name(), platform))
                    continue;

                if (vendor && !matches(device.vendor(), vendor))
                    continue;

                return device;
            }

            if(enforce && enforce[0] == '1')
                BOOST_THROW_EXCEPTION(no_device_found());
        }

        // find the first gpu device
        for(size_t i = 0; i < devices_.size(); i++){
            const device& device = devices_[i];

            if(device.type() & device::gpu){
                return device;
            }
        }

        // find the first cpu device
        for(size_t i = 0; i < devices_.size(); i++){
            const device& device = devices_[i];

            if(device.type() & device::cpu){
                return device;
            }
        }

        // return the first device found
        return devices_[0];
    }

    /// \internal_
    static bool matches(const std::string &str, const std::string &pattern)
    {
        return str.find(pattern) != std::string::npos;
    }
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_SYSTEM_HPP
