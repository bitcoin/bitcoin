//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_POSIX_SEMAPHORE_WRAPPER_HPP
#define BOOST_INTERPROCESS_POSIX_SEMAPHORE_WRAPPER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/detail/shared_dir_helpers.hpp>
#include <boost/interprocess/detail/timed_utils.hpp>
#include <boost/interprocess/permissions.hpp>

#include <fcntl.h>      //O_CREAT, O_*...
#include <unistd.h>     //close
#include <string>       //std::string
#include <semaphore.h>  //sem_* family, SEM_VALUE_MAX
#include <sys/stat.h>   //mode_t, S_IRWXG, S_IRWXO, S_IRWXU,
#include <boost/assert.hpp>

#ifdef SEM_FAILED
#define BOOST_INTERPROCESS_POSIX_SEM_FAILED (reinterpret_cast<sem_t*>(SEM_FAILED))
#else
#define BOOST_INTERPROCESS_POSIX_SEM_FAILED (reinterpret_cast<sem_t*>(-1))
#endif

#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
#include <boost/interprocess/sync/posix/timepoint_to_timespec.hpp>
#else
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/detail/locks.hpp>
#include <boost/interprocess/sync/detail/common_algorithms.hpp>
#endif

namespace boost {
namespace interprocess {
namespace ipcdetail {

#ifdef BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES

inline bool semaphore_open
   (sem_t *&handle, create_enum_t type, const char *origname,
    unsigned int count = 0, const permissions &perm = permissions())
{
   std::string name;
   #ifndef BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SEMAPHORES
   add_leading_slash(origname, name);
   #else
   create_shared_dir_cleaning_old_and_get_filepath(origname, name);
   #endif

   //Create new mapping
   int oflag = 0;
   switch(type){
      case DoOpen:
      {
         //No addition
         handle = ::sem_open(name.c_str(), oflag);
      }
      break;
      case DoOpenOrCreate:
      case DoCreate:
      {
         while(1){
            oflag = (O_CREAT | O_EXCL);
            handle = ::sem_open(name.c_str(), oflag, perm.get_permissions(), count);
            if(handle != BOOST_INTERPROCESS_POSIX_SEM_FAILED){
               //We can't change semaphore permissions!
               //::fchmod(handle, perm.get_permissions());
               break;
            }
            else if(errno == EEXIST && type == DoOpenOrCreate){
               oflag = 0;
               if( (handle = ::sem_open(name.c_str(), oflag)) != BOOST_INTERPROCESS_POSIX_SEM_FAILED
                   || (errno != ENOENT) ){
                  break;
               }
            }
            else{
               break;
            }
         }
      }
      break;
      default:
      {
         error_info err(other_error);
         throw interprocess_exception(err);
      }
   }

   //Check for error
   if(handle == BOOST_INTERPROCESS_POSIX_SEM_FAILED){
      throw interprocess_exception(error_info(errno));
   }

   return true;
}

inline void semaphore_close(sem_t *handle)
{
   int ret = sem_close(handle);
   if(ret != 0){
      BOOST_ASSERT(0);
   }
}

inline bool semaphore_unlink(const char *semname)
{
   try{
      std::string sem_str;
      #ifndef BOOST_INTERPROCESS_FILESYSTEM_BASED_POSIX_SEMAPHORES
      add_leading_slash(semname, sem_str);
      #else
      shared_filepath(semname, sem_str);
      #endif
      return 0 == sem_unlink(sem_str.c_str());
   }
   catch(...){
      return false;
   }
}

#endif   //BOOST_INTERPROCESS_POSIX_NAMED_SEMAPHORES

#ifdef BOOST_INTERPROCESS_POSIX_UNNAMED_SEMAPHORES

inline void semaphore_init(sem_t *handle, unsigned int initialCount)
{
   int ret = sem_init(handle, 1, initialCount);
   //According to SUSV3 version 2003 edition, the return value of a successful
   //sem_init call is not defined, but -1 is returned on failure.
   //In the future, a successful call might be required to return 0.
   if(ret == -1){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

inline void semaphore_destroy(sem_t *handle)
{
   int ret = sem_destroy(handle);
   if(ret != 0){
      BOOST_ASSERT(0);
   }
}

#endif   //BOOST_INTERPROCESS_POSIX_UNNAMED_SEMAPHORES

inline void semaphore_post(sem_t *handle)
{
   int ret = sem_post(handle);
   if(ret != 0){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

inline void semaphore_wait(sem_t *handle)
{
   int ret = sem_wait(handle);
   if(ret != 0){
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
}

inline bool semaphore_try_wait(sem_t *handle)
{
   int res = sem_trywait(handle);
   if(res == 0)
      return true;
   if(system_error_code() == EAGAIN){
      return false;
   }
   error_info err = system_error_code();
   throw interprocess_exception(err);
}

#ifndef BOOST_INTERPROCESS_POSIX_TIMEOUTS

struct semaphore_wrapper_try_wrapper
{
   explicit semaphore_wrapper_try_wrapper(sem_t *handle)
      : m_handle(handle)
   {}

   void wait()
   {  semaphore_wait(m_handle);  }

   bool try_wait()
   {  return semaphore_try_wait(m_handle);  }

   private:
   sem_t *m_handle;
};

#endif

template<class TimePoint>
inline bool semaphore_timed_wait(sem_t *handle, const TimePoint &abs_time)
{
   #ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
   //Posix does not support infinity absolute time so handle it here
   if(ipcdetail::is_pos_infinity(abs_time)){
      semaphore_wait(handle);
      return true;
   }

   timespec tspec = timepoint_to_timespec(abs_time);
   for (;;){
      int res = sem_timedwait(handle, &tspec);
      if(res == 0)
         return true;
      if (res > 0){
         //buggy glibc, copy the returned error code to errno
         errno = res;
      }
      if(system_error_code() == ETIMEDOUT){
         return false;
      }
      error_info err = system_error_code();
      throw interprocess_exception(err);
   }
   return false;
   #else //#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS

   semaphore_wrapper_try_wrapper swtw(handle);
   ipcdetail::lock_to_wait<semaphore_wrapper_try_wrapper> lw(swtw);
   return ipcdetail::try_based_timed_lock(lw, abs_time);

   #endif   //#ifdef BOOST_INTERPROCESS_POSIX_TIMEOUTS
}

}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#endif   //#ifndef BOOST_INTERPROCESS_POSIX_SEMAPHORE_WRAPPER_HPP
