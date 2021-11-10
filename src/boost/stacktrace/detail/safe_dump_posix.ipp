// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_SAFE_DUMP_POSIX_IPP
#define BOOST_STACKTRACE_DETAIL_SAFE_DUMP_POSIX_IPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/stacktrace/safe_dump_to.hpp>

#include <unistd.h>     // ::write
#include <fcntl.h>      // ::open
#include <sys/stat.h>   // S_IWUSR and friends


namespace boost { namespace stacktrace { namespace detail {

std::size_t dump(int fd, const native_frame_ptr_t* frames, std::size_t frames_count) BOOST_NOEXCEPT {
    // We do not retry, because this function must be typically called from signal handler so it's:
    //  * to scary to continue in case of EINTR
    //  * EAGAIN or EWOULDBLOCK may occur only in case of O_NONBLOCK is set for fd,
    // so it seems that user does not want to block
    if (::write(fd, frames, sizeof(native_frame_ptr_t) * frames_count) == -1) {
        return 0;
    }

    return frames_count;
}

std::size_t dump(const char* file, const native_frame_ptr_t* frames, std::size_t frames_count) BOOST_NOEXCEPT {
    const int fd = ::open(
        file,
        O_CREAT | O_WRONLY | O_TRUNC,
#if defined(S_IWUSR) && defined(S_IRUSR)    // Workarounds for some Android OSes
        S_IWUSR | S_IRUSR
#elif defined(S_IWRITE) && defined(S_IREAD)
        S_IWRITE | S_IREAD
#else
        0
#endif
    );
    if (fd == -1) {
        return 0;
    }

    const std::size_t size = boost::stacktrace::detail::dump(fd, frames, frames_count);
    ::close(fd);
    return size;
}

}}} // namespace boost::stacktrace::detail

#endif // BOOST_STACKTRACE_DETAIL_SAFE_DUMP_POSIX_IPP
