//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//Thread launching functions are adapted from boost/detail/lightweight_thread.hpp
//
//  boost/detail/lightweight_thread.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_INTERPROCESS_DETAIL_OS_THREAD_FUNCTIONS_HPP
#define BOOST_INTERPROCESS_DETAIL_OS_THREAD_FUNCTIONS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <cstddef>
#include <ostream>

#if defined(BOOST_INTERPROCESS_WINDOWS)
#  include <boost/interprocess/detail/win32_api.hpp>
#  include <boost/winapi/thread.hpp>
#else
#  include <pthread.h>
#  include <unistd.h>
#  include <sched.h>
#  include <time.h>
#  ifdef BOOST_INTERPROCESS_BSD_DERIVATIVE
      //Some *BSD systems (OpenBSD & NetBSD) need sys/param.h before sys/sysctl.h, whereas
      //others (FreeBSD & Darwin) need sys/types.h
#     include <sys/types.h>
#     include <sys/param.h>
#     include <sys/sysctl.h>
#  endif
#if defined(__VXWORKS__) 
#include <vxCpuLib.h>
#endif 
//According to the article "C/C++ tip: How to measure elapsed real time for benchmarking"
//Check MacOs first as macOS 10.12 SDK defines both CLOCK_MONOTONIC and
//CLOCK_MONOTONIC_RAW and no clock_gettime.
#  if (defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__))
#     include <mach/mach_time.h>  // mach_absolute_time, mach_timebase_info_data_t
#     define BOOST_INTERPROCESS_MATCH_ABSOLUTE_TIME
#  elif defined(CLOCK_MONOTONIC_PRECISE)   //BSD
#     define BOOST_INTERPROCESS_CLOCK_MONOTONIC CLOCK_MONOTONIC_PRECISE
#  elif defined(CLOCK_MONOTONIC_RAW)     //Linux
#     define BOOST_INTERPROCESS_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW
#  elif defined(CLOCK_HIGHRES)           //Solaris
#     define BOOST_INTERPROCESS_CLOCK_MONOTONIC CLOCK_HIGHRES
#  elif defined(CLOCK_MONOTONIC)         //POSIX (AIX, BSD, Linux, Solaris)
#     define BOOST_INTERPROCESS_CLOCK_MONOTONIC CLOCK_MONOTONIC
#  else
#     error "No high resolution steady clock in your system, please provide a patch"
#  endif
#endif

namespace boost {
namespace interprocess {
namespace ipcdetail{

#if defined (BOOST_INTERPROCESS_WINDOWS)

typedef unsigned long OS_process_id_t;
typedef unsigned long OS_thread_id_t;
struct OS_thread_t
{
   OS_thread_t()
      : m_handle()
   {}

   
   void* handle() const
   {  return m_handle;  }

   void* m_handle;
};

typedef OS_thread_id_t OS_systemwide_thread_id_t;

//process
inline OS_process_id_t get_current_process_id()
{  return winapi::get_current_process_id();  }

inline OS_process_id_t get_invalid_process_id()
{  return OS_process_id_t(0);  }

//thread
inline OS_thread_id_t get_current_thread_id()
{  return winapi::get_current_thread_id();  }

inline OS_thread_id_t get_invalid_thread_id()
{  return OS_thread_id_t(0xffffffff);  }

inline bool equal_thread_id(OS_thread_id_t id1, OS_thread_id_t id2)
{  return id1 == id2;  }

//return the system tick in ns
inline unsigned long get_system_tick_ns()
{
   unsigned long curres, ignore1, ignore2;
   winapi::query_timer_resolution(&ignore1, &ignore2, &curres);
   //Windows API returns the value in hundreds of ns
   return (curres - 1ul)*100ul;
}

//return the system tick in us
inline unsigned long get_system_tick_us()
{
   unsigned long curres, ignore1, ignore2;
   winapi::query_timer_resolution(&ignore1, &ignore2, &curres);
   //Windows API returns the value in hundreds of ns
   return (curres - 1ul)/10ul + 1ul;
}

typedef unsigned __int64 OS_highres_count_t;

inline unsigned long get_system_tick_in_highres_counts()
{
   __int64 freq;
   unsigned long curres, ignore1, ignore2;
   winapi::query_timer_resolution(&ignore1, &ignore2, &curres);
   //Frequency in counts per second
   if(!winapi::query_performance_frequency(&freq)){
      //Tick resolution in ms
      return (curres-1ul)/10000ul + 1ul;
   }
   else{
      //In femtoseconds
      __int64 count_fs    = (1000000000000000LL - 1LL)/freq + 1LL;
      __int64 tick_counts = (static_cast<__int64>(curres)*100000000LL - 1LL)/count_fs + 1LL;
      return static_cast<unsigned long>(tick_counts);
   }
}

inline OS_highres_count_t get_current_system_highres_count()
{
   __int64 count;
   if(!winapi::query_performance_counter(&count)){
      count = winapi::get_tick_count();
   }
   return count;
}

inline void zero_highres_count(OS_highres_count_t &count)
{  count = 0;  }

inline bool is_highres_count_zero(const OS_highres_count_t &count)
{  return count == 0;  }

template <class Ostream>
inline Ostream &ostream_highres_count(Ostream &ostream, const OS_highres_count_t &count)
{
   ostream << count;
   return ostream;
}

inline OS_highres_count_t system_highres_count_subtract(const OS_highres_count_t &l, const OS_highres_count_t &r)
{  return l - r;  }

inline bool system_highres_count_less(const OS_highres_count_t &l, const OS_highres_count_t &r)
{  return l < r;  }

inline bool system_highres_count_less_ul(const OS_highres_count_t &l, unsigned long r)
{  return l < static_cast<OS_highres_count_t>(r);  }

inline void thread_sleep_tick()
{  winapi::sleep_tick();   }

inline void thread_yield()
{  winapi::sched_yield();  }

inline void thread_sleep(unsigned int ms)
{  winapi::sleep(ms);  }

//systemwide thread
inline OS_systemwide_thread_id_t get_current_systemwide_thread_id()
{
   return get_current_thread_id();
}

inline void systemwide_thread_id_copy
   (const volatile OS_systemwide_thread_id_t &from, volatile OS_systemwide_thread_id_t &to)
{
   to = from;
}

inline bool equal_systemwide_thread_id(const OS_systemwide_thread_id_t &id1, const OS_systemwide_thread_id_t &id2)
{
   return equal_thread_id(id1, id2);
}

inline OS_systemwide_thread_id_t get_invalid_systemwide_thread_id()
{
   return get_invalid_thread_id();
}

inline long double get_current_process_creation_time()
{
   winapi::interprocess_filetime CreationTime, ExitTime, KernelTime, UserTime;

   winapi::get_process_times
      ( winapi::get_current_process(), &CreationTime, &ExitTime, &KernelTime, &UserTime);

   typedef long double ldouble_t;
   const ldouble_t resolution = (100.0l/1000000000.0l);
   return CreationTime.dwHighDateTime*(ldouble_t(1u<<31u)*2.0l*resolution) +
              CreationTime.dwLowDateTime*resolution;
}

inline unsigned int get_num_cores()
{
   winapi::interprocess_system_info sysinfo;
   winapi::get_system_info( &sysinfo );
   //in Windows dw is long which is equal in bits to int
   return static_cast<unsigned>(sysinfo.dwNumberOfProcessors);
}

#else    //#if defined (BOOST_INTERPROCESS_WINDOWS)

typedef pthread_t OS_thread_t;
typedef pthread_t OS_thread_id_t;
typedef pid_t     OS_process_id_t;

struct OS_systemwide_thread_id_t
{
   OS_systemwide_thread_id_t()
      :  pid(), tid()
   {}

   OS_systemwide_thread_id_t(pid_t p, pthread_t t)
      :  pid(p), tid(t)
   {}

   OS_systemwide_thread_id_t(const OS_systemwide_thread_id_t &x)
      :  pid(x.pid), tid(x.tid)
   {}

   OS_systemwide_thread_id_t(const volatile OS_systemwide_thread_id_t &x)
      :  pid(x.pid), tid(x.tid)
   {}

   OS_systemwide_thread_id_t & operator=(const OS_systemwide_thread_id_t &x)
   {  pid = x.pid;   tid = x.tid;   return *this;   }

   OS_systemwide_thread_id_t & operator=(const volatile OS_systemwide_thread_id_t &x)
   {  pid = x.pid;   tid = x.tid;   return *this;  }

   void operator=(const OS_systemwide_thread_id_t &x) volatile
   {  pid = x.pid;   tid = x.tid;   }

   pid_t       pid;
   pthread_t   tid;
};

inline void systemwide_thread_id_copy
   (const volatile OS_systemwide_thread_id_t &from, volatile OS_systemwide_thread_id_t &to)
{
   to.pid = from.pid;
   to.tid = from.tid;
}

//process
inline OS_process_id_t get_current_process_id()
{  return ::getpid();  }

inline OS_process_id_t get_invalid_process_id()
{  return pid_t(0);  }

//thread
inline OS_thread_id_t get_current_thread_id()
{  return ::pthread_self();  }

inline OS_thread_id_t get_invalid_thread_id()
{
   static pthread_t invalid_id;
   return invalid_id;
}

inline bool equal_thread_id(OS_thread_id_t id1, OS_thread_id_t id2)
{  return 0 != pthread_equal(id1, id2);  }

inline void thread_yield()
{  ::sched_yield();  }

#ifndef BOOST_INTERPROCESS_MATCH_ABSOLUTE_TIME
typedef struct timespec OS_highres_count_t;
#else
typedef unsigned long long OS_highres_count_t;
#endif

inline unsigned long get_system_tick_ns()
{
   #ifdef _SC_CLK_TCK
   long ticks_per_second =::sysconf(_SC_CLK_TCK); // ticks per sec
   if(ticks_per_second <= 0){   //Try a typical value on error
      ticks_per_second = 100;
   }
   return 999999999ul/static_cast<unsigned long>(ticks_per_second)+1ul;
   #else
      #error "Can't obtain system tick value for your system, please provide a patch"
   #endif
}

inline unsigned long get_system_tick_in_highres_counts()
{
   #ifndef BOOST_INTERPROCESS_MATCH_ABSOLUTE_TIME
   return get_system_tick_ns();
   #else
   mach_timebase_info_data_t info;
   mach_timebase_info(&info);
            //ns
   return static_cast<unsigned long>
   (
      static_cast<double>(get_system_tick_ns())
         / (static_cast<double>(info.numer) / info.denom)
   );
   #endif
}

//return system ticks in us
inline unsigned long get_system_tick_us()
{
   return (get_system_tick_ns()-1)/1000ul + 1ul;
}

inline OS_highres_count_t get_current_system_highres_count()
{
   #if defined(BOOST_INTERPROCESS_CLOCK_MONOTONIC)
      struct timespec count;
      ::clock_gettime(BOOST_INTERPROCESS_CLOCK_MONOTONIC, &count);
      return count;
   #elif defined(BOOST_INTERPROCESS_MATCH_ABSOLUTE_TIME)
      return ::mach_absolute_time();
   #endif
}

#ifndef BOOST_INTERPROCESS_MATCH_ABSOLUTE_TIME

inline void zero_highres_count(OS_highres_count_t &count)
{  count.tv_sec = 0; count.tv_nsec = 0;  }

inline bool is_highres_count_zero(const OS_highres_count_t &count)
{  return count.tv_sec == 0 && count.tv_nsec == 0;  }

template <class Ostream>
inline Ostream &ostream_highres_count(Ostream &ostream, const OS_highres_count_t &count)
{
   ostream << count.tv_sec << "s:" << count.tv_nsec << "ns";
   return ostream;
}

inline OS_highres_count_t system_highres_count_subtract(const OS_highres_count_t &l, const OS_highres_count_t &r)
{
   OS_highres_count_t res;

   if (l.tv_nsec < r.tv_nsec){
      res.tv_nsec = 1000000000 + l.tv_nsec - r.tv_nsec;
      res.tv_sec  = l.tv_sec - 1 - r.tv_sec;
   }
   else{
      res.tv_nsec = l.tv_nsec - r.tv_nsec;
      res.tv_sec  = l.tv_sec - r.tv_sec;
   }

   return res;
}

inline bool system_highres_count_less(const OS_highres_count_t &l, const OS_highres_count_t &r)
{  return l.tv_sec < r.tv_sec || (l.tv_sec == r.tv_sec && l.tv_nsec < r.tv_nsec);  }

inline bool system_highres_count_less_ul(const OS_highres_count_t &l, unsigned long r)
{  return !l.tv_sec && (static_cast<unsigned long>(l.tv_nsec) < r);  }

#else

inline void zero_highres_count(OS_highres_count_t &count)
{  count = 0;  }

inline bool is_highres_count_zero(const OS_highres_count_t &count)
{  return count == 0;  }

template <class Ostream>
inline Ostream &ostream_highres_count(Ostream &ostream, const OS_highres_count_t &count)
{
   ostream << count ;
   return ostream;
}

inline OS_highres_count_t system_highres_count_subtract(const OS_highres_count_t &l, const OS_highres_count_t &r)
{  return l - r;  }

inline bool system_highres_count_less(const OS_highres_count_t &l, const OS_highres_count_t &r)
{  return l < r;  }

inline bool system_highres_count_less_ul(const OS_highres_count_t &l, unsigned long r)
{  return l < static_cast<OS_highres_count_t>(r);  }

#endif

inline void thread_sleep_tick()
{
   struct timespec rqt;
   //Sleep for the half of the tick time
   rqt.tv_sec  = 0;
   rqt.tv_nsec = get_system_tick_ns()/2;
   ::nanosleep(&rqt, 0);
}

inline void thread_sleep(unsigned int ms)
{
   struct timespec rqt;
   rqt.tv_sec = ms/1000u;
   rqt.tv_nsec = (ms%1000u)*1000000u;
   ::nanosleep(&rqt, 0);
}

//systemwide thread
inline OS_systemwide_thread_id_t get_current_systemwide_thread_id()
{
   return OS_systemwide_thread_id_t(::getpid(), ::pthread_self());
}

inline bool equal_systemwide_thread_id(const OS_systemwide_thread_id_t &id1, const OS_systemwide_thread_id_t &id2)
{
   return (0 != pthread_equal(id1.tid, id2.tid)) && (id1.pid == id2.pid);
}

inline OS_systemwide_thread_id_t get_invalid_systemwide_thread_id()
{
   return OS_systemwide_thread_id_t(get_invalid_process_id(), get_invalid_thread_id());
}

inline long double get_current_process_creation_time()
{ return 0.0L; }

inline unsigned int get_num_cores()
{
   #ifdef _SC_NPROCESSORS_ONLN
      long cores = ::sysconf(_SC_NPROCESSORS_ONLN);
      // sysconf returns -1 if the name is invalid, the option does not exist or
      // does not have a definite limit.
      // if sysconf returns some other negative number, we have no idea
      // what is going on. Default to something safe.
      if(cores <= 0){
         return 1;
      }
      //Check for overflow (unlikely)
      else if(static_cast<unsigned long>(cores) >=
              static_cast<unsigned long>(static_cast<unsigned int>(-1))){
         return static_cast<unsigned int>(-1);
      }
      else{
         return static_cast<unsigned int>(cores);
      }
   #elif defined(BOOST_INTERPROCESS_BSD_DERIVATIVE) && defined(HW_NCPU)
      int request[2] = { CTL_HW, HW_NCPU };
      int num_cores;
      std::size_t result_len = sizeof(num_cores);
      if ( (::sysctl (request, 2, &num_cores, &result_len, 0, 0) < 0) || (num_cores <= 0) ){
         //Return a safe value
         return 1;
      }
      else{
         return static_cast<unsigned int>(num_cores);
      }
   #elif defined(__VXWORKS__)
      cpuset_t set =  ::vxCpuEnabledGet();
    #ifdef __DCC__
      int i;
      for( i = 0; set; ++i)
          {
               set &= set -1;
          }
      return(i);
    #else  
      return (__builtin_popcount(set) );
    #endif  
   #endif
}

inline int thread_create(OS_thread_t * thread, void *(*start_routine)(void*), void* arg)
{  return pthread_create(thread, 0, start_routine, arg); }

inline void thread_join(OS_thread_t thread)
{  (void)pthread_join(thread, 0);  }

#endif   //#if defined (BOOST_INTERPROCESS_WINDOWS)

typedef char pid_str_t[sizeof(OS_process_id_t)*3+1];

inline void get_pid_str(pid_str_t &pid_str, OS_process_id_t pid)
{
   bufferstream bstream(pid_str, sizeof(pid_str));
   bstream << pid << std::ends;
}

inline void get_pid_str(pid_str_t &pid_str)
{  get_pid_str(pid_str, get_current_process_id());  }

#if defined(BOOST_INTERPROCESS_WINDOWS)

inline int thread_create( OS_thread_t * thread, boost::ipwinapiext::LPTHREAD_START_ROUTINE_ start_routine, void* arg )
{
   void* h = boost::ipwinapiext::CreateThread(0, 0, start_routine, arg, 0, 0);

   if( h != 0 ){
      thread->m_handle = h;
      return 0;
   }
   else{
      return 1;
   }
}

inline void thread_join( OS_thread_t thread)
{
   winapi::wait_for_single_object( thread.handle(), winapi::infinite_time );
   winapi::close_handle( thread.handle() );
}

#endif

class abstract_thread
{
   public:
   virtual ~abstract_thread() {}
   virtual void run() = 0;
};

template<class T>
class os_thread_func_ptr_deleter
{
   public:
   explicit os_thread_func_ptr_deleter(T* p)
      : m_p(p)
   {}

   T *release()
   {  T *p = m_p; m_p = 0; return p;  }

   T *get() const
   {  return m_p;  }

   T *operator ->() const
   {  return m_p;  }

   ~os_thread_func_ptr_deleter()
   {  delete m_p; }

   private:
   T *m_p;
};

#if defined(BOOST_INTERPROCESS_WINDOWS)

inline boost::winapi::DWORD_ __stdcall launch_thread_routine(boost::winapi::LPVOID_ pv)
{
   os_thread_func_ptr_deleter<abstract_thread> pt( static_cast<abstract_thread *>( pv ) );
   pt->run();
   return 0;
}

#else

extern "C" void * launch_thread_routine( void * pv );

inline void * launch_thread_routine( void * pv )
{
   os_thread_func_ptr_deleter<abstract_thread> pt( static_cast<abstract_thread *>( pv ) );
   pt->run();
   return 0;
}

#endif

template<class F>
class launch_thread_impl
   : public abstract_thread
{
   public:
   explicit launch_thread_impl( F f )
      : f_( f )
   {}

   void run()
   {  f_();  }

   private:
   F f_;
};

template<class F>
inline int thread_launch( OS_thread_t & pt, F f )
{
   os_thread_func_ptr_deleter<abstract_thread> p( new launch_thread_impl<F>( f ) );

   int r = thread_create(&pt, launch_thread_routine, p.get());
   if( r == 0 ){
      p.release();
   }

   return r;
}

}  //namespace ipcdetail{
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_OS_THREAD_FUNCTIONS_HPP
