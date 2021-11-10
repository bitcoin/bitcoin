// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_SAFE_DUMP_WIN_IPP
#define BOOST_STACKTRACE_DETAIL_SAFE_DUMP_WIN_IPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/stacktrace/safe_dump_to.hpp>

#include <boost/core/noncopyable.hpp>

#include <boost/winapi/get_current_process.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/access_rights.hpp>

namespace boost { namespace stacktrace { namespace detail {

std::size_t dump(void* /*fd*/, const native_frame_ptr_t* /*frames*/, std::size_t /*frames_count*/) BOOST_NOEXCEPT {
#if 0 // This code potentially could cause deadlocks (according to the MSDN). Disabled
    boost::winapi::DWORD_ written;
    const boost::winapi::DWORD_ bytes_to_write = static_cast<boost::winapi::DWORD_>(
        sizeof(native_frame_ptr_t) * frames_count
    );
    if (!boost::winapi::WriteFile(fd, frames, bytes_to_write, &written, 0)) {
        return 0;
    }

    return frames_count;
#endif
    return 0;
}

std::size_t dump(const char* /*file*/, const native_frame_ptr_t* /*frames*/, std::size_t /*frames_count*/) BOOST_NOEXCEPT {
#if 0 // This code causing deadlocks on some platforms. Disabled
    void* const fd = boost::winapi::CreateFileA(
        file,
        boost::winapi::GENERIC_WRITE_,
        0,
        0,
        boost::winapi::CREATE_ALWAYS_,
        boost::winapi::FILE_ATTRIBUTE_NORMAL_,
        0
    );

    if (fd == boost::winapi::invalid_handle_value) {
        return 0;
    }

    const std::size_t size = boost::stacktrace::detail::dump(fd, frames, frames_count);
    boost::winapi::CloseHandle(fd);
    return size;
#endif
    return 0;
}

}}} // namespace boost::stacktrace::detail

#endif // BOOST_STACKTRACE_DETAIL_SAFE_DUMP_WIN_IPP
