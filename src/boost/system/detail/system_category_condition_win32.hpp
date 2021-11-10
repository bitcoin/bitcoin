#ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_CONDITION_WIN32_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_CONDITION_WIN32_HPP_INCLUDED

// Copyright Beman Dawes 2002, 2006
// Copyright (c) Microsoft Corporation 2014
// Copyright 2018 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/errc.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/config.hpp>

//

namespace boost
{

namespace system
{

namespace detail
{

inline int system_category_condition_win32( int ev ) BOOST_NOEXCEPT
{
    // When using the Windows Runtime, most system errors are reported as HRESULTs.
    // We want to map the common Win32 errors to their equivalent error condition,
    // whether or not they are reported via an HRESULT.

#define BOOST_SYSTEM_FAILED(hr)           ((hr) < 0)
#define BOOST_SYSTEM_HRESULT_FACILITY(hr) (((hr) >> 16) & 0x1fff)
#define BOOST_SYSTEM_HRESULT_CODE(hr)     ((hr) & 0xFFFF)
#define BOOST_SYSTEM_FACILITY_WIN32       7

    if( BOOST_SYSTEM_FAILED( ev ) && BOOST_SYSTEM_HRESULT_FACILITY( ev ) == BOOST_SYSTEM_FACILITY_WIN32 )
    {
        ev = BOOST_SYSTEM_HRESULT_CODE( ev );
    }

#undef BOOST_SYSTEM_FAILED
#undef BOOST_SYSTEM_HRESULT_FACILITY
#undef BOOST_SYSTEM_HRESULT_CODE
#undef BOOST_SYSTEM_FACILITY_WIN32

    using namespace boost::winapi;
    using namespace errc;

    // Windows system -> posix_errno decode table
    // see WinError.h comments for descriptions of errors

    switch ( ev )
    {
    case 0: return success;

    case ERROR_ACCESS_DENIED_: return permission_denied;
    case ERROR_ALREADY_EXISTS_: return file_exists;
    case ERROR_BAD_UNIT_: return no_such_device;
    case ERROR_BUFFER_OVERFLOW_: return filename_too_long;
    case ERROR_BUSY_: return device_or_resource_busy;
    case ERROR_BUSY_DRIVE_: return device_or_resource_busy;
    case ERROR_CANNOT_MAKE_: return permission_denied;
    case ERROR_CANTOPEN_: return io_error;
    case ERROR_CANTREAD_: return io_error;
    case ERROR_CANTWRITE_: return io_error;
    case ERROR_CONNECTION_ABORTED_: return connection_aborted;
    case ERROR_CURRENT_DIRECTORY_: return permission_denied;
    case ERROR_DEV_NOT_EXIST_: return no_such_device;
    case ERROR_DEVICE_IN_USE_: return device_or_resource_busy;
    case ERROR_DIR_NOT_EMPTY_: return directory_not_empty;
    case ERROR_DIRECTORY_: return invalid_argument; // WinError.h: "The directory name is invalid"
    case ERROR_DISK_FULL_: return no_space_on_device;
    case ERROR_FILE_EXISTS_: return file_exists;
    case ERROR_FILE_NOT_FOUND_: return no_such_file_or_directory;
    case ERROR_HANDLE_DISK_FULL_: return no_space_on_device;
    case ERROR_INVALID_ACCESS_: return permission_denied;
    case ERROR_INVALID_DRIVE_: return no_such_device;
    case ERROR_INVALID_FUNCTION_: return function_not_supported;
    case ERROR_INVALID_HANDLE_: return invalid_argument;
    case ERROR_INVALID_NAME_: return invalid_argument;
    case ERROR_LOCK_VIOLATION_: return no_lock_available;
    case ERROR_LOCKED_: return no_lock_available;
    case ERROR_NEGATIVE_SEEK_: return invalid_argument;
    case ERROR_NOACCESS_: return permission_denied;
    case ERROR_NOT_ENOUGH_MEMORY_: return not_enough_memory;
    case ERROR_NOT_READY_: return resource_unavailable_try_again;
    case ERROR_NOT_SAME_DEVICE_: return cross_device_link;
    case ERROR_OPEN_FAILED_: return io_error;
    case ERROR_OPEN_FILES_: return device_or_resource_busy;
    case ERROR_OPERATION_ABORTED_: return operation_canceled;
    case ERROR_OUTOFMEMORY_: return not_enough_memory;
    case ERROR_PATH_NOT_FOUND_: return no_such_file_or_directory;
    case ERROR_READ_FAULT_: return io_error;
    case ERROR_RETRY_: return resource_unavailable_try_again;
    case ERROR_SEEK_: return io_error;
    case ERROR_SHARING_VIOLATION_: return permission_denied;
    case ERROR_NOT_SUPPORTED_: return not_supported; // WinError.h: "The request is not supported."
    case ERROR_TOO_MANY_OPEN_FILES_: return too_many_files_open;
    case ERROR_WRITE_FAULT_: return io_error;
    case ERROR_WRITE_PROTECT_: return permission_denied;

    case WSAEACCES_: return permission_denied;
    case WSAEADDRINUSE_: return address_in_use;
    case WSAEADDRNOTAVAIL_: return address_not_available;
    case WSAEAFNOSUPPORT_: return address_family_not_supported;
    case WSAEALREADY_: return connection_already_in_progress;
    case WSAEBADF_: return bad_file_descriptor;
    case WSAECONNABORTED_: return connection_aborted;
    case WSAECONNREFUSED_: return connection_refused;
    case WSAECONNRESET_: return connection_reset;
    case WSAEDESTADDRREQ_: return destination_address_required;
    case WSAEFAULT_: return bad_address;
    case WSAEHOSTUNREACH_: return host_unreachable;
    case WSAEINPROGRESS_: return operation_in_progress;
    case WSAEINTR_: return interrupted;
    case WSAEINVAL_: return invalid_argument;
    case WSAEISCONN_: return already_connected;
    case WSAEMFILE_: return too_many_files_open;
    case WSAEMSGSIZE_: return message_size;
    case WSAENAMETOOLONG_: return filename_too_long;
    case WSAENETDOWN_: return network_down;
    case WSAENETRESET_: return network_reset;
    case WSAENETUNREACH_: return network_unreachable;
    case WSAENOBUFS_: return no_buffer_space;
    case WSAENOPROTOOPT_: return no_protocol_option;
    case WSAENOTCONN_: return not_connected;
    case WSAENOTSOCK_: return not_a_socket;
    case WSAEOPNOTSUPP_: return operation_not_supported;
    case WSAEPROTONOSUPPORT_: return protocol_not_supported;
    case WSAEPROTOTYPE_: return wrong_protocol_type;
    case WSAETIMEDOUT_: return timed_out;
    case WSAEWOULDBLOCK_: return operation_would_block;

    default: return -1;
    }
}

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_CONDITION_WIN32_HPP_INCLUDED
