//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/scoped_ptr.hpp
//
// (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
// (C) Copyright Peter Dimov 2001, 2002
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SCOPED_PTR_HPP_INCLUDED
#define BOOST_INTERPROCESS_SCOPED_PTR_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/pointer_type.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/assert.hpp>
#include <boost/move/adl_move_swap.hpp>

//!\file
//!Describes the smart pointer scoped_ptr

namespace boost {
namespace interprocess {

//!scoped_ptr stores a pointer to a dynamically allocated object.
//!The object pointed to is guaranteed to be deleted, either on destruction
//!of the scoped_ptr, or via an explicit reset. The user can avoid this
//!deletion using release().
//!scoped_ptr is parameterized on T (the type of the object pointed to) and
//!Deleter (the functor to be executed to delete the internal pointer).
//!The internal pointer will be of the same pointer type as typename
//!Deleter::pointer type (that is, if typename Deleter::pointer is
//!offset_ptr<void>, the internal pointer will be offset_ptr<T>).
template<class T, class Deleter>
class scoped_ptr
   : private Deleter
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   scoped_ptr(scoped_ptr const &);
   scoped_ptr & operator=(scoped_ptr const &);

   typedef scoped_ptr<T, Deleter> this_type;
   typedef typename ipcdetail::add_reference<T>::type reference;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:

   typedef T element_type;
   typedef Deleter deleter_type;
   typedef typename ipcdetail::pointer_type<T, Deleter>::type pointer;

   //!Constructs a scoped_ptr, storing a copy of p(which can be 0) and d.
   //!Does not throw.
   explicit scoped_ptr(const pointer &p = 0, const Deleter &d = Deleter())
      : Deleter(d), m_ptr(p) // throws if pointer/Deleter copy ctor throws
   {}

   //!If the stored pointer is not 0, destroys the object pointed to by the stored pointer.
   //!calling the operator() of the stored deleter. Never throws
   ~scoped_ptr()
   {
      if(m_ptr){
         Deleter &del = static_cast<Deleter&>(*this);
         del(m_ptr);
      }
   }

   //!Deletes the object pointed to by the stored pointer and then
   //!stores a copy of p. Never throws
   void reset(const pointer &p = 0) // never throws
   {  BOOST_ASSERT(p == 0 || p != m_ptr); this_type(p).swap(*this);  }

   //!Deletes the object pointed to by the stored pointer and then
   //!stores a copy of p and a copy of d.
   void reset(const pointer &p, const Deleter &d) // never throws
   {  BOOST_ASSERT(p == 0 || p != m_ptr); this_type(p, d).swap(*this);  }

   //!Assigns internal pointer as 0 and returns previous pointer. This will
   //!avoid deletion on destructor
   pointer release() BOOST_NOEXCEPT
   {  pointer tmp(m_ptr);  m_ptr = 0;  return tmp; }

   //!Returns a reference to the object pointed to by the stored pointer.
   //!Never throws.
   reference operator*() const BOOST_NOEXCEPT
   {  BOOST_ASSERT(m_ptr != 0);  return *m_ptr; }

   //!Returns the internal stored pointer.
   //!Never throws.
   pointer &operator->() BOOST_NOEXCEPT
   {  BOOST_ASSERT(m_ptr != 0);  return m_ptr;  }

   //!Returns the internal stored pointer.
   //!Never throws.
   const pointer &operator->() const BOOST_NOEXCEPT
   {  BOOST_ASSERT(m_ptr != 0);  return m_ptr;  }

   //!Returns the stored pointer.
   //!Never throws.
   pointer & get() BOOST_NOEXCEPT
   {  return m_ptr;  }

   //!Returns the stored pointer.
   //!Never throws.
   const pointer & get() const BOOST_NOEXCEPT
   {  return m_ptr;  }

   typedef pointer this_type::*unspecified_bool_type;

   //!Conversion to bool
   //!Never throws
   operator unspecified_bool_type() const BOOST_NOEXCEPT
   {  return m_ptr == 0? 0: &this_type::m_ptr;  }

   //!Returns true if the stored pointer is 0.
   //!Never throws.
   bool operator! () const BOOST_NOEXCEPT // never throws
   {  return m_ptr == 0;   }

   //!Exchanges the internal pointer and deleter with other scoped_ptr
   //!Never throws.
   void swap(scoped_ptr & b) BOOST_NOEXCEPT // never throws
   {
      ::boost::adl_move_swap(static_cast<Deleter&>(*this), static_cast<Deleter&>(b));
      ::boost::adl_move_swap(m_ptr, b.m_ptr);
   }

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:
   pointer m_ptr;
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

//!Exchanges the internal pointer and deleter with other scoped_ptr
//!Never throws.
template<class T, class D> inline
void swap(scoped_ptr<T, D> & a, scoped_ptr<T, D> & b) BOOST_NOEXCEPT
{  a.swap(b); }

//!Returns a copy of the stored pointer
//!Never throws
template<class T, class D> inline
typename scoped_ptr<T, D>::pointer to_raw_pointer(scoped_ptr<T, D> const & p)
{  return p.get();   }

} // namespace interprocess

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

#if defined(_MSC_VER) && (_MSC_VER < 1400)
template<class T, class D> inline
T *to_raw_pointer(boost::interprocess::scoped_ptr<T, D> const & p)
{  return p.get();   }
#endif

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

} // namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // #ifndef BOOST_INTERPROCESS_SCOPED_PTR_HPP_INCLUDED
