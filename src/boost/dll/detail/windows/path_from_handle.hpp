// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_WINDOWS_PATH_FROM_HANDLE_HPP
#define BOOST_DLL_DETAIL_WINDOWS_PATH_FROM_HANDLE_HPP

#include <boost/dll/config.hpp>
#include <boost/dll/detail/system_error.hpp>
#include <boost/winapi/dll.hpp>
#include <boost/winapi/get_last_error.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace dll { namespace detail {

    inline boost::dll::fs::error_code last_error_code() BOOST_NOEXCEPT {
        boost::winapi::DWORD_ err = boost::winapi::GetLastError();
        return boost::dll::fs::error_code(
            static_cast<int>(err),
            boost::dll::fs::system_category()
        );
    }

    inline boost::dll::fs::path path_from_handle(boost::winapi::HMODULE_ handle, boost::dll::fs::error_code &ec) {
        BOOST_STATIC_CONSTANT(boost::winapi::DWORD_, ERROR_INSUFFICIENT_BUFFER_ = 0x7A);
        BOOST_STATIC_CONSTANT(boost::winapi::DWORD_, DEFAULT_PATH_SIZE_ = 260);

        // On success, GetModuleFileNameW() doesn't reset last error to ERROR_SUCCESS. Resetting it manually.
        boost::winapi::GetLastError();

        // If `handle` parameter is NULL, GetModuleFileName retrieves the path of the
        // executable file of the current process.
        boost::winapi::WCHAR_ path_hldr[DEFAULT_PATH_SIZE_];
        boost::winapi::GetModuleFileNameW(handle, path_hldr, DEFAULT_PATH_SIZE_);
        ec = boost::dll::detail::last_error_code();
        if (!ec) {
            return boost::dll::fs::path(path_hldr);
        }

        for (unsigned i = 2; i < 1025 && static_cast<boost::winapi::DWORD_>(ec.value()) == ERROR_INSUFFICIENT_BUFFER_; i *= 2) {
            std::wstring p(DEFAULT_PATH_SIZE_ * i, L'\0');
            const std::size_t size = boost::winapi::GetModuleFileNameW(handle, &p[0], DEFAULT_PATH_SIZE_ * i);
            ec = boost::dll::detail::last_error_code();

            if (!ec) {
                p.resize(size);
                return boost::dll::fs::path(p);
            }
        }

        // Error other than ERROR_INSUFFICIENT_BUFFER_ occurred or failed to allocate buffer big enough.
        return boost::dll::fs::path();
    }

}}} // namespace boost::dll::detail

#endif // BOOST_DLL_DETAIL_WINDOWS_PATH_FROM_HANDLE_HPP

