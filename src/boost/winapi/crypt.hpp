/*
 * Copyright 2014 Antony Polukhin
 * Copyright 2015 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_CRYPT_HPP_INCLUDED_
#define BOOST_WINAPI_CRYPT_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#if defined( BOOST_USE_WINDOWS_H )
// This header is not always included as part of windows.h
#include <wincrypt.h>
#endif
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
namespace boost { namespace winapi {
typedef ULONG_PTR_ HCRYPTPROV_;
}}

extern "C" {
#if BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM
#if !defined( BOOST_NO_ANSI_APIS )
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptEnumProvidersA(
    boost::winapi::DWORD_ dwIndex,
    boost::winapi::DWORD_ *pdwReserved,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::DWORD_ *pdwProvType,
    boost::winapi::LPSTR_ szProvName,
    boost::winapi::DWORD_ *pcbProvName);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptAcquireContextA(
    boost::winapi::HCRYPTPROV_ *phProv,
    boost::winapi::LPCSTR_ pszContainer,
    boost::winapi::LPCSTR_ pszProvider,
    boost::winapi::DWORD_ dwProvType,
    boost::winapi::DWORD_ dwFlags);
#endif // !defined( BOOST_NO_ANSI_APIS )

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptEnumProvidersW(
    boost::winapi::DWORD_ dwIndex,
    boost::winapi::DWORD_ *pdwReserved,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::DWORD_ *pdwProvType,
    boost::winapi::LPWSTR_ szProvName,
    boost::winapi::DWORD_ *pcbProvName);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptAcquireContextW(
    boost::winapi::HCRYPTPROV_ *phProv,
    boost::winapi::LPCWSTR_ szContainer,
    boost::winapi::LPCWSTR_ szProvider,
    boost::winapi::DWORD_ dwProvType,
    boost::winapi::DWORD_ dwFlags);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptGenRandom(
    boost::winapi::HCRYPTPROV_ hProv,
    boost::winapi::DWORD_ dwLen,
    boost::winapi::BYTE_ *pbBuffer);
#endif // BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
#if defined(_MSC_VER) && (_MSC_VER+0) >= 1500 && (_MSC_VER+0) < 1900 && BOOST_USE_NTDDI_VERSION < BOOST_WINAPI_NTDDI_WINXP
// Standalone MS Windows SDK 6.0A and later until 10.0 provide a different declaration of CryptReleaseContext for Windows 2000 and older.
// This is not the case for (a) MinGW and MinGW-w64, (b) MSVC 7.1 and 8, which are shipped with their own Windows SDK,
// and (c) MSVC 14.0 and later, which are used with Windows SDK 10.
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptReleaseContext(
    boost::winapi::HCRYPTPROV_ hProv,
    boost::winapi::ULONG_PTR_ dwFlags);
#else
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
CryptReleaseContext(
    boost::winapi::HCRYPTPROV_ hProv,
    boost::winapi::DWORD_ dwFlags);
#endif
#endif // BOOST_WINAPI_PARTITION_APP_SYSTEM
}
#endif // !defined( BOOST_USE_WINDOWS_H )

namespace boost {
namespace winapi {

#if defined( BOOST_USE_WINDOWS_H )

typedef ::HCRYPTPROV HCRYPTPROV_;

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_CONSTEXPR_OR_CONST DWORD_ PROV_RSA_FULL_         = PROV_RSA_FULL;

BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_VERIFYCONTEXT_   = CRYPT_VERIFYCONTEXT;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_NEWKEYSET_       = CRYPT_NEWKEYSET;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_DELETEKEYSET_    = CRYPT_DELETEKEYSET;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_MACHINE_KEYSET_  = CRYPT_MACHINE_KEYSET;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_SILENT_          = CRYPT_SILENT;
#endif

#else

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_CONSTEXPR_OR_CONST DWORD_ PROV_RSA_FULL_         = 1;

BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_VERIFYCONTEXT_   = 0xF0000000;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_NEWKEYSET_       = 8;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_DELETEKEYSET_    = 16;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_MACHINE_KEYSET_  = 32;
BOOST_CONSTEXPR_OR_CONST DWORD_ CRYPT_SILENT_          = 64;
#endif

#endif

#if BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM

#if !defined( BOOST_NO_ANSI_APIS )
using ::CryptEnumProvidersA;
using ::CryptAcquireContextA;
#endif
using ::CryptEnumProvidersW;
using ::CryptAcquireContextW;
using ::CryptGenRandom;

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE BOOL_ crypt_enum_providers(
    DWORD_ dwIndex,
    DWORD_ *pdwReserved,
    DWORD_ dwFlags,
    DWORD_ *pdwProvType,
    LPSTR_ szProvName,
    DWORD_ *pcbProvName)
{
    return ::CryptEnumProvidersA(dwIndex, pdwReserved, dwFlags, pdwProvType, szProvName, pcbProvName);
}

BOOST_FORCEINLINE BOOL_ crypt_acquire_context(
    HCRYPTPROV_ *phProv,
    LPCSTR_ pszContainer,
    LPCSTR_ pszProvider,
    DWORD_ dwProvType,
    DWORD_ dwFlags)
{
    return ::CryptAcquireContextA(phProv, pszContainer, pszProvider, dwProvType, dwFlags);
}
#endif

BOOST_FORCEINLINE BOOL_ crypt_enum_providers(
    DWORD_ dwIndex,
    DWORD_ *pdwReserved,
    DWORD_ dwFlags,
    DWORD_ *pdwProvType,
    LPWSTR_ szProvName,
    DWORD_ *pcbProvName)
{
    return ::CryptEnumProvidersW(dwIndex, pdwReserved, dwFlags, pdwProvType, szProvName, pcbProvName);
}

BOOST_FORCEINLINE BOOL_ crypt_acquire_context(
    HCRYPTPROV_ *phProv,
    LPCWSTR_ szContainer,
    LPCWSTR_ szProvider,
    DWORD_ dwProvType,
    DWORD_ dwFlags)
{
    return ::CryptAcquireContextW(phProv, szContainer, szProvider, dwProvType, dwFlags);
}

#endif // BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_FORCEINLINE BOOL_ CryptReleaseContext(HCRYPTPROV_ hProv, DWORD_ dwFlags)
{
    return ::CryptReleaseContext(hProv, dwFlags);
}
#endif

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_CRYPT_HPP_INCLUDED_
