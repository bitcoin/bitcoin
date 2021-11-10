//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_EXCEPTION_CONTEXT_ERROR_HPP
#define BOOST_COMPUTE_EXCEPTION_CONTEXT_ERROR_HPP

#include <exception>

namespace boost {
namespace compute {

class context;

/// \class context_error
/// \brief A run-time OpenCL context error.
///
/// The context_error exception is thrown when the OpenCL context encounters
/// an error condition. Boost.Compute is notified of these error conditions by
/// registering an error handler when creating context objects (via the
/// \c pfn_notify argument to the \c clCreateContext() function).
///
/// This exception is different than the opencl_error exception which is thrown
/// as a result of error caused when calling a single OpenCL API function.
///
/// \see opencl_error
class context_error : public std::exception
{
public:
    /// Creates a new context error exception object.
    context_error(const context *context,
                  const char *errinfo,
                  const void *private_info,
                  size_t private_info_size) throw()
        : m_context(context),
          m_errinfo(errinfo),
          m_private_info(private_info),
          m_private_info_size(private_info_size)
    {
    }

    /// Destroys the context error object.
    ~context_error() throw()
    {
    }

    /// Returns a string with a description of the error.
    const char* what() const throw()
    {
        return m_errinfo;
    }

    /// Returns a pointer to the context object which generated the error
    /// notification.
    const context* get_context_ptr() const throw()
    {
        return m_context;
    }

    /// Returns a pointer to the private info memory block.
    const void* get_private_info_ptr() const throw()
    {
        return m_private_info;
    }

    /// Returns the size of the private info memory block.
    size_t get_private_info_size() const throw()
    {
        return m_private_info_size;
    }

private:
    const context *m_context;
    const char *m_errinfo;
    const void *m_private_info;
    size_t m_private_info_size;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_EXCEPTION_CONTEXT_ERROR_HPP
