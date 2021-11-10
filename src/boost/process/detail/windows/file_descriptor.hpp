// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_FILE_DESCRIPTOR_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_FILE_DESCRIPTOR_HPP_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/file_management.hpp>
#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/core/exchange.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

struct file_descriptor
{
    enum mode_t
    {
        read  = 1,
        write = 2,
        read_write = 3
    };
    static ::boost::winapi::DWORD_ desired_access(mode_t mode)
    {
        switch(mode)
        {
        case read:
            return ::boost::winapi::GENERIC_READ_;
        case write:
            return ::boost::winapi::GENERIC_WRITE_;
        case read_write:
            return ::boost::winapi::GENERIC_READ_
                 | ::boost::winapi::GENERIC_WRITE_;
        default:
            return 0u;
        }
    }

    file_descriptor() = default;
    file_descriptor(const boost::filesystem::path& p, mode_t mode = read_write)
        : file_descriptor(p.native(), mode)
    {
    }

    file_descriptor(const std::string & path , mode_t mode = read_write)
#if defined(BOOST_NO_ANSI_APIS)
        : file_descriptor(::boost::process::detail::convert(path), mode)
#else
        : file_descriptor(path.c_str(), mode)
#endif
    {}
    file_descriptor(const std::wstring & path, mode_t mode = read_write)
        : file_descriptor(path.c_str(), mode) {}

    file_descriptor(const char*    path, mode_t mode = read_write)
#if defined(BOOST_NO_ANSI_APIS)
        : file_descriptor(std::string(path), mode)
#else
        : _handle(
                ::boost::winapi::create_file(
                        path,
                        desired_access(mode),
                        ::boost::winapi::FILE_SHARE_READ_ |
                        ::boost::winapi::FILE_SHARE_WRITE_,
                        nullptr,
                        ::boost::winapi::OPEN_ALWAYS_,

                        ::boost::winapi::FILE_ATTRIBUTE_NORMAL_,
                        nullptr
                ))
#endif
    {
    }
    file_descriptor(const wchar_t * path, mode_t mode = read_write)
        : _handle(
            ::boost::winapi::create_file(
                    path,
                    desired_access(mode),
                    ::boost::winapi::FILE_SHARE_READ_ |
                    ::boost::winapi::FILE_SHARE_WRITE_,
                    nullptr,
                    ::boost::winapi::OPEN_ALWAYS_,

                    ::boost::winapi::FILE_ATTRIBUTE_NORMAL_,
                    nullptr
            ))
{

}
    file_descriptor(const file_descriptor & ) = delete;
    file_descriptor(file_descriptor &&other)
        : _handle( boost::exchange(other._handle, ::boost::winapi::INVALID_HANDLE_VALUE_) )
    {
    }

    file_descriptor& operator=(const file_descriptor & ) = delete;
    file_descriptor& operator=(file_descriptor &&other)
    {
        if (_handle != ::boost::winapi::INVALID_HANDLE_VALUE_)
            ::boost::winapi::CloseHandle(_handle);
        _handle = boost::exchange(other._handle, ::boost::winapi::INVALID_HANDLE_VALUE_);
    }

    ~file_descriptor()
    {
        if (_handle != ::boost::winapi::INVALID_HANDLE_VALUE_)
            ::boost::winapi::CloseHandle(_handle);
    }

    ::boost::winapi::HANDLE_ handle() const { return _handle;}

private:
    ::boost::winapi::HANDLE_ _handle = ::boost::winapi::INVALID_HANDLE_VALUE_;
};

}}}}

#endif /* BOOST_PROCESS_DETAIL_WINDOWS_FILE_DESCRIPTOR_HPP_ */
