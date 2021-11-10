//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2015. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WIN32_API_HPP
#define BOOST_INTERPROCESS_WIN32_API_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/cstdint.hpp>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <boost/assert.hpp>
#include <string>
#include <vector>

#ifdef BOOST_USE_WINDOWS_H
#include <windows.h>
#endif

#if defined(_MSC_VER)
#  pragma once
#  pragma comment( lib, "Advapi32.lib" )
#  pragma comment( lib, "oleaut32.lib" )
#  pragma comment( lib, "Ole32.lib" )
#endif

#if defined (BOOST_INTERPROCESS_WINDOWS)
#  include <cstdarg>
#  include <boost/detail/interlocked.hpp>
#else
# error "This file can only be included in Windows OS"
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Declaration of Windows structures or typedefs if BOOST_USE_WINDOWS_H is used
//
//////////////////////////////////////////////////////////////////////////////


#if defined(BOOST_GCC)
//Ignore -pedantic errors here (anonymous structs, etc.)
#  if (BOOST_GCC >= 40600)
#     pragma GCC diagnostic push
#     if (BOOST_GCC >= 40800)
#        pragma GCC diagnostic ignored "-Wpedantic"
#     else
#        pragma GCC diagnostic ignored "-pedantic"
#     endif
#     pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#  else
#     pragma GCC system_header
#  endif
//When loading DLLs we have no option but reinterpret casting function types  
#  if (BOOST_GCC >= 80000)
#        pragma GCC diagnostic ignored "-Wcast-function-type"
#  endif
#endif


//#define BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED
//#define BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED

#ifdef BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED
#  define BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED_VALUE 1 
#else
#  define BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED_VALUE 0
#endif

#ifdef BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED
#  define BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED_VALUE 1 
#else
#  define BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED_VALUE 0
#endif

#define BOOST_INTERPROCESS_BOOTSTAMP_VALUE_SUM \
   (BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED_VALUE + \
    BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED_VALUE)

#if 1 < BOOST_INTERPROCESS_BOOTSTAMP_VALUE_SUM
#  error "Only one of \
          BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED and \
          BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED can be defined"
#endif

#if 0 == BOOST_INTERPROCESS_BOOTSTAMP_VALUE_SUM
#  define BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED
#endif


namespace boost  {
namespace interprocess  {
namespace winapi {

//Own defines
static const unsigned long MaxPath           = 260;

//////////////////////////////////////////////////////////////////////////////
//
// Nt native structures
//
//////////////////////////////////////////////////////////////////////////////

struct interprocess_semaphore_basic_information
{
   unsigned int count;      // current semaphore count
   unsigned int limit;      // max semaphore count
};

struct interprocess_section_basic_information
{
  void *          base_address;
  unsigned long   section_attributes;
  __int64         section_size;
};

struct file_rename_information_t {
   int Replace;
   void *RootDir;
   unsigned long FileNameLength;
   wchar_t FileName[1];
};

struct unicode_string_t {
   unsigned short Length;
   unsigned short MaximumLength;
   wchar_t *Buffer;
};

struct object_attributes_t {
   unsigned long Length;
   void * RootDirectory;
   unicode_string_t *ObjectName;
   unsigned long Attributes;
   void *SecurityDescriptor;
   void *SecurityQualityOfService;
};

struct io_status_block_t {
   union {
      long Status;
      void *Pointer;
   };

   unsigned long *Information;
};

union system_timeofday_information
{
   struct data_t
   {
      __int64 liKeBootTime;
      __int64 liKeSystemTime;
      __int64 liExpTimeZoneBias;
      unsigned long uCurrentTimeZoneId;
      unsigned long dwReserved;
      ::boost::ulong_long_type ullBootTimeBias;
      ::boost::ulong_long_type ullSleepTimeBias;
   } data;
   unsigned char Reserved1[sizeof(data_t)];
};

static const long BootstampLength            = sizeof(__int64);
static const long BootAndSystemstampLength   = sizeof(__int64)*2;
static const long SystemTimeOfDayInfoLength  = sizeof(system_timeofday_information::data_t);

struct object_name_information_t
{
   unicode_string_t Name;
   wchar_t NameBuffer[1];
};

enum file_information_class_t {
   file_directory_information = 1,
   file_full_directory_information,
   file_both_directory_information,
   file_basic_information,
   file_standard_information,
   file_internal_information,
   file_ea_information,
   file_access_information,
   file_name_information,
   file_rename_information,
   file_link_information,
   file_names_information,
   file_disposition_information,
   file_position_information,
   file_full_ea_information,
   file_mode_information,
   file_alignment_information,
   file_all_information,
   file_allocation_information,
   file_end_of_file_information,
   file_alternate_name_information,
   file_stream_information,
   file_pipe_information,
   file_pipe_local_information,
   file_pipe_remote_information,
   file_mailslot_query_information,
   file_mailslot_set_information,
   file_compression_information,
   file_copy_on_write_information,
   file_completion_information,
   file_move_cluster_information,
   file_quota_information,
   file_reparse_point_information,
   file_network_open_information,
   file_object_id_information,
   file_tracking_information,
   file_ole_directory_information,
   file_content_index_information,
   file_inherit_content_index_information,
   file_ole_information,
   file_maximum_information
};

enum semaphore_information_class {
   semaphore_basic_information = 0
};


enum system_information_class {
   system_basic_information = 0,
   system_performance_information = 2,
   system_time_of_day_information = 3,
   system_process_information = 5,
   system_processor_performance_information = 8,
   system_interrupt_information = 23,
   system_exception_information = 33,
   system_registry_quota_information = 37,
   system_lookaside_information = 45
};

enum object_information_class
{
   object_basic_information,
   object_name_information,
   object_type_information,
   object_all_information,
   object_data_information
};

enum section_information_class
{
   section_basic_information,
   section_image_information
};

}  //namespace winapi {
}  //namespace interprocess  {
}  //namespace boost  {


//////////////////////////////////////////////////////////////////////////////
//
// Forward declaration of winapi
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/winapi/get_current_process_id.hpp>
#include <boost/winapi/get_current_thread_id.hpp>
#include <boost/winapi/get_current_process.hpp>
#include <boost/winapi/get_process_times.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/winapi/thread.hpp>
#include <boost/winapi/system.hpp>
#include <boost/winapi/time.hpp>
#include <boost/winapi/timers.hpp>
#include <boost/winapi/get_last_error.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/file_management.hpp>
#include <boost/winapi/mutex.hpp>
#include <boost/winapi/wait.hpp>
#include <boost/winapi/file_mapping.hpp>
#include <boost/winapi/semaphore.hpp>
#include <boost/winapi/system.hpp>
#include <boost/winapi/error_handling.hpp>
#include <boost/winapi/local_memory.hpp>
#include <boost/winapi/directory_management.hpp>
#include <boost/winapi/security.hpp>
#include <boost/winapi/dll.hpp>
#include <boost/winapi/basic_types.hpp>

//This should go in winapi's basic_types.hpp 
namespace boost {
namespace ipwinapiext {
typedef boost::winapi::LONG_ LSTATUS;

typedef boost::winapi::DWORD_ (__stdcall *LPTHREAD_START_ROUTINE_)
   (boost::winapi::LPVOID_ lpThreadParameter);


//#ifndef BOOST_USE_WINDOWS_H
//typedef boost::winapi::LARGE_INTEGER_ LARGE_INTEGER_EXT;
//#else
//typedef LARGE_INTEGER LARGE_INTEGER_EXT;
//#endif

}} //namespace boost::ipwinapiext

#ifndef BOOST_USE_WINDOWS_H

extern "C" {

//Error handling
BOOST_SYMBOL_IMPORT BOOST_WINAPI_DETAIL_VOID BOOST_WINAPI_WINAPI_CC SetLastError(boost::winapi::DWORD_ dwErrCode);

//File management
BOOST_SYMBOL_IMPORT boost::winapi::DWORD_ BOOST_WINAPI_WINAPI_CC GetFileType(boost::winapi::HANDLE_ hTemplateFile);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC FlushFileBuffers(boost::winapi::HANDLE_ hFile);

//threading

BOOST_SYMBOL_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC CreateThread
   ( ::_SECURITY_ATTRIBUTES* lpThreadAttributes
   , boost::winapi::SIZE_T_ dwStackSize
   , boost::ipwinapiext::LPTHREAD_START_ROUTINE_ lpStartAddress
   , boost::winapi::LPVOID_ lpParameter
   , boost::winapi::DWORD_ dwCreationFlags
   , boost::winapi::LPDWORD_ lpThreadId
   );

//Virtual Memory
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC VirtualLock(boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC VirtualUnlock(boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC VirtualProtect( boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize
                                                                              , boost::winapi::DWORD_ flNewProtect, boost::winapi::PDWORD_ lpflOldProtect);
//registry.hpp
BOOST_WINAPI_DETAIL_DECLARE_HANDLE(HKEY);


BOOST_SYMBOL_IMPORT boost::ipwinapiext::LSTATUS BOOST_WINAPI_WINAPI_CC RegOpenKeyExA
   (::HKEY hKey, const char *lpSubKey, boost::winapi::DWORD_ ulOptions, boost::winapi::DWORD_ samDesired, ::HKEY *phkResult);
BOOST_SYMBOL_IMPORT boost::ipwinapiext::LSTATUS BOOST_WINAPI_WINAPI_CC RegOpenKeyExW
   (::HKEY hKey, const wchar_t *lpSubKey, boost::winapi::DWORD_ ulOptions, boost::winapi::DWORD_ samDesired, ::HKEY *phkResult);
BOOST_SYMBOL_IMPORT boost::ipwinapiext::LSTATUS BOOST_WINAPI_WINAPI_CC RegQueryValueExA
   (::HKEY hKey, const char *lpValueName, boost::winapi::DWORD_ *lpReserved, boost::winapi::DWORD_ *lpType, boost::winapi::BYTE_ *lpData, boost::winapi::DWORD_ *lpcbData);
BOOST_SYMBOL_IMPORT boost::ipwinapiext::LSTATUS BOOST_WINAPI_WINAPI_CC RegQueryValueExW
   (::HKEY hKey, const wchar_t *lpValueName, boost::winapi::DWORD_ *lpReserved, boost::winapi::DWORD_ *lpType, boost::winapi::BYTE_ *lpData, boost::winapi::DWORD_ *lpcbData);
BOOST_SYMBOL_IMPORT boost::ipwinapiext::LSTATUS BOOST_WINAPI_WINAPI_CC RegCloseKey(::HKEY hKey);


//Event Log
BOOST_SYMBOL_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC OpenEventLogA(const char* lpUNCServerName, const char* lpSourceName);
BOOST_SYMBOL_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC OpenEventLogW(const wchar_t* lpUNCServerName, const wchar_t* lpSourceName);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_   BOOST_WINAPI_WINAPI_CC CloseEventLog(boost::winapi::HANDLE_ hEventLog);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_   BOOST_WINAPI_WINAPI_CC ReadEventLogA
   ( boost::winapi::HANDLE_ hEventLog, boost::winapi::DWORD_ dwReadFlags, boost::winapi::DWORD_ dwRecordOffset, void* lpBuffer
   , boost::winapi::DWORD_ nNumberOfBytesToRead, boost::winapi::DWORD_ *pnBytesRead, boost::winapi::DWORD_ *pnMinNumberOfBytesNeeded);
BOOST_SYMBOL_IMPORT boost::winapi::BOOL_   BOOST_WINAPI_WINAPI_CC ReadEventLogW
   ( boost::winapi::HANDLE_ hEventLog, boost::winapi::DWORD_ dwReadFlags, boost::winapi::DWORD_ dwRecordOffset, void* lpBuffer
   , boost::winapi::DWORD_ nNumberOfBytesToRead, boost::winapi::DWORD_ *pnBytesRead, boost::winapi::DWORD_ *pnMinNumberOfBytesNeeded); 

}  //extern "C" {

#endif   //#ifndef BOOST_USE_WINDOWS_H

namespace boost {
namespace ipwinapiext {

typedef ::HKEY HKEY_;

#if BOOST_WINAPI_PARTITION_APP_SYSTEM

//Threads
BOOST_FORCEINLINE boost::winapi::HANDLE_ CreateThread
   ( boost::winapi::SECURITY_ATTRIBUTES_* lpThreadAttributes
   , boost::winapi::SIZE_T_ dwStackSize
   , boost::ipwinapiext::LPTHREAD_START_ROUTINE_ lpStartAddress
   , boost::winapi::LPVOID_ lpParameter
   , boost::winapi::DWORD_ dwCreationFlags
   , boost::winapi::LPDWORD_ lpThreadId
)
{
   return ::CreateThread( reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpThreadAttributes)
                        , dwStackSize, lpStartAddress
                        , lpParameter, dwCreationFlags, lpThreadId);
}

//Error handling
BOOST_FORCEINLINE BOOST_WINAPI_DETAIL_VOID SetLastError(boost::winapi::DWORD_ dwErrCode)
{  ::SetLastError(dwErrCode); }

//File management
BOOST_FORCEINLINE boost::winapi::DWORD_ GetFileType(boost::winapi::HANDLE_ hTemplateFile)
{  return ::GetFileType(hTemplateFile);   }

BOOST_FORCEINLINE boost::winapi::BOOL_ FlushFileBuffers(boost::winapi::HANDLE_ hFile)
{  return ::FlushFileBuffers(hFile);   }

//Virtual Memory
BOOST_FORCEINLINE boost::winapi::BOOL_ VirtualLock(boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize)
{  return ::VirtualLock(lpAddress, dwSize);  }

BOOST_FORCEINLINE boost::winapi::BOOL_ VirtualUnlock(boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize)
{  return ::VirtualUnlock(lpAddress, dwSize);   }

BOOST_FORCEINLINE boost::winapi::BOOL_ VirtualProtect( boost::winapi::LPVOID_ lpAddress, boost::winapi::SIZE_T_ dwSize
                                                     , boost::winapi::DWORD_ flNewProtect, boost::winapi::PDWORD_ lpflOldProtect)
{  return ::VirtualProtect(lpAddress, dwSize, flNewProtect, lpflOldProtect);  }

//registry.hpp
BOOST_FORCEINLINE boost::ipwinapiext::LSTATUS RegOpenKeyExA
   (boost::ipwinapiext::HKEY_ hKey, const char *lpSubKey, boost::winapi::DWORD_ ulOptions, boost::winapi::DWORD_ samDesired, boost::ipwinapiext::HKEY_ *phkResult)
{
   return ::RegOpenKeyExA(reinterpret_cast< ::HKEY >(hKey), lpSubKey, ulOptions, samDesired, reinterpret_cast< ::HKEY* >(phkResult));
}

BOOST_FORCEINLINE boost::ipwinapiext::LSTATUS RegOpenKeyExW
   (boost::ipwinapiext::HKEY_ hKey, const wchar_t *lpSubKey, boost::winapi::DWORD_ ulOptions, boost::winapi::DWORD_ samDesired, boost::ipwinapiext::HKEY_ *phkResult)
{
   return ::RegOpenKeyExW(reinterpret_cast< ::HKEY >(hKey), lpSubKey, ulOptions, samDesired, reinterpret_cast< ::HKEY* >(phkResult));
}

BOOST_FORCEINLINE boost::ipwinapiext::LSTATUS RegQueryValueExA
   (boost::ipwinapiext::HKEY_ hKey, const char *lpValueName, boost::winapi::DWORD_ *lpReserved, boost::winapi::DWORD_ *lpType, boost::winapi::BYTE_ *lpData, boost::winapi::DWORD_ *lpcbData)
{
   return ::RegQueryValueExA(reinterpret_cast< ::HKEY >(hKey), lpValueName, lpReserved, lpType, lpData, lpcbData);
}

BOOST_FORCEINLINE boost::ipwinapiext::LSTATUS RegQueryValueExW
   (boost::ipwinapiext::HKEY_ hKey, const wchar_t *lpValueName, boost::winapi::DWORD_ *lpReserved, boost::winapi::DWORD_ *lpType, boost::winapi::BYTE_ *lpData, boost::winapi::DWORD_ *lpcbData)
{
   return ::RegQueryValueExW(reinterpret_cast< ::HKEY >(hKey), lpValueName, lpReserved, lpType, lpData, lpcbData);
}

BOOST_FORCEINLINE boost::ipwinapiext::LSTATUS RegCloseKey(boost::ipwinapiext::HKEY_ hKey)
{
   return ::RegCloseKey(reinterpret_cast< ::HKEY >(hKey));
}

BOOST_FORCEINLINE void GetSystemInfo(boost::winapi::LPSYSTEM_INFO_ lpSystemInfo)
{  return ::GetSystemInfo(reinterpret_cast< ::_SYSTEM_INFO* >(lpSystemInfo));   }

#endif   //BOOST_WINAPI_PARTITION_APP_SYSTEM

}  //namespace ipwinapiext {
}  //namespace boost {

namespace boost  {
namespace interprocess  {
namespace winapi {

typedef boost::winapi::SYSTEM_INFO_ interprocess_system_info;
typedef boost::winapi::OVERLAPPED_ interprocess_overlapped;
typedef boost::winapi::FILETIME_ interprocess_filetime;
typedef boost::winapi::WIN32_FIND_DATAA_ win32_find_data_a;
typedef boost::winapi::WIN32_FIND_DATAW_ win32_find_data_w;
typedef boost::winapi::SECURITY_ATTRIBUTES_ interprocess_security_attributes;
typedef boost::winapi::SECURITY_DESCRIPTOR_ interprocess_security_descriptor;
typedef boost::winapi::BY_HANDLE_FILE_INFORMATION_ interprocess_by_handle_file_information;
typedef boost::winapi::HMODULE_ hmodule;
typedef boost::ipwinapiext::HKEY_ hkey;
typedef boost::winapi::FARPROC_ farproc_t;

//ntdll.dll
typedef long (__stdcall *NtDeleteFile_t)(object_attributes_t *ObjectAttributes);
typedef long (__stdcall *NtSetInformationFile_t)(void *FileHandle, io_status_block_t *IoStatusBlock, void *FileInformation, unsigned long Length, int FileInformationClass );
typedef long (__stdcall *NtOpenFile)(void **FileHandle, unsigned long DesiredAccess, object_attributes_t *ObjectAttributes
                                    , io_status_block_t *IoStatusBlock, unsigned long ShareAccess, unsigned long Length, unsigned long OpenOptions);
typedef long (__stdcall *NtQuerySystemInformation_t)(int, void*, unsigned long, unsigned long *);
typedef long (__stdcall *NtQueryObject_t)(void*, object_information_class, void *, unsigned long, unsigned long *);
typedef long (__stdcall *NtQuerySemaphore_t)(void*, unsigned int info_class, interprocess_semaphore_basic_information *pinfo, unsigned int info_size, unsigned int *ret_len);
typedef long (__stdcall *NtQuerySection_t)(void*, section_information_class, interprocess_section_basic_information *pinfo, unsigned long info_size, unsigned long *ret_len);
typedef long (__stdcall *NtQueryInformationFile_t)(void *,io_status_block_t *,void *, long, int);
typedef long (__stdcall *NtOpenFile_t)(void*,unsigned long ,object_attributes_t*,io_status_block_t*,unsigned long,unsigned long);
typedef long (__stdcall *NtClose_t) (void*);
typedef long (__stdcall *NtQueryTimerResolution_t) (unsigned long* LowestResolution, unsigned long* HighestResolution, unsigned long* CurrentResolution);
typedef long (__stdcall *NtSetTimerResolution_t) (unsigned long RequestedResolution, int Set, unsigned long* ActualResolution);

}  //namespace winapi {
}  //namespace interprocess  {
}  //namespace boost  {

//////////////////////////////////////////////////////////////////////////////
//
// Forward declaration of constants
//
//////////////////////////////////////////////////////////////////////////////

namespace boost {
namespace interprocess {
namespace winapi {

//Some used constants
static const unsigned long infinite_time        = 0xFFFFFFFF;
static const unsigned long error_already_exists = 183L;
static const unsigned long error_invalid_handle = 6L;
static const unsigned long error_sharing_violation = 32L;
static const unsigned long error_file_not_found = 2u;
static const unsigned long error_no_more_files  = 18u;
static const unsigned long error_not_locked     = 158L;
//Retries in CreateFile, see http://support.microsoft.com/kb/316609
static const unsigned long error_sharing_violation_tries = 3L;
static const unsigned long error_sharing_violation_sleep_ms = 250L;
static const unsigned long error_file_too_large = 223L;
static const unsigned long error_insufficient_buffer = 122L;
static const unsigned long error_handle_eof = 38L;
static const unsigned long semaphore_all_access = (0x000F0000L)|(0x00100000L)|0x3;
static const unsigned long mutex_all_access     = (0x000F0000L)|(0x00100000L)|0x0001;

static const unsigned long page_readonly        = 0x02;
static const unsigned long page_readwrite       = 0x04;
static const unsigned long page_writecopy       = 0x08;
static const unsigned long page_noaccess        = 0x01;

static const unsigned long standard_rights_required   = 0x000F0000L;
static const unsigned long section_query              = 0x0001;
static const unsigned long section_map_write          = 0x0002;
static const unsigned long section_map_read           = 0x0004;
static const unsigned long section_map_execute        = 0x0008;
static const unsigned long section_extend_size        = 0x0010;
static const unsigned long section_all_access         = standard_rights_required |
                                                        section_query            |
                                                        section_map_write        |
                                                        section_map_read         |
                                                        section_map_execute      |
                                                        section_extend_size;

static const unsigned long file_map_copy        = section_query;
static const unsigned long file_map_write       = section_map_write;
static const unsigned long file_map_read        = section_map_read;
static const unsigned long file_map_all_access  = section_all_access;
static const unsigned long delete_access = 0x00010000L;
static const unsigned long file_flag_backup_semantics = 0x02000000;
static const long file_flag_delete_on_close = 0x04000000;

//Native API constants
static const unsigned long file_open_for_backup_intent = 0x00004000;
static const int file_share_valid_flags = 0x00000007;
static const long file_delete_on_close = 0x00001000L;
static const long obj_case_insensitive = 0x00000040L;
static const long delete_flag = 0x00010000L;

static const unsigned long movefile_copy_allowed            = 0x02;
static const unsigned long movefile_delay_until_reboot      = 0x04;
static const unsigned long movefile_replace_existing        = 0x01;
static const unsigned long movefile_write_through           = 0x08;
static const unsigned long movefile_create_hardlink         = 0x10;
static const unsigned long movefile_fail_if_not_trackable   = 0x20;

static const unsigned long file_share_read      = 0x00000001;
static const unsigned long file_share_write     = 0x00000002;
static const unsigned long file_share_delete    = 0x00000004;

static const unsigned long file_attribute_readonly    = 0x00000001;
static const unsigned long file_attribute_hidden      = 0x00000002;
static const unsigned long file_attribute_system      = 0x00000004;
static const unsigned long file_attribute_directory   = 0x00000010;
static const unsigned long file_attribute_archive     = 0x00000020;
static const unsigned long file_attribute_device      = 0x00000040;
static const unsigned long file_attribute_normal      = 0x00000080;
static const unsigned long file_attribute_temporary   = 0x00000100;

static const unsigned long generic_read         = 0x80000000L;
static const unsigned long generic_write        = 0x40000000L;

static const unsigned long wait_object_0        = 0;
static const unsigned long wait_abandoned       = 0x00000080L;
static const unsigned long wait_timeout         = 258L;
static const unsigned long wait_failed          = (unsigned long)0xFFFFFFFF;

static const unsigned long duplicate_close_source  = (unsigned long)0x00000001;
static const unsigned long duplicate_same_access   = (unsigned long)0x00000002;

static const unsigned long format_message_allocate_buffer
   = (unsigned long)0x00000100;
static const unsigned long format_message_ignore_inserts
   = (unsigned long)0x00000200;
static const unsigned long format_message_from_string
   = (unsigned long)0x00000400;
static const unsigned long format_message_from_hmodule
   = (unsigned long)0x00000800;
static const unsigned long format_message_from_system
   = (unsigned long)0x00001000;
static const unsigned long format_message_argument_array
   = (unsigned long)0x00002000;
static const unsigned long format_message_max_width_mask
   = (unsigned long)0x000000FF;
static const unsigned long lang_neutral         = (unsigned long)0x00;
static const unsigned long sublang_default      = (unsigned long)0x01;
static const unsigned long invalid_file_size    = (unsigned long)0xFFFFFFFF;
static const unsigned long invalid_file_attributes =  ((unsigned long)-1);
static       void * const  invalid_handle_value = ((void*)(long)(-1));

static const unsigned long file_type_char    =  0x0002L;
static const unsigned long file_type_disk    =  0x0001L;
static const unsigned long file_type_pipe    =  0x0003L;
static const unsigned long file_type_remote  =  0x8000L;
static const unsigned long file_type_unknown =  0x0000L;

static const unsigned long create_new        = 1;
static const unsigned long create_always     = 2;
static const unsigned long open_existing     = 3;
static const unsigned long open_always       = 4;
static const unsigned long truncate_existing = 5;

static const unsigned long file_begin     = 0;
static const unsigned long file_current   = 1;
static const unsigned long file_end       = 2;

static const unsigned long lockfile_fail_immediately  = 1;
static const unsigned long lockfile_exclusive_lock    = 2;
static const unsigned long error_lock_violation       = 33;
static const unsigned long security_descriptor_revision = 1;

const unsigned long max_record_buffer_size = 0x10000L;   // 64K
const unsigned long max_path = 260;

//Keys
static const  hkey hkey_local_machine = (hkey)(unsigned long*)(long)(0x80000002);
static unsigned long key_query_value    = 0x0001;

// Registry types
#define reg_none                       ( 0 )   // No value type
#define reg_sz                         ( 1 )   // Unicode nul terminated string
#define reg_expand_sz                  ( 2 )   // Unicode nul terminated string
                                               // (with environment variable references)
#define reg_binary                     ( 3 )   // Free form binary
#define reg_dword                      ( 4 )   // 32-bit number
#define reg_dword_little_endian        ( 4 )   // 32-bit number (same as REG_DWORD)
#define reg_dword_big_endian           ( 5 )   // 32-bit number
#define reg_link                       ( 6 )   // Symbolic Link (unicode)
#define reg_multi_sz                   ( 7 )   // Multiple Unicode strings
#define reg_resource_list              ( 8 )   // Resource list in the resource map
#define reg_full_resource_descriptor   ( 9 )  // Resource list in the hardware description
#define reg_resource_requirements_list ( 10 )
#define reg_qword                      ( 11 )  // 64-bit number
#define reg_qword_little_endian        ( 11 )  // 64-bit number (same as reg_qword)


}  //namespace winapi {
}  //namespace interprocess  {
}  //namespace boost  {


namespace boost {
namespace interprocess {
namespace winapi {

inline unsigned long get_last_error()
{  return GetLastError();  }

inline void set_last_error(unsigned long err)
{  return SetLastError(err);  }

inline unsigned long format_message
   (unsigned long dwFlags, const void *lpSource,
    unsigned long dwMessageId, unsigned long dwLanguageId,
    char *lpBuffer, unsigned long nSize, std::va_list *Arguments)
{
   return FormatMessageA
      (dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments);
}

//And now, wrapper functions
inline void * local_free(void *hmem)
{  return LocalFree(hmem); }

inline unsigned long make_lang_id(unsigned long p, unsigned long s)
{  return ((((unsigned short)(s)) << 10) | (unsigned short)(p));   }

inline void sched_yield()
{
   if(!SwitchToThread()){
      Sleep(0);
   }
}

inline void sleep_tick()
{  Sleep(1);   }

inline void sleep(unsigned long ms)
{  Sleep(ms);  }

inline unsigned long get_current_thread_id()
{  return GetCurrentThreadId();  }

inline bool get_process_times
   ( void *hProcess, interprocess_filetime* lpCreationTime
   , interprocess_filetime *lpExitTime, interprocess_filetime *lpKernelTime
   , interprocess_filetime *lpUserTime )
{  return 0 != GetProcessTimes(hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime); }

inline unsigned long get_current_process_id()
{  return GetCurrentProcessId();  }

inline unsigned int close_handle(void* handle)
{  return CloseHandle(handle);   }

inline void * find_first_file(const char *lpFileName, win32_find_data_a *lpFindFileData)
{  return FindFirstFileA(lpFileName, lpFindFileData);   }

inline void * find_first_file(const wchar_t *lpFileName, win32_find_data_w *lpFindFileData)
{  return FindFirstFileW(lpFileName, lpFindFileData);   }

inline bool find_next_file(void *hFindFile, win32_find_data_a *lpFindFileData)
{  return FindNextFileA(hFindFile, lpFindFileData) != 0;   }

inline bool find_next_file(void *hFindFile, win32_find_data_w *lpFindFileData)
{  return FindNextFileW(hFindFile, lpFindFileData) != 0;   }

inline bool find_close(void *handle)
{  return FindClose(handle) != 0;   }

inline bool duplicate_current_process_handle
   (void *hSourceHandle, void **lpTargetHandle)
{
   return 0 != DuplicateHandle
      ( GetCurrentProcess(),  hSourceHandle,    GetCurrentProcess()
      , lpTargetHandle,       0,                0
      , duplicate_same_access);
}

inline unsigned long get_file_type(void *hFile)
{
   return GetFileType(hFile);
}

/*
inline void get_system_time_as_file_time(interprocess_filetime *filetime)
{  GetSystemTimeAsFileTime(filetime);  }

inline bool file_time_to_local_file_time
   (const interprocess_filetime *in, const interprocess_filetime *out)
{  return 0 != FileTimeToLocalFileTime(in, out);  }
*/
inline void *open_or_create_mutex(const char *name, bool initial_owner, interprocess_security_attributes *attr)
{  return CreateMutexA(attr, (int)initial_owner, name);  }

inline void *open_or_create_mutex(const wchar_t *name, bool initial_owner, interprocess_security_attributes *attr)
{  return CreateMutexW(attr, (int)initial_owner, name);  }

inline unsigned long wait_for_single_object(void *handle, unsigned long time)
{  return WaitForSingleObject(handle, time); }

inline int release_mutex(void *handle)
{  return ReleaseMutex(handle);  }

inline int unmap_view_of_file(void *address)
{  return UnmapViewOfFile(address); }

inline void *open_or_create_semaphore(const char *name, long initial_count, long maximum_count, interprocess_security_attributes *attr)
{  return CreateSemaphoreA(attr, initial_count, maximum_count, name);  }

inline void *open_or_create_semaphore(const wchar_t *name, long initial_count, long maximum_count, interprocess_security_attributes *attr)
{  return CreateSemaphoreW(attr, initial_count, maximum_count, name);  }

inline void *open_semaphore(const char *name)
{  return OpenSemaphoreA(semaphore_all_access, 0, name);  }

inline void *open_semaphore(const wchar_t *name)
{  return OpenSemaphoreW(semaphore_all_access, 0, name);  }

inline int release_semaphore(void *handle, long release_count, long *prev_count)
{  return ReleaseSemaphore(handle, release_count, prev_count); }

class interprocess_all_access_security
{
   interprocess_security_attributes sa;
   interprocess_security_descriptor sd;
   bool initialized;

   public:
   interprocess_all_access_security()
      : initialized(false)
   {
      if(!boost::winapi::InitializeSecurityDescriptor(&sd, security_descriptor_revision))
         return;
      if(!boost::winapi::SetSecurityDescriptorDacl(&sd, true, 0, false))
         return;
      sa.lpSecurityDescriptor = &sd;
      sa.nLength = sizeof(interprocess_security_attributes);
      sa.bInheritHandle = false;
      initialized = true;
   }

   interprocess_security_attributes *get_attributes()
   {  return &sa; }
};

inline void * create_file_mapping (void * handle, unsigned long access, ::boost::ulong_long_type file_offset, const char * name, interprocess_security_attributes *psec)
{
   const boost::winapi::DWORD_ high_size(file_offset >> 32), low_size((boost::winapi::DWORD_)file_offset);
   return CreateFileMappingA (handle, psec, access, high_size, low_size, name);
}

inline void * create_file_mapping (void * handle, unsigned long access, ::boost::ulong_long_type file_offset, const wchar_t * name, interprocess_security_attributes *psec)
{
   const boost::winapi::DWORD_ high_size(file_offset >> 32), low_size((boost::winapi::DWORD_)file_offset);
   return CreateFileMappingW (handle, psec, access, high_size, low_size, name);
}

inline void * open_file_mapping (unsigned long access, const char *name)
{  return OpenFileMappingA (access, 0, name);   }

inline void * open_file_mapping (unsigned long access, const wchar_t *name)
{  return OpenFileMappingW (access, 0, name);   }

inline void *map_view_of_file_ex(void *handle, unsigned long file_access, ::boost::ulong_long_type offset, std::size_t numbytes, void *base_addr)
{
   const unsigned long offset_low  = (unsigned long)(offset & ((::boost::ulong_long_type)0xFFFFFFFF));
   const unsigned long offset_high = offset >> 32;
   return MapViewOfFileEx(handle, file_access, offset_high, offset_low, numbytes, base_addr);
}

template<class CharT>
inline void *create_file(const CharT *name, unsigned long access, unsigned long creation_flags, unsigned long attributes, interprocess_security_attributes *psec)
{
   for (unsigned int attempt(0); attempt < error_sharing_violation_tries; ++attempt){
      void * const handle = boost::winapi::create_file(name, access,
                                        file_share_read | file_share_write | file_share_delete,
                                        psec, creation_flags, attributes, 0);
      bool const invalid(invalid_handle_value == handle);
      if (!invalid){
         return handle;
      }
      if (error_sharing_violation != get_last_error()){
         return handle;
      }
      sleep(error_sharing_violation_sleep_ms);
   }
   return invalid_handle_value;
}

inline void get_system_info(interprocess_system_info *info)
{  boost::ipwinapiext::GetSystemInfo(info); }

inline bool flush_view_of_file(void *base_addr, std::size_t numbytes)
{  return 0 != boost::winapi::FlushViewOfFile(base_addr, numbytes); }

inline bool virtual_unlock(void *base_addr, std::size_t numbytes)
{  return 0 != boost::ipwinapiext::VirtualUnlock(base_addr, numbytes); }

inline bool virtual_protect(void *base_addr, std::size_t numbytes, unsigned long flNewProtect, unsigned long &lpflOldProtect)
{  return 0 != boost::ipwinapiext::VirtualProtect(base_addr, numbytes, flNewProtect, &lpflOldProtect); }

inline bool flush_file_buffers(void *handle)
{  return 0 != boost::ipwinapiext::FlushFileBuffers(handle); }

inline bool get_file_size(void *handle, __int64 &size)
{  return 0 != boost::winapi::GetFileSizeEx(handle, (boost::winapi::LARGE_INTEGER_*)&size);  }

template<class CharT>
inline bool create_directory(const CharT *name)
{
   interprocess_all_access_security sec;
   return 0 != boost::winapi::create_directory(name, sec.get_attributes());
}

template<class CharT>
inline bool remove_directory(const CharT *lpPathName)
{  return 0 != boost::winapi::remove_directory(lpPathName);   }

template<class CharT>
inline unsigned long get_temp_path(unsigned long length, CharT *buffer)
{  return boost::winapi::get_temp_path(length, buffer);   }

inline int set_end_of_file(void *handle)
{  return 0 != boost::winapi::SetEndOfFile(handle);   }

inline bool set_file_pointer(void *handle, __int64 distance, __int64 *new_file_pointer, unsigned long move_method)
{
   long highPart = distance >> 32u;
   boost::winapi::DWORD_ r = boost::winapi::SetFilePointer(handle, (unsigned long)distance, &highPart, move_method);
   bool br = r != boost::winapi::INVALID_SET_FILE_POINTER_ || boost::winapi::GetLastError() != 0;
   if (br && new_file_pointer){
      *new_file_pointer = (unsigned __int64)r + ((__int64)highPart << 32);
   }

   return br;
}

inline bool lock_file_ex(void *hnd, unsigned long flags, unsigned long reserved, unsigned long size_low, unsigned long size_high, interprocess_overlapped *overlapped)
{  return 0 != boost::winapi::LockFileEx(hnd, flags, reserved, size_low, size_high, overlapped); }

inline bool unlock_file_ex(void *hnd, unsigned long reserved, unsigned long size_low, unsigned long size_high, interprocess_overlapped *overlapped)
{  return 0 != boost::winapi::UnlockFileEx(hnd, reserved, size_low, size_high, overlapped);  }

inline bool write_file(void *hnd, const void *buffer, unsigned long bytes_to_write, unsigned long *bytes_written, interprocess_overlapped* overlapped)
{  return 0 != boost::winapi::WriteFile(hnd, buffer, bytes_to_write, bytes_written, overlapped);  }

inline bool read_file(void *hnd, void *buffer, unsigned long bytes_to_read, unsigned long *bytes_read, interprocess_overlapped* overlapped)
{  return 0 != boost::winapi::ReadFile(hnd, buffer, bytes_to_read, bytes_read, overlapped);  }

inline bool get_file_information_by_handle(void *hnd, interprocess_by_handle_file_information *info)
{  return 0 != boost::winapi::GetFileInformationByHandle(hnd, info);  }

inline long interlocked_increment(long volatile *addr)
{  return BOOST_INTERLOCKED_INCREMENT(const_cast<long*>(addr));  }

inline long interlocked_decrement(long volatile *addr)
{  return BOOST_INTERLOCKED_DECREMENT(const_cast<long*>(addr));  }

inline long interlocked_compare_exchange(long volatile *addr, long val1, long val2)
{  return BOOST_INTERLOCKED_COMPARE_EXCHANGE(const_cast<long*>(addr), val1, val2);  }

inline long interlocked_exchange_add(long volatile* addend, long value)
{  return BOOST_INTERLOCKED_EXCHANGE_ADD(const_cast<long*>(addend), value);  }

inline long interlocked_exchange(long volatile* addend, long value)
{  return BOOST_INTERLOCKED_EXCHANGE(const_cast<long*>(addend), value);  }

//Forward functions
inline hmodule load_library(const char *name)
{  return boost::winapi::LoadLibraryA(name); }

inline bool free_library(hmodule module)
{  return 0 != boost::winapi::FreeLibrary(module); }

inline farproc_t get_proc_address(hmodule module, const char *name)
{  return boost::winapi::GetProcAddress(module, name); }

inline void *get_current_process()
{  return boost::winapi::GetCurrentProcess();  }

inline hmodule get_module_handle(const char *name)
{  return boost::winapi::GetModuleHandleA(name); }

inline long reg_open_key_ex(hkey hKey, const char *lpSubKey, unsigned long ulOptions, unsigned long samDesired, hkey *phkResult)
{  return boost::ipwinapiext::RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult); }

inline long reg_open_key_ex(hkey hKey, const wchar_t *lpSubKey, unsigned long ulOptions, unsigned long samDesired, hkey *phkResult)
{  return boost::ipwinapiext::RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult); }


inline long reg_query_value_ex(hkey hKey, const char *lpValueName, unsigned long*lpReserved, unsigned long*lpType, unsigned char *lpData, unsigned long*lpcbData)
{  return boost::ipwinapiext::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData); }

inline long reg_query_value_ex(hkey hKey, const wchar_t *lpValueName, unsigned long*lpReserved, unsigned long*lpType, unsigned char *lpData, unsigned long*lpcbData)
{  return boost::ipwinapiext::RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData); }

inline long reg_close_key(hkey hKey)
{  return boost::ipwinapiext::RegCloseKey(hKey); }

inline void initialize_object_attributes
( object_attributes_t *pobject_attr, unicode_string_t *name
 , unsigned long attr, void *rootdir, void *security_descr)

{
   pobject_attr->Length = sizeof(object_attributes_t);
   pobject_attr->RootDirectory = rootdir;
   pobject_attr->Attributes = attr;
   pobject_attr->ObjectName = name;
   pobject_attr->SecurityDescriptor = security_descr;
   pobject_attr->SecurityQualityOfService = 0;
}

inline void rtl_init_empty_unicode_string(unicode_string_t *ucStr, wchar_t *buf, unsigned short bufSize)
{
   ucStr->Buffer = buf;
   ucStr->Length = 0;
   ucStr->MaximumLength = bufSize;
}

//A class that locates and caches loaded DLL function addresses.
template<int Dummy>
struct function_address_holder
{
   enum  { NtSetInformationFile
         , NtQuerySystemInformation
         , NtQueryObject
         , NtQuerySemaphore
         , NtQuerySection
         , NtOpenFile
         , NtClose
         , NtQueryTimerResolution
         , NumFunction
         };
   enum { NtDll_dll, Kernel32_dll, NumModule };

   private:
   static const char *FunctionNames[NumFunction];
   static const char *ModuleNames[NumModule];
   static farproc_t FunctionAddresses[NumFunction];
   static unsigned int FunctionModules[NumFunction];
   static volatile long FunctionStates[NumFunction];
   static hmodule ModuleAddresses[NumModule];
   static volatile long ModuleStates[NumModule];

   static hmodule get_module_from_id(unsigned int id)
   {
      BOOST_ASSERT(id < (unsigned int)NumModule);
      hmodule addr = get_module_handle(ModuleNames[id]);
      BOOST_ASSERT(addr);
      return addr;
   }

   static hmodule get_module(const unsigned int id)
   {
      BOOST_ASSERT(id < (unsigned int)NumModule);
      for(unsigned i = 0; ModuleStates[id] < 2; ++i){
         if(interlocked_compare_exchange(&ModuleStates[id], 1, 0) == 0){
            ModuleAddresses[id] = get_module_from_id(id);
            interlocked_increment(&ModuleStates[id]);
            break;
         }
         else if(i & 1){
            sched_yield();
         }
         else{
            sleep_tick();
         }
      }
      return ModuleAddresses[id];
   }

   static farproc_t get_address_from_dll(const unsigned int id)
   {
      BOOST_ASSERT(id < (unsigned int)NumFunction);
      farproc_t addr = get_proc_address(get_module(FunctionModules[id]), FunctionNames[id]);
      BOOST_ASSERT(addr);
      return addr;
   }

   public:
   static farproc_t get(const unsigned int id)
   {
      BOOST_ASSERT(id < (unsigned int)NumFunction);
      for(unsigned i = 0; FunctionStates[id] < 2; ++i){
         if(interlocked_compare_exchange(&FunctionStates[id], 1, 0) == 0){
            FunctionAddresses[id] = get_address_from_dll(id);
            interlocked_increment(&FunctionStates[id]);
            break;
         }
         else if(i & 1){
            sched_yield();
         }
         else{
            sleep_tick();
         }
      }
      return FunctionAddresses[id];
   }
};

template<int Dummy>
const char *function_address_holder<Dummy>::FunctionNames[function_address_holder<Dummy>::NumFunction] =
{
   "NtSetInformationFile",
   "NtQuerySystemInformation",
   "NtQueryObject",
   "NtQuerySemaphore",
   "NtQuerySection",
   "NtOpenFile",
   "NtClose",
   "NtQueryTimerResolution",
};

template<int Dummy>
unsigned int function_address_holder<Dummy>::FunctionModules[function_address_holder<Dummy>::NumFunction] =
{
   NtDll_dll,
   NtDll_dll,
   NtDll_dll,
   NtDll_dll,
   NtDll_dll,
   NtDll_dll,
   NtDll_dll,
   NtDll_dll,
};

template<int Dummy>
const char *function_address_holder<Dummy>::ModuleNames[function_address_holder<Dummy>::NumModule] =
{
   "ntdll.dll"//, "kernel32.dll"
};


template<int Dummy>
farproc_t function_address_holder<Dummy>::FunctionAddresses[function_address_holder<Dummy>::NumFunction];

template<int Dummy>
volatile long function_address_holder<Dummy>::FunctionStates[function_address_holder<Dummy>::NumFunction];

template<int Dummy>
hmodule function_address_holder<Dummy>::ModuleAddresses[function_address_holder<Dummy>::NumModule];

template<int Dummy>
volatile long function_address_holder<Dummy>::ModuleStates[function_address_holder<Dummy>::NumModule];


struct dll_func
   : public function_address_holder<0>
{};

//Complex winapi based functions...
struct library_unloader
{
   hmodule lib_;
   library_unloader(hmodule module) : lib_(module){}
   ~library_unloader(){ free_library(lib_);  }
};


inline bool get_system_time_of_day_information(system_timeofday_information &info)
{
   NtQuerySystemInformation_t pNtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformation_t>
         (dll_func::get(dll_func::NtQuerySystemInformation));
   unsigned long res;
   long status = pNtQuerySystemInformation(system_time_of_day_information, &info, sizeof(info), &res);
   if(status){
      return false;
   }
   return true;
}

inline bool get_boot_time(unsigned char (&bootstamp) [BootstampLength])
{
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }
   std::memcpy(&bootstamp[0], &info.Reserved1, sizeof(bootstamp));
   return true;
}

inline bool get_boot_and_system_time(unsigned char (&bootsystemstamp) [BootAndSystemstampLength])
{
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }
   std::memcpy(&bootsystemstamp[0], &info.Reserved1, sizeof(bootsystemstamp));
   return true;
}

//Writes the hexadecimal value of the buffer, in the wide character string.
//str must be twice length
inline void buffer_to_wide_str(const void *buf, std::size_t length, wchar_t *str)
{
   const wchar_t Characters [] =
      { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7'
      , L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
   std::size_t char_counter = 0;
   const char *chbuf = static_cast<const char *>(buf);
   for(std::size_t i = 0; i != length; ++i){
      str[char_counter++] = Characters[(chbuf[i]&0xF0)>>4];
      str[char_counter++] = Characters[(chbuf[i]&0x0F)];
   }
}

//Writes the hexadecimal value of the buffer, in the narrow character string.
//str must be twice length
inline void buffer_to_narrow_str(const void *buf, std::size_t length, char *str)
{
   const char Characters [] =
      { '0', '1', '2', '3', '4', '5', '6', '7'
      , '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
   std::size_t char_counter = 0;
   const char *chbuf = static_cast<const char *>(buf);
   for(std::size_t i = 0; i != length; ++i){
      str[char_counter++] = Characters[(chbuf[i]&0xF0)>>4];
      str[char_counter++] = Characters[(chbuf[i]&0x0F)];
   }
}

inline bool get_boot_time_str(char *bootstamp_str, std::size_t &s)
   //will write BootstampLength chars
{
   if(s < (BootstampLength*2))
      return false;
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }

   buffer_to_narrow_str(info.Reserved1, BootstampLength, bootstamp_str);
   s = BootstampLength*2;
   return true;
}

inline bool get_boot_and_system_time_wstr(wchar_t *bootsystemstamp, std::size_t &s)
   //will write BootAndSystemstampLength chars
{
   if(s < (BootAndSystemstampLength*2))
      return false;
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }

   buffer_to_wide_str(&info.Reserved1[0], BootAndSystemstampLength, bootsystemstamp);
   s = BootAndSystemstampLength*2;
   return true;
}

class handle_closer
{
   void *handle_;
   handle_closer(const handle_closer &);
   handle_closer& operator=(const handle_closer &);
   public:
   explicit handle_closer(void *handle) : handle_(handle){}
   ~handle_closer()
   {  close_handle(handle_);  }
};

union ntquery_mem_t
{
   object_name_information_t name;
   struct ren_t
   {
      file_rename_information_t info;
      wchar_t buf[1];
   } ren;
};

class nt_query_mem_deleter
{
   static const std::size_t rename_offset = offsetof(ntquery_mem_t, ren.info.FileName) -
      offsetof(ntquery_mem_t, name.Name.Buffer);
   //                                           Timestamp                      process id              atomic count
   static const std::size_t rename_suffix =
      (SystemTimeOfDayInfoLength + sizeof(unsigned long) + sizeof(boost::winapi::DWORD_))*2;

   public:
   explicit nt_query_mem_deleter(std::size_t object_name_information_size)
      : m_size(object_name_information_size + rename_offset + rename_suffix)
      , m_buf(new char [m_size])
   {}

   ~nt_query_mem_deleter()
   {
      delete[]m_buf;
   }

   void realloc_mem(std::size_t num_bytes)
   {
      num_bytes += rename_suffix + rename_offset;
      char *buf = m_buf;
      m_buf = new char[num_bytes];
      delete[]buf;
      m_size = num_bytes;
   }

   ntquery_mem_t *query_mem() const
   {  return static_cast<ntquery_mem_t *>(static_cast<void*>(m_buf));  }

   unsigned long object_name_information_size() const
   {
      return static_cast<unsigned long>(m_size - rename_offset - SystemTimeOfDayInfoLength*2);
   }

   std::size_t file_rename_information_size() const
   {  return static_cast<unsigned long>(m_size);  }

   private:
   std::size_t m_size;
   char *m_buf;
};

class c_heap_deleter
{
   public:
   explicit c_heap_deleter(std::size_t size)
      : m_buf(::malloc(size))
   {}

   ~c_heap_deleter()
   {
      if(m_buf) ::free(m_buf);
   }

   void realloc_mem(std::size_t num_bytes)
   {
      void *oldBuf = m_buf;
      m_buf = ::realloc(m_buf, num_bytes);
      if (!m_buf){
         free(oldBuf);
      }
   }

   void *get() const
   {  return m_buf;  }

   private:
   void *m_buf;
};

template<class CharT>
inline bool unlink_file(const CharT *filename)
{
   //Don't try to optimize doing a DeleteFile first
   //as there are interactions with permissions and
   //in-use files.
   //
   //if(!delete_file(filename)){
   //   (...)
   //

   //This functions tries to emulate UNIX unlink semantics in windows.
   //
   //- Open the file and mark the handle as delete-on-close
   //- Rename the file to an arbitrary name based on a random number
   //- Close the handle. If there are no file users, it will be deleted.
   //  Otherwise it will be used by already connected handles but the
   //  file name can't be used to open this file again
   try{
      NtSetInformationFile_t pNtSetInformationFile =
         reinterpret_cast<NtSetInformationFile_t>(dll_func::get(dll_func::NtSetInformationFile));

      NtQueryObject_t pNtQueryObject = reinterpret_cast<NtQueryObject_t>(dll_func::get(dll_func::NtQueryObject));

      //First step: Obtain a handle to the file using Win32 rules. This resolves relative paths
      void *fh = create_file(filename, generic_read | delete_access, open_existing, 0, 0);
      if(fh == invalid_handle_value){
         return false;
      }

      handle_closer h_closer(fh);
      {
         //Obtain name length
         unsigned long size;
         const std::size_t initial_string_mem = 512u;

         nt_query_mem_deleter nt_query_mem(sizeof(ntquery_mem_t)+initial_string_mem);
         //Obtain file name with guessed length
         if(pNtQueryObject(fh, object_name_information, nt_query_mem.query_mem(), nt_query_mem.object_name_information_size(), &size)){
            //Obtain file name with exact length buffer
            nt_query_mem.realloc_mem(size);
            if(pNtQueryObject(fh, object_name_information, nt_query_mem.query_mem(), nt_query_mem.object_name_information_size(), &size)){
               return false;
            }
         }
         ntquery_mem_t *pmem = nt_query_mem.query_mem();
         file_rename_information_t *pfri = &pmem->ren.info;
         const std::size_t RenMaxNumChars =
            (((char*)(pmem) + nt_query_mem.file_rename_information_size()) - (char*)&pmem->ren.info.FileName[0])/sizeof(wchar_t);

         //Copy filename to the rename member
         std::memmove(pmem->ren.info.FileName, pmem->name.Name.Buffer, pmem->name.Name.Length);
         std::size_t filename_string_length = pmem->name.Name.Length/sizeof(wchar_t);

         //Search '\\' character to replace from it
         for(std::size_t i = filename_string_length; i != 0; --filename_string_length){
            if(pmem->ren.info.FileName[--i] == L'\\')
               break;
         }

         //Add random number
         std::size_t s = RenMaxNumChars - filename_string_length;
         if(!get_boot_and_system_time_wstr(&pfri->FileName[filename_string_length], s)){
            return false;
         }
         filename_string_length += s;

         //Sometimes the precission of the timestamp is not enough and we need to add another random number.
         //The process id (to exclude concurrent processes) and an atomic count (to exclude concurrent threads).
         //should be enough
         const unsigned long pid = get_current_process_id();
         buffer_to_wide_str(&pid, sizeof(pid), &pfri->FileName[filename_string_length]);
         filename_string_length += sizeof(pid)*2;

         static volatile boost::uint32_t u32_count = 0;
         interlocked_decrement(reinterpret_cast<volatile long*>(&u32_count));
         buffer_to_wide_str(const_cast<const boost::uint32_t *>(&u32_count), sizeof(boost::uint32_t), &pfri->FileName[filename_string_length]);
         filename_string_length += sizeof(boost::uint32_t)*2;

         //Fill rename information (FileNameLength is in bytes)
         pfri->FileNameLength = static_cast<unsigned long>(sizeof(wchar_t)*(filename_string_length));
         pfri->Replace = 1;
         pfri->RootDir = 0;

         //Cange the name of the in-use file...
         io_status_block_t io;
         if(0 != pNtSetInformationFile(fh, &io, pfri, nt_query_mem.file_rename_information_size(), file_rename_information)){
            return false;
         }
      }
      //...and mark it as delete-on-close
      {
         //Don't use pNtSetInformationFile with file_disposition_information as it can return STATUS_CANNOT_DELETE
         //if the file is still mapped. Reopen it with NtOpenFile and file_delete_on_close
         NtOpenFile_t pNtOpenFile = reinterpret_cast<NtOpenFile_t>(dll_func::get(dll_func::NtOpenFile));
         NtClose_t pNtClose = reinterpret_cast<NtClose_t>(dll_func::get(dll_func::NtClose));
         const wchar_t empty_str [] = L"";
         unicode_string_t ustring = { sizeof(empty_str) - sizeof (wchar_t)   //length in bytes without null
                                    , sizeof(empty_str)   //total size in bytes of memory allocated for Buffer.
                                    , const_cast<wchar_t*>(empty_str)
                                    };
         object_attributes_t object_attr;
         initialize_object_attributes(&object_attr, &ustring, 0, fh, 0);
         void* fh2 = 0;
         io_status_block_t io;
         pNtOpenFile( &fh2, delete_flag, &object_attr, &io
                    , file_share_read | file_share_write | file_share_delete, file_delete_on_close);
         pNtClose(fh2);
         //Even if NtOpenFile fails, the file was renamed and the original no longer exists, so return a success status
         return true;
      }
   }
   catch(...){
      return false;
   }
   return true;
}

struct reg_closer
{
   hkey key_;
   reg_closer(hkey key) : key_(key){}
   ~reg_closer(){ reg_close_key(key_);  }
};

template <class CharT>
inline bool get_registry_value_buffer(hkey key_type, const CharT *subkey_name, const CharT *value_name, void *buf, std::size_t &buflen)
{
   bool bret = false;
   hkey key;
   if (reg_open_key_ex( key_type
                     , subkey_name
                     , 0
                     , key_query_value
                     , &key) == 0){
      reg_closer key_closer(key);

      //Obtain the value
      unsigned long size = buflen;
      unsigned long type;
      buflen = 0;
      bret = 0 == reg_query_value_ex( key, value_name, 0, &type, (unsigned char*)buf, &size);
      if(bret)
         buflen = (std::size_t)size;
   }
   return bret;
}

template<class CharT>
inline bool get_registry_value_string(hkey key_type, const CharT *subkey_name, const CharT *value_name, std::basic_string<CharT> &s)
{
   bool bret = false;
   s.clear();
   hkey key;
   if (reg_open_key_ex( key_type
                     , subkey_name
                     , 0
                     , key_query_value
                     , &key) == 0){
      reg_closer key_closer(key);

      //Obtain the value
      unsigned long size;
      unsigned long type;
      long err = reg_query_value_ex( key, value_name, 0, &type, 0, &size);
      if((reg_sz == type || reg_expand_sz == type) && !err){
         //Size includes terminating NULL
         s.resize(size/sizeof(CharT));
         err = reg_query_value_ex( key, value_name, 0, &type, (unsigned char*)(&s[0]), &size);
         if(!err){
            s.erase(s.end()-1);
            bret = true;
         }
         (void)err;
      }
   }
   return bret;
}

template<class CharT>
inline void get_shared_documents_folder(std::basic_string<CharT> &s)
{
   get_registry_value_string( hkey_local_machine
                            , "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"
                            , "Common AppData"
                            , s);
}

inline void get_shared_documents_folder(std::wstring &s)
{
   get_registry_value_string( hkey_local_machine
                            , L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"
                            , L"Common AppData"
                            , s);
}

template<class CharT>
inline void get_registry_value(const CharT *folder, const CharT *value_key, std::vector<unsigned char> &s)
{
   s.clear();
   hkey key;
   if (reg_open_key_ex( hkey_local_machine
                     , folder
                     , 0
                     , key_query_value
                     , &key) == 0){
      reg_closer key_closer(key);

      //Obtain the value
      unsigned long size;
      unsigned long type;
      const char *const reg_value = value_key;
      //long err = (*pRegQueryValue)( key, reg_value, 0, &type, 0, &size);
      long err = reg_query_value_ex( key, reg_value, 0, &type, 0, &size);
      if(!err){
         //Size includes terminating NULL
         s.resize(size);
         //err = (*pRegQueryValue)( key, reg_value, 0, &type, (unsigned char*)(&s[0]), &size);
         err = reg_query_value_ex( key, reg_value, 0, &type, (unsigned char*)(&s[0]), &size);
         if(!err)
            s.erase(s.end()-1);
         (void)err;
      }
   }
}

inline bool is_directory(const char *path)
{
   unsigned long attrib = GetFileAttributesA(path);

   return (attrib != invalid_file_attributes &&
           (attrib & file_attribute_directory));
}

inline bool get_file_mapping_size(void *file_mapping_hnd, __int64 &size)
{
   NtQuerySection_t pNtQuerySection =
      reinterpret_cast<NtQuerySection_t>(dll_func::get(dll_func::NtQuerySection));
   //Obtain file name
   interprocess_section_basic_information info;
   unsigned long ntstatus =
      pNtQuerySection(file_mapping_hnd, section_basic_information, &info, sizeof(info), 0);
   size = info.section_size;
   return !ntstatus;
}

inline bool get_semaphore_info(void *handle, long &count, long &limit)
{
   winapi::interprocess_semaphore_basic_information info;
   winapi::NtQuerySemaphore_t pNtQuerySemaphore =
         reinterpret_cast<winapi::NtQuerySemaphore_t>(dll_func::get(winapi::dll_func::NtQuerySemaphore));
   unsigned int ret_len;
   long status = pNtQuerySemaphore(handle, winapi::semaphore_basic_information, &info, sizeof(info), &ret_len);
   count = info.count;
   limit = info.limit;
   return !status;
}

inline bool query_timer_resolution(unsigned long *lowres, unsigned long *highres, unsigned long *curres)
{
   winapi::NtQueryTimerResolution_t pNtQueryTimerResolution =
         reinterpret_cast<winapi::NtQueryTimerResolution_t>(dll_func::get(winapi::dll_func::NtQueryTimerResolution));
   return !pNtQueryTimerResolution(lowres, highres, curres);
}

inline bool query_performance_counter(__int64 *lpPerformanceCount)
{
   return 0 != boost::winapi::QueryPerformanceCounter(reinterpret_cast<boost::winapi::LARGE_INTEGER_*>(lpPerformanceCount));
}

inline bool query_performance_frequency(__int64 *lpFrequency)
{
   return 0 != boost::winapi::QueryPerformanceFrequency(reinterpret_cast<boost::winapi::LARGE_INTEGER_*>(lpFrequency));
}

inline unsigned long get_tick_count()
{  return GetTickCount();  }


template<class CharT>
struct winapi_traits;

template<>
struct winapi_traits<char>
{
   static int cmp(const char *a, const char *b)
   {  return std::strcmp(a, b); }
};

template<>
struct winapi_traits<wchar_t>
{
   static int cmp(const wchar_t *a, const wchar_t *b)
   {  return std::wcscmp(a, b); }
};


#if defined(BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED)


inline bool get_last_bootup_time(std::string &stamp)
{
   unsigned dword_val = 0;
   std::size_t dword_size = sizeof(dword_val);
   bool b_ret = get_registry_value_buffer( hkey_local_machine
      , "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"
      , "BootId", &dword_val, dword_size);
   if (b_ret)
   {
      char dword_str[sizeof(dword_val)*2u+1];
      buffer_to_narrow_str(&dword_val, dword_size, dword_str);
      dword_str[sizeof(dword_val)*2] = '\0';
      stamp = dword_str;

      b_ret = get_registry_value_buffer( hkey_local_machine
         , "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"
         , "HybridBootAnimationTime", &dword_val, dword_size);
      //Old Windows versions have no HybridBootAnimationTime
      if(b_ret)
      {
         buffer_to_narrow_str(&dword_val, dword_size, dword_str);
         dword_str[sizeof(dword_val)*2] = '\0';
         stamp += "_";
         stamp += dword_str;
      }
      b_ret = true;
   }
   return b_ret;
}

inline bool get_last_bootup_time(std::wstring &stamp)
{
   unsigned dword_val = 0;
   std::size_t dword_size = sizeof(dword_val);
   bool b_ret = get_registry_value_buffer( hkey_local_machine
      , L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"
      , L"BootId", &dword_val, dword_size);
   if (b_ret)
   {
      wchar_t dword_str[sizeof(dword_val)*2u+1];
      buffer_to_wide_str(&dword_val, dword_size, dword_str);
      dword_str[sizeof(dword_val)*2] = L'\0';
      stamp = dword_str;

      b_ret = get_registry_value_buffer( hkey_local_machine
         , L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"
         , L"HybridBootAnimationTime", &dword_val, dword_size);
      //Old Windows versions have no HybridBootAnimationTime
      if(b_ret)
      {
         buffer_to_wide_str(&dword_val, dword_size, dword_str);
         dword_str[sizeof(dword_val)*2] = L'\0';
         stamp += L"_";
         stamp += dword_str;
      }
      b_ret = true;
   }
   return b_ret;
}

#elif defined(BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED)

static const unsigned long eventlog_sequential_read = 0x0001;
static const unsigned long eventlog_backwards_read  = 0x0008;

struct interprocess_eventlogrecord
{
    unsigned long  Length;        // Length of full record
    unsigned long  Reserved;      // Used by the service
    unsigned long  RecordNumber;  // Absolute record number
    unsigned long  TimeGenerated; // Seconds since 1-1-1970
    unsigned long  TimeWritten;   // Seconds since 1-1-1970
    unsigned long  EventID;
    unsigned short EventType;
    unsigned short NumStrings;
    unsigned short EventCategory;
    unsigned short ReservedFlags; // For use with paired events (auditing)
    unsigned long  ClosingRecordNumber; // For use with paired events (auditing)
    unsigned long  StringOffset;  // Offset from beginning of record
    unsigned long  UserSidLength;
    unsigned long  UserSidOffset;
    unsigned long  DataLength;
    unsigned long  DataOffset;    // Offset from beginning of record
    //
    // Then follow:
    //
    // wchar_t SourceName[]
    // wchar_t Computername[]
    // SID   UserSid
    // wchar_t Strings[]
    // BYTE  Data[]
    // CHAR  Pad[]
    // unsigned long Length;
    //
};

class eventlog_handle_closer
{
   void *handle_;
   eventlog_handle_closer(const handle_closer &);
   eventlog_handle_closer& operator=(const eventlog_handle_closer &);
   public:
   explicit eventlog_handle_closer(void *handle) : handle_(handle){}
   ~eventlog_handle_closer()
   {  CloseEventLog(handle_);  }
};

// Loop through the buffer and obtain the contents of the
// requested record in the buffer.
template<class CharT>
inline bool find_record_in_buffer( const void* pBuffer, unsigned long dwBytesRead, const CharT *provider_name
                                 , unsigned int id_to_find, interprocess_eventlogrecord *&pevent_log_record)
{
   const unsigned char * pRecord = static_cast<const unsigned char*>(pBuffer);
   const unsigned char * pEndOfRecords = pRecord + dwBytesRead;

   while (pRecord < pEndOfRecords){
      interprocess_eventlogrecord *pTypedRecord = (interprocess_eventlogrecord*)pRecord;
      // Check provider, written at the end of the fixed-part of the record

      if (0 == winapi_traits<CharT>::cmp(provider_name, (CharT*)(pRecord + sizeof(interprocess_eventlogrecord))))
      {
         // Check event id
         if(id_to_find == (pTypedRecord->EventID & 0xFFFF)){
            pevent_log_record = pTypedRecord;
            return true;
         }
      }

      pRecord += pTypedRecord->Length;
   }
   pevent_log_record = 0;
   return false;
}

//Obtains the bootup time from the System Event Log,
//event ID == 6005 (event log started).
//Adapted from http://msdn.microsoft.com/en-us/library/windows/desktop/bb427356.aspx
inline bool get_last_bootup_time(std::string &stamp)
{
   const char *source_name = "System";
   const char *provider_name = "EventLog";
   const unsigned short event_id = 6005u;

   unsigned long status = 0;
   unsigned long dwBytesToRead = 0;
   unsigned long dwBytesRead = 0;
   unsigned long dwMinimumBytesToRead = 0;

   // The source name (provider) must exist as a subkey of Application.
   void *hEventLog = OpenEventLogA(0, source_name);
   if (hEventLog){
      eventlog_handle_closer hnd_closer(hEventLog); (void)hnd_closer;
      // Allocate an initial block of memory used to read event records. The number
      // of records read into the buffer will vary depending on the size of each event.
      // The size of each event will vary based on the size of the user-defined
      // data included with each event, the number and length of insertion
      // strings, and other data appended to the end of the event record.
      dwBytesToRead = max_record_buffer_size;
      c_heap_deleter heap_deleter(dwBytesToRead);

      // Read blocks of records until you reach the end of the log or an
      // error occurs. The records are read from newest to oldest. If the buffer
      // is not big enough to hold a complete event record, reallocate the buffer.
      if (heap_deleter.get() != 0){
         while (0 == status){
            if (!ReadEventLogA(hEventLog,
                  eventlog_sequential_read | eventlog_backwards_read,
                  0,
                  heap_deleter.get(),
                  dwBytesToRead,
                  &dwBytesRead,
                  &dwMinimumBytesToRead)) {
               status = get_last_error();
               if (error_insufficient_buffer == status) {
                  status = 0;
                  dwBytesToRead = dwMinimumBytesToRead;
                  heap_deleter.realloc_mem(dwMinimumBytesToRead);
                  if (!heap_deleter.get()){
                     return false;
                  }
               }
               else{  //Not found or EOF
                  return false;
               }
            }
            else
            {
               interprocess_eventlogrecord *pTypedRecord;
               // Print the contents of each record in the buffer.
               if(find_record_in_buffer(heap_deleter.get(), dwBytesRead, provider_name, event_id, pTypedRecord)){
                  char stamp_str[sizeof(unsigned long)*3+1];
                  std::sprintf(&stamp_str[0], "%u", ((unsigned int)pTypedRecord->TimeGenerated));
                  stamp = stamp_str;
                  break;
               }
            }
         }
      }
   }
   return true;
}


inline bool get_last_bootup_time(std::wstring &stamp)
{
   const wchar_t *source_name = L"System";
   const wchar_t *provider_name = L"EventLog";
   const unsigned short event_id = 6005u;

   unsigned long status = 0;
   unsigned long dwBytesToRead = 0;
   unsigned long dwBytesRead = 0;
   unsigned long dwMinimumBytesToRead = 0;

   // The source name (provider) must exist as a subkey of Application.
   void *hEventLog = OpenEventLogW(0, source_name);
   if (hEventLog){
      eventlog_handle_closer hnd_closer(hEventLog); (void)hnd_closer;
      // Allocate an initial block of memory used to read event records. The number
      // of records read into the buffer will vary depending on the size of each event.
      // The size of each event will vary based on the size of the user-defined
      // data included with each event, the number and length of insertion
      // strings, and other data appended to the end of the event record.
      dwBytesToRead = max_record_buffer_size;
      c_heap_deleter heap_deleter(dwBytesToRead);

      // Read blocks of records until you reach the end of the log or an
      // error occurs. The records are read from newest to oldest. If the buffer
      // is not big enough to hold a complete event record, reallocate the buffer.
      if (heap_deleter.get() != 0){
         while (0 == status){
            if (!ReadEventLogW(hEventLog,
                  eventlog_sequential_read | eventlog_backwards_read,
                  0,
                  heap_deleter.get(),
                  dwBytesToRead,
                  &dwBytesRead,
                  &dwMinimumBytesToRead)) {
               status = get_last_error();
               if (error_insufficient_buffer == status) {
                  status = 0;
                  dwBytesToRead = dwMinimumBytesToRead;
                  heap_deleter.realloc_mem(dwMinimumBytesToRead);
                  if (!heap_deleter.get()){
                     return false;
                  }
               }
               else{  //Not found or EOF
                  return false;
               }
            }
            else
            {
               interprocess_eventlogrecord *pTypedRecord;
               // Print the contents of each record in the buffer.
               if(find_record_in_buffer(heap_deleter.get(), dwBytesRead, provider_name, event_id, pTypedRecord)){
                  wchar_t stamp_str[sizeof(unsigned long)*3+1];
                  std::swprintf(&stamp_str[0], L"%u", ((unsigned int)pTypedRecord->TimeGenerated));
                  stamp = stamp_str;
                  break;
               }
            }
         }
      }
   }
   return true;
}

#endif   //BOOST_INTERPROCESS_BOOTSTAMP_IS_EVENTLOG_BASED


}  //namespace winapi
}  //namespace interprocess
}  //namespace boost

#if defined(BOOST_GCC) && (BOOST_GCC >= 40600)
#  pragma GCC diagnostic pop
#endif

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifdef BOOST_INTERPROCESS_WIN32_API_HPP
