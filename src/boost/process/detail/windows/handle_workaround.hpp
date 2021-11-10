// Copyright (c) 2018 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_HANDLE_WORKAROUND_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_HANDLE_WORKAROUND_HPP_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/dll.hpp>
#include <boost/winapi/access_rights.hpp>
//#define BOOST_USE_WINDOWS_H 1

#if defined( BOOST_USE_WINDOWS_H )
#include <Winternl.h>
#endif


namespace boost { namespace process { namespace detail { namespace windows { namespace workaround
{


typedef struct _SYSTEM_HANDLE_ENTRY_
{
    ::boost::winapi::ULONG_ OwnerPid;
    ::boost::winapi::BYTE_ ObjectType;
    ::boost::winapi::BYTE_ HandleFlags;
    ::boost::winapi::USHORT_ HandleValue;
    ::boost::winapi::PVOID_ ObjectPointer;
    ::boost::winapi::ULONG_ AccessMask;
} SYSTEM_HANDLE_ENTRY_, *PSYSTEM_HANDLE_ENTRY_;

typedef struct _SYSTEM_HANDLE_INFORMATION_
{
    ::boost::winapi::ULONG_ Count;
    SYSTEM_HANDLE_ENTRY_ Handle[1];
} SYSTEM_HANDLE_INFORMATION_, *PSYSTEM_HANDLE_INFORMATION_;

#if defined( BOOST_USE_WINDOWS_H )

using UNICODE_STRING_  = ::UNICODE_STRING;
using GENERIC_MAPPING_ = ::GENERIC_MAPPING;
using OBJECT_INFORMATION_CLASS_ = ::OBJECT_INFORMATION_CLASS;

constexpr static OBJECT_INFORMATION_CLASS_  ObjectTypeInformation = ::OBJECT_INFORMATION_CLASS::ObjectTypeInformation;

typedef struct _OBJECT_TYPE_INFORMATION_ {
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    UCHAR   TypeIndex;
    CHAR    ReservedByte;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION_, *POBJECT_TYPE_INFORMATION_;

#else

typedef enum _OBJECT_INFORMATION_CLASS_
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
} OBJECT_INFORMATION_CLASS_, *POBJECT_INFORMATION_CLASS_;

typedef struct _UNICODE_STRING_ {
    ::boost::winapi::USHORT_ Length;
    ::boost::winapi::USHORT_ MaximumLength;
    ::boost::winapi::LPWSTR_ Buffer;
} UNICODE_STRING_, *PUNICODE_STRING_;

typedef struct _GENERIC_MAPPING_ {
    ::boost::winapi::ACCESS_MASK_ GenericRead;
    ::boost::winapi::ACCESS_MASK_ GenericWrite;
    ::boost::winapi::ACCESS_MASK_ GenericExecute;
    ::boost::winapi::ACCESS_MASK_ GenericAll;
} GENERIC_MAPPING_;

#endif

typedef struct _OBJECT_BASIC_INFORMATION {
    ::boost::winapi::ULONG_  Attributes;
    ::boost::winapi::ACCESS_MASK_  GrantedAccess;
    ::boost::winapi::ULONG_  HandleCount;
    ::boost::winapi::ULONG_  PointerCount;
    ::boost::winapi::ULONG_  PagedPoolUsage;
    ::boost::winapi::ULONG_  NonPagedPoolUsage;
    ::boost::winapi::ULONG_  Reserved[3];
    ::boost::winapi::ULONG_  NameInformationLength;
    ::boost::winapi::ULONG_  TypeInformationLength;
    ::boost::winapi::ULONG_  SecurityDescriptorLength;
    ::boost::winapi::LARGE_INTEGER_  CreateTime;
} OBJECT_BASIC_INFORMATION_, *POBJECT_BASIC_INFORMATION_;

typedef struct _OBJECT_NAME_INFORMATION {
    UNICODE_STRING_ Name;
} OBJECT_NAME_INFORMATION_, *POBJECT_NAME_INFORMATION_;


#if defined( BOOST_USE_WINDOWS_H )

extern "C"
{

using SYSTEM_INFORMATION_CLASS_  = ::SYSTEM_INFORMATION_CLASS;
constexpr static SYSTEM_INFORMATION_CLASS_  SystemHandleInformation_ = static_cast<SYSTEM_INFORMATION_CLASS_>(16);

inline ::boost::winapi::NTSTATUS_ nt_system_query_information(
        SYSTEM_INFORMATION_CLASS SystemInformationClass,
        void * SystemInformation,
        ::boost::winapi::ULONG_ SystemInformationLength,
        ::boost::winapi::PULONG_ ReturnLength)
{
    return ::NtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
}

inline ::boost::winapi::NTSTATUS_ nt_query_object(
        ::boost::winapi::HANDLE_ Handle,
        OBJECT_INFORMATION_CLASS_ ObjectInformationClass,
        ::boost::winapi::PVOID_  ObjectInformation,
        ::boost::winapi::ULONG_  ObjectInformationLength,
        ::boost::winapi::PULONG_ ReturnLength
)
{
    return ::NtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

}

#else

//this import workaround is to keep it a header-only library. and enums cannot be imported from the winapi.

extern "C"
{

typedef enum _SYSTEM_INFORMATION_CLASS_
{
    SystemBasicInformation_ = 0,
    SystemProcessorInformation_ = 1,
    SystemPerformanceInformation_ = 2,
    SystemTimeOfDayInformation_ = 3,
    SystemProcessInformation_ = 5,
    SystemProcessorPerformanceInformation_ = 8,
    SystemHandleInformation_ = 16,
    SystemPagefileInformation_ = 18,
    SystemInterruptInformation_ = 23,
    SystemExceptionInformation_ = 33,
    SystemRegistryQuotaInformation_ = 37,
    SystemLookasideInformation_ = 45
} SYSTEM_INFORMATION_CLASS_;


typedef struct _OBJECT_TYPE_INFORMATION_ {
    UNICODE_STRING_ TypeName;
    ::boost::winapi::ULONG_ TotalNumberOfObjects;
    ::boost::winapi::ULONG_ TotalNumberOfHandles;
    ::boost::winapi::ULONG_ TotalPagedPoolUsage;
    ::boost::winapi::ULONG_ TotalNonPagedPoolUsage;
    ::boost::winapi::ULONG_ TotalNamePoolUsage;
    ::boost::winapi::ULONG_ TotalHandleTableUsage;
    ::boost::winapi::ULONG_ HighWaterNumberOfObjects;
    ::boost::winapi::ULONG_ HighWaterNumberOfHandles;
    ::boost::winapi::ULONG_ HighWaterPagedPoolUsage;
    ::boost::winapi::ULONG_ HighWaterNonPagedPoolUsage;
    ::boost::winapi::ULONG_ HighWaterNamePoolUsage;
    ::boost::winapi::ULONG_ HighWaterHandleTableUsage;
    ::boost::winapi::ULONG_ InvalidAttributes;
    GENERIC_MAPPING_ GenericMapping;
    ::boost::winapi::ULONG_ ValidAccessMask;
    ::boost::winapi::BOOLEAN_ SecurityRequired;
    ::boost::winapi::BOOLEAN_ MaintainHandleCount;
    ::boost::winapi::UCHAR_   TypeIndex;
    ::boost::winapi::CHAR_    ReservedByte;
    ::boost::winapi::ULONG_ PoolType;
    ::boost::winapi::ULONG_ DefaultPagedPoolCharge;
    ::boost::winapi::ULONG_ DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION_, *POBJECT_TYPE_INFORMATION_;



/*
__kernel_entry NTSTATUS NtQuerySystemInformation(
  IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
  OUT PVOID                   SystemInformation,
  IN ULONG                    SystemInformationLength,
  OUT PULONG                  ReturnLength
);
 */
typedef ::boost::winapi::NTSTATUS_ (__kernel_entry *nt_system_query_information_p )(
        SYSTEM_INFORMATION_CLASS_,
        void *,
        ::boost::winapi::ULONG_,
        ::boost::winapi::PULONG_);
/*
__kernel_entry NTSYSCALLAPI NTSTATUS NtQueryObject(
  HANDLE                   Handle,
  OBJECT_INFORMATION_CLASS ObjectInformationClass,
  PVOID                    ObjectInformation,
  ULONG                    ObjectInformationLength,
  PULONG                   ReturnLength
);
 */

typedef ::boost::winapi::NTSTATUS_ (__kernel_entry *nt_query_object_p )(
        ::boost::winapi::HANDLE_,
        OBJECT_INFORMATION_CLASS_,
        void *,
        ::boost::winapi::ULONG_,
        ::boost::winapi::PULONG_);

}

inline ::boost::winapi::NTSTATUS_ nt_system_query_information(
        SYSTEM_INFORMATION_CLASS_ SystemInformationClass,
        void *SystemInformation,
        ::boost::winapi::ULONG_ SystemInformationLength,
        ::boost::winapi::PULONG_ ReturnLength)
{
    static ::boost::winapi::HMODULE_ h = ::boost::winapi::get_module_handle(L"Ntdll.dll");
    static nt_system_query_information_p f = reinterpret_cast<nt_system_query_information_p>(::boost::winapi::get_proc_address(h, "NtQuerySystemInformation"));

    return (*f)(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
}


inline ::boost::winapi::BOOL_ nt_query_object(
        ::boost::winapi::HANDLE_ Handle,
        OBJECT_INFORMATION_CLASS_ ObjectInformationClass,
        void *ObjectInformation,
        ::boost::winapi::ULONG_ ObjectInformationLength,
        ::boost::winapi::PULONG_ ReturnLength)
{
    static ::boost::winapi::HMODULE_ h = ::boost::winapi::get_module_handle(L"Ntdll.dll");
    static nt_query_object_p f = reinterpret_cast<nt_query_object_p>(::boost::winapi::get_proc_address(h, "NtQueryObject"));

    return (*f)(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
}

#endif

}}}}}

#endif /* BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_ */
