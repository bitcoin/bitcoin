/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_ALLOCATOR_TRAITS_HPP
#define BOOST_MULTI_INDEX_DETAIL_ALLOCATOR_TRAITS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */

#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#include <boost/type_traits/is_empty.hpp>
#include <memory>
#else
#include <boost/detail/workaround.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <new>
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* poor man's replacement of std::allocator_traits */

#if !defined(BOOST_NO_CXX11_ALLOCATOR)

template<typename T> struct void_helper{typedef void type;};

template<typename Allocator,typename=void>
struct allocator_is_always_equal:boost::is_empty<Allocator>{};

template<typename Allocator>
struct allocator_is_always_equal<
  Allocator,
  typename void_helper<
    typename std::allocator_traits<Allocator>::is_always_equal
  >::type
>:std::allocator_traits<Allocator>::is_always_equal{};

template<typename Allocator>
struct allocator_traits:std::allocator_traits<Allocator>
{
  /* wrap std::allocator_traits alias templates for use in C++03 codebase */

  typedef std::allocator_traits<Allocator> super;

  /* pre-C++17 compatibilty */

  typedef allocator_is_always_equal<Allocator> is_always_equal;

  template<typename T>
  struct rebind_alloc
  {
    typedef typename super::template rebind_alloc<T> type;
  };

  template<typename T>
  struct rebind_traits
  {
    typedef typename super::template rebind_traits<T> type;
  };
};

#else

/* not a full std::allocator_traits rewrite (not needed) */

template<typename Allocator>
struct allocator_traits
{
  typedef Allocator                           allocator_type;
  typedef typename Allocator::value_type      value_type;
  typedef typename Allocator::pointer         pointer;
  typedef typename Allocator::const_pointer   const_pointer;

  /* [const_]void_pointer not provided as boost::pointer_traits's
   * rebind_to has been seen to fail with things like
   * boost::interprocess::offset_ptr in relatively old environments.
   */

  typedef typename Allocator::difference_type difference_type;
  typedef typename Allocator::size_type       size_type;

  typedef boost::false_type          propagate_on_container_copy_assignment;
  typedef boost::false_type          propagate_on_container_move_assignment;
  typedef boost::false_type          propagate_on_container_swap;
  typedef boost::is_empty<Allocator> is_always_equal;

  template<typename T>
  struct rebind_alloc
  {
    typedef typename Allocator::template rebind<T>::other type;
  };

  template<typename T>
  struct rebind_traits
  {
    typedef allocator_traits<typename rebind_alloc<T>::type> type;
  };

  static pointer   allocate(Allocator& a,size_type n){return a.allocate(n);}
  static pointer   allocate(Allocator& a,size_type n,const_pointer p)
                                   /* should've been const_void_pointer p */
                     {return a.allocate(n,p);} 
  static void      deallocate(Allocator& a,pointer p,size_type n)
                     {a.deallocate(p,n);}
  template<typename T>
  static void      construct(Allocator&,T* p,const T& x)
                     {::new (static_cast<void*>(p)) T(x);}
  template<typename T>
  static void      construct(Allocator&,T* p,BOOST_RV_REF(T) x)
                     {::new (static_cast<void*>(p)) T(boost::move(x));}
 
  template<typename T,BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  static void construct(Allocator&,T* p,BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    vartempl_placement_new(p,BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
  }

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
/* MSVC issues spurious warnings about unreferencend formal parameters in
 * destroy<T> when T is a class with trivial dtor.
 */

#pragma warning(push)
#pragma warning(disable:4100)
#endif

  template<typename T>
  static void destroy(Allocator&,T* p){p->~T();}

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
#pragma warning(pop)
#endif

  static size_type max_size(Allocator& a)BOOST_NOEXCEPT{return a.max_size();}

  static Allocator select_on_container_copy_construction(const Allocator& a)
  {
    return a;
  }
};

#endif

template<typename Allocator,typename T>
struct rebind_alloc_for
{
  typedef typename allocator_traits<Allocator>::
    template rebind_alloc<T>::type               type;
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
