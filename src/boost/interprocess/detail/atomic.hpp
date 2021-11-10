//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012
// (C) Copyright Markus Schoepflin 2007
// (C) Copyright Bryce Lelbach 2010
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
#define BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP

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

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically increment an boost::uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem);

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem);

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val);

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp);

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#if defined (BOOST_INTERPROCESS_WINDOWS)

#include <boost/interprocess/detail/win32_api.hpp>

#if defined( _MSC_VER )
   extern "C" void _ReadWriteBarrier(void);
   #pragma intrinsic(_ReadWriteBarrier)

#define BOOST_INTERPROCESS_READ_WRITE_BARRIER \
            BOOST_INTERPROCESS_DISABLE_DEPRECATED_WARNING \
            _ReadWriteBarrier() \
            BOOST_INTERPROCESS_RESTORE_WARNING

#elif defined(__GNUC__)
   #if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
      #define BOOST_INTERPROCESS_READ_WRITE_BARRIER __sync_synchronize()
   #else
      #define BOOST_INTERPROCESS_READ_WRITE_BARRIER __asm__ __volatile__("" : : : "memory")
   #endif
#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return winapi::interlocked_decrement(reinterpret_cast<volatile long*>(mem)) + 1;  }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return winapi::interlocked_increment(reinterpret_cast<volatile long*>(mem))-1;  }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
    const boost::uint32_t val = *mem;
    BOOST_INTERPROCESS_READ_WRITE_BARRIER;
    return val;
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  winapi::interlocked_exchange(reinterpret_cast<volatile long*>(mem), val);  }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with": what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return winapi::interlocked_compare_exchange(reinterpret_cast<volatile long*>(mem), with, cmp);  }

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)) && !defined(_CRAYC)

namespace boost {
namespace interprocess {
namespace ipcdetail{

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   boost::uint32_t prev = cmp;
   // This version by Mans Rullgard of Pathscale
   __asm__ __volatile__ ( "lock\n\t"
                          "cmpxchg %2,%0"
                        : "+m"(*mem), "+a"(prev)
                        : "r"(with)
                        : "cc");

   return prev;
}

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{
   // int r = *pw;
   // *mem += val;
   // return r;
   int r;

   asm volatile
   (
      "lock\n\t"
      "xadd %1, %0":
      "+m"( *mem ), "=r"( r ): // outputs (%0, %1)
      "1"( val ): // inputs (%2 == %1)
      "memory", "cc" // clobbers
   );

   return r;
}

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, (boost::uint32_t)-1);  }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
   const boost::uint32_t val = *mem;
   __asm__ __volatile__ ( "" ::: "memory" );
   return val;
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   __asm__ __volatile__
   (
      "xchgl %0, %1"
      : "+r" (val), "+m" (*mem)
      :: "memory"
   );
}

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__GNUC__) && (defined(__PPC__) || defined(__ppc__))

namespace boost {
namespace interprocess {
namespace ipcdetail{

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t prev, temp;

   asm volatile ("1:\n\t"
                 "lwarx  %0,0,%2\n\t"
                 "add    %1,%0,%3\n\t"
                 "stwcx. %1,0,%2\n\t"
                 "bne-   1b"
                 : "=&r" (prev), "=&r" (temp)
                 : "b" (mem), "r" (val)
                 : "cc", "memory");
   return prev;
}

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   boost::uint32_t prev;

   asm volatile ("1:\n\t"
                 "lwarx  %0,0,%1\n\t"
                 "cmpw   %0,%3\n\t"
                 "bne-   2f\n\t"
                 "stwcx. %2,0,%1\n\t"
                 "bne-   1b\n\t"
                 "2:"
                 : "=&r"(prev)
                 : "b" (mem), "r" (with), "r" (cmp)
                 : "cc", "memory");
   return prev;
}

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, boost::uint32_t(-1u));  }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{
   const boost::uint32_t val = *mem;
   __asm__ __volatile__ ( "" ::: "memory" );
   return val;
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  *mem = val; }

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif (defined(sun) || defined(__sun))

#include <atomic.h>

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32(volatile boost::uint32_t *mem, boost::uint32_t val)
{   return atomic_add_32_nv(reinterpret_cast<volatile ::uint32_t*>(mem), (int32_t)val) - val;   }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return atomic_cas_32(reinterpret_cast<volatile ::uint32_t*>(mem), cmp, with);  }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add_32_nv(reinterpret_cast<volatile ::uint32_t*>(mem), 1) - 1; }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_add_32_nv(reinterpret_cast<volatile ::uint32_t*>(mem), (boost::uint32_t)-1) + 1; }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  *mem = val; }

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__osf__) && defined(__DECCXX)

#include <machine/builtins.h>
#include <c_asm.h>

namespace boost{
namespace interprocess{
namespace ipcdetail{

//! Atomically decrement a uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
//! Acquire, memory barrier after decrement.
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  boost::uint32_t old_val = __ATOMIC_DECREMENT_LONG(mem); __MB(); return old_val; }

//! Atomically increment a uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
//! Release, memory barrier before increment.
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  __MB(); return __ATOMIC_INCREMENT_LONG(mem); }

// Rational for the implementation of the atomic read and write functions.
//
// 1. The Alpha Architecture Handbook requires that access to a byte,
// an aligned word, an aligned longword, or an aligned quadword is
// atomic. (See 'Alpha Architecture Handbook', version 4, chapter 5.2.2.)
//
// 2. The CXX User's Guide states that volatile quantities are accessed
// with single assembler instructions, and that a compilation error
// occurs when declaring a quantity as volatile which is not properly
// aligned.

//! Atomically read an boost::uint32_t from memory
//! Acquire, memory barrier after load.
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  boost::uint32_t old_val = *mem; __MB(); return old_val;  }

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
//! Release, memory barrier before store.
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  __MB(); *mem = val; }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
//! Memory barrier between load and store.
inline boost::uint32_t atomic_cas32(
  volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
  // Note:
  //
  // Branch prediction prefers backward branches, and the Alpha Architecture
  // Handbook explicitely states that the loop should not be implemented like
  // it is below. (See chapter 4.2.5.) Therefore the code should probably look
  // like this:
  //
  // return asm(
  //   "10: ldl_l %v0,(%a0) ;"
  //   "    cmpeq %v0,%a2,%t0 ;"
  //   "    beq %t0,20f ;"
  //   "    mb ;"
  //   "    mov %a1,%t0 ;"
  //   "    stl_c %t0,(%a0) ;"
  //   "    beq %t0,30f ;"
  //   "20: ret ;"
  //   "30: br 10b;",
  //   mem, with, cmp);
  //
  // But as the compiler always transforms this into the form where a backward
  // branch is taken on failure, we can as well implement it in the straight
  // forward form, as this is what it will end up in anyway.

  return asm(
    "10: ldl_l %v0,(%a0) ;"    // load prev value from mem and lock mem
    "    cmpeq %v0,%a2,%t0 ;"  // compare with given value
    "    beq %t0,20f ;"        // if not equal, we're done
    "    mb ;"                 // memory barrier
    "    mov %a1,%t0 ;"        // load new value into scratch register
    "    stl_c %t0,(%a0) ;"    // store new value to locked mem (overwriting scratch)
    "    beq %t0,10b ;"        // store failed because lock has been stolen, retry
    "20: ",
    mem, with, cmp);
}

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#elif defined(__IBMCPP__) && (__IBMCPP__ >= 800) && defined(_AIX)

#include <builtins.h>

namespace boost {
namespace interprocess {
namespace ipcdetail{

//first define boost::uint32_t versions of __lwarx and __stwcx to avoid poluting
//all the functions with casts

//! From XLC documenation :
//! This function can be used with a subsequent stwcxu call to implement a
//! read-modify-write on a specified memory location. The two functions work
//! together to ensure that if the store is successfully performed, no other
//! processor or mechanism can modify the target doubleword between the time
//! lwarxu function is executed and the time the stwcxu functio ncompletes.
//! "mem" : pointer to the object
//! Returns the value at pointed to by mem
inline boost::uint32_t lwarxu(volatile boost::uint32_t *mem)
{
   return static_cast<boost::uint32_t>(__lwarx(reinterpret_cast<volatile int*>(mem)));
}

//! "mem" : pointer to the object
//! "val" : the value to store
//! Returns true if the update of mem is successful and false if it is
//!unsuccessful
inline bool stwcxu(volatile boost::uint32_t* mem, boost::uint32_t val)
{
   return (__stwcx(reinterpret_cast<volatile int*>(mem), static_cast<int>(val)) != 0);
}

//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{
   boost::uint32_t oldValue;
   do
   {
      oldValue = lwarxu(mem);
   }while (!stwcxu(mem, oldValue+val));
   return oldValue;
}

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, (boost::uint32_t)-1);   }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return *mem;   }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{
   boost::uint32_t oldValue;
   boost::uint32_t valueToStore;
   do
   {
      oldValue = lwarxu(mem);
   } while (!stwcxu(mem, (oldValue == with) ? cmp : oldValue));

   return oldValue;
}

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  *mem = val; }

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#elif defined(__GNUC__) && ( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 )

namespace boost {
namespace interprocess {
namespace ipcdetail{

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{  return __sync_fetch_and_add(const_cast<boost::uint32_t *>(mem), val);   }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, 1);  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return atomic_add32(mem, (boost::uint32_t)-1);   }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  boost::uint32_t old_val = *mem; __sync_synchronize(); return old_val;  }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return __sync_val_compare_and_swap(const_cast<boost::uint32_t *>(mem), cmp, with);   }

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  __sync_synchronize(); *mem = val;  }

}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{
#elif defined(__VXWORKS__)

#include <vxAtomicLib.h>
// VxWorks atomic32_t is not volatile, for some unknown reason
#define vx_atomic_cast(_i)   (reinterpret_cast< ::atomic32_t *>( const_cast<boost::uint32_t *>(_i)))

namespace boost {
namespace interprocess {
namespace ipcdetail{

//! Atomically add 'val' to an boost::uint32_t
//! "mem": pointer to the object
//! "val": amount to add
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_add32
   (volatile boost::uint32_t *mem, boost::uint32_t val)
{  return ::vxAtomic32Add( vx_atomic_cast(mem), val);   }

//! Atomically increment an apr_uint32_t by 1
//! "mem": pointer to the object
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_inc32(volatile boost::uint32_t *mem)
{  return ::vxAtomic32Inc( vx_atomic_cast(mem) );  }

//! Atomically decrement an boost::uint32_t by 1
//! "mem": pointer to the atomic value
//! Returns the old value pointed to by mem
inline boost::uint32_t atomic_dec32(volatile boost::uint32_t *mem)
{  return ::vxAtomic32Dec( vx_atomic_cast(mem) );   }

//! Atomically read an boost::uint32_t from memory
inline boost::uint32_t atomic_read32(volatile boost::uint32_t *mem)
{  return ::vxAtomic32Get( vx_atomic_cast(mem) );  }

//! Compare an boost::uint32_t's value with "cmp".
//! If they are the same swap the value with "with"
//! "mem": pointer to the value
//! "with" what to swap it with
//! "cmp": the value to compare it to
//! Returns the old value of *mem
inline boost::uint32_t atomic_cas32
   (volatile boost::uint32_t *mem, boost::uint32_t with, boost::uint32_t cmp)
{  return ::vxAtomic32Cas( vx_atomic_cast(mem), cmp, with);  }

//! Atomically set an boost::uint32_t in memory
//! "mem": pointer to the object
//! "param": val value that the object will assume
inline void atomic_write32(volatile boost::uint32_t *mem, boost::uint32_t val)
{  ::vxAtomic32Set( vx_atomic_cast(mem), val);  }


}  //namespace ipcdetail{
}  //namespace interprocess{
}  //namespace boost{

#else

#error No atomic operations implemented for this platform, sorry!

#endif

namespace boost{
namespace interprocess{
namespace ipcdetail{

inline bool atomic_add_unless32
   (volatile boost::uint32_t *mem, boost::uint32_t value, boost::uint32_t unless_this)
{
   boost::uint32_t old, c(atomic_read32(mem));
   while(c != unless_this && (old = atomic_cas32(mem, c + value, c)) != c){
      c = old;
   }
   return c != unless_this;
}

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost


#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_ATOMIC_HPP
