//---------------------------------------------------------------------------//
// Copyright (c) 2013-2015 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_EXCEPTION_NO_DEVICE_FOUND_HPP
#define BOOST_COMPUTE_EXCEPTION_NO_DEVICE_FOUND_HPP

#include <exception>

namespace boost {
namespace compute {

/// \class no_device_found
/// \brief Exception thrown when no OpenCL device is found
///
/// This exception is thrown when no valid OpenCL device can be found.
///
/// \see opencl_error
class no_device_found : public std::exception
{
public:
    /// Creates a new no_device_found exception object.
    no_device_found() throw()
    {
    }

    /// Destroys the no_device_found exception object.
    ~no_device_found() throw()
    {
    }

    /// Returns a string containing a human-readable error message.
    const char* what() const throw()
    {
        return "No OpenCL device found";
    }
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_EXCEPTION_NO_DEVICE_FOUND_HPP
