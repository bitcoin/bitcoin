//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_MAPPED_VIEW_HPP
#define BOOST_COMPUTE_CONTAINER_MAPPED_VIEW_HPP

#include <cstddef>
#include <exception>

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <boost/compute/buffer.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>

namespace boost {
namespace compute {

/// \class mapped_view
/// \brief A mapped view of host memory.
///
/// The mapped_view class simplifies mapping host-memory to a compute
/// device. This allows for host-allocated memory to be used with the
/// Boost.Compute algorithms.
///
/// The following example shows how to map a simple C-array containing
/// data on the host to the device and run the reduce() algorithm to
/// calculate the sum:
///
/// \snippet test/test_mapped_view.cpp reduce
///
/// \see buffer
template<class T>
class mapped_view
{
public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef buffer_iterator<T> iterator;
    typedef buffer_iterator<T> const_iterator;

    /// Creates a null mapped_view object.
    mapped_view()
    {
        m_mapped_ptr = 0;
    }

    /// Creates a mapped_view for \p host_ptr with \p n elements. After
    /// constructing a mapped_view the data is available for use by a
    /// compute device. Use the \p unmap() method to make the updated data
    /// available to the host.
    mapped_view(T *host_ptr,
                size_type n,
                const context &context = system::default_context())
        : m_buffer(_make_mapped_buffer(host_ptr, n, context))
    {
        m_mapped_ptr = 0;
    }

    /// Creates a read-only mapped_view for \p host_ptr with \p n elements.
    /// After constructing a mapped_view the data is available for use by a
    /// compute device. Use the \p unmap() method to make the updated data
    /// available to the host.
    mapped_view(const T *host_ptr,
                size_type n,
                const context &context = system::default_context())
        : m_buffer(_make_mapped_buffer(host_ptr, n, context))
    {
        m_mapped_ptr = 0;
    }

    /// Creates a copy of \p other.
    mapped_view(const mapped_view<T> &other)
        : m_buffer(other.m_buffer)
    {
        m_mapped_ptr = 0;
    }

    /// Copies the mapped buffer from \p other.
    mapped_view<T>& operator=(const mapped_view<T> &other)
    {
        if(this != &other){
            m_buffer = other.m_buffer;
            m_mapped_ptr = 0;
        }

        return *this;
    }

    /// Destroys the mapped_view object.
    ~mapped_view()
    {
    }

    /// Returns an iterator to the first element in the mapped_view.
    iterator begin()
    {
        return ::boost::compute::make_buffer_iterator<T>(m_buffer, 0);
    }

    /// Returns a const_iterator to the first element in the mapped_view.
    const_iterator begin() const
    {
        return ::boost::compute::make_buffer_iterator<T>(m_buffer, 0);
    }

    /// Returns a const_iterator to the first element in the mapped_view.
    const_iterator cbegin() const
    {
        return begin();
    }

    /// Returns an iterator to one past the last element in the mapped_view.
    iterator end()
    {
        return ::boost::compute::make_buffer_iterator<T>(m_buffer, size());
    }

    /// Returns a const_iterator to one past the last element in the mapped_view.
    const_iterator end() const
    {
        return ::boost::compute::make_buffer_iterator<T>(m_buffer, size());
    }

    /// Returns a const_iterator to one past the last element in the mapped_view.
    const_iterator cend() const
    {
        return end();
    }

    /// Returns the number of elements in the mapped_view.
    size_type size() const
    {
        return m_buffer.size() / sizeof(T);
    }

    /// Returns the host data pointer.
    T* get_host_ptr()
    {
        return static_cast<T *>(m_buffer.get_info<void *>(CL_MEM_HOST_PTR));
    }

    /// Returns the host data pointer.
    const T* get_host_ptr() const
    {
        return static_cast<T *>(m_buffer.get_info<void *>(CL_MEM_HOST_PTR));
    }

    /// Resizes the mapped_view to \p size elements.
    void resize(size_type size)
    {
        T *old_ptr = get_host_ptr();

        m_buffer = _make_mapped_buffer(old_ptr, size, m_buffer.get_context());
    }

    /// Returns \c true if the mapped_view is empty.
    bool empty() const
    {
        return size() == 0;
    }

    /// Returns the mapped buffer.
    const buffer& get_buffer() const
    {
        return m_buffer;
    }

    /// Maps the buffer into the host address space.
    ///
    /// \see_opencl_ref{clEnqueueMapBuffer}
    void map(cl_map_flags flags, command_queue &queue)
    {
        BOOST_ASSERT(m_mapped_ptr == 0);

        m_mapped_ptr = queue.enqueue_map_buffer(
            m_buffer, flags, 0, m_buffer.size()
        );
    }

    /// Maps the buffer into the host address space for reading and writing.
    ///
    /// Equivalent to:
    /// \code
    /// map(CL_MAP_READ | CL_MAP_WRITE, queue);
    /// \endcode
    void map(command_queue &queue)
    {
        map(CL_MAP_READ | CL_MAP_WRITE, queue);
    }

    /// Unmaps the buffer from the host address space.
    ///
    /// \see_opencl_ref{clEnqueueUnmapMemObject}
    void unmap(command_queue &queue)
    {
        BOOST_ASSERT(m_mapped_ptr != 0);

        queue.enqueue_unmap_buffer(m_buffer, m_mapped_ptr);

        m_mapped_ptr = 0;
    }

private:
    /// \internal_
    static buffer _make_mapped_buffer(T *host_ptr,
                                      size_t n,
                                      const context &context)
    {
        return buffer(
            context,
            n * sizeof(T),
            buffer::read_write | buffer::use_host_ptr,
            host_ptr
        );
    }

    /// \internal_
    static buffer _make_mapped_buffer(const T *host_ptr,
                                      size_t n,
                                      const context &context)
    {
        return buffer(
            context,
            n * sizeof(T),
            buffer::read_only | buffer::use_host_ptr,
            const_cast<void *>(static_cast<const void *>(host_ptr))
        );
    }

private:
    buffer m_buffer;
    void *m_mapped_ptr;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_MAPPED_VIEW_HPP
