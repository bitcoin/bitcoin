// Copyright Antony Polukhin, 2016-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_DETAIL_LIBBACKTRACE_IMPLS_HPP
#define BOOST_STACKTRACE_DETAIL_LIBBACKTRACE_IMPLS_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/stacktrace/detail/to_hex_array.hpp>
#include <boost/stacktrace/detail/to_dec_array.hpp>
#include <boost/stacktrace/detail/location_from_symbol.hpp>
#include <boost/core/demangle.hpp>

#ifdef BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE
#   include BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE
#else
#   include <backtrace.h>
#endif

namespace boost { namespace stacktrace { namespace detail {


struct pc_data {
    std::string* function;
    std::string* filename;
    std::size_t line;
};

inline void libbacktrace_syminfo_callback(void *data, uintptr_t /*pc*/, const char *symname, uintptr_t /*symval*/, uintptr_t /*symsize*/) {
    pc_data& d = *static_cast<pc_data*>(data);
    if (d.function && symname) {
        *d.function = symname;
    }
}

// Old versions of libbacktrace have different signature for the callback
inline void libbacktrace_syminfo_callback(void *data, uintptr_t pc, const char *symname, uintptr_t symval) {
    boost::stacktrace::detail::libbacktrace_syminfo_callback(data, pc, symname, symval, 0);
}

inline int libbacktrace_full_callback(void *data, uintptr_t /*pc*/, const char *filename, int lineno, const char *function) {
    pc_data& d = *static_cast<pc_data*>(data);
    if (d.filename && filename) {
        *d.filename = filename;
    }
    if (d.function && function) {
        *d.function = function;
    }
    d.line = lineno;
    return 0;
}

inline void libbacktrace_error_callback(void* /*data*/, const char* /*msg*/, int /*errnum*/) BOOST_NOEXCEPT {
    // Do nothing, just return.
}

// Not async-signal-safe, so this method is not called from async-safe functions.
//
// This function is not async signal safe because:
// * Dynamic initialization of a block-scope variable with static storage duration could lock a mutex
// * No guarantees on `backtrace_create_state` function.
//
// Currently `backtrace_create_state` can not detect file name on Windows https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82543
// That's why we provide a `prog_location` here.
BOOST_SYMBOL_VISIBLE inline ::backtrace_state* construct_state(const program_location& prog_location) BOOST_NOEXCEPT {
    // [dcl.inline]: A static local variable in an inline function with external linkage always refers to the same object.

    // TODO: The most obvious solution:
    //
    //static ::backtrace_state* state = ::backtrace_create_state(
    //    prog_location.name(),
    //    1, // allow safe concurrent usage of the same state
    //    boost::stacktrace::detail::libbacktrace_error_callback,
    //    0 // pointer to data that will be passed to callback
    //);
    //
    //
    // Unfortunately, that solution segfaults when `construct_state()` function is in .so file
    // and multiple threads concurrently work with state.


#ifndef BOOST_HAS_THREADS
    static
#else

    // Result of `construct_state()` invocation is not stored by the callers, so `thread_local`
    // gives a single `state` per thread and that state is not shared between threads in any way.

#   ifndef BOOST_NO_CXX11_THREAD_LOCAL
    thread_local
#   elif defined(__GNUC__) && !defined(__clang__)
    static __thread
#   else
    /* just a local variable */
#   endif

#endif
      ::backtrace_state* state = ::backtrace_create_state(
        prog_location.name(),
        0,
        boost::stacktrace::detail::libbacktrace_error_callback,
        0
    );
    return state;
}

struct to_string_using_backtrace {
    std::string res;
    boost::stacktrace::detail::program_location prog_location;
    ::backtrace_state* state;
    std::string filename;
    std::size_t line;

    void prepare_function_name(const void* addr) {
        boost::stacktrace::detail::pc_data data = {&res, &filename, 0};
        if (state) {
            ::backtrace_pcinfo(
                state,
                reinterpret_cast<uintptr_t>(addr),
                boost::stacktrace::detail::libbacktrace_full_callback,
                boost::stacktrace::detail::libbacktrace_error_callback,
                &data
            ) 
            ||
            ::backtrace_syminfo(
                state,
                reinterpret_cast<uintptr_t>(addr),
                boost::stacktrace::detail::libbacktrace_syminfo_callback,
                boost::stacktrace::detail::libbacktrace_error_callback,
                &data
            );
        }
        line = data.line;
    }

    bool prepare_source_location(const void* /*addr*/) {
        if (filename.empty() || !line) {
            return false;
        }

        res += " at ";
        res += filename;
        res += ':';
        res += boost::stacktrace::detail::to_dec_array(line).data();
        return true;
    }

    to_string_using_backtrace() BOOST_NOEXCEPT {
        state = boost::stacktrace::detail::construct_state(prog_location);
    }
};

template <class Base> class to_string_impl_base;
typedef to_string_impl_base<to_string_using_backtrace> to_string_impl;

inline std::string name_impl(const void* addr) {
    std::string res;

    boost::stacktrace::detail::program_location prog_location;
    ::backtrace_state* state = boost::stacktrace::detail::construct_state(prog_location);

    boost::stacktrace::detail::pc_data data = {&res, 0, 0};
    if (state) {
        ::backtrace_pcinfo(
            state,
            reinterpret_cast<uintptr_t>(addr),
            boost::stacktrace::detail::libbacktrace_full_callback,
            boost::stacktrace::detail::libbacktrace_error_callback,
            &data
        )
        ||
        ::backtrace_syminfo(
            state,
            reinterpret_cast<uintptr_t>(addr),
            boost::stacktrace::detail::libbacktrace_syminfo_callback,
            boost::stacktrace::detail::libbacktrace_error_callback,
            &data
        );
    }
    if (!res.empty()) {
        res = boost::core::demangle(res.c_str());
    }

    return res;
}

} // namespace detail

std::string frame::source_file() const {
    std::string res;

    if (!addr_) {
        return res;
    }

    boost::stacktrace::detail::program_location prog_location;
    ::backtrace_state* state = boost::stacktrace::detail::construct_state(prog_location);

    boost::stacktrace::detail::pc_data data = {0, &res, 0};
    if (state) {
        ::backtrace_pcinfo(
            state,
            reinterpret_cast<uintptr_t>(addr_),
            boost::stacktrace::detail::libbacktrace_full_callback,
            boost::stacktrace::detail::libbacktrace_error_callback,
            &data
        );
    }

    return res;
}

std::size_t frame::source_line() const {
    if (!addr_) {
        return 0;
    }

    boost::stacktrace::detail::program_location prog_location;
    ::backtrace_state* state = boost::stacktrace::detail::construct_state(prog_location);

    boost::stacktrace::detail::pc_data data = {0, 0, 0};
    if (state) {
        ::backtrace_pcinfo(
            state,
            reinterpret_cast<uintptr_t>(addr_),
            boost::stacktrace::detail::libbacktrace_full_callback,
            boost::stacktrace::detail::libbacktrace_error_callback,
            &data
        );
    }

    return data.line;
}


}} // namespace boost::stacktrace

#endif // BOOST_STACKTRACE_DETAIL_LIBBACKTRACE_IMPLS_HPP
