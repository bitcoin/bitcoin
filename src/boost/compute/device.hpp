//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DEVICE_HPP
#define BOOST_COMPUTE_DEVICE_HPP

#include <algorithm>
#include <string>
#include <vector>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/types/fundamental.hpp>
#include <boost/compute/detail/duration.hpp>
#include <boost/compute/detail/get_object_info.hpp>
#include <boost/compute/detail/assert_cl_success.hpp>

namespace boost {
namespace compute {

class platform;

/// \class device
/// \brief A compute device.
///
/// Typical compute devices include GPUs and multi-core CPUs. A list
/// of all compute devices available on a platform can be obtained
/// via the platform::devices() method.
///
/// The default compute device for the system can be obtained with
/// the system::default_device() method. For example:
///
/// \snippet test/test_device.cpp default_gpu
///
/// \see platform, context, command_queue
class device
{
public:
    enum type {
        cpu = CL_DEVICE_TYPE_CPU,
        gpu = CL_DEVICE_TYPE_GPU,
        accelerator = CL_DEVICE_TYPE_ACCELERATOR
    };

    /// Creates a null device object.
    device()
        : m_id(0)
    {
    }

    /// Creates a new device object for \p id. If \p retain is \c true,
    /// the reference count for the device will be incremented.
    explicit device(cl_device_id id, bool retain = true)
        : m_id(id)
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        if(m_id && retain && is_subdevice()){
            clRetainDevice(m_id);
        }
        #else
        (void) retain;
        #endif
    }

    /// Creates a new device object as a copy of \p other.
    device(const device &other)
        : m_id(other.m_id)
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        if(m_id && is_subdevice()){
            clRetainDevice(m_id);
        }
        #endif
    }

    /// Copies the device from \p other to \c *this.
    device& operator=(const device &other)
    {
        if(this != &other){
            #ifdef BOOST_COMPUTE_CL_VERSION_1_2
            if(m_id && is_subdevice()){
                clReleaseDevice(m_id);
            }
            #endif

            m_id = other.m_id;

            #ifdef BOOST_COMPUTE_CL_VERSION_1_2
            if(m_id && is_subdevice()){
                clRetainDevice(m_id);
            }
            #endif
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new device object from \p other.
    device(device&& other) BOOST_NOEXCEPT
        : m_id(other.m_id)
    {
        other.m_id = 0;
    }

    /// Move-assigns the device from \p other to \c *this.
    device& operator=(device&& other) BOOST_NOEXCEPT
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        if(m_id && is_subdevice()){
            clReleaseDevice(m_id);
        }
        #endif

        m_id = other.m_id;
        other.m_id = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the device object.
    ~device()
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        if(m_id && is_subdevice()){
            BOOST_COMPUTE_ASSERT_CL_SUCCESS(
                clReleaseDevice(m_id)
            );
        }
        #endif
    }

    /// Returns the ID of the device.
    cl_device_id id() const
    {
        return m_id;
    }

    /// Returns a reference to the underlying OpenCL device id.
    cl_device_id& get() const
    {
        return const_cast<cl_device_id&>(m_id);
    }

    /// Returns the type of the device.
    cl_device_type type() const
    {
        return get_info<cl_device_type>(CL_DEVICE_TYPE);
    }

    #ifdef BOOST_COMPUTE_DOXYGEN_INVOKED
    /// Returns the platform for the device.
    platform platform() const;
    #else
    boost::compute::platform platform() const;
    #endif

    /// Returns the name of the device.
    std::string name() const
    {
        return get_info<std::string>(CL_DEVICE_NAME);
    }

    /// Returns the name of the vendor for the device.
    std::string vendor() const
    {
        return get_info<std::string>(CL_DEVICE_VENDOR);
    }

    /// Returns the device profile string.
    std::string profile() const
    {
        return get_info<std::string>(CL_DEVICE_PROFILE);
    }

    /// Returns the device version string.
    std::string version() const
    {
        return get_info<std::string>(CL_DEVICE_VERSION);
    }

    /// Returns the driver version string.
    std::string driver_version() const
    {
        return get_info<std::string>(CL_DRIVER_VERSION);
    }

    /// Returns a list of extensions supported by the device.
    std::vector<std::string> extensions() const
    {
        std::string extensions_string =
            get_info<std::string>(CL_DEVICE_EXTENSIONS);
        std::vector<std::string> extensions_vector;
        boost::split(extensions_vector,
                     extensions_string,
                     boost::is_any_of("\t "),
                     boost::token_compress_on);
        return extensions_vector;
    }

    /// Returns \c true if the device supports the extension with
    /// \p name.
    bool supports_extension(const std::string &name) const
    {
        const std::vector<std::string> extensions = this->extensions();

        return std::find(
            extensions.begin(), extensions.end(), name) != extensions.end();
    }

    /// Returns the number of address bits.
    uint_ address_bits() const
    {
        return get_info<uint_>(CL_DEVICE_ADDRESS_BITS);
    }

    /// Returns the global memory size in bytes.
    ulong_ global_memory_size() const
    {
        return get_info<ulong_>(CL_DEVICE_GLOBAL_MEM_SIZE);
    }

    /// Returns the local memory size in bytes.
    ulong_ local_memory_size() const
    {
        return get_info<ulong_>(CL_DEVICE_LOCAL_MEM_SIZE);
    }

    /// Returns the clock frequency for the device's compute units.
    uint_ clock_frequency() const
    {
        return get_info<uint_>(CL_DEVICE_MAX_CLOCK_FREQUENCY);
    }

    /// Returns the number of compute units in the device.
    uint_ compute_units() const
    {
        return get_info<uint_>(CL_DEVICE_MAX_COMPUTE_UNITS);
    }

    /// \internal_
    ulong_ max_memory_alloc_size() const
    {
        return get_info<ulong_>(CL_DEVICE_MAX_MEM_ALLOC_SIZE);
    }

    /// \internal_
    size_t max_work_group_size() const
    {
        return get_info<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE);
    }

    /// \internal_
    uint_ max_work_item_dimensions() const
    {
        return get_info<uint_>(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
    }

    /// Returns the preferred vector width for type \c T.
    template<class T>
    uint_ preferred_vector_width() const
    {
        return 0;
    }

    /// Returns the profiling timer resolution in nanoseconds.
    size_t profiling_timer_resolution() const
    {
        return get_info<size_t>(CL_DEVICE_PROFILING_TIMER_RESOLUTION);
    }

    /// Returns \c true if the device is a sub-device.
    bool is_subdevice() const
    {
    #if defined(BOOST_COMPUTE_CL_VERSION_1_2)
        try {
            return get_info<cl_device_id>(CL_DEVICE_PARENT_DEVICE) != 0;
        }
        catch(opencl_error&){
            // the get_info() call above will throw if the device's opencl version
            // is less than 1.2 (in which case it can't be a sub-device).
            return false;
        }
    #else
        return false;
    #endif
    }

    /// Returns information about the device.
    ///
    /// For example, to get the number of compute units:
    /// \code
    /// device.get_info<cl_uint>(CL_DEVICE_MAX_COMPUTE_UNITS);
    /// \endcode
    ///
    /// Alternatively, the template-specialized version can be used which
    /// automatically determines the result type:
    /// \code
    /// device.get_info<CL_DEVICE_MAX_COMPUTE_UNITS>();
    /// \endcode
    ///
    /// \see_opencl_ref{clGetDeviceInfo}
    template<class T>
    T get_info(cl_device_info info) const
    {
        return detail::get_object_info<T>(clGetDeviceInfo, m_id, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<device, Enum>::type
    get_info() const;

    #if defined(BOOST_COMPUTE_CL_VERSION_1_2) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Partitions the device into multiple sub-devices according to
    /// \p properties.
    ///
    /// \opencl_version_warning{1,2}
    std::vector<device>
    partition(const cl_device_partition_property *properties) const
    {
        // get sub-device count
        uint_ count = 0;
        int_ ret = clCreateSubDevices(m_id, properties, 0, 0, &count);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        // get sub-device ids
        std::vector<cl_device_id> ids(count);
        ret = clCreateSubDevices(m_id, properties, count, &ids[0], 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        // convert ids to device objects
        std::vector<device> devices(count);
        for(size_t i = 0; i < count; i++){
            devices[i] = device(ids[i], false);
        }

        return devices;
    }

    /// \opencl_version_warning{1,2}
    std::vector<device> partition_equally(size_t count) const
    {
        cl_device_partition_property properties[] = {
            CL_DEVICE_PARTITION_EQUALLY,
            static_cast<cl_device_partition_property>(count),
            0
        };

        return partition(properties);
    }

    /// \opencl_version_warning{1,2}
    std::vector<device>
    partition_by_counts(const std::vector<size_t> &counts) const
    {
        std::vector<cl_device_partition_property> properties;

        properties.push_back(CL_DEVICE_PARTITION_BY_COUNTS);
        for(size_t i = 0; i < counts.size(); i++){
            properties.push_back(
                static_cast<cl_device_partition_property>(counts[i]));
        }
        properties.push_back(CL_DEVICE_PARTITION_BY_COUNTS_LIST_END);
        properties.push_back(0);

        return partition(&properties[0]);
    }

    /// \opencl_version_warning{1,2}
    std::vector<device>
    partition_by_affinity_domain(cl_device_affinity_domain domain) const
    {
        cl_device_partition_property properties[] = {
            CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN,
            static_cast<cl_device_partition_property>(domain),
            0
        };

        return partition(properties);
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_2

    #if defined(BOOST_COMPUTE_CL_VERSION_2_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Returns the current value of the host clock as seen by device
    /// in nanoseconds.
    ///
    /// \see_opencl21_ref{clGetHostTimer}
    ///
    /// \opencl_version_warning{2,1}
    ulong_ get_host_timer() const
    {
        ulong_ host_timestamp = 0;
        cl_int ret = clGetHostTimer(m_id, &host_timestamp);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
        return host_timestamp;
    }

    /// Returns a reasonably synchronized pair of timestamps from the device timer
    /// and the host timer as seen by device in nanoseconds. The first of returned
    /// std::pair is a device timer timestamp, the second is a host timer timestamp.
    ///
    /// \see_opencl21_ref{clGetDeviceAndHostTimer}
    ///
    /// \opencl_version_warning{2,1}
    std::pair<ulong_, ulong_> get_device_and_host_timer() const
    {
        ulong_ host_timestamp;
        ulong_ device_timestamp;
        cl_int ret = clGetDeviceAndHostTimer(
            m_id, &device_timestamp, &host_timestamp
        );
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
        return std::make_pair(
            device_timestamp, host_timestamp
        );
    }

    #if !defined(BOOST_COMPUTE_NO_HDR_CHRONO) || !defined(BOOST_COMPUTE_NO_BOOST_CHRONO)
    /// Returns the current value of the host clock as seen by device
    /// as duration.
    ///
    /// For example, to print the current value of the host clock as seen by device
    /// in milliseconds:
    /// \code
    /// std::cout << device.get_host_timer<std::chrono::milliseconds>().count() << " ms";
    /// \endcode
    ///
    /// \see_opencl21_ref{clGetHostTimer}
    ///
    /// \opencl_version_warning{2,1}
    template<class Duration>
    Duration get_host_timer() const
    {
        const ulong_ nanoseconds = this->get_host_timer();
        return detail::make_duration_from_nanoseconds(Duration(), nanoseconds);
    }

    /// Returns a reasonably synchronized pair of timestamps from the device timer
    /// and the host timer as seen by device as a std::pair<Duration, Duration> value.
    /// The first of returned std::pair is a device timer timestamp, the second is
    /// a host timer timestamp.
    ///
    /// \see_opencl21_ref{clGetDeviceAndHostTimer}
    ///
    /// \opencl_version_warning{2,1}
    template<class Duration>
    std::pair<Duration, Duration> get_device_and_host_timer() const
    {
        const std::pair<ulong_, ulong_> timestamps = this->get_device_and_host_timer();
        return std::make_pair(
            detail::make_duration_from_nanoseconds(Duration(), timestamps.first),
            detail::make_duration_from_nanoseconds(Duration(), timestamps.second)
        );
    }
    #endif // !defined(BOOST_COMPUTE_NO_HDR_CHRONO) || !defined(BOOST_COMPUTE_NO_BOOST_CHRONO)
    #endif // BOOST_COMPUTE_CL_VERSION_2_1

    /// Returns \c true if the device is the same at \p other.
    bool operator==(const device &other) const
    {
        return m_id == other.m_id;
    }

    /// Returns \c true if the device is different from \p other.
    bool operator!=(const device &other) const
    {
        return m_id != other.m_id;
    }

    /// Returns \c true if the device OpenCL version is major.minor
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
    cl_device_id m_id;
};

/// \internal_
template<>
inline uint_ device::preferred_vector_width<short_>() const
{
    return get_info<uint_>(CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
}

/// \internal_
template<>
inline uint_ device::preferred_vector_width<int_>() const
{
    return get_info<uint_>(CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
}

/// \internal_
template<>
inline uint_ device::preferred_vector_width<long_>() const
{
    return get_info<uint_>(CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
}

/// \internal_
template<>
inline uint_ device::preferred_vector_width<float_>() const
{
    return get_info<uint_>(CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
}

/// \internal_
template<>
inline uint_ device::preferred_vector_width<double_>() const
{
    return get_info<uint_>(CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
}

/// \internal_ define get_info() specializations for device
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((cl_uint, CL_DEVICE_ADDRESS_BITS))
    ((bool, CL_DEVICE_AVAILABLE))
    ((bool, CL_DEVICE_COMPILER_AVAILABLE))
    ((bool, CL_DEVICE_ENDIAN_LITTLE))
    ((bool, CL_DEVICE_ERROR_CORRECTION_SUPPORT))
    ((cl_device_exec_capabilities, CL_DEVICE_EXECUTION_CAPABILITIES))
    ((std::string, CL_DEVICE_EXTENSIONS))
    ((cl_ulong, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE))
    ((cl_device_mem_cache_type, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE))
    ((cl_uint, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE))
    ((cl_ulong, CL_DEVICE_GLOBAL_MEM_SIZE))
    ((bool, CL_DEVICE_IMAGE_SUPPORT))
    ((size_t, CL_DEVICE_IMAGE2D_MAX_HEIGHT))
    ((size_t, CL_DEVICE_IMAGE2D_MAX_WIDTH))
    ((size_t, CL_DEVICE_IMAGE3D_MAX_DEPTH))
    ((size_t, CL_DEVICE_IMAGE3D_MAX_HEIGHT))
    ((size_t, CL_DEVICE_IMAGE3D_MAX_WIDTH))
    ((cl_ulong, CL_DEVICE_LOCAL_MEM_SIZE))
    ((cl_device_local_mem_type, CL_DEVICE_LOCAL_MEM_TYPE))
    ((cl_uint, CL_DEVICE_MAX_CLOCK_FREQUENCY))
    ((cl_uint, CL_DEVICE_MAX_COMPUTE_UNITS))
    ((cl_uint, CL_DEVICE_MAX_CONSTANT_ARGS))
    ((cl_ulong, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE))
    ((cl_ulong, CL_DEVICE_MAX_MEM_ALLOC_SIZE))
    ((size_t, CL_DEVICE_MAX_PARAMETER_SIZE))
    ((cl_uint, CL_DEVICE_MAX_READ_IMAGE_ARGS))
    ((cl_uint, CL_DEVICE_MAX_SAMPLERS))
    ((size_t, CL_DEVICE_MAX_WORK_GROUP_SIZE))
    ((cl_uint, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS))
    ((std::vector<size_t>, CL_DEVICE_MAX_WORK_ITEM_SIZES))
    ((cl_uint, CL_DEVICE_MAX_WRITE_IMAGE_ARGS))
    ((cl_uint, CL_DEVICE_MEM_BASE_ADDR_ALIGN))
    ((cl_uint, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE))
    ((std::string, CL_DEVICE_NAME))
    ((cl_platform_id, CL_DEVICE_PLATFORM))
    ((cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR))
    ((cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT))
    ((cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT))
    ((cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG))
    ((cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT))
    ((cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE))
    ((std::string, CL_DEVICE_PROFILE))
    ((size_t, CL_DEVICE_PROFILING_TIMER_RESOLUTION))
    ((cl_command_queue_properties, CL_DEVICE_QUEUE_PROPERTIES))
    ((cl_device_fp_config, CL_DEVICE_SINGLE_FP_CONFIG))
    ((cl_device_type, CL_DEVICE_TYPE))
    ((std::string, CL_DEVICE_VENDOR))
    ((cl_uint, CL_DEVICE_VENDOR_ID))
    ((std::string, CL_DEVICE_VERSION))
    ((std::string, CL_DRIVER_VERSION))
)

#ifdef CL_DEVICE_DOUBLE_FP_CONFIG
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((cl_device_fp_config, CL_DEVICE_DOUBLE_FP_CONFIG))
)
#endif

#ifdef CL_DEVICE_HALF_FP_CONFIG
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((cl_device_fp_config, CL_DEVICE_HALF_FP_CONFIG))
)
#endif

#ifdef BOOST_COMPUTE_CL_VERSION_1_1
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((bool, CL_DEVICE_HOST_UNIFIED_MEMORY))
    ((cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR))
    ((cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT))
    ((cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT))
    ((cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG))
    ((cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT))
    ((cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE))
    ((std::string, CL_DEVICE_OPENCL_C_VERSION))
)
#endif // BOOST_COMPUTE_CL_VERSION_1_1

#ifdef BOOST_COMPUTE_CL_VERSION_1_2
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((std::string, CL_DEVICE_BUILT_IN_KERNELS))
    ((bool, CL_DEVICE_LINKER_AVAILABLE))
    ((cl_device_id, CL_DEVICE_PARENT_DEVICE))
    ((cl_uint, CL_DEVICE_PARTITION_MAX_SUB_DEVICES))
    ((cl_device_partition_property, CL_DEVICE_PARTITION_PROPERTIES))
    ((cl_device_affinity_domain, CL_DEVICE_PARTITION_AFFINITY_DOMAIN))
    ((cl_device_partition_property, CL_DEVICE_PARTITION_TYPE))
    ((size_t, CL_DEVICE_PRINTF_BUFFER_SIZE))
    ((bool, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC))
    ((cl_uint, CL_DEVICE_REFERENCE_COUNT))
)
#endif // BOOST_COMPUTE_CL_VERSION_1_2

#ifdef BOOST_COMPUTE_CL_VERSION_2_0
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((size_t, CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE))
    ((size_t, CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE))
    ((cl_uint, CL_DEVICE_MAX_ON_DEVICE_EVENTS))
    ((cl_uint, CL_DEVICE_MAX_ON_DEVICE_QUEUES))
    ((cl_uint, CL_DEVICE_MAX_PIPE_ARGS))
    ((cl_uint, CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS))
    ((cl_uint, CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS))
    ((cl_uint, CL_DEVICE_PIPE_MAX_PACKET_SIZE))
    ((cl_uint, CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT))
    ((cl_uint, CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT))
    ((cl_uint, CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT))
    ((cl_uint, CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE))
    ((cl_uint, CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE))
    ((cl_command_queue_properties, CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES))
    ((cl_device_svm_capabilities, CL_DEVICE_SVM_CAPABILITIES))
    ((cl_uint, CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT))
    ((cl_uint, CL_DEVICE_IMAGE_PITCH_ALIGNMENT))
)
#endif // BOOST_COMPUTE_CL_VERSION_2_0

#ifdef BOOST_COMPUTE_CL_VERSION_2_1
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(device,
    ((std::string, CL_DEVICE_IL_VERSION))
    ((cl_uint, CL_DEVICE_MAX_NUM_SUB_GROUPS))
    ((bool, CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS))
)
#endif // BOOST_COMPUTE_CL_VERSION_2_1

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DEVICE_HPP
