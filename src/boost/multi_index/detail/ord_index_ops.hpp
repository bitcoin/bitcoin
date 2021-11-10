/* Copyright 2003-2014 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 *
 * The internal implementation of red-black trees is based on that of SGI STL
 * stl_tree.h file: 
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_ORD_INDEX_OPS_HPP
#define BOOST_MULTI_INDEX_DETAIL_ORD_INDEX_OPS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/mpl/and.hpp>
#include <boost/multi_index/detail/promotes_arg.hpp>
#include <utility>

namespace boost{

namespace multi_index{

namespace detail{

/* Common code for index memfuns having templatized and
 * non-templatized versions.
 * Implementation note: When CompatibleKey is consistently promoted to
 * KeyFromValue::result_type for comparison, the promotion is made once in
 * advance to increase efficiency.
 */

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline Node* ordered_index_find(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp)
{
  typedef typename KeyFromValue::result_type key_type;

  return ordered_index_find(
    top,y,key,x,comp,
    mpl::and_<
      promotes_1st_arg<CompatibleCompare,CompatibleKey,key_type>,
      promotes_2nd_arg<CompatibleCompare,key_type,CompatibleKey> >());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleCompare
>
inline Node* ordered_index_find(
  Node* top,Node* y,const KeyFromValue& key,
  const BOOST_DEDUCED_TYPENAME KeyFromValue::result_type& x,
  const CompatibleCompare& comp,mpl::true_)
{
  return ordered_index_find(top,y,key,x,comp,mpl::false_());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline Node* ordered_index_find(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp,mpl::false_)
{
  Node* y0=y;

  while (top){
    if(!comp(key(top->value()),x)){
      y=top;
      top=Node::from_impl(top->left());
    }
    else top=Node::from_impl(top->right());
  }
    
  return (y==y0||comp(x,key(y->value())))?y0:y;
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline Node* ordered_index_lower_bound(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp)
{
  typedef typename KeyFromValue::result_type key_type;

  return ordered_index_lower_bound(
    top,y,key,x,comp,
    promotes_2nd_arg<CompatibleCompare,key_type,CompatibleKey>());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleCompare
>
inline Node* ordered_index_lower_bound(
  Node* top,Node* y,const KeyFromValue& key,
  const BOOST_DEDUCED_TYPENAME KeyFromValue::result_type& x,
  const CompatibleCompare& comp,mpl::true_)
{
  return ordered_index_lower_bound(top,y,key,x,comp,mpl::false_());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline Node* ordered_index_lower_bound(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp,mpl::false_)
{
  while(top){
    if(!comp(key(top->value()),x)){
      y=top;
      top=Node::from_impl(top->left());
    }
    else top=Node::from_impl(top->right());
  }

  return y;
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline Node* ordered_index_upper_bound(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp)
{
  typedef typename KeyFromValue::result_type key_type;

  return ordered_index_upper_bound(
    top,y,key,x,comp,
    promotes_1st_arg<CompatibleCompare,CompatibleKey,key_type>());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleCompare
>
inline Node* ordered_index_upper_bound(
  Node* top,Node* y,const KeyFromValue& key,
  const BOOST_DEDUCED_TYPENAME KeyFromValue::result_type& x,
  const CompatibleCompare& comp,mpl::true_)
{
  return ordered_index_upper_bound(top,y,key,x,comp,mpl::false_());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline Node* ordered_index_upper_bound(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp,mpl::false_)
{
  while(top){
    if(comp(x,key(top->value()))){
      y=top;
      top=Node::from_impl(top->left());
    }
    else top=Node::from_impl(top->right());
  }

  return y;
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline std::pair<Node*,Node*> ordered_index_equal_range(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp)
{
  typedef typename KeyFromValue::result_type key_type;

  return ordered_index_equal_range(
    top,y,key,x,comp,
    mpl::and_<
      promotes_1st_arg<CompatibleCompare,CompatibleKey,key_type>,
      promotes_2nd_arg<CompatibleCompare,key_type,CompatibleKey> >());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleCompare
>
inline std::pair<Node*,Node*> ordered_index_equal_range(
  Node* top,Node* y,const KeyFromValue& key,
  const BOOST_DEDUCED_TYPENAME KeyFromValue::result_type& x,
  const CompatibleCompare& comp,mpl::true_)
{
  return ordered_index_equal_range(top,y,key,x,comp,mpl::false_());
}

template<
  typename Node,typename KeyFromValue,
  typename CompatibleKey,typename CompatibleCompare
>
inline std::pair<Node*,Node*> ordered_index_equal_range(
  Node* top,Node* y,const KeyFromValue& key,const CompatibleKey& x,
  const CompatibleCompare& comp,mpl::false_)
{
  while(top){
    if(comp(key(top->value()),x)){
      top=Node::from_impl(top->right());
    }
    else if(comp(x,key(top->value()))){
      y=top;
      top=Node::from_impl(top->left());
    }
    else{
      return std::pair<Node*,Node*>(
        ordered_index_lower_bound(
          Node::from_impl(top->left()),top,key,x,comp,mpl::false_()),
        ordered_index_upper_bound(
          Node::from_impl(top->right()),y,key,x,comp,mpl::false_()));
    }
  }

  return std::pair<Node*,Node*>(y,y);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
