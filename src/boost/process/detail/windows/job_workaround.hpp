// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_

#include <boost/winapi/config.hpp>
#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/dll.hpp>
#include <boost/winapi/overlapped.hpp>

#if defined( BOOST_USE_WINDOWS_H )
#include <windows.h>
#else
extern "C"
{
BOOST_SYMBOL_IMPORT ::boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC CreateIoCompletionPort(
        ::boost::winapi::HANDLE_    FileHandle,
        ::boost::winapi::HANDLE_    ExistingCompletionPort,
        ::boost::winapi::ULONG_PTR_ CompletionKey,
        ::boost::winapi::DWORD_     NumberOfConcurrentThreads
);

BOOST_SYMBOL_IMPORT ::boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC GetQueuedCompletionStatus(
        ::boost::winapi::HANDLE_       CompletionPort,
        ::boost::winapi::LPDWORD_      lpNumberOfBytes,
        ::boost::winapi::ULONG_PTR_    *lpCompletionKey,
        _OVERLAPPED **lpOverlapped,
        ::boost::winapi::DWORD_        dwMilliseconds
);

}
#endif
namespace boost { namespace process { namespace detail { namespace windows { namespace workaround {

extern "C"
{

struct JOBOBJECT_ASSOCIATE_COMPLETION_PORT_
{
    ::boost::winapi::PVOID_  CompletionKey;
    ::boost::winapi::HANDLE_ CompletionPort;
};

constexpr static int JOB_OBJECT_MSG_END_OF_JOB_TIME_          = 1;
constexpr static int JOB_OBJECT_MSG_END_OF_PROCESS_TIME_      = 2;
constexpr static int JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT_     = 3;
constexpr static int JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO_      = 4;
constexpr static int JOB_OBJECT_MSG_NEW_PROCESS_              = 6;
constexpr static int JOB_OBJECT_MSG_EXIT_PROCESS_             = 7;
constexpr static int JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS_    = 8;
constexpr static int JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT_     = 9;
constexpr static int JOB_OBJECT_MSG_JOB_MEMORY_LIMIT_         = 10;
constexpr static int JOB_OBJECT_MSG_NOTIFICATION_LIMIT_       = 11;
constexpr static int JOB_OBJECT_MSG_JOB_CYCLE_TIME_LIMIT_     = 12;
constexpr static int JOB_OBJECT_MSG_SILO_TERMINATED_          = 13;

}

BOOST_FORCEINLINE ::boost::winapi::BOOL_  get_queued_completion_status(
        ::boost::winapi::HANDLE_       CompletionPort,
        ::boost::winapi::LPDWORD_      lpNumberOfBytes,
        ::boost::winapi::ULONG_PTR_    *lpCompletionKey,
        ::boost::winapi::LPOVERLAPPED_ *lpOverlapped,
        ::boost::winapi::DWORD_        dwMilliseconds)
{
    return ::GetQueuedCompletionStatus(
                CompletionPort,
                lpNumberOfBytes,
                lpCompletionKey,
                reinterpret_cast<::_OVERLAPPED**>(lpOverlapped),
                dwMilliseconds);
}

#if defined( BOOST_USE_WINDOWS_H )

constexpr auto static JobObjectExtendedLimitInformation_ = ::JobObjectExtendedLimitInformation;
constexpr auto static JobObjectAssociateCompletionPortInformation_ = ::JobObjectAssociateCompletionPortInformation;
constexpr auto static JobObjectBasicAccountingInformation_ = ::JobObjectBasicAccountingInformation;

using JOBOBJECT_BASIC_LIMIT_INFORMATION_ = ::JOBOBJECT_BASIC_LIMIT_INFORMATION;
using JOBOBJECTINFOCLASS_ = ::JOBOBJECTINFOCLASS;
using IO_COUNTERS_ = ::IO_COUNTERS;
using JOBOBJECT_EXTENDED_LIMIT_INFORMATION_ = ::JOBOBJECT_EXTENDED_LIMIT_INFORMATION;
using JOBOBJECT_BASIC_ACCOUNTING_INFORMATION_ = ::JOBOBJECT_BASIC_ACCOUNTING_INFORMATION;

inline ::boost::winapi::BOOL_ query_information_job_object(
        ::boost::winapi::HANDLE_ hJob,
        JOBOBJECTINFOCLASS_ JobObjectInfoClass,
        void * lpJobObjectInfo,
        ::boost::winapi::DWORD_ cbJobObjectInfoLength,
        ::boost::winapi::DWORD_ *lpReturnLength)
{
    return ::QueryInformationJobObject(hJob, JobObjectInfoClass, lpJobObjectInfo, cbJobObjectInfoLength, lpReturnLength);
}

inline ::boost::winapi::BOOL_ set_information_job_object(
        ::boost::winapi::HANDLE_ hJob,
        JOBOBJECTINFOCLASS_ JobObjectInfoClass,
        void * lpJobObjectInfo,
        ::boost::winapi::DWORD_ cbJobObjectInfoLength)
{
    return ::SetInformationJobObject(hJob, JobObjectInfoClass, lpJobObjectInfo, cbJobObjectInfoLength);
}


#else

//this import workaround is to keep it a header-only library. and enums cannot be imported from the winapi.

extern "C"
{

typedef enum _JOBOBJECTINFOCLASS_
{
    JobObjectBasicAccountingInformation_ = 1,
    JobObjectBasicLimitInformation_,
    JobObjectBasicProcessIdList_,
    JobObjectBasicUIRestrictions_,
    JobObjectSecurityLimitInformation_,
    JobObjectEndOfJobTimeInformation_,
    JobObjectAssociateCompletionPortInformation_,
    JobObjectBasicAndIoAccountingInformation_,
    JobObjectExtendedLimitInformation_,
    JobObjectJobSetInformation_,
    JobObjectGroupInformation_,
    JobObjectNotificationLimitInformation_,
    JobObjectLimitViolationInformation_,
    JobObjectGroupInformationEx_,
    JobObjectCpuRateControlInformation_,
    JobObjectCompletionFilter_,
    JobObjectCompletionCounter_,
    JobObjectReserved1Information_ = 18,
    JobObjectReserved2Information_,
    JobObjectReserved3Information_,
    JobObjectReserved4Information_,
    JobObjectReserved5Information_,
    JobObjectReserved6Information_,
    JobObjectReserved7Information_,
    JobObjectReserved8Information_,
    MaxJobObjectInfoClass_
} JOBOBJECTINFOCLASS_;

typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION_
{
    ::boost::winapi::LARGE_INTEGER_ PerProcessUserTimeLimit;
    ::boost::winapi::LARGE_INTEGER_ PerJobUserTimeLimit;
    ::boost::winapi::DWORD_ LimitFlags;
    ::boost::winapi::SIZE_T_ MinimumWorkingSetSize;
    ::boost::winapi::SIZE_T_ MaximumWorkingSetSize;
    ::boost::winapi::DWORD_ ActiveProcessLimit;
    ::boost::winapi::ULONG_PTR_ Affinity;
    ::boost::winapi::DWORD_ PriorityClass;
    ::boost::winapi::DWORD_ SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION_;


typedef struct _JOBOBJECT_BASIC_ACCOUNTING_INFORMATION_ {
    ::boost::winapi::LARGE_INTEGER_ TotalUserTime;
    ::boost::winapi::LARGE_INTEGER_ TotalKernelTime;
    ::boost::winapi::LARGE_INTEGER_ ThisPeriodTotalUserTime;
    ::boost::winapi::LARGE_INTEGER_ ThisPeriodTotalKernelTime;
    ::boost::winapi::DWORD_         TotalPageFaultCount;
    ::boost::winapi::DWORD_         TotalProcesses;
    ::boost::winapi::DWORD_         ActiveProcesses;
    ::boost::winapi::DWORD_         TotalTerminatedProcesses;
} JOBOBJECT_BASIC_ACCOUNTING_INFORMATION_;

typedef struct _IO_COUNTERS_
{
    ::boost::winapi::ULONGLONG_ ReadOperationCount;
    ::boost::winapi::ULONGLONG_ WriteOperationCount;
    ::boost::winapi::ULONGLONG_ OtherOperationCount;
    ::boost::winapi::ULONGLONG_ ReadTransferCount;
    ::boost::winapi::ULONGLONG_ WriteTransferCount;
    ::boost::winapi::ULONGLONG_ OtherTransferCount;
} IO_COUNTERS_;


typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION_
{
    JOBOBJECT_BASIC_LIMIT_INFORMATION_ BasicLimitInformation;
    IO_COUNTERS_ IoInfo;
    ::boost::winapi::SIZE_T_ ProcessMemoryLimit;
    ::boost::winapi::SIZE_T_ JobMemoryLimit;
    ::boost::winapi::SIZE_T_ PeakProcessMemoryUsed;
    ::boost::winapi::SIZE_T_ PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION_;


/*BOOL WINAPI QueryInformationJobObject(
  _In_opt_  HANDLE             hJob,
  _In_      JOBOBJECTINFOCLASS JobObjectInfoClass,
  _Out_     LPVOID             lpJobObjectInfo,
  _In_      DWORD              cbJobObjectInfoLength,
  _Out_opt_ LPDWORD            lpReturnLength
);
 */
typedef ::boost::winapi::BOOL_ (BOOST_WINAPI_WINAPI_CC *query_information_job_object_p)(
        ::boost::winapi::HANDLE_,
        JOBOBJECTINFOCLASS_,
        void *,
        ::boost::winapi::DWORD_,
        ::boost::winapi::DWORD_ *);


inline ::boost::winapi::BOOL_ query_information_job_object(
        ::boost::winapi::HANDLE_ hJob,
        JOBOBJECTINFOCLASS_ JobObjectInfoClass,
        void *lpJobObjectInfo,
        ::boost::winapi::DWORD_ cbJobObjectInfoLength,
        ::boost::winapi::DWORD_ *lpReturnLength)
{
    static ::boost::winapi::HMODULE_ h = ::boost::winapi::get_module_handle(
            L"Kernel32.dll");
    static query_information_job_object_p f = reinterpret_cast<query_information_job_object_p>(::boost::winapi::get_proc_address(
            h, "QueryInformationJobObject"));

    return (*f)(hJob, JobObjectInfoClass, lpJobObjectInfo,
                cbJobObjectInfoLength, lpReturnLength);
}

/*BOOL WINAPI SetInformationJobObject(
  _In_ HANDLE             hJob,
  _In_ JOBOBJECTINFOCLASS JobObjectInfoClass,
  _In_ LPVOID             lpJobObjectInfo,
  _In_ DWORD              cbJobObjectInfoLength
);*/

typedef ::boost::winapi::BOOL_ (BOOST_WINAPI_WINAPI_CC *set_information_job_object_p)(
        ::boost::winapi::HANDLE_,
        JOBOBJECTINFOCLASS_,
        void *,
        ::boost::winapi::DWORD_);

}

inline ::boost::winapi::BOOL_ set_information_job_object(
        ::boost::winapi::HANDLE_ hJob,
        JOBOBJECTINFOCLASS_ JobObjectInfoClass,
        void *lpJobObjectInfo,
        ::boost::winapi::DWORD_ cbJobObjectInfoLength)
{
    static ::boost::winapi::HMODULE_ h = ::boost::winapi::get_module_handle(
            L"Kernel32.dll");
    static set_information_job_object_p f = reinterpret_cast<set_information_job_object_p>(::boost::winapi::get_proc_address(
            h, "SetInformationJobObject"));

    return (*f)(hJob, JobObjectInfoClass, lpJobObjectInfo,
                cbJobObjectInfoLength);
}

#endif

constexpr static ::boost::winapi::DWORD_ JOB_OBJECT_LIMIT_BREAKAWAY_OK_ = 0x00000800;

}}}}}

#endif /* BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_ */
