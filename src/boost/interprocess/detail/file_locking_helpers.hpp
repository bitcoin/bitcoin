//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_FILE_LOCKING_HELPERS_HPP
#define BOOST_INTERPROCESS_FILE_LOCKING_HELPERS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstddef>
#include <boost/interprocess/detail/os_file_functions.hpp>

#include <boost/interprocess/detail/shared_dir_helpers.hpp>

#if defined(BOOST_INTERPROCESS_WINDOWS)

#include <fcntl.h>
#include <io.h>
#include <sys/locking.h>

#else //defined(BOOST_INTERPROCESS_WINDOWS)

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#endif   //defined(BOOST_INTERPROCESS_WINDOWS)

namespace boost{
namespace interprocess{
namespace ipcdetail{

#if defined(BOOST_INTERPROCESS_WINDOWS)

struct locking_file_serial_id
{
   int fd;
   unsigned long dwVolumeSerialNumber;
   unsigned long nFileIndexHigh;
   unsigned long nFileIndexLow;
   //This reference count counts the number of modules attached
   //to the shared memory and lock file. This serves to unlink
   //the locking file and shared memory when all modules are
   //done with the global memory (shared memory)
   volatile boost::uint32_t modules_attached_to_gmem_count;
};

inline bool lock_locking_file(int fd)
{
   int ret = 0;
   while(ret != 0 && errno == EDEADLK){
      ret = _locking(fd, _LK_LOCK, 1/*lock_file_contents_length()*/);
   }
   return 0 == ret;
}

inline bool try_lock_locking_file(int fd)
{
   return 0 == _locking(fd, _LK_NBLCK , 1);
}

inline int open_or_create_and_lock_file(const char *name)
{
   permissions p;
   p.set_unrestricted();
   while(1){
      file_handle_t handle = create_or_open_file(name, read_write, p);
      int fd = _open_osfhandle((intptr_t)handle, _O_TEXT);
      if(fd < 0){
         close_file(handle);
         return fd;
      }
      if(!try_lock_locking_file(fd)){
         _close(fd);
         return -1;
      }
      struct _stat s;
      if(0 == _stat(name, &s)){
         return fd;
      }
      else{
         _close(fd);
      }
   }
}

inline int try_open_and_lock_file(const char *name)
{
   file_handle_t handle = open_existing_file(name, read_write);
   int fd = _open_osfhandle((intptr_t)handle, _O_TEXT);
   if(fd < 0){
      close_file(handle);
      return fd;
   }
   if(!try_lock_locking_file(fd)){
      _close(fd);
      return -1;
   }
   return fd;
}

inline void close_lock_file(int fd)
{  _close(fd); }

inline bool is_valid_fd(int fd)
{
   struct _stat s;
   return EBADF != _fstat(fd, &s);
}

inline bool is_normal_file(int fd)
{
   if(_isatty(fd))
      return false;
   struct _stat s;
   if(0 != _fstat(fd, &s))
      return false;
   return 0 != (s.st_mode & _S_IFREG);
}

inline std::size_t get_size(int fd)
{
   struct _stat s;
   if(0 != _fstat(fd, &s))
      return 0u;
   return (std::size_t)s.st_size;
}

inline bool fill_file_serial_id(int fd, locking_file_serial_id &id)
{
   winapi::interprocess_by_handle_file_information info;
   if(!winapi::get_file_information_by_handle((void*)_get_osfhandle(fd), &info))
      return false;
   id.fd = fd;
   id.dwVolumeSerialNumber = info.dwVolumeSerialNumber;
   id.nFileIndexHigh = info.nFileIndexHigh;
   id.nFileIndexLow = info.nFileIndexLow;
   id.modules_attached_to_gmem_count = 1; //Initialize attached count
   return true;
}

inline bool compare_file_serial(int fd, const locking_file_serial_id &id)
{
   winapi::interprocess_by_handle_file_information info;
   if(!winapi::get_file_information_by_handle((void*)_get_osfhandle(fd), &info))
      return false;

   return   id.dwVolumeSerialNumber == info.dwVolumeSerialNumber  &&
            id.nFileIndexHigh       == info.nFileIndexHigh        &&
            id.nFileIndexLow        == info.nFileIndexLow;
}

#else //UNIX

struct locking_file_serial_id
{
   int fd;
   dev_t st_dev;
   ino_t st_ino;
   //This reference count counts the number of modules attached
   //to the shared memory and lock file. This serves to unlink
   //the locking file and shared memory when all modules are
   //done with the global memory (shared memory)
   volatile boost::uint32_t modules_attached_to_gmem_count;
};

inline bool lock_locking_file(int fd)
{
   int ret = 0;
   while(ret != 0 && errno != EINTR){
      struct flock lock;
      lock.l_type = F_WRLCK;
      lock.l_whence = SEEK_SET;
      lock.l_start = 0;
      lock.l_len = 1;
      ret = fcntl (fd, F_SETLKW, &lock);
   }
   return 0 == ret;
}

inline bool try_lock_locking_file(int fd)
{
   struct flock lock;
   lock.l_type = F_WRLCK;
   lock.l_whence = SEEK_SET;
   lock.l_start = 0;
   lock.l_len = 1;
   return 0 == fcntl (fd, F_SETLK, &lock);
}

inline int open_or_create_and_lock_file(const char *name)
{
   permissions p;
   p.set_unrestricted();
   while(1){
      int fd = create_or_open_file(name, read_write, p);
      if(fd < 0){
         return fd;
      }
      if(!try_lock_locking_file(fd)){
         close(fd);
         return -1;
      }
      struct stat s;
      if(0 == stat(name, &s)){
         return fd;
      }
      else{
         close(fd);
      }
   }
}

inline int try_open_and_lock_file(const char *name)
{
   int fd = open_existing_file(name, read_write);
   if(fd < 0){
      return fd;
   }
   if(!try_lock_locking_file(fd)){
      close(fd);
      return -1;
   }
   return fd;
}

inline void close_lock_file(int fd)
{  close(fd); }

inline bool is_valid_fd(int fd)
{
   struct stat s;
   return EBADF != fstat(fd, &s);
}

inline bool is_normal_file(int fd)
{
   struct stat s;
   if(0 != fstat(fd, &s))
      return false;
   return 0 != (s.st_mode & S_IFREG);
}

inline std::size_t get_size(int fd)
{
   struct stat s;
   if(0 != fstat(fd, &s))
      return 0u;
   return (std::size_t)s.st_size;
}

inline bool fill_file_serial_id(int fd, locking_file_serial_id &id)
{
   struct stat s;
   if(0 != fstat(fd, &s))
      return false;
   id.fd = fd;
   id.st_dev = s.st_dev;
   id.st_ino = s.st_ino;
   id.modules_attached_to_gmem_count = 1; //Initialize attached count
   return true;
}

inline bool compare_file_serial(int fd, const locking_file_serial_id &id)
{
   struct stat info;
   if(0 != fstat(fd, &info))
      return false;

   return   id.st_dev == info.st_dev  &&
            id.st_ino == info.st_ino;
}

#endif

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_FILE_LOCKING_HELPERS_HPP
