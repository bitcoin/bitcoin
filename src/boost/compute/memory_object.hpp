//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_MEMORY_OBJECT_HPP
#define BOOST_COMPUTE_MEMORY_OBJECT_HPP

#include <boost/compute/config.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/detail/get_object_info.hpp>
#include <boost/compute/detail/assert_cl_success.hpp>

namespace boost {
namespace compute {

/// \class memory_object
/// \brief Base-class for memory objects.
///
/// The memory_object class is the base-class for memory objects on
/// compute devices.
///
/// \see buffer, vector
class memory_object
{
public:
    /// Flags for the creation of memory objects.
    enum mem_flags {
        read_write = CL_MEM_READ_WRITE,
        read_only = CL_MEM_READ_ONLY,
        write_only = CL_MEM_WRITE_ONLY,
        use_host_ptr = CL_MEM_USE_HOST_PTR,
        alloc_host_ptr = CL_MEM_ALLOC_HOST_PTR,
        copy_host_ptr = CL_MEM_COPY_HOST_PTR
        #ifdef BOOST_COMPUTE_CL_VERSION_1_2
        ,
        host_write_only = CL_MEM_HOST_WRITE_ONLY,
        host_read_only = CL_MEM_HOST_READ_ONLY,
        host_no_access = CL_MEM_HOST_NO_ACCESS
        #endif
    };

    /// Symbolic names for the OpenCL address spaces.
    enum address_space {
        global_memory,
        local_memory,
        private_memory,
        constant_memory
    };

    /// Returns the underlying OpenCL memory object.
    cl_mem& get() const
    {
        return const_cast<cl_mem &>(m_mem);
    }

    /// Returns the size of the memory object in bytes.
    size_t get_memory_size() const
    {
        return get_memory_info<size_t>(CL_MEM_SIZE);
    }

    /// Returns the type for the memory object.
    cl_mem_object_type get_memory_type() const
    {
        return get_memory_info<cl_mem_object_type>(CL_MEM_TYPE);
    }

    /// Returns the flags for the memory object.
    cl_mem_flags get_memory_flags() const
    {
        return get_memory_info<cl_mem_flags>(CL_MEM_FLAGS);
    }

    /// Returns the context for the memory object.
    context get_context() const
    {
        return context(get_memory_info<cl_context>(CL_MEM_CONTEXT));
    }

    /// Returns the host pointer associated with the memory object.
    void* get_host_ptr() const
    {
        return get_memory_info<void *>(CL_MEM_HOST_PTR);
    }

    /// Returns the reference count for the memory object.
    uint_ reference_count() const
    {
        return get_memory_info<uint_>(CL_MEM_REFERENCE_COUNT);
    }

    /// Returns information about the memory object.
    ///
    /// \see_opencl_ref{clGetMemObjectInfo}
    template<class T>
    T get_memory_info(cl_mem_info info) const
    {
        return detail::get_object_info<T>(clGetMemObjectInfo, m_mem, info);
    }

    #if defined(BOOST_COMPUTE_CL_VERSION_1_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Registers a function to be called when the memory object is deleted
    /// and its resources freed.
    ///
    /// \see_opencl_ref{clSetMemObjectDestructorCallback}
    ///
    /// \opencl_version_warning{1,1}
    void set_destructor_callback(void (BOOST_COMPUTE_CL_CALLBACK *callback)(
                                     cl_mem memobj, void *user_data
                                 ),
                                 void *user_data = 0)
    {
        cl_int ret = clSetMemObjectDestructorCallback(m_mem, callback, user_data);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
    }
    /// Registers a function to be called when the memory object is deleted
    /// and its resources freed.
    ///
    /// The function specified by \p callback must be invokable with zero
    /// arguments (e.g. \c callback()).
    ///
    /// \opencl_version_warning{1,1}
    template<class Function>
    void set_destructor_callback(Function callback)
    {
        set_destructor_callback(
            destructor_callback_invoker,
            new boost::function<void()>(callback)
        );
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_1

    /// Returns \c true if the memory object is the same as \p other.
    bool operator==(const memory_object &other) const
    {
        return m_mem == other.m_mem;
    }

    /// Returns \c true if the memory object is different from \p other.
    bool operator!=(const memory_object &other) const
    {
        return m_mem != other.m_mem;
    }

private:
    #ifdef BOOST_COMPUTE_CL_VERSION_1_1
    /// \internal_
    static void BOOST_COMPUTE_CL_CALLBACK
    destructor_callback_invoker(cl_mem, void *user_data)
    {
        boost::function<void()> *callback =
            static_cast<boost::function<void()> *>(user_data);

        (*callback)();

        delete callback;
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_1

protected:
    /// \internal_
    memory_object()
        : m_mem(0)
    {
    }

    /// \internal_
    explicit memory_object(cl_mem mem, bool retain = true)
        : m_mem(mem)
    {
        if(m_mem && retain){
            clRetainMemObject(m_mem);
        }
    }

    /// \internal_
    memory_object(const memory_object &other)
        : m_mem(other.m_mem)
    {
        if(m_mem){
            clRetainMemObject(m_mem);
        }
    }

    /// \internal_
    memory_object& operator=(const memory_object &other)
    {
        if(this != &other){
            if(m_mem){
                clReleaseMemObject(m_mem);
            }

            m_mem = other.m_mem;

            if(m_mem){
                clRetainMemObject(m_mem);
            }
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// \internal_
    memory_object(memory_object&& other) BOOST_NOEXCEPT
        : m_mem(other.m_mem)
    {
        other.m_mem = 0;
    }

    /// \internal_
    memory_object& operator=(memory_object&& other) BOOST_NOEXCEPT
    {
        if(m_mem){
            clReleaseMemObject(m_mem);
        }

        m_mem = other.m_mem;
        other.m_mem = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// \internal_
    ~memory_object()
    {
        if(m_mem){
            BOOST_COMPUTE_ASSERT_CL_SUCCESS(
                clReleaseMemObject(m_mem)
            );
        }
    }

protected:
    cl_mem m_mem;
};

namespace detail {

// set_kernel_arg specialization for memory_object
template<>
struct set_kernel_arg<memory_object>
{
    void operator()(kernel &kernel_, size_t index, const memory_object &mem)
    {
        kernel_.set_arg(index, mem.get());
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_MEMORY_OBJECT_HPP
