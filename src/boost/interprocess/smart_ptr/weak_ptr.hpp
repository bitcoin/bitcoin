//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/weak_ptr.hpp
//
// (C) Copyright Peter Dimov 2001, 2002, 2003
// (C) Copyright Ion Gaztanaga 2006-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WEAK_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_WEAK_PTR_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/smart_ptr/deleter.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/move/adl_move_swap.hpp>

//!\file
//!Describes the smart pointer weak_ptr.

namespace boost{
namespace interprocess{

//!The weak_ptr class template stores a "weak reference" to an object
//!that's already managed by a shared_ptr. To access the object, a weak_ptr
//!can be converted to a shared_ptr using  the shared_ptr constructor or the
//!member function  lock. When the last shared_ptr to the object goes away
//!and the object is deleted, the attempt to obtain a shared_ptr from the
//!weak_ptr instances that refer to the deleted object will fail: the constructor
//!will throw an exception of type bad_weak_ptr, and weak_ptr::lock will
//!return an empty shared_ptr.
//!
//!Every weak_ptr meets the CopyConstructible and Assignable requirements
//!of the C++ Standard Library, and so can be used in standard library containers.
//!Comparison operators are supplied so that weak_ptr works with the standard
//!library's associative containers.
//!
//!weak_ptr operations never throw exceptions.
//!
//!The class template is parameterized on T, the type of the object pointed to.
template<class T, class A, class D>
class weak_ptr
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   // Borland 5.5.1 specific workarounds
   typedef weak_ptr<T, A, D> this_type;
   typedef typename boost::intrusive::
      pointer_traits<typename A::pointer>::template
         rebind_pointer<T>::type                         pointer;
   typedef typename ipcdetail::add_reference
                     <T>::type            reference;
   typedef typename ipcdetail::add_reference
                     <T>::type            const_reference;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   typedef T element_type;
   typedef T value_type;

   //!Effects: Constructs an empty weak_ptr.
   //!Postconditions: use_count() == 0.
   weak_ptr()
   : m_pn() // never throws
   {}
   //  generated copy constructor, assignment, destructor are fine
   //
   //  The "obvious" converting constructor implementation:
   //
   //  template<class Y>
   //  weak_ptr(weak_ptr<Y> const & r): m_px(r.m_px), m_pn(r.m_pn) // never throws
   //  {
   //  }
   //
   //  has a serious problem.
   //
   //  r.m_px may already have been invalidated. The m_px(r.m_px)
   //  conversion may require access to *r.m_px (virtual inheritance).
   //
   //  It is not possible to avoid spurious access violations since
   //  in multithreaded programs r.m_px may be invalidated at any point.

   //!Effects: If r is empty, constructs an empty weak_ptr; otherwise,
   //!constructs a weak_ptr that shares ownership with r as if by storing a
   //!copy of the pointer stored in r.
   //!
   //!Postconditions: use_count() == r.use_count().
   //!
   //!Throws: nothing.
   template<class Y>
   weak_ptr(weak_ptr<Y, A, D> const & r)
      : m_pn(r.m_pn) // never throws
   {
      //Construct a temporary shared_ptr so that nobody
      //can destroy the value while constructing this
      const shared_ptr<T, A, D> &ref = r.lock();
      m_pn.set_pointer(ref.get());
   }

   //!Effects: If r is empty, constructs an empty weak_ptr; otherwise,
   //!constructs a weak_ptr that shares ownership with r as if by storing a
   //!copy of the pointer stored in r.
   //!
   //!Postconditions: use_count() == r.use_count().
   //!
   //!Throws: nothing.
   template<class Y>
   weak_ptr(shared_ptr<Y, A, D> const & r)
      : m_pn(r.m_pn) // never throws
   {}

   //!Effects: Equivalent to weak_ptr(r).swap(*this).
   //!
   //!Throws: nothing.
   //!
   //!Notes: The implementation is free to meet the effects (and the
   //!implied guarantees) via different means, without creating a temporary.
   template<class Y>
   weak_ptr & operator=(weak_ptr<Y, A, D> const & r) // never throws
   {
      //Construct a temporary shared_ptr so that nobody
      //can destroy the value while constructing this
      const shared_ptr<T, A, D> &ref = r.lock();
      m_pn = r.m_pn;
      m_pn.set_pointer(ref.get());
      return *this;
   }

   //!Effects: Equivalent to weak_ptr(r).swap(*this).
   //!
   //!Throws: nothing.
   //!
   //!Notes: The implementation is free to meet the effects (and the
   //!implied guarantees) via different means, without creating a temporary.
   template<class Y>
   weak_ptr & operator=(shared_ptr<Y, A, D> const & r) // never throws
   {  m_pn = r.m_pn;  return *this;  }

   //!Returns: expired()? shared_ptr<T>(): shared_ptr<T>(*this).
   //!
   //!Throws: nothing.
   shared_ptr<T, A, D> lock() const // never throws
   {
      // optimization: avoid throw overhead
      if(expired()){
         return shared_ptr<element_type, A, D>();
      }
      BOOST_TRY{
         return shared_ptr<element_type, A, D>(*this);
      }
      BOOST_CATCH(bad_weak_ptr const &){
         // Q: how can we get here?
         // A: another thread may have invalidated r after the use_count test above.
         return shared_ptr<element_type, A, D>();
      }
      BOOST_CATCH_END
   }

   //!Returns: 0 if *this is empty; otherwise, the number of shared_ptr objects
   //!that share ownership with *this.
   //!
   //!Throws: nothing.
   //!
   //!Notes: use_count() is not necessarily efficient. Use only for debugging and
   //!testing purposes, not for production code.
   long use_count() const // never throws
   {  return m_pn.use_count();  }

   //!Returns: Returns: use_count() == 0.
   //!
   //!Throws: nothing.
   //!
   //!Notes: expired() may be faster than use_count().
   bool expired() const // never throws
   {  return m_pn.use_count() == 0;   }

   //!Effects: Equivalent to:
   //!weak_ptr().swap(*this).
   void reset() // never throws in 1.30+
   {  this_type().swap(*this);   }

   //!Effects: Exchanges the contents of the two
   //!smart pointers.
   //!
   //!Throws: nothing.
   void swap(this_type & other) // never throws
   {  ::boost::adl_move_swap(m_pn, other.m_pn);   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   template<class T2, class A2, class D2>
   bool _internal_less(weak_ptr<T2, A2, D2> const & rhs) const
   {  return m_pn < rhs.m_pn;  }

   template<class Y>
   void _internal_assign(const ipcdetail::shared_count<Y, A, D> & pn2)
   {

      m_pn = pn2;
   }

   private:

   template<class T2, class A2, class D2> friend class shared_ptr;
   template<class T2, class A2, class D2> friend class weak_ptr;

   ipcdetail::weak_count<T, A, D> m_pn;      // reference counter
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};  // weak_ptr

template<class T, class A, class D, class U, class A2, class D2> inline
bool operator<(weak_ptr<T, A, D> const & a, weak_ptr<U, A2, D2> const & b)
{  return a._internal_less(b);   }

template<class T, class A, class D> inline
void swap(weak_ptr<T, A, D> & a, weak_ptr<T, A, D> & b)
{  a.swap(b);  }

//!Returns the type of a weak pointer
//!of type T with the allocator boost::interprocess::allocator allocator
//!and boost::interprocess::deleter deleter
//!that can be constructed in the given managed segment type.
template<class T, class ManagedMemory>
struct managed_weak_ptr
{
   typedef weak_ptr
   < T
   , typename ManagedMemory::template allocator<void>::type
   , typename ManagedMemory::template deleter<T>::type
   > type;
};

//!Returns an instance of a weak pointer constructed
//!with the default allocator and deleter from a pointer
//!of type T that has been allocated in the passed managed segment
template<class T, class ManagedMemory>
inline typename managed_weak_ptr<T, ManagedMemory>::type
   make_managed_weak_ptr(T *constructed_object, ManagedMemory &managed_memory)
{
   return typename managed_weak_ptr<T, ManagedMemory>::type
   ( constructed_object
   , managed_memory.template get_allocator<void>()
   , managed_memory.template get_deleter<T>()
   );
}

} // namespace interprocess
} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif  // #ifndef BOOST_INTERPROCESS_WEAK_PTR_HPP_INCLUDED
