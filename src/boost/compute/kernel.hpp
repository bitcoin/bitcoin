//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_KERNEL_HPP
#define BOOST_COMPUTE_KERNEL_HPP

#include <string>

#include <boost/assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/optional.hpp>

#include <boost/compute/cl_ext.hpp> // cl_khr_subgroups

#include <boost/compute/config.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/platform.hpp>
#include <boost/compute/type_traits/is_fundamental.hpp>
#include <boost/compute/detail/diagnostic.hpp>
#include <boost/compute/detail/get_object_info.hpp>
#include <boost/compute/detail/assert_cl_success.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T> struct set_kernel_arg;

} // end detail namespace

/// \class kernel
/// \brief A compute kernel.
///
/// \see command_queue, program
class kernel
{
public:
    /// Creates a null kernel object.
    kernel()
        : m_kernel(0)
    {
    }

    /// Creates a new kernel object for \p kernel. If \p retain is
    /// \c true, the reference count for \p kernel will be incremented.
    explicit kernel(cl_kernel kernel, bool retain = true)
        : m_kernel(kernel)
    {
        if(m_kernel && retain){
            clRetainKernel(m_kernel);
        }
    }

    /// Creates a new kernel object with \p name from \p program.
    kernel(const program &program, const std::string &name)
    {
        cl_int error = 0;
        m_kernel = clCreateKernel(program.get(), name.c_str(), &error);

        if(!m_kernel){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new kernel object as a copy of \p other.
    kernel(const kernel &other)
        : m_kernel(other.m_kernel)
    {
        if(m_kernel){
            clRetainKernel(m_kernel);
        }
    }

    /// Copies the kernel object from \p other to \c *this.
    kernel& operator=(const kernel &other)
    {
        if(this != &other){
            if(m_kernel){
                clReleaseKernel(m_kernel);
            }

            m_kernel = other.m_kernel;

            if(m_kernel){
                clRetainKernel(m_kernel);
            }
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new kernel object from \p other.
    kernel(kernel&& other) BOOST_NOEXCEPT
        : m_kernel(other.m_kernel)
    {
        other.m_kernel = 0;
    }

    /// Move-assigns the kernel from \p other to \c *this.
    kernel& operator=(kernel&& other) BOOST_NOEXCEPT
    {
        if(m_kernel){
            clReleaseKernel(m_kernel);
        }

        m_kernel = other.m_kernel;
        other.m_kernel = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the kernel object.
    ~kernel()
    {
        if(m_kernel){
            BOOST_COMPUTE_ASSERT_CL_SUCCESS(
                clReleaseKernel(m_kernel)
            );
        }
    }

    #if defined(BOOST_COMPUTE_CL_VERSION_2_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Creates a new kernel object based on a shallow copy of
    /// the undelying OpenCL kernel object.
    ///
    /// \opencl_version_warning{2,1}
    ///
    /// \see_opencl21_ref{clCloneKernel}
    kernel clone()
    {
        cl_int ret = 0;
        cl_kernel k = clCloneKernel(m_kernel, &ret);
        return kernel(k, false);
    }
    #endif // BOOST_COMPUTE_CL_VERSION_2_1

    /// Returns a reference to the underlying OpenCL kernel object.
    cl_kernel& get() const
    {
        return const_cast<cl_kernel &>(m_kernel);
    }

    /// Returns the function name for the kernel.
    std::string name() const
    {
        return get_info<std::string>(CL_KERNEL_FUNCTION_NAME);
    }

    /// Returns the number of arguments for the kernel.
    size_t arity() const
    {
        return get_info<cl_uint>(CL_KERNEL_NUM_ARGS);
    }

    /// Returns the program for the kernel.
    program get_program() const
    {
        return program(get_info<cl_program>(CL_KERNEL_PROGRAM));
    }

    /// Returns the context for the kernel.
    context get_context() const
    {
        return context(get_info<cl_context>(CL_KERNEL_CONTEXT));
    }

    /// Returns information about the kernel.
    ///
    /// \see_opencl_ref{clGetKernelInfo}
    template<class T>
    T get_info(cl_kernel_info info) const
    {
        return detail::get_object_info<T>(clGetKernelInfo, m_kernel, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<kernel, Enum>::type
    get_info() const;

    #if defined(BOOST_COMPUTE_CL_VERSION_1_2) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Returns information about the argument at \p index.
    ///
    /// For example, to get the name of the first argument:
    /// \code
    /// std::string arg = kernel.get_arg_info<std::string>(0, CL_KERNEL_ARG_NAME);
    /// \endcode
    ///
    /// Note, this function requires that the program be compiled with the
    /// \c "-cl-kernel-arg-info" flag. For example:
    /// \code
    /// program.build("-cl-kernel-arg-info");
    /// \endcode
    ///
    /// \opencl_version_warning{1,2}
    ///
    /// \see_opencl_ref{clGetKernelArgInfo}
    template<class T>
    T get_arg_info(size_t index, cl_kernel_arg_info info) const
    {
        return detail::get_object_info<T>(
            clGetKernelArgInfo, m_kernel, info, static_cast<cl_uint>(index)
        );
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<kernel, Enum>::type
    get_arg_info(size_t index) const;
    #endif // BOOST_COMPUTE_CL_VERSION_1_2

    /// Returns work-group information for the kernel with \p device.
    ///
    /// \see_opencl_ref{clGetKernelWorkGroupInfo}
    template<class T>
    T get_work_group_info(const device &device, cl_kernel_work_group_info info) const
    {
        return detail::get_object_info<T>(clGetKernelWorkGroupInfo, m_kernel, info, device.id());
    }

    #if defined(BOOST_COMPUTE_CL_VERSION_2_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Returns sub-group information for the kernel with \p device. Returns a null
    /// optional if \p device is not 2.1 device, or is not 2.0 device with support
    /// for cl_khr_subgroups extension.
    ///
    /// \opencl_version_warning{2,1}
    /// \see_opencl21_ref{clGetKernelSubGroupInfo}
    /// \see_opencl2_ref{clGetKernelSubGroupInfoKHR}
    template<class T>
    boost::optional<T> get_sub_group_info(const device &device, cl_kernel_sub_group_info info,
                                          const size_t input_size, const void * input) const
    {
        if(device.check_version(2, 1))
        {
            return detail::get_object_info<T>(
                clGetKernelSubGroupInfo, m_kernel, info, device.id(), input_size, input
            );
        }
        else if(!device.check_version(2, 0) || !device.supports_extension("cl_khr_subgroups"))
        {
            return boost::optional<T>();
        }
        // Only CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE and CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE
        // are supported in cl_khr_subgroups extension for 2.0 devices.
        else if(info != CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE && info != CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE)
        {
            return boost::optional<T>();
        }

        BOOST_COMPUTE_DISABLE_DEPRECATED_DECLARATIONS();
        clGetKernelSubGroupInfoKHR_fn clGetKernelSubGroupInfoKHR_fptr =
            reinterpret_cast<clGetKernelSubGroupInfoKHR_fn>(
                reinterpret_cast<size_t>(
                    device.platform().get_extension_function_address("clGetKernelSubGroupInfoKHR")
                )
            );
        BOOST_COMPUTE_ENABLE_DEPRECATED_DECLARATIONS();

        return detail::get_object_info<T>(
            clGetKernelSubGroupInfoKHR_fptr, m_kernel, info, device.id(), input_size, input
        );
    }

    /// \overload
    template<class T>
    boost::optional<T> get_sub_group_info(const device &device, cl_kernel_sub_group_info info) const
    {
        return get_sub_group_info<T>(device, info, 0, 0);
    }

    /// \overload
    template<class T>
    boost::optional<T> get_sub_group_info(const device &device, cl_kernel_sub_group_info info,
                                          const size_t input) const
    {
        return get_sub_group_info<T>(device, info, sizeof(size_t), &input);
    }
    #endif // BOOST_COMPUTE_CL_VERSION_2_1

    #if defined(BOOST_COMPUTE_CL_VERSION_2_0) && !defined(BOOST_COMPUTE_CL_VERSION_2_1)
    /// Returns sub-group information for the kernel with \p device. Returns a null
    /// optional if cl_khr_subgroups extension is not supported by \p device.
    ///
    /// \opencl_version_warning{2,0}
    /// \see_opencl2_ref{clGetKernelSubGroupInfoKHR}
    template<class T>
    boost::optional<T> get_sub_group_info(const device &device, cl_kernel_sub_group_info info,
                                          const size_t input_size, const void * input) const
    {
        if(!device.check_version(2, 0) || !device.supports_extension("cl_khr_subgroups"))
        {
            return boost::optional<T>();
        }

        BOOST_COMPUTE_DISABLE_DEPRECATED_DECLARATIONS();
        clGetKernelSubGroupInfoKHR_fn clGetKernelSubGroupInfoKHR_fptr =
            reinterpret_cast<clGetKernelSubGroupInfoKHR_fn>(
                reinterpret_cast<size_t>(
                    device.platform().get_extension_function_address("clGetKernelSubGroupInfoKHR")
                )
            );
        BOOST_COMPUTE_ENABLE_DEPRECATED_DECLARATIONS();

        return detail::get_object_info<T>(
            clGetKernelSubGroupInfoKHR_fptr, m_kernel, info, device.id(), input_size, input
        );
    }
    #endif // defined(BOOST_COMPUTE_CL_VERSION_2_0) && !defined(BOOST_COMPUTE_CL_VERSION_2_1)

    #if defined(BOOST_COMPUTE_CL_VERSION_2_0) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// \overload
    template<class T>
    boost::optional<T> get_sub_group_info(const device &device, cl_kernel_sub_group_info info,
                                          const std::vector<size_t> input) const
    {
        BOOST_ASSERT(input.size() > 0);
        return get_sub_group_info<T>(device, info, input.size() * sizeof(size_t), &input[0]);
    }
    #endif // BOOST_COMPUTE_CL_VERSION_2_0

    /// Sets the argument at \p index to \p value with \p size.
    ///
    /// \see_opencl_ref{clSetKernelArg}
    void set_arg(size_t index, size_t size, const void *value)
    {
        BOOST_ASSERT(index < arity());

        cl_int ret = clSetKernelArg(m_kernel,
                                    static_cast<cl_uint>(index),
                                    size,
                                    value);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
    }

    /// Sets the argument at \p index to \p value.
    ///
    /// For built-in types (e.g. \c float, \c int4_), this is equivalent to
    /// calling set_arg(index, sizeof(type), &value).
    ///
    /// Additionally, this method is specialized for device memory objects
    /// such as buffer and image2d. This allows for them to be passed directly
    /// without having to extract their underlying cl_mem object.
    ///
    /// This method is also specialized for device container types such as
    /// vector<T> and array<T, N>. This allows for them to be passed directly
    /// as kernel arguments without having to extract their underlying buffer.
    ///
    /// For setting local memory arguments (e.g. "__local float *buf"), the
    /// local_buffer<T> class may be used:
    /// \code
    /// // set argument to a local buffer with storage for 32 float's
    /// kernel.set_arg(0, local_buffer<float>(32));
    /// \endcode
    template<class T>
    void set_arg(size_t index, const T &value)
    {
        // if you get a compilation error pointing here it means you
        // attempted to set a kernel argument from an invalid type.
        detail::set_kernel_arg<T>()(*this, index, value);
    }

    /// \internal_
    void set_arg(size_t index, const cl_mem mem)
    {
        set_arg(index, sizeof(cl_mem), static_cast<const void *>(&mem));
    }

    /// \internal_
    void set_arg(size_t index, const cl_sampler sampler)
    {
        set_arg(index, sizeof(cl_sampler), static_cast<const void *>(&sampler));
    }

    /// \internal_
    void set_arg_svm_ptr(size_t index, void* ptr)
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_2_0
        cl_int ret = clSetKernelArgSVMPointer(m_kernel, static_cast<cl_uint>(index), ptr);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
        #else
        (void) index;
        (void) ptr;
        BOOST_THROW_EXCEPTION(opencl_error(CL_INVALID_ARG_VALUE));
        #endif
    }

    #ifndef BOOST_COMPUTE_NO_VARIADIC_TEMPLATES
    /// Sets the arguments for the kernel to \p args.
    template<class... T>
    void set_args(T&&... args)
    {
        BOOST_ASSERT(sizeof...(T) <= arity());

        _set_args<0>(args...);
    }
    #endif // BOOST_COMPUTE_NO_VARIADIC_TEMPLATES

    #if defined(BOOST_COMPUTE_CL_VERSION_2_0) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Sets additional execution information for the kernel.
    ///
    /// \opencl_version_warning{2,0}
    ///
    /// \see_opencl2_ref{clSetKernelExecInfo}
    void set_exec_info(cl_kernel_exec_info info, size_t size, const void *value)
    {
        cl_int ret = clSetKernelExecInfo(m_kernel, info, size, value);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
    }
    #endif // BOOST_COMPUTE_CL_VERSION_2_0

    /// Returns \c true if the kernel is the same at \p other.
    bool operator==(const kernel &other) const
    {
        return m_kernel == other.m_kernel;
    }

    /// Returns \c true if the kernel is different from \p other.
    bool operator!=(const kernel &other) const
    {
        return m_kernel != other.m_kernel;
    }

    /// \internal_
    operator cl_kernel() const
    {
        return m_kernel;
    }

    /// \internal_
    static kernel create_with_source(const std::string &source,
                                     const std::string &name,
                                     const context &context)
    {
        return program::build_with_source(source, context).create_kernel(name);
    }

private:
    #ifndef BOOST_COMPUTE_NO_VARIADIC_TEMPLATES
    /// \internal_
    template<size_t N>
    void _set_args()
    {
    }

    /// \internal_
    template<size_t N, class T, class... Args>
    void _set_args(T&& arg, Args&&... rest)
    {
        set_arg(N, arg);
        _set_args<N+1>(rest...);
    }
    #endif // BOOST_COMPUTE_NO_VARIADIC_TEMPLATES

private:
    cl_kernel m_kernel;
};

inline kernel program::create_kernel(const std::string &name) const
{
    return kernel(*this, name);
}

/// \internal_ define get_info() specializations for kernel
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(kernel,
    ((std::string, CL_KERNEL_FUNCTION_NAME))
    ((cl_uint, CL_KERNEL_NUM_ARGS))
    ((cl_uint, CL_KERNEL_REFERENCE_COUNT))
    ((cl_context, CL_KERNEL_CONTEXT))
    ((cl_program, CL_KERNEL_PROGRAM))
)

#ifdef BOOST_COMPUTE_CL_VERSION_1_2
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(kernel,
    ((std::string, CL_KERNEL_ATTRIBUTES))
)
#endif // BOOST_COMPUTE_CL_VERSION_1_2

/// \internal_ define get_arg_info() specializations for kernel
#ifdef BOOST_COMPUTE_CL_VERSION_1_2
#define BOOST_COMPUTE_DETAIL_DEFINE_KERNEL_GET_ARG_INFO_SPECIALIZATION(result_type, value) \
    namespace detail { \
        template<> struct get_object_info_type<kernel, value> { typedef result_type type; }; \
    } \
    template<> inline result_type kernel::get_arg_info<value>(size_t index) const { \
        return get_arg_info<result_type>(index, value); \
    }

BOOST_COMPUTE_DETAIL_DEFINE_KERNEL_GET_ARG_INFO_SPECIALIZATION(cl_kernel_arg_address_qualifier, CL_KERNEL_ARG_ADDRESS_QUALIFIER)
BOOST_COMPUTE_DETAIL_DEFINE_KERNEL_GET_ARG_INFO_SPECIALIZATION(cl_kernel_arg_access_qualifier, CL_KERNEL_ARG_ACCESS_QUALIFIER)
BOOST_COMPUTE_DETAIL_DEFINE_KERNEL_GET_ARG_INFO_SPECIALIZATION(std::string, CL_KERNEL_ARG_TYPE_NAME)
BOOST_COMPUTE_DETAIL_DEFINE_KERNEL_GET_ARG_INFO_SPECIALIZATION(cl_kernel_arg_type_qualifier, CL_KERNEL_ARG_TYPE_QUALIFIER)
BOOST_COMPUTE_DETAIL_DEFINE_KERNEL_GET_ARG_INFO_SPECIALIZATION(std::string, CL_KERNEL_ARG_NAME)
#endif // BOOST_COMPUTE_CL_VERSION_1_2

namespace detail {

// set_kernel_arg implementation for built-in types
template<class T>
struct set_kernel_arg
{
    typename boost::enable_if<is_fundamental<T> >::type
    operator()(kernel &kernel_, size_t index, const T &value)
    {
        kernel_.set_arg(index, sizeof(T), &value);
    }
};

// set_kernel_arg specialization for char (different from built-in cl_char)
template<>
struct set_kernel_arg<char>
{
    void operator()(kernel &kernel_, size_t index, const char c)
    {
        kernel_.set_arg(index, sizeof(char), &c);
    }
};

} // end detail namespace
} // end namespace compute
} // end namespace boost

#endif // BOOST_COMPUTE_KERNEL_HPP
