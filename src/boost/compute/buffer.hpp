//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_BUFFER_HPP
#define BOOST_COMPUTE_BUFFER_HPP

#include <boost/compute/config.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/memory_object.hpp>
#include <boost/compute/detail/get_object_info.hpp>

namespace boost {
namespace compute {

// forward declarations
class command_queue;

/// \class buffer
/// \brief A memory buffer on a compute device.
///
/// The buffer class represents a memory buffer on a compute device.
///
/// Buffers are allocated within a compute context. For example, to allocate
/// a memory buffer for 32 float's:
///
/// \snippet test/test_buffer.cpp constructor
///
/// Once created, data can be copied to and from the buffer using the
/// \c enqueue_*_buffer() methods in the command_queue class. For example, to
/// copy a set of \c int values from the host to the device:
/// \code
/// int data[] = { 1, 2, 3, 4 };
///
/// queue.enqueue_write_buffer(buf, 0, 4 * sizeof(int), data);
/// \endcode
///
/// Also see the copy() algorithm for a higher-level interface to copying data
/// between the host and the device. For a higher-level, dynamically-resizable,
/// type-safe container for data on a compute device, use the vector<T> class.
///
/// Buffer objects have reference semantics. Creating a copy of a buffer
/// object simply creates another reference to the underlying OpenCL memory
/// object. To create an actual copy use the buffer::clone() method.
///
/// \see context, command_queue
class buffer : public memory_object
{
public:
    /// Creates a null buffer object.
    buffer()
        : memory_object()
    {
    }

    /// Creates a buffer object for \p mem. If \p retain is \c true, the
    /// reference count for \p mem will be incremented.
    explicit buffer(cl_mem mem, bool retain = true)
        : memory_object(mem, retain)
    {
    }

    /// Create a new memory buffer in of \p size with \p flags in
    /// \p context.
    ///
    /// \see_opencl_ref{clCreateBuffer}
    buffer(const context &context,
           size_t size,
           cl_mem_flags flags = read_write,
           void *host_ptr = 0)
    {
        cl_int error = 0;
        m_mem = clCreateBuffer(context,
                               flags,
                               (std::max)(size, size_t(1)),
                               host_ptr,
                               &error);
        if(!m_mem){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new buffer object as a copy of \p other.
    buffer(const buffer &other)
        : memory_object(other)
    {
    }

    /// Copies the buffer object from \p other to \c *this.
    buffer& operator=(const buffer &other)
    {
        if(this != &other){
            memory_object::operator=(other);
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new buffer object from \p other.
    buffer(buffer&& other) BOOST_NOEXCEPT
        : memory_object(std::move(other))
    {
    }

    /// Move-assigns the buffer from \p other to \c *this.
    buffer& operator=(buffer&& other) BOOST_NOEXCEPT
    {
        memory_object::operator=(std::move(other));

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the buffer object.
    ~buffer()
    {
    }

    /// Returns the size of the buffer in bytes.
    size_t size() const
    {
        return get_memory_size();
    }

    /// \internal_
    size_t max_size() const
    {
        return get_context().get_device().max_memory_alloc_size();
    }

    /// Returns information about the buffer.
    ///
    /// \see_opencl_ref{clGetMemObjectInfo}
    template<class T>
    T get_info(cl_mem_info info) const
    {
        return get_memory_info<T>(info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<buffer, Enum>::type
    get_info() const;

    /// Creates a new buffer with a copy of the data in \c *this. Uses
    /// \p queue to perform the copy.
    buffer clone(command_queue &queue) const;

    #if defined(BOOST_COMPUTE_CL_VERSION_1_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Creates a new buffer out of this buffer.
    /// The new buffer is a sub region of this buffer.
    /// \p flags The mem_flags which should be used to create the new buffer
    /// \p origin The start index in this buffer
    /// \p size The size of the new sub buffer
    ///
    /// \see_opencl_ref{clCreateSubBuffer}
    ///
    /// \opencl_version_warning{1,1}
    buffer create_subbuffer(cl_mem_flags flags, size_t origin,
                            size_t size)
    {
        BOOST_ASSERT(origin + size <= this->size());
        BOOST_ASSERT(origin % (get_context().
                               get_device().
                               get_info<CL_DEVICE_MEM_BASE_ADDR_ALIGN>() / 8) == 0);
        cl_int error = 0;

        cl_buffer_region region = { origin, size };

        cl_mem mem = clCreateSubBuffer(m_mem,
                                       flags,
                                       CL_BUFFER_CREATE_TYPE_REGION,
                                       &region,
                                       &error);

        if(!mem){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }

        return buffer(mem, false);
    }
  #endif // BOOST_COMPUTE_CL_VERSION_1_1
};

/// \internal_ define get_info() specializations for buffer
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(buffer,
    ((cl_mem_object_type, CL_MEM_TYPE))
    ((cl_mem_flags, CL_MEM_FLAGS))
    ((size_t, CL_MEM_SIZE))
    ((void *, CL_MEM_HOST_PTR))
    ((cl_uint, CL_MEM_MAP_COUNT))
    ((cl_uint, CL_MEM_REFERENCE_COUNT))
    ((cl_context, CL_MEM_CONTEXT))
)

#ifdef BOOST_COMPUTE_CL_VERSION_1_1
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(buffer,
    ((cl_mem, CL_MEM_ASSOCIATED_MEMOBJECT))
    ((size_t, CL_MEM_OFFSET))
)
#endif // BOOST_COMPUTE_CL_VERSION_1_1

namespace detail {

// set_kernel_arg specialization for buffer
template<>
struct set_kernel_arg<buffer>
{
    void operator()(kernel &kernel_, size_t index, const buffer &buffer_)
    {
        kernel_.set_arg(index, buffer_.get());
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_BUFFER_HPP
