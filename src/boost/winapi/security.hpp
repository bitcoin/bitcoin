/*
 * Copyright 2010 Vicente J. Botet Escriba
 * Copyright 2015 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_SECURITY_HPP_INCLUDED_
#define BOOST_WINAPI_SECURITY_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if BOOST_WINAPI_PARTITION_APP_SYSTEM

#include <boost/winapi/detail/header.hpp>

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
struct _ACL;
struct _SECURITY_DESCRIPTOR;
#if defined( BOOST_WINAPI_IS_MINGW )
typedef _SECURITY_DESCRIPTOR *PSECURITY_DESCRIPTOR;
#else
typedef boost::winapi::PVOID_ PSECURITY_DESCRIPTOR;
#endif

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
InitializeSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    boost::winapi::DWORD_ dwRevision);
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
SetSecurityDescriptorDacl(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    boost::winapi::BOOL_ bDaclPresent,
    ::_ACL* pDacl,
    boost::winapi::BOOL_ bDaclDefaulted);
}
#endif

namespace boost {
namespace winapi {

typedef PVOID_ PSID_;
typedef WORD_ SECURITY_DESCRIPTOR_CONTROL_, *PSECURITY_DESCRIPTOR_CONTROL_;

typedef struct BOOST_MAY_ALIAS _ACL {
    BYTE_ AclRevision;
    BYTE_ Sbz1;
    WORD_ AclSize;
    WORD_ AceCount;
    WORD_ Sbz2;
} ACL_, *PACL_;

typedef struct BOOST_MAY_ALIAS _SECURITY_DESCRIPTOR {
    BYTE_ Revision;
    BYTE_ Sbz1;
    SECURITY_DESCRIPTOR_CONTROL_ Control;
    PSID_ Owner;
    PSID_ Group;
    PACL_ Sacl;
    PACL_ Dacl;
} SECURITY_DESCRIPTOR_, *PISECURITY_DESCRIPTOR_;

// To abstract away the different ::PSECURITY_DESCRIPTOR on MinGW, we use PVOID_ universally here
// and cast the pointers to ::PSECURITY_DESCRIPTOR in wrapper functions.
typedef PVOID_ PSECURITY_DESCRIPTOR_;

BOOST_FORCEINLINE BOOL_ InitializeSecurityDescriptor(PSECURITY_DESCRIPTOR_ pSecurityDescriptor, DWORD_ dwRevision)
{
    return ::InitializeSecurityDescriptor(reinterpret_cast< ::PSECURITY_DESCRIPTOR >(pSecurityDescriptor), dwRevision);
}

BOOST_FORCEINLINE BOOL_ SetSecurityDescriptorDacl(PSECURITY_DESCRIPTOR_ pSecurityDescriptor, BOOL_ bDaclPresent, PACL_ pDacl, BOOL_ bDaclDefaulted)
{
    return ::SetSecurityDescriptorDacl(reinterpret_cast< ::PSECURITY_DESCRIPTOR >(pSecurityDescriptor), bDaclPresent, reinterpret_cast< ::_ACL* >(pDacl), bDaclDefaulted);
}

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_PARTITION_APP_SYSTEM
#endif // BOOST_WINAPI_SECURITY_HPP_INCLUDED_
