/*
 * Copyright 2017 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_STACK_BACKTRACE_HPP_INCLUDED_
#define BOOST_WINAPI_STACK_BACKTRACE_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

// MinGW does not provide RtlCaptureStackBackTrace
#if !defined( BOOST_WINAPI_IS_MINGW )

// Note: RtlCaptureStackBackTrace is available in WinXP SP1 and later
#if (BOOST_USE_NTDDI_VERSION > BOOST_WINAPI_NTDDI_WINXP)

#if BOOST_WINAPI_PARTITION_APP_SYSTEM

#include <boost/winapi/detail/header.hpp>

// Windows SDK shipped with MSVC 7.1 and 8 does not declare RtlCaptureStackBackTrace in headers but allows to link with it
#if !defined( BOOST_USE_WINDOWS_H ) || (defined(_MSC_VER) && (_MSC_VER+0) < 1500)
extern "C" {

BOOST_WINAPI_IMPORT boost::winapi::WORD_
BOOST_WINAPI_NTAPI_CC RtlCaptureStackBackTrace(
    boost::winapi::DWORD_ FramesToSkip,
    boost::winapi::DWORD_ FramesToCapture,
    boost::winapi::PVOID_* BackTrace,
    boost::winapi::PDWORD_ BackTraceHash);

} // extern "C"
#endif

namespace boost {
namespace winapi {

using ::RtlCaptureStackBackTrace;

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_PARTITION_APP_SYSTEM
#endif // (BOOST_USE_NTDDI_VERSION > BOOST_WINAPI_NTDDI_WINXP)
#endif // !defined( BOOST_WINAPI_IS_MINGW )
#endif // BOOST_WINAPI_STACK_BACKTRACE_HPP_INCLUDED_
