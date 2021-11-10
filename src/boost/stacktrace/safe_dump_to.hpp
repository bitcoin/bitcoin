// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_SAFE_DUMP_TO_HPP
#define BOOST_STACKTRACE_SAFE_DUMP_TO_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#if defined(BOOST_WINDOWS)
#include <boost/winapi/config.hpp>
#endif

#include <boost/stacktrace/detail/push_options.h>

#ifdef BOOST_INTEL
#   pragma warning(push)
#   pragma warning(disable:2196) // warning #2196: routine is both "inline" and "noinline"
#endif

/// @file safe_dump_to.hpp This header contains low-level async-signal-safe functions for dumping call stacks. Dumps are binary serialized arrays of `void*`,
/// so you could read them by using 'od -tx8 -An stacktrace_dump_failename' Linux command or using boost::stacktrace::stacktrace::from_dump functions.

namespace boost { namespace stacktrace {

/// @cond
namespace detail {

    typedef const void* native_frame_ptr_t; // TODO: change to `typedef void(*native_frame_ptr_t)();`
    enum helper{ max_frames_dump = 128 };

    BOOST_STACKTRACE_FUNCTION std::size_t from_dump(const char* filename, native_frame_ptr_t* out_frames);
    BOOST_STACKTRACE_FUNCTION std::size_t dump(const char* file, const native_frame_ptr_t* frames, std::size_t frames_count) BOOST_NOEXCEPT;
#if defined(BOOST_WINDOWS)
    BOOST_STACKTRACE_FUNCTION std::size_t dump(void* fd, const native_frame_ptr_t* frames, std::size_t frames_count) BOOST_NOEXCEPT;
#else
    // POSIX
    BOOST_STACKTRACE_FUNCTION std::size_t dump(int fd, const native_frame_ptr_t* frames, std::size_t frames_count) BOOST_NOEXCEPT;
#endif


struct this_thread_frames { // struct is required to avoid warning about usage of inline+BOOST_NOINLINE
    BOOST_NOINLINE BOOST_STACKTRACE_FUNCTION static std::size_t collect(native_frame_ptr_t* out_frames, std::size_t max_frames_count, std::size_t skip) BOOST_NOEXCEPT;

    BOOST_NOINLINE static std::size_t safe_dump_to_impl(void* memory, std::size_t size, std::size_t skip) BOOST_NOEXCEPT {
        typedef boost::stacktrace::detail::native_frame_ptr_t native_frame_ptr_t;

        if (size < sizeof(native_frame_ptr_t)) {
            return 0;
        }

        native_frame_ptr_t* mem = static_cast<native_frame_ptr_t*>(memory);
        const std::size_t frames_count = boost::stacktrace::detail::this_thread_frames::collect(mem, size / sizeof(native_frame_ptr_t) - 1, skip + 1);
        mem[frames_count] = 0;
        return frames_count + 1;
    }

    template <class T>
    BOOST_NOINLINE static std::size_t safe_dump_to_impl(T file, std::size_t skip, std::size_t max_depth) BOOST_NOEXCEPT {
        typedef boost::stacktrace::detail::native_frame_ptr_t native_frame_ptr_t;

        native_frame_ptr_t buffer[boost::stacktrace::detail::max_frames_dump + 1];
        if (max_depth > boost::stacktrace::detail::max_frames_dump) {
            max_depth = boost::stacktrace::detail::max_frames_dump;
        }

        const std::size_t frames_count = boost::stacktrace::detail::this_thread_frames::collect(buffer, max_depth, skip + 1);
        buffer[frames_count] = 0;
        return boost::stacktrace::detail::dump(file, buffer, frames_count + 1);
    }
};

} // namespace detail
/// @endcond

/// @brief Stores current function call sequence into the memory.
///
/// @b Complexity: O(N) where N is call sequence length, O(1) if BOOST_STACKTRACE_USE_NOOP is defined.
///
/// @b Async-Handler-Safety: Safe.
///
/// @returns Stored call sequence depth including terminating zero frame. To get the actually consumed bytes multiply this value by the sizeof(boost::stacktrace::frame::native_frame_ptr_t)
///
/// @param memory Preallocated buffer to store current function call sequence into.
///
/// @param size Size of the preallocated buffer.
BOOST_FORCEINLINE std::size_t safe_dump_to(void* memory, std::size_t size) BOOST_NOEXCEPT {
    return  boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(memory, size, 0);
}

/// @brief Stores current function call sequence into the memory.
///
/// @b Complexity: O(N) where N is call sequence length, O(1) if BOOST_STACKTRACE_USE_NOOP is defined.
///
/// @b Async-Handler-Safety: Safe.
///
/// @returns Stored call sequence depth including terminating zero frame.  To get the actually consumed bytes multiply this value by the sizeof(boost::stacktrace::frame::native_frame_ptr_t)
///
/// @param skip How many top calls to skip and do not store.
///
/// @param memory Preallocated buffer to store current function call sequence into.
///
/// @param size Size of the preallocated buffer.
BOOST_FORCEINLINE std::size_t safe_dump_to(std::size_t skip, void* memory, std::size_t size) BOOST_NOEXCEPT {
    return  boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(memory, size, skip);
}


/// @brief Opens a file and rewrites its content with current function call sequence if such operations are async signal safe.
///
/// @b Complexity: O(N) where N is call sequence length, O(1) if BOOST_STACKTRACE_USE_NOOP is defined.
///
/// @b Async-Handler-Safety: Safe.
///
/// @returns Stored call sequence depth including terminating zero frame.
///
/// @param file File to store current function call sequence.
BOOST_FORCEINLINE std::size_t safe_dump_to(const char* file) BOOST_NOEXCEPT {
    return boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(file, 0, boost::stacktrace::detail::max_frames_dump);
}

/// @brief Opens a file and rewrites its content with current function call sequence if such operations are async signal safe.
///
/// @b Complexity: O(N) where N is call sequence length, O(1) if BOOST_STACKTRACE_USE_NOOP is defined.
///
/// @b Async-Handler-Safety: Safe.
///
/// @returns Stored call sequence depth including terminating zero frame.
///
/// @param skip How many top calls to skip and do not store.
///
/// @param max_depth Max call sequence depth to collect.
///
/// @param file File to store current function call sequence.
BOOST_FORCEINLINE std::size_t safe_dump_to(std::size_t skip, std::size_t max_depth, const char* file) BOOST_NOEXCEPT {
    return boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(file, skip, max_depth);
}

#ifdef BOOST_STACKTRACE_DOXYGEN_INVOKED

/// @brief Writes into the provided file descriptor the current function call sequence if such operation is async signal safe.
///
/// @b Complexity: O(N) where N is call sequence length, O(1) if BOOST_STACKTRACE_USE_NOOP is defined.
///
/// @b Async-Handler-Safety: Safe.
///
/// @returns Stored call sequence depth including terminating zero frame.
///
/// @param file File to store current function call sequence.
BOOST_FORCEINLINE std::size_t safe_dump_to(platform_specific_descriptor fd) BOOST_NOEXCEPT;

/// @brief Writes into the provided file descriptor the current function call sequence if such operation is async signal safe.
///
/// @b Complexity: O(N) where N is call sequence length, O(1) if BOOST_STACKTRACE_USE_NOOP is defined.
///
/// @b Async-Handler-Safety: Safe.
///
/// @returns Stored call sequence depth including terminating zero frame.
///
/// @param skip How many top calls to skip and do not store.
///
/// @param max_depth Max call sequence depth to collect.
///
/// @param file File to store current function call sequence.
BOOST_FORCEINLINE std::size_t safe_dump_to(std::size_t skip, std::size_t max_depth, platform_specific_descriptor fd) BOOST_NOEXCEPT;

#elif defined(BOOST_WINDOWS)

BOOST_FORCEINLINE std::size_t safe_dump_to(void* fd) BOOST_NOEXCEPT {
    return boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(fd, 0, boost::stacktrace::detail::max_frames_dump);
}

BOOST_FORCEINLINE std::size_t safe_dump_to(std::size_t skip, std::size_t max_depth, void* fd) BOOST_NOEXCEPT {
    return boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(fd, skip, max_depth);
}

#else

// POSIX
BOOST_FORCEINLINE std::size_t safe_dump_to(int fd) BOOST_NOEXCEPT {
    return boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(fd, 0, boost::stacktrace::detail::max_frames_dump);
}

BOOST_FORCEINLINE std::size_t safe_dump_to(std::size_t skip, std::size_t max_depth, int fd) BOOST_NOEXCEPT {
    return boost::stacktrace::detail::this_thread_frames::safe_dump_to_impl(fd, skip, max_depth);
}

#endif


}} // namespace boost::stacktrace

#ifdef BOOST_INTEL
#   pragma warning(pop)
#endif

#include <boost/stacktrace/detail/pop_options.h>

#if !defined(BOOST_STACKTRACE_LINK) || defined(BOOST_STACKTRACE_INTERNAL_BUILD_LIBS)
#   if defined(BOOST_STACKTRACE_USE_NOOP)
#       include <boost/stacktrace/detail/safe_dump_noop.ipp>
#       include <boost/stacktrace/detail/collect_noop.ipp>
#   else
#       if defined(BOOST_WINDOWS)
#           include <boost/stacktrace/detail/safe_dump_win.ipp>
#       else
#           include <boost/stacktrace/detail/safe_dump_posix.ipp>
#       endif
#       if defined(BOOST_WINDOWS) && !defined(BOOST_WINAPI_IS_MINGW) // MinGW does not provide RtlCaptureStackBackTrace. MinGW-w64 does.
#           include <boost/stacktrace/detail/collect_msvc.ipp>
#       else
#           include <boost/stacktrace/detail/collect_unwind.ipp>
#       endif
#   endif
#endif

#endif // BOOST_STACKTRACE_SAFE_DUMP_TO_HPP
