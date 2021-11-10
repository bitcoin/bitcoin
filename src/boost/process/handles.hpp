// Copyright (c) 2019 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_HANDLES_HPP_
#define BOOST_PROCESS_HANDLES_HPP_

/**
 * \file boost/process/handles.hpp
 *
 * Defines functions to obtain handles of the current process and limit the amount for inherited ones.
 */

#include <boost/process/detail/config.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/handles.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/handles.hpp>
#endif

#include <boost/process/detail/used_handles.hpp>


namespace boost { namespace this_process
{

///The native type for handles
using native_handle_type = ::boost::process::detail::api::native_handle_type;

/**
 * Get a snapshot of all handles of the process (i.e. file descriptors on posix and handles on windows) of the current process.
 *
 * \note This function might not work on certain posix systems.
 *
 * \note On Windows version older than windows 8 this function will iterate all the system handles, meaning it might be quite slow.
 *
 * \warning This functionality is utterly prone to race conditions, since other threads might open or close handles.
 *
 * \return The list of all open handles of the current process
 */
inline std::vector<native_handle_type> get_handles()
{
    return ::boost::process::detail::api::get_handles();
}


/** \overload std::vector<native_handle_type> get_handles() */
inline std::vector<native_handle_type> get_handles(std::error_code &ec)
{
    return ::boost::process::detail::api::get_handles(ec);
}

/** Determines if a given handle is a a stream-handle, i.e. any handle that can be used with read and write functions.
 * Stream handles include pipes, regular files and sockets.
 *
 * \return Indicates if it's a stream handle.
 */
inline bool is_stream_handle(native_handle_type handle)
{
    return ::boost::process::detail::api::is_stream_handle(handle);
}


/** \overload bool is_stream_handle(native_handle_type handle) */
inline bool is_stream_handle(native_handle_type handle, std::error_code &ec)
{
    return ::boost::process::detail::api::is_stream_handle(handle, ec);
}

}
namespace process
{

namespace detail
{

using limit_handles_ = ::boost::process::detail::api::limit_handles_;


}

/**
 * The limit_handles property sets all properties to be inherited only expcitly. It closes all unused file-descriptors on posix after the fork and
 * removes the inherit flags on windows.
 *
 * \note This is executed after the fork on posix.
 *
 * \code{.cpp}
 * system("gcc", limit_handles);
 * \endcode
 *
 * Since limit also closes the standard handles unless they are explicitly redirected they can be ignored by `limit_handles` in the following way.
 *
 * \code{.cpp}
 * system("gcc", limit_handles.allowStd())
 * \endcode
 *
*/
const static ::boost::process::detail::api::limit_handles_ limit_handles;


}
}

#endif //BOOST_PROCESS_HANDLES_HPP_
