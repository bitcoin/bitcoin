//
//  boost/signals2/detail/lwm_win32_cs.hpp
//
//  Copyright (c) 2002, 2003 Peter Dimov
//  Copyright (c) 2008 Frank Mori Hess
//  Copyright (c) Microsoft Corporation 2014
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SIGNALS2_LWM_WIN32_CS_HPP
#define BOOST_SIGNALS2_LWM_WIN32_CS_HPP

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/predef.h>
#include <boost/assert.hpp>

#ifdef BOOST_USE_WINDOWS_H

#include <windows.h>

#else

struct _RTL_CRITICAL_SECTION;

#endif

namespace boost
{

namespace signals2
{

namespace detail
{

#ifndef BOOST_USE_WINDOWS_H

struct critical_section
{
    struct critical_section_debug * DebugInfo;
    long LockCount;
    long RecursionCount;
    void * OwningThread;
    void * LockSemaphore;
#if defined(_WIN64)
    unsigned __int64 SpinCount;
#else
    unsigned long SpinCount;
#endif
};

#if BOOST_PLAT_WINDOWS_RUNTIME
extern "C" __declspec(dllimport) void __stdcall InitializeCriticalSectionEx(::_RTL_CRITICAL_SECTION *, unsigned long, unsigned long);
#else
extern "C" __declspec(dllimport) void __stdcall InitializeCriticalSection(::_RTL_CRITICAL_SECTION *);
#endif
extern "C" __declspec(dllimport) void __stdcall EnterCriticalSection(::_RTL_CRITICAL_SECTION *);
extern "C" __declspec(dllimport) int __stdcall TryEnterCriticalSection(::_RTL_CRITICAL_SECTION *);
extern "C" __declspec(dllimport) void __stdcall LeaveCriticalSection(::_RTL_CRITICAL_SECTION *);
extern "C" __declspec(dllimport) void __stdcall DeleteCriticalSection(::_RTL_CRITICAL_SECTION *);

typedef ::_RTL_CRITICAL_SECTION rtl_critical_section;

#else // #ifndef BOOST_USE_WINDOWS_H

typedef ::CRITICAL_SECTION critical_section;

#if BOOST_PLAT_WINDOWS_RUNTIME
using ::InitializeCriticalSectionEx;
#else
using ::InitializeCriticalSection;
#endif
using ::EnterCriticalSection;
using ::TryEnterCriticalSection;
using ::LeaveCriticalSection;
using ::DeleteCriticalSection;

typedef ::CRITICAL_SECTION rtl_critical_section;

#endif // #ifndef BOOST_USE_WINDOWS_H

} // namespace detail

class mutex
{
private:

    boost::signals2::detail::critical_section cs_;

    mutex(mutex const &);
    mutex & operator=(mutex const &);

public:

    mutex()
    {
#if BOOST_PLAT_WINDOWS_RUNTIME
        boost::signals2::detail::InitializeCriticalSectionEx(reinterpret_cast< boost::signals2::detail::rtl_critical_section* >(&cs_), 4000, 0);
#else
        boost::signals2::detail::InitializeCriticalSection(reinterpret_cast< boost::signals2::detail::rtl_critical_section* >(&cs_)); 
#endif
    }

    ~mutex()
    {
        boost::signals2::detail::DeleteCriticalSection(reinterpret_cast< boost::signals2::detail::rtl_critical_section* >(&cs_)); 
    }

    void lock()
    {
        boost::signals2::detail::EnterCriticalSection(reinterpret_cast< boost::signals2::detail::rtl_critical_section* >(&cs_)); 
    }
// TryEnterCriticalSection only exists on Windows NT 4.0 and later
#if (defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400))
    bool try_lock()
    {
        return boost::signals2::detail::TryEnterCriticalSection(reinterpret_cast< boost::signals2::detail::rtl_critical_section* >(&cs_)) != 0;
    }
#else
    bool try_lock()
    {
        BOOST_ASSERT(false);
        return false;
    }
#endif
    void unlock()
    {
        boost::signals2::detail::LeaveCriticalSection(reinterpret_cast< boost::signals2::detail::rtl_critical_section* >(&cs_));
    }
};

} // namespace signals2

} // namespace boost

#endif // #ifndef BOOST_SIGNALS2_LWM_WIN32_CS_HPP
