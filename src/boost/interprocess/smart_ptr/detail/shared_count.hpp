//////////////////////////////////////////////////////////////////////////////
//
// This file is the adaptation for Interprocess of boost/detail/shared_count.hpp
//
// (C) Copyright Peter Dimov and Multi Media Ltd. 2001, 2002, 2003
// (C) Copyright Peter Dimov 2004-2005
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTERPROCESS_DETAIL_SHARED_COUNT_HPP_INCLUDED
#define BOOST_INTERPROCESS_DETAIL_SHARED_COUNT_HPP_INCLUDED

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
# pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/checked_delete.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/interprocess/smart_ptr/detail/bad_weak_ptr.hpp>
#include <boost/interprocess/smart_ptr/detail/sp_counted_impl.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/container/allocator_traits.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/move/adl_move_swap.hpp>
#include <boost/intrusive/detail/minimal_less_equal_header.hpp>   //std::less
#include <boost/container/detail/placement_new.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail{

template<class T, class VoidAllocator, class Deleter>
class weak_count;

template<class T, class VoidAllocator, class Deleter>
class shared_count
{
   public:
   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<T>::type                         pointer;

   private:
   typedef sp_counted_impl_pd<VoidAllocator, Deleter>       counted_impl;

   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<counted_impl>::type                         counted_impl_ptr;
   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<sp_counted_base>::type                       counted_base_ptr;

   typedef boost::container::allocator_traits<VoidAllocator>         vallocator_traits;

   typedef typename vallocator_traits::template
      portable_rebind_alloc<counted_impl>::type                      counted_impl_allocator;

   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<const Deleter>::type                         const_deleter_pointer;

   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<const VoidAllocator>::type                   const_allocator_pointer;

   pointer           m_px;
   counted_impl_ptr  m_pi;

   template <class T2, class VoidAllocator2, class Deleter2>
   friend class weak_count;

   template <class T2, class VoidAllocator2, class Deleter2>
   friend class shared_count;

   public:

   shared_count()
      :  m_px(0), m_pi(0) // nothrow
   {}

   template <class Ptr>
   shared_count(const shared_count &other_shared_count, const Ptr &p)
      :  m_px(p), m_pi(other_shared_count.m_pi)
   {}

   template <class Ptr>
   shared_count(const Ptr &p, const VoidAllocator &a, Deleter d)
      :  m_px(p), m_pi(0)
   {
      BOOST_TRY{
         if(p){
            counted_impl_allocator alloc(a);
            m_pi = alloc.allocate(1);
            //Anti-exception deallocator
            scoped_ptr<counted_impl,
                     scoped_ptr_dealloc_functor<counted_impl_allocator> >
                        deallocator(m_pi, alloc);
            //It's more correct to use VoidAllocator::construct but
            //this needs copy constructor and we don't like it
            ::new(ipcdetail::to_raw_pointer(m_pi), boost_container_new_t())counted_impl(p, a, d);
            deallocator.release();
         }
      }
      BOOST_CATCH (...){
         d(p); // delete p
         BOOST_RETHROW
      }
      BOOST_CATCH_END
   }

   ~shared_count() // nothrow
   {
      if(m_pi)
         m_pi->release();
   }

   shared_count(shared_count const & r)
      :  m_px(r.m_px), m_pi(r.m_pi) // nothrow
   { if( m_pi != 0 ) m_pi->add_ref_copy(); }

   //this is a test
   template<class Y>
   explicit shared_count(shared_count<Y, VoidAllocator, Deleter> const & r)
      :  m_px(r.m_px), m_pi(r.m_pi) // nothrow
   {  if( m_pi != 0 ) m_pi->add_ref_copy();  }

   //this is a test
   template<class Y>
   explicit shared_count(const pointer & ptr, shared_count<Y, VoidAllocator, Deleter> const & r)
      :  m_px(ptr), m_pi(r.m_pi) // nothrow
   {  if( m_pi != 0 ) m_pi->add_ref_copy();  }

/*
   explicit shared_count(weak_count<Y, VoidAllocator, Deleter> const & r)
      // throws bad_weak_ptr when r.use_count() == 0
      : m_pi( r.m_pi )
   {
      if( m_pi == 0 || !m_pi->add_ref_lock() ){
         boost::throw_exception( boost::interprocess::bad_weak_ptr() );
      }
   }
*/
   template<class Y>
   explicit shared_count(weak_count<Y, VoidAllocator, Deleter> const & r)
      // throws bad_weak_ptr when r.use_count() == 0
      : m_px(r.m_px), m_pi( r.m_pi )
   {
      if( m_pi == 0 || !m_pi->add_ref_lock() ){
         throw( boost::interprocess::bad_weak_ptr() );
      }
   }

   const pointer &to_raw_pointer() const
   {  return m_px;   }

   pointer &to_raw_pointer()
   {  return m_px;   }

   shared_count & operator= (shared_count const & r) // nothrow
   {
      m_px = r.m_px;
      counted_impl_ptr tmp = r.m_pi;
      if( tmp != m_pi ){
         if(tmp != 0)   tmp->add_ref_copy();
         if(m_pi != 0)  m_pi->release();
         m_pi = tmp;
      }
      return *this;
   }

   template<class Y>
   shared_count & operator= (shared_count<Y, VoidAllocator, Deleter> const & r) // nothrow
   {
      m_px = r.m_px;
      counted_impl_ptr tmp = r.m_pi;
      if( tmp != m_pi ){
         if(tmp != 0)   tmp->add_ref_copy();
         if(m_pi != 0)  m_pi->release();
         m_pi = tmp;
      }
      return *this;
   }

   void swap(shared_count & r) // nothrow
   {  ::boost::adl_move_swap(m_px, r.m_px);  ::boost::adl_move_swap(m_pi, r.m_pi);   }

   long use_count() const // nothrow
   {  return m_pi != 0? m_pi->use_count(): 0;  }

   bool unique() const // nothrow
   {  return use_count() == 1;   }

   const_deleter_pointer get_deleter() const
   {  return m_pi ? m_pi->get_deleter() : 0; }

//   const_allocator_pointer get_allocator() const
//   {  return m_pi ? m_pi->get_allocator() : 0; }

   template<class T2, class VoidAllocator2, class Deleter2>
   bool internal_equal (shared_count<T2, VoidAllocator2, Deleter2> const & other) const
   {  return this->m_pi == other.m_pi;   }

   template<class T2, class VoidAllocator2, class Deleter2>
   bool internal_less  (shared_count<T2, VoidAllocator2, Deleter2> const & other) const
   {  return std::less<counted_base_ptr>()(this->m_pi, other.m_pi);  }
};

template<class T, class VoidAllocator, class Deleter, class T2, class VoidAllocator2, class Deleter2> inline
bool operator==(shared_count<T, VoidAllocator, Deleter> const & a, shared_count<T2, VoidAllocator2, Deleter2> const & b)
{  return a.internal_equal(b);  }

template<class T, class VoidAllocator, class Deleter, class T2, class VoidAllocator2, class Deleter2> inline
bool operator<(shared_count<T, VoidAllocator, Deleter> const & a, shared_count<T2, VoidAllocator2, Deleter2> const & b)
{  return a.internal_less(b);   }


template<class T, class VoidAllocator, class Deleter>
class weak_count
{
   public:
   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<T>::type                         pointer;

   private:

   typedef sp_counted_impl_pd<VoidAllocator, Deleter>                counted_impl;

   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<counted_impl>::type                          counted_impl_ptr;
   typedef typename boost::intrusive::
      pointer_traits<typename VoidAllocator::pointer>::template
         rebind_pointer<sp_counted_base>::type                       counted_base_ptr;

   pointer           m_px;
   counted_impl_ptr  m_pi;

   template <class T2, class VoidAllocator2, class Deleter2>
   friend class weak_count;

   template <class T2, class VoidAllocator2, class Deleter2>
   friend class shared_count;

   public:

   weak_count(): m_px(0), m_pi(0) // nothrow
   {}

   template <class Y>
   explicit weak_count(shared_count<Y, VoidAllocator, Deleter> const & r)
      :  m_px(r.m_px), m_pi(r.m_pi) // nothrow
   {  if(m_pi != 0) m_pi->weak_add_ref(); }

   weak_count(weak_count const & r)
      :  m_px(r.m_px), m_pi(r.m_pi) // nothrow
   {  if(m_pi != 0) m_pi->weak_add_ref(); }

   template<class Y>
   weak_count(weak_count<Y, VoidAllocator, Deleter> const & r)
      : m_px(r.m_px), m_pi(r.m_pi) // nothrow
   {  if(m_pi != 0) m_pi->weak_add_ref(); }

   ~weak_count() // nothrow
   {  if(m_pi != 0) m_pi->weak_release(); }

   template<class Y>
   weak_count & operator= (shared_count<Y, VoidAllocator, Deleter> const & r) // nothrow
   {
      m_px = r.m_px;
      counted_impl_ptr tmp = r.m_pi;
      if(tmp != 0)   tmp->weak_add_ref();
      if(m_pi != 0)  m_pi->weak_release();
      m_pi = tmp;
      return *this;
   }

   weak_count & operator= (weak_count const & r) // nothrow
   {
      m_px = r.m_px;
      counted_impl_ptr tmp = r.m_pi;
      if(tmp != 0) tmp->weak_add_ref();
      if(m_pi != 0) m_pi->weak_release();
      m_pi = tmp;
      return *this;
   }

   void set_pointer(const pointer &ptr)
   {  m_px = ptr; }

   template<class Y>
   weak_count & operator= (weak_count<Y, VoidAllocator, Deleter> const& r) // nothrow
   {
      counted_impl_ptr tmp = r.m_pi;
      if(tmp != 0) tmp->weak_add_ref();
      if(m_pi != 0) m_pi->weak_release();
      m_pi = tmp;
      return *this;
   }

   void swap(weak_count & r) // nothrow
   {  ::boost::adl_move_swap(m_px, r.m_px);  ::boost::adl_move_swap(m_pi, r.m_pi);   }

   long use_count() const // nothrow
   {  return m_pi != 0? m_pi->use_count() : 0;   }

   template<class T2, class VoidAllocator2, class Deleter2>
   bool internal_equal (weak_count<T2, VoidAllocator2, Deleter2> const & other) const
   {  return this->m_pi == other.m_pi;   }

   template<class T2, class VoidAllocator2, class Deleter2>
   bool internal_less (weak_count<T2, VoidAllocator2, Deleter2> const & other) const
   {  return std::less<counted_base_ptr>()(this->m_pi, other.m_pi);  }
};

template<class T, class VoidAllocator, class Deleter, class T2, class VoidAllocator2, class Deleter2> inline
bool operator==(weak_count<T, VoidAllocator, Deleter> const & a, weak_count<T2, VoidAllocator2, Deleter2> const & b)
{  return a.internal_equal(b);  }

template<class T, class VoidAllocator, class Deleter, class T2, class VoidAllocator2, class Deleter2> inline
bool operator<(weak_count<T, VoidAllocator, Deleter> const & a, weak_count<T2, VoidAllocator2, Deleter2> const & b)
{  return a.internal_less(b);   }

} // namespace ipcdetail
} // namespace interprocess
} // namespace boost


#include <boost/interprocess/detail/config_end.hpp>


#endif  // #ifndef BOOST_INTERPROCESS_DETAIL_SHARED_COUNT_HPP_INCLUDED
