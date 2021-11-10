//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_PIPE_HPP
#define BOOST_COMPUTE_PIPE_HPP

#include <boost/compute/cl.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/memory_object.hpp>
#include <boost/compute/exception/opencl_error.hpp>
#include <boost/compute/detail/get_object_info.hpp>

// pipe objects require opencl 2.0
#if defined(BOOST_COMPUTE_CL_VERSION_2_0) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)

namespace boost {
namespace compute {

/// \class pipe
/// \brief A FIFO data pipe
///
/// \opencl_version_warning{2,0}
///
/// \see memory_object
class pipe : public memory_object
{
public:
    /// Creates a null pipe object.
    pipe()
        : memory_object()
    {
    }

    /// Creates a pipe object for \p mem. If \p retain is \c true, the
    /// reference count for \p mem will be incremented.
    explicit pipe(cl_mem mem, bool retain = true)
        : memory_object(mem, retain)
    {
    }

    /// Creates a new pipe in \p context.
    pipe(const context &context,
         uint_ pipe_packet_size,
         uint_ pipe_max_packets,
         cl_mem_flags flags = read_write,
         const cl_pipe_properties *properties = 0)
    {
        cl_int error = 0;
        m_mem = clCreatePipe(context,
                             flags,
                             pipe_packet_size,
                             pipe_max_packets,
                             properties,
                             &error);
        if(!m_mem){
            BOOST_THROW_EXCEPTION(opencl_error(error));
        }
    }

    /// Creates a new pipe object as a copy of \p other.
    pipe(const pipe &other)
        : memory_object(other)
    {
    }

    /// Copies the pipe object from \p other to \c *this.
    pipe& operator=(const pipe &other)
    {
        if(this != &other){
            memory_object::operator=(other);
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new pipe object from \p other.
    pipe(pipe&& other) BOOST_NOEXCEPT
        : memory_object(std::move(other))
    {
    }

    /// Move-assigns the pipe from \p other to \c *this.
    pipe& operator=(pipe&& other) BOOST_NOEXCEPT
    {
        memory_object::operator=(std::move(other));

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the pipe object.
    ~pipe()
    {
    }

    /// Returns the packet size.
    uint_ packet_size() const
    {
        return get_info<uint_>(CL_PIPE_PACKET_SIZE);
    }

    /// Returns the max number of packets.
    uint_ max_packets() const
    {
        return get_info<uint_>(CL_PIPE_MAX_PACKETS);
    }

    /// Returns information about the pipe.
    ///
    /// \see_opencl2_ref{clGetPipeInfo}
    template<class T>
    T get_info(cl_pipe_info info) const
    {
        return detail::get_object_info<T>(clGetPipeInfo, m_mem, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<pipe, Enum>::type get_info() const;
};

/// \internal_ define get_info() specializations for pipe
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(pipe,
    ((cl_uint, CL_PIPE_PACKET_SIZE))
    ((cl_uint, CL_PIPE_MAX_PACKETS))
)

namespace detail {

// set_kernel_arg specialization for pipe
template<>
struct set_kernel_arg<pipe>
{
    void operator()(kernel &kernel_, size_t index, const pipe &pipe_)
    {
        kernel_.set_arg(index, pipe_.get());
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CL_VERSION_2_0

#endif // BOOST_COMPUTE_PIPE_HPP
