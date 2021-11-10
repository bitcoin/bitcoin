//
// Copyright (c) 2015-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_FILE_WIN32_IPP
#define BOOST_BEAST_CORE_IMPL_FILE_WIN32_IPP

#include <boost/beast/core/file_win32.hpp>

#if BOOST_BEAST_USE_WIN32_FILE

#include <boost/beast/core/detail/win32_unicode_path.hpp>
#include <boost/core/exchange.hpp>
#include <boost/winapi/access_rights.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/winapi/get_last_error.hpp>
#include <limits>
#include <utility>

namespace boost {
namespace beast {

namespace detail {

// VFALCO Can't seem to get boost/detail/winapi to work with
//        this so use the non-Ex version for now.
BOOST_BEAST_DECL
boost::winapi::BOOL_
set_file_pointer_ex(
    boost::winapi::HANDLE_ hFile,
    boost::winapi::LARGE_INTEGER_ lpDistanceToMove,
    boost::winapi::PLARGE_INTEGER_ lpNewFilePointer,
    boost::winapi::DWORD_ dwMoveMethod)
{
    auto dwHighPart = lpDistanceToMove.u.HighPart;
    auto dwLowPart = boost::winapi::SetFilePointer(
        hFile,
        lpDistanceToMove.u.LowPart,
        &dwHighPart,
        dwMoveMethod);
    if(dwLowPart == boost::winapi::INVALID_SET_FILE_POINTER_)
        return 0;
    if(lpNewFilePointer)
    {
        lpNewFilePointer->u.LowPart = dwLowPart;
        lpNewFilePointer->u.HighPart = dwHighPart;
    }
    return 1;
}

} // detail

file_win32::
~file_win32()
{
    if(h_ != boost::winapi::INVALID_HANDLE_VALUE_)
        boost::winapi::CloseHandle(h_);
}

file_win32::
file_win32(file_win32&& other)
    : h_(boost::exchange(other.h_,
        boost::winapi::INVALID_HANDLE_VALUE_))
{
}

file_win32&
file_win32::
operator=(file_win32&& other)
{
    if(&other == this)
        return *this;
    if(h_)
        boost::winapi::CloseHandle(h_);
    h_ = other.h_;
    other.h_ = boost::winapi::INVALID_HANDLE_VALUE_;
    return *this;
}

void
file_win32::
native_handle(native_handle_type h)
{
     if(h_ != boost::winapi::INVALID_HANDLE_VALUE_)
        boost::winapi::CloseHandle(h_);
    h_ = h;
}

void
file_win32::
close(error_code& ec)
{
    if(h_ != boost::winapi::INVALID_HANDLE_VALUE_)
    {
        if(! boost::winapi::CloseHandle(h_))
            ec.assign(boost::winapi::GetLastError(),
                system_category());
        else
            ec = {};
        h_ = boost::winapi::INVALID_HANDLE_VALUE_;
    }
    else
    {
        ec = {};
    }
}

void
file_win32::
open(char const* path, file_mode mode, error_code& ec)
{
    if(h_ != boost::winapi::INVALID_HANDLE_VALUE_)
    {
        boost::winapi::CloseHandle(h_);
        h_ = boost::winapi::INVALID_HANDLE_VALUE_;
    }
    boost::winapi::DWORD_ share_mode = 0;
    boost::winapi::DWORD_ desired_access = 0;
    boost::winapi::DWORD_ creation_disposition = 0;
    boost::winapi::DWORD_ flags_and_attributes = 0;
/*
                             |                    When the file...
    This argument:           |             Exists            Does not exist
    -------------------------+------------------------------------------------------
    CREATE_ALWAYS            |            Truncates             Creates
    CREATE_NEW         +-----------+        Fails               Creates
    OPEN_ALWAYS     ===| does this |===>    Opens               Creates
    OPEN_EXISTING      +-----------+        Opens                Fails
    TRUNCATE_EXISTING        |            Truncates              Fails
*/
    switch(mode)
    {
    default:
    case file_mode::read:
        desired_access = boost::winapi::GENERIC_READ_;
        share_mode = boost::winapi::FILE_SHARE_READ_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case file_mode::scan:           
        desired_access = boost::winapi::GENERIC_READ_;
        share_mode = boost::winapi::FILE_SHARE_READ_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x08000000; // FILE_FLAG_SEQUENTIAL_SCAN
        break;

    case file_mode::write:          
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::CREATE_ALWAYS_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case file_mode::write_new:      
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::CREATE_NEW_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case file_mode::write_existing: 
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x10000000; // FILE_FLAG_RANDOM_ACCESS
        break;

    case file_mode::append:         
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;

        creation_disposition = boost::winapi::CREATE_ALWAYS_;
        flags_and_attributes = 0x08000000; // FILE_FLAG_SEQUENTIAL_SCAN
        break;

    case file_mode::append_existing:
        desired_access = boost::winapi::GENERIC_READ_ |
                         boost::winapi::GENERIC_WRITE_;
        creation_disposition = boost::winapi::OPEN_EXISTING_;
        flags_and_attributes = 0x08000000; // FILE_FLAG_SEQUENTIAL_SCAN
        break;
    }
    
    detail::win32_unicode_path unicode_path(path, ec);
    if (ec)
        return;
    h_ = ::CreateFileW(
        unicode_path.c_str(),
        desired_access,
        share_mode,
        NULL,
        creation_disposition,
        flags_and_attributes,
        NULL);
    if(h_ == boost::winapi::INVALID_HANDLE_VALUE_)
        ec.assign(boost::winapi::GetLastError(),
            system_category());
    else
        ec = {};
}

std::uint64_t
file_win32::
size(error_code& ec) const
{
    if(h_ == boost::winapi::INVALID_HANDLE_VALUE_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    boost::winapi::LARGE_INTEGER_ fileSize;
    if(! boost::winapi::GetFileSizeEx(h_, &fileSize))
    {
        ec.assign(boost::winapi::GetLastError(),
            system_category());
        return 0;
    }
    ec = {};
    return fileSize.QuadPart;
}

std::uint64_t
file_win32::
pos(error_code& ec)
{
    if(h_ == boost::winapi::INVALID_HANDLE_VALUE_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    boost::winapi::LARGE_INTEGER_ in;
    boost::winapi::LARGE_INTEGER_ out;
    in.QuadPart = 0;
    if(! detail::set_file_pointer_ex(h_, in, &out,
        boost::winapi::FILE_CURRENT_))
    {
        ec.assign(boost::winapi::GetLastError(),
            system_category());
        return 0;
    }
    ec = {};
    return out.QuadPart;
}

void
file_win32::
seek(std::uint64_t offset, error_code& ec)
{
    if(h_ == boost::winapi::INVALID_HANDLE_VALUE_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return;
    }
    boost::winapi::LARGE_INTEGER_ in;
    in.QuadPart = offset;
    if(! detail::set_file_pointer_ex(h_, in, 0,
        boost::winapi::FILE_BEGIN_))
    {
        ec.assign(boost::winapi::GetLastError(),
            system_category());
        return;
    }
    ec = {};
}

std::size_t
file_win32::
read(void* buffer, std::size_t n, error_code& ec)
{
    if(h_ == boost::winapi::INVALID_HANDLE_VALUE_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    std::size_t nread = 0;
    while(n > 0)
    {
        boost::winapi::DWORD_ amount;
        if(n > (std::numeric_limits<
                boost::winapi::DWORD_>::max)())
            amount = (std::numeric_limits<
                boost::winapi::DWORD_>::max)();
        else
            amount = static_cast<
                boost::winapi::DWORD_>(n);
        boost::winapi::DWORD_ bytesRead;
        if(! ::ReadFile(h_, buffer, amount, &bytesRead, 0))
        {
            auto const dwError = boost::winapi::GetLastError();
            if(dwError != boost::winapi::ERROR_HANDLE_EOF_)
                ec.assign(dwError, system_category());
            else
                ec = {};
            return nread;
        }
        if(bytesRead == 0)
            return nread;
        n -= bytesRead;
        nread += bytesRead;
        buffer = static_cast<char*>(buffer) + bytesRead;
    }
    ec = {};
    return nread;
}

std::size_t
file_win32::
write(void const* buffer, std::size_t n, error_code& ec)
{
    if(h_ == boost::winapi::INVALID_HANDLE_VALUE_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    std::size_t nwritten = 0;
    while(n > 0)
    {
        boost::winapi::DWORD_ amount;
        if(n > (std::numeric_limits<
                boost::winapi::DWORD_>::max)())
            amount = (std::numeric_limits<
                boost::winapi::DWORD_>::max)();
        else
            amount = static_cast<
                boost::winapi::DWORD_>(n);
        boost::winapi::DWORD_ bytesWritten;
        if(! ::WriteFile(h_, buffer, amount, &bytesWritten, 0))
        {
            auto const dwError = boost::winapi::GetLastError();
            if(dwError != boost::winapi::ERROR_HANDLE_EOF_)
                ec.assign(dwError, system_category());
            else
                ec = {};
            return nwritten;
        }
        if(bytesWritten == 0)
            return nwritten;
        n -= bytesWritten;
        nwritten += bytesWritten;
        buffer = static_cast<char const*>(buffer) + bytesWritten;
    }
    ec = {};
    return nwritten;
}

} // beast
} // boost

#endif

#endif
