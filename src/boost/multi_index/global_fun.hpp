/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_GLOBAL_FUN_HPP
#define BOOST_MULTI_INDEX_GLOBAL_FUN_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/workaround.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>

#if !defined(BOOST_NO_SFINAE)
#include <boost/type_traits/is_convertible.hpp>
#endif

namespace boost{

template<class T> class reference_wrapper; /* fwd decl. */

namespace multi_index{

namespace detail{

/* global_fun is a read-only key extractor from Value based on a given global
 * (or static member) function with signature:
 *
 *   Type f([const] Value [&]);
 *
 * Additionally, global_fun  and const_global_fun are overloaded to support
 * referece_wrappers of Value and "chained pointers" to Value's. By chained
 * pointer to T we  mean a type P such that, given a p of Type P
 *   *...n...*x is convertible to T&, for some n>=1.
 * Examples of chained pointers are raw and smart pointers, iterators and
 * arbitrary combinations of these (vg. T** or unique_ptr<T*>.)
 */

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct const_ref_global_fun_base
{
  typedef typename remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<const ChainedPtr&,Value>,Type>::type
#else
  Type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(Value x)const
  {
    return PtrToFunction(x);
  }

  Type operator()(
    const reference_wrapper<
      typename remove_reference<Value>::type>& x)const
  { 
    return operator()(x.get());
  }

  Type operator()(
    const reference_wrapper<
      typename remove_const<
        typename remove_reference<Value>::type>::type>& x

#if BOOST_WORKAROUND(BOOST_MSVC,==1310)
/* http://lists.boost.org/Archives/boost/2015/10/226135.php */
    ,int=0
#endif

  )const
  { 
    return operator()(x.get());
  }
};

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct non_const_ref_global_fun_base
{
  typedef typename remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<ChainedPtr&,Value>,Type>::type
#else
  Type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(Value x)const
  {
    return PtrToFunction(x);
  }

  Type operator()(
    const reference_wrapper<
      typename remove_reference<Value>::type>& x)const
  { 
    return operator()(x.get());
  }
};

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct non_ref_global_fun_base
{
  typedef typename remove_reference<Type>::type result_type;

  template<typename ChainedPtr>

#if !defined(BOOST_NO_SFINAE)
  typename disable_if<
    is_convertible<const ChainedPtr&,const Value&>,Type>::type
#else
  Type
#endif

  operator()(const ChainedPtr& x)const
  {
    return operator()(*x);
  }

  Type operator()(const Value& x)const
  {
    return PtrToFunction(x);
  }

  Type operator()(const reference_wrapper<const Value>& x)const
  { 
    return operator()(x.get());
  }

  Type operator()(
    const reference_wrapper<typename remove_const<Value>::type>& x)const
  { 
    return operator()(x.get());
  }
};

} /* namespace multi_index::detail */

template<class Value,typename Type,Type (*PtrToFunction)(Value)>
struct global_fun:
  mpl::if_c<
    is_reference<Value>::value,
    typename mpl::if_c<
      is_const<typename remove_reference<Value>::type>::value,
      detail::const_ref_global_fun_base<Value,Type,PtrToFunction>,
      detail::non_const_ref_global_fun_base<Value,Type,PtrToFunction>
    >::type,
    detail::non_ref_global_fun_base<Value,Type,PtrToFunction>
  >::type
{
};

} /* namespace multi_index */

} /* namespace boost */

#endif
