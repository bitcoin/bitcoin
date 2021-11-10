// Copyright 2014 Renato Tegon Forti, Antony Polukhin.
// Copyright 2015-2021 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_SYSTEM_ERROR_HPP
#define BOOST_DLL_SYSTEM_ERROR_HPP

#include <boost/dll/config.hpp>
#include <boost/predef/os.h>
#include <boost/throw_exception.hpp>

#if !BOOST_OS_WINDOWS
#   include <dlfcn.h>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

namespace boost { namespace dll { namespace detail {

    inline void reset_dlerror() BOOST_NOEXCEPT {
#if !BOOST_OS_WINDOWS
        const char* const error_txt = dlerror();
        (void)error_txt;
#endif
    }

    inline void report_error(const boost::dll::fs::error_code& ec, const char* message) {
#if !BOOST_OS_WINDOWS
        const char* const error_txt = dlerror();
        if (error_txt) {
            boost::throw_exception(
                boost::dll::fs::system_error(
                    ec,
                    message + std::string(" (dlerror system message: ") + error_txt + std::string(")")
                )
            );
        }
#endif

        boost::throw_exception(
            boost::dll::fs::system_error(
                ec, message
            )
        );
    }

}}} // boost::dll::detail

#endif // BOOST_DLL_SYSTEM_ERROR_HPP

