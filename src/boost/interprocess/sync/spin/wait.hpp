//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Peter Dimov 2008.
// (C) Copyright Ion Gaztanaga 2013-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//Parts of this file come from boost/smart_ptr/detail/yield_k.hpp
//Many thanks to Peter Dimov.

#ifndef BOOST_INTERPROCESS_SYNC_WAIT_HPP_INCLUDED
#define BOOST_INTERPROCESS_SYNC_WAIT_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
# pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>

//#define BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
#ifdef BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
#include <iostream>
#endif

// BOOST_INTERPROCESS_SMT_PAUSE

#if defined(_MSC_VER) && ( defined(_M_IX86) || defined(_M_X64) )

extern "C" void _mm_pause();
#pragma intrinsic( _mm_pause )

#define BOOST_INTERPROCESS_SMT_PAUSE _mm_pause();

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) ) && !defined(_CRAYC)

#define BOOST_INTERPROCESS_SMT_PAUSE __asm__ __volatile__( "rep; nop" : : : "memory" );

#endif

namespace boost{
namespace interprocess{
namespace ipcdetail {

template<int Dummy = 0>
class num_core_holder
{
   public:
   static unsigned int get()
   {
      if(!num_cores){
         return ipcdetail::get_num_cores();
      }
      else{
         return num_cores;
      }
   }

   private:
   static unsigned int num_cores;
};

template<int Dummy>
unsigned int num_core_holder<Dummy>::num_cores = ipcdetail::get_num_cores();

}  //namespace ipcdetail {

class spin_wait
{
   public:

   static const unsigned int nop_pause_limit = 32u;
   spin_wait()
      : m_count_start(), m_ul_yield_only_counts(), m_k()
   {}

   #ifdef BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
   ~spin_wait()
   {
      if(m_k){
         std::cout << "final m_k: " << m_k
                   << " system tick(us): " << ipcdetail::get_system_tick_us() << std::endl;
      }
   }
   #endif

   unsigned int count() const
   {  return m_k;  }

   void yield()
   {
      //Lazy initialization of limits
      if( !m_k){
         this->init_limits();
      }
      //Nop tries
      if( m_k < (nop_pause_limit >> 2) ){

      }
      //Pause tries if the processor supports it
      #if defined(BOOST_INTERPROCESS_SMT_PAUSE)
      else if( m_k < nop_pause_limit ){
         BOOST_INTERPROCESS_SMT_PAUSE
      }
      #endif
      //Yield/Sleep strategy
      else{
         //Lazy initialization of tick information
         if(m_k == nop_pause_limit){
            this->init_tick_info();
         }
         else if( this->yield_or_sleep() ){
            ipcdetail::thread_yield();
         }
         else{
            ipcdetail::thread_sleep_tick();
         }
      }
      ++m_k;
   }

   void reset()
   {
      m_k = 0u;
   }

   private:

   void init_limits()
   {
      unsigned int num_cores = ipcdetail::num_core_holder<0>::get();
      m_k = num_cores > 1u ? 0u : nop_pause_limit;
   }

   void init_tick_info()
   {
      m_ul_yield_only_counts = ipcdetail::get_system_tick_in_highres_counts();
      m_count_start = ipcdetail::get_current_system_highres_count();
   }

   //Returns true if yield must be called, false is sleep must be called
   bool yield_or_sleep()
   {
      if(!m_ul_yield_only_counts){  //If yield-only limit was reached then yield one in every two tries
         return (m_k & 1u) != 0;
      }
      else{ //Try to see if we've reched yield-only time limit
         const ipcdetail::OS_highres_count_t now = ipcdetail::get_current_system_highres_count();
         const ipcdetail::OS_highres_count_t elapsed = ipcdetail::system_highres_count_subtract(now, m_count_start);
         if(!ipcdetail::system_highres_count_less_ul(elapsed, m_ul_yield_only_counts)){
            #ifdef BOOST_INTERPROCESS_SPIN_WAIT_DEBUG
            std::cout << "elapsed!\n"
                      << "  m_ul_yield_only_counts: " << m_ul_yield_only_counts
                     << " system tick(us): " << ipcdetail::get_system_tick_us() << '\n'
                      << "  m_k: " << m_k << " elapsed counts: ";
                     ipcdetail::ostream_highres_count(std::cout, elapsed) << std::endl;
            #endif
            //Yield-only time reached, now it's time to sleep
            m_ul_yield_only_counts = 0ul;
            return false;
         }
      }
      return true;   //Otherwise yield
   }

   ipcdetail::OS_highres_count_t m_count_start;
   unsigned long m_ul_yield_only_counts;
   unsigned int  m_k;
};

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // #ifndef BOOST_INTERPROCESS_SYNC_WAIT_HPP_INCLUDED
