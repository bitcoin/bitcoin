/*
 * Copyright 2010 Vicente J. Botet Escriba
 * Copyright 2015 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_CRITICAL_SECTION_HPP_INCLUDED_
#define BOOST_WINAPI_CRITICAL_SECTION_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/cast_ptr.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )

extern "C" {
#if !defined( BOOST_WINAPI_IS_MINGW )

// Windows CE uses a different name for the structure
#if defined (_WIN32_WCE)
struct CRITICAL_SECTION;
namespace boost {
namespace winapi {
namespace detail {
    typedef CRITICAL_SECTION winsdk_critical_section;
}
}
}
#else
struct _RTL_CRITICAL_SECTION;
namespace boost {
namespace winapi {
namespace detail {
    typedef _RTL_CRITICAL_SECTION winsdk_critical_section;
}
}
}
#endif

#else
// MinGW uses a different name for the structure
struct _CRITICAL_SECTION;

namespace boost {
namespace winapi {
namespace detail {
    typedef _CRITICAL_SECTION winapi_critical_section;
}
}
}
#endif

#if !defined( BOOST_WINAPI_IS_MINGW )

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
InitializeCriticalSection(boost::winapi::detail::winsdk_critical_section* lpCriticalSection);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
EnterCriticalSection(boost::winapi::detail::winsdk_critical_section* lpCriticalSection);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
LeaveCriticalSection(boost::winapi::detail::winsdk_critical_section* lpCriticalSection);

#if BOOST_USE_WINAPI_VERSION >= 0x0403
#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
InitializeCriticalSectionAndSpinCount(
    boost::winapi::detail::winsdk_critical_section* lpCriticalSection,
    boost::winapi::DWORD_ dwSpinCount);

BOOST_WINAPI_IMPORT boost::winapi::DWORD_ BOOST_WINAPI_WINAPI_CC
SetCriticalSectionSpinCount(
    boost::winapi::detail::winsdk_critical_section* lpCriticalSection,
    boost::winapi::DWORD_ dwSpinCount);
#endif

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
InitializeCriticalSectionEx(
    boost::winapi::detail::winsdk_critical_section* lpCriticalSection,
    boost::winapi::DWORD_ dwSpinCount,
    boost::winapi::DWORD_ Flags);
#endif
#endif

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_NT4
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
TryEnterCriticalSection(boost::winapi::detail::winsdk_critical_section* lpCriticalSection);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
DeleteCriticalSection(boost::winapi::detail::winsdk_critical_section* lpCriticalSection);

#else // defined( BOOST_WINAPI_IS_MINGW )

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
InitializeCriticalSection(boost::winapi::detail::winapi_critical_section* lpCriticalSection);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
EnterCriticalSection(boost::winapi::detail::winapi_critical_section* lpCriticalSection);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
LeaveCriticalSection(boost::winapi::detail::winapi_critical_section* lpCriticalSection);

#if BOOST_USE_WINAPI_VERSION >= 0x0403
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
InitializeCriticalSectionAndSpinCount(
    boost::winapi::detail::winapi_critical_section* lpCriticalSection,
    boost::winapi::DWORD_ dwSpinCount);

BOOST_WINAPI_IMPORT boost::winapi::DWORD_ BOOST_WINAPI_WINAPI_CC
SetCriticalSectionSpinCount(
    boost::winapi::detail::winapi_critical_section* lpCriticalSection,
    boost::winapi::DWORD_ dwSpinCount);
#endif

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
InitializeCriticalSectionEx(
    boost::winapi::detail::winapi_critical_section* lpCriticalSection,
    boost::winapi::DWORD_ dwSpinCount,
    boost::winapi::DWORD_ Flags);
#endif

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_NT4
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
TryEnterCriticalSection(boost::winapi::detail::winapi_critical_section* lpCriticalSection);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
DeleteCriticalSection(boost::winapi::detail::winapi_critical_section* lpCriticalSection);

#endif // defined( BOOST_WINAPI_IS_MINGW )
} // extern "C"
#endif

namespace boost {
namespace winapi {


#pragma pack(push, 8)

#if !defined(_WIN32_WCE)

struct _RTL_CRITICAL_SECTION_DEBUG;

typedef struct BOOST_MAY_ALIAS _RTL_CRITICAL_SECTION {
    _RTL_CRITICAL_SECTION_DEBUG* DebugInfo;
    LONG_ LockCount;
    LONG_ RecursionCount;
    HANDLE_ OwningThread;
    HANDLE_ LockSemaphore;
    ULONG_PTR_ SpinCount;
} CRITICAL_SECTION_, *PCRITICAL_SECTION_;

#else

// Windows CE has different layout
typedef struct BOOST_MAY_ALIAS CRITICAL_SECTION {
    unsigned int LockCount;
    HANDLE OwnerThread;
    HANDLE hCrit;
    DWORD needtrap;
    DWORD dwContentions;
} CRITICAL_SECTION_, *LPCRITICAL_SECTION_;

#endif

#pragma pack(pop)

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_FORCEINLINE VOID_ InitializeCriticalSection(CRITICAL_SECTION_* lpCriticalSection)
{
    ::InitializeCriticalSection(winapi::detail::cast_ptr(lpCriticalSection));
}
#endif

BOOST_FORCEINLINE VOID_ EnterCriticalSection(CRITICAL_SECTION_* lpCriticalSection)
{
    ::EnterCriticalSection(winapi::detail::cast_ptr(lpCriticalSection));
}

BOOST_FORCEINLINE VOID_ LeaveCriticalSection(CRITICAL_SECTION_* lpCriticalSection)
{
    ::LeaveCriticalSection(winapi::detail::cast_ptr(lpCriticalSection));
}

#if BOOST_USE_WINAPI_VERSION >= 0x0403
#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_FORCEINLINE BOOL_ InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION_* lpCriticalSection, DWORD_ dwSpinCount)
{
    return ::InitializeCriticalSectionAndSpinCount(winapi::detail::cast_ptr(lpCriticalSection), dwSpinCount);
}

BOOST_FORCEINLINE DWORD_ SetCriticalSectionSpinCount(CRITICAL_SECTION_* lpCriticalSection, DWORD_ dwSpinCount)
{
    return ::SetCriticalSectionSpinCount(winapi::detail::cast_ptr(lpCriticalSection), dwSpinCount);
}
#endif

// CRITICAL_SECTION_NO_DEBUG_INFO is defined for WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
BOOST_CONSTEXPR_OR_CONST DWORD_ CRITICAL_SECTION_NO_DEBUG_INFO_ = 0x01000000;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRITICAL_SECTION_FLAG_NO_DEBUG_INFO_ = CRITICAL_SECTION_NO_DEBUG_INFO_;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRITICAL_SECTION_FLAG_DYNAMIC_SPIN_ = 0x02000000; // undocumented
BOOST_CONSTEXPR_OR_CONST DWORD_ CRITICAL_SECTION_FLAG_STATIC_INIT_ = 0x04000000; // undocumented

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_FORCEINLINE BOOL_ InitializeCriticalSectionEx(CRITICAL_SECTION_* lpCriticalSection, DWORD_ dwSpinCount, DWORD_ Flags)
{
    return ::InitializeCriticalSectionEx(winapi::detail::cast_ptr(lpCriticalSection), dwSpinCount, Flags);
}
#endif // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
#endif // BOOST_USE_WINAPI_VERSION >= 0x0403

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_NT4
BOOST_FORCEINLINE BOOL_ TryEnterCriticalSection(CRITICAL_SECTION_* lpCriticalSection)
{
    return ::TryEnterCriticalSection(winapi::detail::cast_ptr(lpCriticalSection));
}
#endif // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_NT4

BOOST_FORCEINLINE VOID_ DeleteCriticalSection(CRITICAL_SECTION_* lpCriticalSection)
{
    ::DeleteCriticalSection(winapi::detail::cast_ptr(lpCriticalSection));
}

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_CRITICAL_SECTION_HPP_INCLUDED_
