/* Copyright 2003-2020 Joaquin M Lopez Munoz.
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

#ifndef BOOST_MULTI_INDEX_DETAIL_ORD_INDEX_IMPL_HPP
#define BOOST_MULTI_INDEX_DETAIL_ORD_INDEX_IMPL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/call_traits.hpp>
#include <boost/core/addressof.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/foreach_fwd.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/multi_index/detail/access_specifier.hpp>
#include <boost/multi_index/detail/adl_swap.hpp>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/bidir_node_iterator.hpp>
#include <boost/multi_index/detail/do_not_copy_elements_tag.hpp>
#include <boost/multi_index/detail/index_node_base.hpp>
#include <boost/multi_index/detail/modify_key_adaptor.hpp>
#include <boost/multi_index/detail/node_handle.hpp>
#include <boost/multi_index/detail/ord_index_node.hpp>
#include <boost/multi_index/detail/ord_index_ops.hpp>
#include <boost/multi_index/detail/safe_mode.hpp>
#include <boost/multi_index/detail/scope_guard.hpp>
#include <boost/multi_index/detail/unbounded.hpp>
#include <boost/multi_index/detail/value_compare.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <boost/multi_index/detail/ord_index_impl_fwd.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>
#include <utility>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/archive/archive_exception.hpp>
#include <boost/bind/bind.hpp>
#include <boost/multi_index/detail/duplicates_iterator.hpp>
#include <boost/throw_exception.hpp> 
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(x)                    \
  detail::scope_guard BOOST_JOIN(check_invariant_,__LINE__)=                 \
    detail::make_obj_guard(x,&ordered_index_impl::check_invariant_);         \
  BOOST_JOIN(check_invariant_,__LINE__).touch();
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT                          \
  BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(*this)
#else
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(x)
#define BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* ordered_index adds a layer of ordered indexing to a given Super and accepts
 * an augmenting policy for optional addition of order statistics.
 */

/* Most of the implementation of unique and non-unique indices is
 * shared. We tell from one another on instantiation time by using
 * these tags.
 */

struct ordered_unique_tag{};
struct ordered_non_unique_tag{};

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
class ordered_index;

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
class ordered_index_impl:
  BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS SuperMeta::type

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  ,public safe_mode::safe_container<
    ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy> >
#endif

{ 
#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
/* The "ISO C++ Template Parser" option in CW8.3 has a problem with the
 * lifetime of const references bound to temporaries --precisely what
 * scopeguards are.
 */

#pragma parse_mfunc_templ off
#endif

  typedef typename SuperMeta::type                   super;

protected:
  typedef ordered_index_node<
    AugmentPolicy,typename super::index_node_type>   index_node_type;

protected: /* for the benefit of AugmentPolicy::augmented_interface */
  typedef typename index_node_type::impl_type        node_impl_type;
  typedef typename node_impl_type::pointer           node_impl_pointer;

public:
  /* types */

  typedef typename KeyFromValue::result_type         key_type;
  typedef typename index_node_type::value_type       value_type;
  typedef KeyFromValue                               key_from_value;
  typedef Compare                                    key_compare;
  typedef value_comparison<
    value_type,KeyFromValue,Compare>                 value_compare;
  typedef tuple<key_from_value,key_compare>          ctor_args;
  typedef typename super::final_allocator_type       allocator_type;
  typedef value_type&                                reference;
  typedef const value_type&                          const_reference;

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef safe_mode::safe_iterator<
    bidir_node_iterator<index_node_type>,
    ordered_index_impl>                              iterator;
#else
  typedef bidir_node_iterator<index_node_type>       iterator;
#endif

  typedef iterator                                   const_iterator;

private:
  typedef allocator_traits<allocator_type>           alloc_traits;

public:
  typedef typename alloc_traits::size_type           size_type;      
  typedef typename alloc_traits::difference_type     difference_type;
  typedef typename alloc_traits::pointer             pointer;
  typedef typename alloc_traits::const_pointer       const_pointer;
  typedef typename
    boost::reverse_iterator<iterator>                reverse_iterator;
  typedef typename
    boost::reverse_iterator<const_iterator>          const_reverse_iterator;
  typedef typename super::final_node_handle_type     node_type;
  typedef detail::insert_return_type<
    iterator,node_type>                              insert_return_type;
  typedef TagList                                    tag_list;

protected:
  typedef typename super::final_node_type            final_node_type;
  typedef tuples::cons<
    ctor_args, 
    typename super::ctor_args_list>                  ctor_args_list;
  typedef typename mpl::push_front<
    typename super::index_type_list,
    ordered_index<
      KeyFromValue,Compare,
      SuperMeta,TagList,Category,AugmentPolicy
    > >::type                                        index_type_list;
  typedef typename mpl::push_front<
    typename super::iterator_type_list,
    iterator>::type    iterator_type_list;
  typedef typename mpl::push_front<
    typename super::const_iterator_type_list,
    const_iterator>::type                            const_iterator_type_list;
  typedef typename super::copy_map_type              copy_map_type;

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  typedef typename super::index_saver_type           index_saver_type;
  typedef typename super::index_loader_type          index_loader_type;
#endif

protected:
#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef safe_mode::safe_container<
    ordered_index_impl>                              safe_super;
#endif

  typedef typename call_traits<
    value_type>::param_type                          value_param_type;
  typedef typename call_traits<
    key_type>::param_type                            key_param_type;

  /* Needed to avoid commas in BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL
   * expansion.
   */

  typedef std::pair<iterator,bool>                   emplace_return_type;

public:

  /* construct/copy/destroy
   * Default and copy ctors are in the protected section as indices are
   * not supposed to be created on their own. No range ctor either.
   * Assignment operators defined at ordered_index rather than here.
   */

  allocator_type get_allocator()const BOOST_NOEXCEPT
  {
    return this->final().get_allocator();
  }

  /* iterators */

  iterator
    begin()BOOST_NOEXCEPT{return make_iterator(leftmost());}
  const_iterator
    begin()const BOOST_NOEXCEPT{return make_iterator(leftmost());}
  iterator
    end()BOOST_NOEXCEPT{return make_iterator(header());}
  const_iterator
    end()const BOOST_NOEXCEPT{return make_iterator(header());}
  reverse_iterator
    rbegin()BOOST_NOEXCEPT{return boost::make_reverse_iterator(end());}
  const_reverse_iterator
    rbegin()const BOOST_NOEXCEPT{return boost::make_reverse_iterator(end());}
  reverse_iterator
    rend()BOOST_NOEXCEPT{return boost::make_reverse_iterator(begin());}
  const_reverse_iterator
    rend()const BOOST_NOEXCEPT{return boost::make_reverse_iterator(begin());}
  const_iterator
    cbegin()const BOOST_NOEXCEPT{return begin();}
  const_iterator
    cend()const BOOST_NOEXCEPT{return end();}
  const_reverse_iterator
    crbegin()const BOOST_NOEXCEPT{return rbegin();}
  const_reverse_iterator
    crend()const BOOST_NOEXCEPT{return rend();}
 
  iterator iterator_to(const value_type& x)
  {
    return make_iterator(
      node_from_value<index_node_type>(boost::addressof(x)));
  }

  const_iterator iterator_to(const value_type& x)const
  {
    return make_iterator(
      node_from_value<index_node_type>(boost::addressof(x)));
  }

  /* capacity */

  bool      empty()const BOOST_NOEXCEPT{return this->final_empty_();}
  size_type size()const BOOST_NOEXCEPT{return this->final_size_();}
  size_type max_size()const BOOST_NOEXCEPT{return this->final_max_size_();}

  /* modifiers */

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL(
    emplace_return_type,emplace,emplace_impl)

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG(
    iterator,emplace_hint,emplace_hint_impl,iterator,position)

  std::pair<iterator,bool> insert(const value_type& x)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_(x);
    return std::pair<iterator,bool>(make_iterator(p.first),p.second);
  }

  std::pair<iterator,bool> insert(BOOST_RV_REF(value_type) x)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_rv_(x);
    return std::pair<iterator,bool>(make_iterator(p.first),p.second);
  }

  iterator insert(iterator position,const value_type& x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_(
      x,static_cast<final_node_type*>(position.get_node()));
    return make_iterator(p.first);
  }
    
  iterator insert(iterator position,BOOST_RV_REF(value_type) x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_rv_(
      x,static_cast<final_node_type*>(position.get_node()));
    return make_iterator(p.first);
  }

  template<typename InputIterator>
  void insert(InputIterator first,InputIterator last)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    index_node_type* hint=header(); /* end() */
    for(;first!=last;++first){
      hint=this->final_insert_ref_(
        *first,static_cast<final_node_type*>(hint)).first;
      index_node_type::increment(hint);
    }
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  void insert(std::initializer_list<value_type> list)
  {
    insert(list.begin(),list.end());
  }
#endif

  insert_return_type insert(BOOST_RV_REF(node_type) nh)
  {
    if(nh)BOOST_MULTI_INDEX_CHECK_EQUAL_ALLOCATORS(*this,nh);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_nh_(nh);
    return insert_return_type(make_iterator(p.first),p.second,boost::move(nh));
  }

  iterator insert(const_iterator position,BOOST_RV_REF(node_type) nh)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    if(nh)BOOST_MULTI_INDEX_CHECK_EQUAL_ALLOCATORS(*this,nh);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_nh_(
      nh,static_cast<final_node_type*>(position.get_node()));
    return make_iterator(p.first);
  }

  node_type extract(const_iterator position)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    return this->final_extract_(
      static_cast<final_node_type*>(position.get_node()));
  }

  node_type extract(key_param_type x)
  {
    iterator position=lower_bound(x);
    if(position==end()||comp_(x,key(*position)))return node_type();
    else return extract(position);
  }

  iterator erase(iterator position)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    this->final_erase_(static_cast<final_node_type*>(position++.get_node()));
    return position;
  }
  
  size_type erase(key_param_type x)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<iterator,iterator> p=equal_range(x);
    size_type s=0;
    while(p.first!=p.second){
      p.first=erase(p.first);
      ++s;
    }
    return s;
  }

  iterator erase(iterator first,iterator last)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(first);
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(last);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(first,*this);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(last,*this);
    BOOST_MULTI_INDEX_CHECK_VALID_RANGE(first,last);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    while(first!=last){
      first=erase(first);
    }
    return first;
  }

  bool replace(iterator position,const value_type& x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    return this->final_replace_(
      x,static_cast<final_node_type*>(position.get_node()));
  }

  bool replace(iterator position,BOOST_RV_REF(value_type) x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    return this->final_replace_rv_(
      x,static_cast<final_node_type*>(position.get_node()));
  }

  template<typename Modifier>
  bool modify(iterator position,Modifier mod)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    /* MSVC++ 6.0 optimizer on safe mode code chokes if this
     * this is not added. Left it for all compilers as it does no
     * harm.
     */

    position.detach();
#endif

    return this->final_modify_(
      mod,static_cast<final_node_type*>(position.get_node()));
  }

  template<typename Modifier,typename Rollback>
  bool modify(iterator position,Modifier mod,Rollback back_)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    /* MSVC++ 6.0 optimizer on safe mode code chokes if this
     * this is not added. Left it for all compilers as it does no
     * harm.
     */

    position.detach();
#endif

    return this->final_modify_(
      mod,back_,static_cast<final_node_type*>(position.get_node()));
  }
  
  template<typename Modifier>
  bool modify_key(iterator position,Modifier mod)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    return modify(
      position,modify_key_adaptor<Modifier,value_type,KeyFromValue>(mod,key));
  }

  template<typename Modifier,typename Rollback>
  bool modify_key(iterator position,Modifier mod,Rollback back_)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    return modify(
      position,
      modify_key_adaptor<Modifier,value_type,KeyFromValue>(mod,key),
      modify_key_adaptor<Rollback,value_type,KeyFromValue>(back_,key));
  }

  void swap(
    ordered_index<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF(x);
    this->final_swap_(x.final());
  }

  void clear()BOOST_NOEXCEPT
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    this->final_clear_();
  }

  /* observers */

  key_from_value key_extractor()const{return key;}
  key_compare    key_comp()const{return comp_;}
  value_compare  value_comp()const{return value_compare(key,comp_);}

  /* set operations */

  /* Internally, these ops rely on const_iterator being the same
   * type as iterator.
   */

  template<typename CompatibleKey>
  iterator find(const CompatibleKey& x)const
  {
    return make_iterator(ordered_index_find(root(),header(),key,x,comp_));
  }

  template<typename CompatibleKey,typename CompatibleCompare>
  iterator find(
    const CompatibleKey& x,const CompatibleCompare& comp)const
  {
    return make_iterator(ordered_index_find(root(),header(),key,x,comp));
  }

  template<typename CompatibleKey>
  size_type count(const CompatibleKey& x)const
  {
    return count(x,comp_);
  }

  template<typename CompatibleKey,typename CompatibleCompare>
  size_type count(const CompatibleKey& x,const CompatibleCompare& comp)const
  {
    std::pair<iterator,iterator> p=equal_range(x,comp);
    size_type n=static_cast<size_type>(std::distance(p.first,p.second));
    return n;
  }

  template<typename CompatibleKey>
  iterator lower_bound(const CompatibleKey& x)const
  {
    return make_iterator(
      ordered_index_lower_bound(root(),header(),key,x,comp_));
  }

  template<typename CompatibleKey,typename CompatibleCompare>
  iterator lower_bound(
    const CompatibleKey& x,const CompatibleCompare& comp)const
  {
    return make_iterator(
      ordered_index_lower_bound(root(),header(),key,x,comp));
  }

  template<typename CompatibleKey>
  iterator upper_bound(const CompatibleKey& x)const
  {
    return make_iterator(
      ordered_index_upper_bound(root(),header(),key,x,comp_));
  }

  template<typename CompatibleKey,typename CompatibleCompare>
  iterator upper_bound(
    const CompatibleKey& x,const CompatibleCompare& comp)const
  {
    return make_iterator(
      ordered_index_upper_bound(root(),header(),key,x,comp));
  }

  template<typename CompatibleKey>
  std::pair<iterator,iterator> equal_range(
    const CompatibleKey& x)const
  {
    std::pair<index_node_type*,index_node_type*> p=
      ordered_index_equal_range(root(),header(),key,x,comp_);
    return std::pair<iterator,iterator>(
      make_iterator(p.first),make_iterator(p.second));
  }

  template<typename CompatibleKey,typename CompatibleCompare>
  std::pair<iterator,iterator> equal_range(
    const CompatibleKey& x,const CompatibleCompare& comp)const
  {
    std::pair<index_node_type*,index_node_type*> p=
      ordered_index_equal_range(root(),header(),key,x,comp);
    return std::pair<iterator,iterator>(
      make_iterator(p.first),make_iterator(p.second));
  }

  /* range */

  template<typename LowerBounder,typename UpperBounder>
  std::pair<iterator,iterator>
  range(LowerBounder lower,UpperBounder upper)const
  {
    typedef typename mpl::if_<
      is_same<LowerBounder,unbounded_type>,
      BOOST_DEDUCED_TYPENAME mpl::if_<
        is_same<UpperBounder,unbounded_type>,
        both_unbounded_tag,
        lower_unbounded_tag
      >::type,
      BOOST_DEDUCED_TYPENAME mpl::if_<
        is_same<UpperBounder,unbounded_type>,
        upper_unbounded_tag,
        none_unbounded_tag
      >::type
    >::type dispatch;

    return range(lower,upper,dispatch());
  }

BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS:
  ordered_index_impl(const ctor_args_list& args_list,const allocator_type& al):
    super(args_list.get_tail(),al),
    key(tuples::get<0>(args_list.get_head())),
    comp_(tuples::get<1>(args_list.get_head()))
  {
    empty_initialize();
  }

  ordered_index_impl(
    const ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x):
    super(x),

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super(),
#endif

    key(x.key),
    comp_(x.comp_)
  {
    /* Copy ctor just takes the key and compare objects from x. The rest is
     * done in a subsequent call to copy_().
     */
  }

  ordered_index_impl(
     const ordered_index_impl<
       KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x,
     do_not_copy_elements_tag):
    super(x,do_not_copy_elements_tag()),

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super(),
#endif

    key(x.key),
    comp_(x.comp_)
  {
    empty_initialize();
  }

  ~ordered_index_impl()
  {
    /* the container is guaranteed to be empty by now */
  }

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  iterator       make_iterator(index_node_type* node)
    {return iterator(node,this);}
  const_iterator make_iterator(index_node_type* node)const
    {return const_iterator(node,const_cast<ordered_index_impl*>(this));}
#else
  iterator       make_iterator(index_node_type* node){return iterator(node);}
  const_iterator make_iterator(index_node_type* node)const
                   {return const_iterator(node);}
#endif

  void copy_(
    const ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x,
    const copy_map_type& map)
  {
    if(!x.root()){
      empty_initialize();
    }
    else{
      header()->color()=x.header()->color();
      AugmentPolicy::copy(x.header()->impl(),header()->impl());

      index_node_type* root_cpy=map.find(
        static_cast<final_node_type*>(x.root()));
      header()->parent()=root_cpy->impl();

      index_node_type* leftmost_cpy=map.find(
        static_cast<final_node_type*>(x.leftmost()));
      header()->left()=leftmost_cpy->impl();

      index_node_type* rightmost_cpy=map.find(
        static_cast<final_node_type*>(x.rightmost()));
      header()->right()=rightmost_cpy->impl();

      typedef typename copy_map_type::const_iterator copy_map_iterator;
      for(copy_map_iterator it=map.begin(),it_end=map.end();it!=it_end;++it){
        index_node_type* org=it->first;
        index_node_type* cpy=it->second;

        cpy->color()=org->color();
        AugmentPolicy::copy(org->impl(),cpy->impl());

        node_impl_pointer parent_org=org->parent();
        if(parent_org==node_impl_pointer(0))cpy->parent()=node_impl_pointer(0);
        else{
          index_node_type* parent_cpy=map.find(
            static_cast<final_node_type*>(
              index_node_type::from_impl(parent_org)));
          cpy->parent()=parent_cpy->impl();
          if(parent_org->left()==org->impl()){
            parent_cpy->left()=cpy->impl();
          }
          else if(parent_org->right()==org->impl()){
            /* header() does not satisfy this nor the previous check */
            parent_cpy->right()=cpy->impl();
          }
        }

        if(org->left()==node_impl_pointer(0))
          cpy->left()=node_impl_pointer(0);
        if(org->right()==node_impl_pointer(0))
          cpy->right()=node_impl_pointer(0);
      }
    }
    
    super::copy_(x,map);
  }

  template<typename Variant>
  final_node_type* insert_(
    value_param_type v,final_node_type*& x,Variant variant)
  {
    link_info inf;
    if(!link_point(key(v),inf,Category())){
      return static_cast<final_node_type*>(
        index_node_type::from_impl(inf.pos));
    }

    final_node_type* res=super::insert_(v,x,variant);
    if(res==x){
      node_impl_type::link(
        static_cast<index_node_type*>(x)->impl(),
        inf.side,inf.pos,header()->impl());
    }
    return res;
  }

  template<typename Variant>
  final_node_type* insert_(
    value_param_type v,index_node_type* position,
    final_node_type*& x,Variant variant)
  {
    link_info inf;
    if(!hinted_link_point(key(v),position,inf,Category())){
      return static_cast<final_node_type*>(
        index_node_type::from_impl(inf.pos));
    }

    final_node_type* res=super::insert_(v,position,x,variant);
    if(res==x){
      node_impl_type::link(
        static_cast<index_node_type*>(x)->impl(),
        inf.side,inf.pos,header()->impl());
    }
    return res;
  }

  void extract_(index_node_type* x)
  {
    node_impl_type::rebalance_for_extract(
      x->impl(),header()->parent(),header()->left(),header()->right());
    super::extract_(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    detach_iterators(x);
#endif
  }

  void delete_all_nodes_()
  {
    delete_all_nodes(root());
  }

  void clear_()
  {
    super::clear_();
    empty_initialize();

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super::detach_dereferenceable_iterators();
#endif
  }

  template<typename BoolConstant>
  void swap_(
    ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x,
    BoolConstant swap_allocators)
  {
    adl_swap(key,x.key);
    adl_swap(comp_,x.comp_);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super::swap(x);
#endif

    super::swap_(x,swap_allocators);
  }

  void swap_elements_(
    ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x)
  {
#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super::swap(x);
#endif

    super::swap_elements_(x);
  }

  template<typename Variant>
  bool replace_(value_param_type v,index_node_type* x,Variant variant)
  {
    if(in_place(v,x,Category())){
      return super::replace_(v,x,variant);
    }

    index_node_type* next=x;
    index_node_type::increment(next);

    node_impl_type::rebalance_for_extract(
      x->impl(),header()->parent(),header()->left(),header()->right());

    BOOST_TRY{
      link_info inf;
      if(link_point(key(v),inf,Category())&&super::replace_(v,x,variant)){
        node_impl_type::link(x->impl(),inf.side,inf.pos,header()->impl());
        return true;
      }
      node_impl_type::restore(x->impl(),next->impl(),header()->impl());
      return false;
    }
    BOOST_CATCH(...){
      node_impl_type::restore(x->impl(),next->impl(),header()->impl());
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  bool modify_(index_node_type* x)
  {
    bool b;
    BOOST_TRY{
      b=in_place(x->value(),x,Category());
    }
    BOOST_CATCH(...){
      extract_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
    if(!b){
      node_impl_type::rebalance_for_extract(
        x->impl(),header()->parent(),header()->left(),header()->right());
      BOOST_TRY{
        link_info inf;
        if(!link_point(key(x->value()),inf,Category())){
          super::extract_(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
          detach_iterators(x);
#endif
          return false;
        }
        node_impl_type::link(x->impl(),inf.side,inf.pos,header()->impl());
      }
      BOOST_CATCH(...){
        super::extract_(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
        detach_iterators(x);
#endif

        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }

    BOOST_TRY{
      if(!super::modify_(x)){
        node_impl_type::rebalance_for_extract(
          x->impl(),header()->parent(),header()->left(),header()->right());

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
        detach_iterators(x);
#endif

        return false;
      }
      else return true;
    }
    BOOST_CATCH(...){
      node_impl_type::rebalance_for_extract(
        x->impl(),header()->parent(),header()->left(),header()->right());

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
      detach_iterators(x);
#endif

      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  bool modify_rollback_(index_node_type* x)
  {
    if(in_place(x->value(),x,Category())){
      return super::modify_rollback_(x);
    }

    index_node_type* next=x;
    index_node_type::increment(next);

    node_impl_type::rebalance_for_extract(
      x->impl(),header()->parent(),header()->left(),header()->right());

    BOOST_TRY{
      link_info inf;
      if(link_point(key(x->value()),inf,Category())&&
         super::modify_rollback_(x)){
        node_impl_type::link(x->impl(),inf.side,inf.pos,header()->impl());
        return true;
      }
      node_impl_type::restore(x->impl(),next->impl(),header()->impl());
      return false;
    }
    BOOST_CATCH(...){
      node_impl_type::restore(x->impl(),next->impl(),header()->impl());
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  bool check_rollback_(index_node_type* x)const
  {
    return in_place(x->value(),x,Category())&&super::check_rollback_(x);
  }

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  /* serialization */

  template<typename Archive>
  void save_(
    Archive& ar,const unsigned int version,const index_saver_type& sm)const
  {
    save_(ar,version,sm,Category());
  }

  template<typename Archive>
  void load_(Archive& ar,const unsigned int version,const index_loader_type& lm)
  {
    load_(ar,version,lm,Category());
  }
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
  /* invariant stuff */

  bool invariant_()const
  {
    if(size()==0||begin()==end()){
      if(size()!=0||begin()!=end()||
         header()->left()!=header()->impl()||
         header()->right()!=header()->impl())return false;
    }
    else{
      if((size_type)std::distance(begin(),end())!=size())return false;

      std::size_t len=node_impl_type::black_count(
        leftmost()->impl(),root()->impl());
      for(const_iterator it=begin(),it_end=end();it!=it_end;++it){
        index_node_type* x=it.get_node();
        index_node_type* left_x=index_node_type::from_impl(x->left());
        index_node_type* right_x=index_node_type::from_impl(x->right());

        if(x->color()==red){
          if((left_x&&left_x->color()==red)||
             (right_x&&right_x->color()==red))return false;
        }
        if(left_x&&comp_(key(x->value()),key(left_x->value())))return false;
        if(right_x&&comp_(key(right_x->value()),key(x->value())))return false;
        if(!left_x&&!right_x&&
           node_impl_type::black_count(x->impl(),root()->impl())!=len)
          return false;
        if(!AugmentPolicy::invariant(x->impl()))return false;
      }
    
      if(leftmost()->impl()!=node_impl_type::minimum(root()->impl()))
        return false;
      if(rightmost()->impl()!=node_impl_type::maximum(root()->impl()))
        return false;
    }

    return super::invariant_();
  }

  
  /* This forwarding function eases things for the boost::mem_fn construct
   * in BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT. Actually,
   * final_check_invariant is already an inherited member function of
   * ordered_index_impl.
   */
  void check_invariant_()const{this->final_check_invariant_();}
#endif

protected: /* for the benefit of AugmentPolicy::augmented_interface */
  index_node_type* header()const
    {return this->final_header();}
  index_node_type* root()const
    {return index_node_type::from_impl(header()->parent());}
  index_node_type* leftmost()const
    {return index_node_type::from_impl(header()->left());}
  index_node_type* rightmost()const
    {return index_node_type::from_impl(header()->right());}

private:
  void empty_initialize()
  {
    header()->color()=red;
    /* used to distinguish header() from root, in iterator.operator++ */
    
    header()->parent()=node_impl_pointer(0);
    header()->left()=header()->impl();
    header()->right()=header()->impl();
  }

  struct link_info
  {
    /* coverity[uninit_ctor]: suppress warning */
    link_info():side(to_left){}

    ordered_index_side side;
    node_impl_pointer  pos;
  };

  bool link_point(key_param_type k,link_info& inf,ordered_unique_tag)
  {
    index_node_type* y=header();
    index_node_type* x=root();
    bool c=true;
    while(x){
      y=x;
      c=comp_(k,key(x->value()));
      x=index_node_type::from_impl(c?x->left():x->right());
    }
    index_node_type* yy=y;
    if(c){
      if(yy==leftmost()){
        inf.side=to_left;
        inf.pos=y->impl();
        return true;
      }
      else index_node_type::decrement(yy);
    }

    if(comp_(key(yy->value()),k)){
      inf.side=c?to_left:to_right;
      inf.pos=y->impl();
      return true;
    }
    else{
      inf.pos=yy->impl();
      return false;
    }
  }

  bool link_point(key_param_type k,link_info& inf,ordered_non_unique_tag)
  {
    index_node_type* y=header();
    index_node_type* x=root();
    bool c=true;
    while (x){
     y=x;
     c=comp_(k,key(x->value()));
     x=index_node_type::from_impl(c?x->left():x->right());
    }
    inf.side=c?to_left:to_right;
    inf.pos=y->impl();
    return true;
  }

  bool lower_link_point(key_param_type k,link_info& inf,ordered_non_unique_tag)
  {
    index_node_type* y=header();
    index_node_type* x=root();
    bool c=false;
    while (x){
     y=x;
     c=comp_(key(x->value()),k);
     x=index_node_type::from_impl(c?x->right():x->left());
    }
    inf.side=c?to_right:to_left;
    inf.pos=y->impl();
    return true;
  }

  bool hinted_link_point(
    key_param_type k,index_node_type* position,
    link_info& inf,ordered_unique_tag)
  {
    if(position->impl()==header()->left()){ 
      if(size()>0&&comp_(k,key(position->value()))){
        inf.side=to_left;
        inf.pos=position->impl();
        return true;
      }
      else return link_point(k,inf,ordered_unique_tag());
    } 
    else if(position==header()){ 
      if(comp_(key(rightmost()->value()),k)){
        inf.side=to_right;
        inf.pos=rightmost()->impl();
        return true;
      }
      else return link_point(k,inf,ordered_unique_tag());
    } 
    else{
      index_node_type* before=position;
      index_node_type::decrement(before);
      if(comp_(key(before->value()),k)&&comp_(k,key(position->value()))){
        if(before->right()==node_impl_pointer(0)){
          inf.side=to_right;
          inf.pos=before->impl();
          return true;
        }
        else{
          inf.side=to_left;
          inf.pos=position->impl();
          return true;
        }
      } 
      else return link_point(k,inf,ordered_unique_tag());
    }
  }

  bool hinted_link_point(
    key_param_type k,index_node_type* position,
    link_info& inf,ordered_non_unique_tag)
  {
    if(position->impl()==header()->left()){ 
      if(size()>0&&!comp_(key(position->value()),k)){
        inf.side=to_left;
        inf.pos=position->impl();
        return true;
      }
      else return lower_link_point(k,inf,ordered_non_unique_tag());
    } 
    else if(position==header()){
      if(!comp_(k,key(rightmost()->value()))){
        inf.side=to_right;
        inf.pos=rightmost()->impl();
        return true;
      }
      else return link_point(k,inf,ordered_non_unique_tag());
    } 
    else{
      index_node_type* before=position;
      index_node_type::decrement(before);
      if(!comp_(k,key(before->value()))){
        if(!comp_(key(position->value()),k)){
          if(before->right()==node_impl_pointer(0)){
            inf.side=to_right;
            inf.pos=before->impl();
            return true;
          }
          else{
            inf.side=to_left;
            inf.pos=position->impl();
            return true;
          }
        }
        else return lower_link_point(k,inf,ordered_non_unique_tag());
      } 
      else return link_point(k,inf,ordered_non_unique_tag());
    }
  }

  void delete_all_nodes(index_node_type* x)
  {
    if(!x)return;

    delete_all_nodes(index_node_type::from_impl(x->left()));
    delete_all_nodes(index_node_type::from_impl(x->right()));
    this->final_delete_node_(static_cast<final_node_type*>(x));
  }

  bool in_place(value_param_type v,index_node_type* x,ordered_unique_tag)const
  {
    index_node_type* y;
    if(x!=leftmost()){
      y=x;
      index_node_type::decrement(y);
      if(!comp_(key(y->value()),key(v)))return false;
    }

    y=x;
    index_node_type::increment(y);
    return y==header()||comp_(key(v),key(y->value()));
  }

  bool in_place(
    value_param_type v,index_node_type* x,ordered_non_unique_tag)const
  {
    index_node_type* y;
    if(x!=leftmost()){
      y=x;
      index_node_type::decrement(y);
      if(comp_(key(v),key(y->value())))return false;
    }

    y=x;
    index_node_type::increment(y);
    return y==header()||!comp_(key(y->value()),key(v));
  }

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  void detach_iterators(index_node_type* x)
  {
    iterator it=make_iterator(x);
    safe_mode::detach_equivalent_iterators(it);
  }
#endif

  template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  std::pair<iterator,bool> emplace_impl(BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool>p=
      this->final_emplace_(BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
    return std::pair<iterator,bool>(make_iterator(p.first),p.second);
  }

  template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  iterator emplace_hint_impl(
    iterator position,BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool>p=
      this->final_emplace_hint_(
        static_cast<final_node_type*>(position.get_node()),
        BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
    return make_iterator(p.first);
  }

  template<typename LowerBounder,typename UpperBounder>
  std::pair<iterator,iterator>
  range(LowerBounder lower,UpperBounder upper,none_unbounded_tag)const
  {
    index_node_type* y=header();
    index_node_type* z=root();

    while(z){
      if(!lower(key(z->value()))){
        z=index_node_type::from_impl(z->right());
      }
      else if(!upper(key(z->value()))){
        y=z;
        z=index_node_type::from_impl(z->left());
      }
      else{
        return std::pair<iterator,iterator>(
          make_iterator(
            lower_range(index_node_type::from_impl(z->left()),z,lower)),
          make_iterator(
            upper_range(index_node_type::from_impl(z->right()),y,upper)));
      }
    }

    return std::pair<iterator,iterator>(make_iterator(y),make_iterator(y));
  }

  template<typename LowerBounder,typename UpperBounder>
  std::pair<iterator,iterator>
  range(LowerBounder,UpperBounder upper,lower_unbounded_tag)const
  {
    return std::pair<iterator,iterator>(
      begin(),
      make_iterator(upper_range(root(),header(),upper)));
  }

  template<typename LowerBounder,typename UpperBounder>
  std::pair<iterator,iterator>
  range(LowerBounder lower,UpperBounder,upper_unbounded_tag)const
  {
    return std::pair<iterator,iterator>(
      make_iterator(lower_range(root(),header(),lower)),
      end());
  }

  template<typename LowerBounder,typename UpperBounder>
  std::pair<iterator,iterator>
  range(LowerBounder,UpperBounder,both_unbounded_tag)const
  {
    return std::pair<iterator,iterator>(begin(),end());
  }

  template<typename LowerBounder>
  index_node_type * lower_range(
    index_node_type* top,index_node_type* y,LowerBounder lower)const
  {
    while(top){
      if(lower(key(top->value()))){
        y=top;
        top=index_node_type::from_impl(top->left());
      }
      else top=index_node_type::from_impl(top->right());
    }

    return y;
  }

  template<typename UpperBounder>
  index_node_type * upper_range(
    index_node_type* top,index_node_type* y,UpperBounder upper)const
  {
    while(top){
      if(!upper(key(top->value()))){
        y=top;
        top=index_node_type::from_impl(top->left());
      }
      else top=index_node_type::from_impl(top->right());
    }

    return y;
  }

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  template<typename Archive>
  void save_(
    Archive& ar,const unsigned int version,const index_saver_type& sm,
    ordered_unique_tag)const
  {
    super::save_(ar,version,sm);
  }

  template<typename Archive>
  void load_(
    Archive& ar,const unsigned int version,const index_loader_type& lm,
    ordered_unique_tag)
  {
    super::load_(ar,version,lm);
  }

  template<typename Archive>
  void save_(
    Archive& ar,const unsigned int version,const index_saver_type& sm,
    ordered_non_unique_tag)const
  {
    typedef duplicates_iterator<index_node_type,value_compare> dup_iterator;

    sm.save(
      dup_iterator(begin().get_node(),end().get_node(),value_comp()),
      dup_iterator(end().get_node(),value_comp()),
      ar,version);
    super::save_(ar,version,sm);
  }

  template<typename Archive>
  void load_(
    Archive& ar,const unsigned int version,const index_loader_type& lm,
    ordered_non_unique_tag)
  {
    lm.load(
      ::boost::bind(
        &ordered_index_impl::rearranger,this,
        ::boost::arg<1>(),::boost::arg<2>()),
      ar,version);
    super::load_(ar,version,lm);
  }

  void rearranger(index_node_type* position,index_node_type *x)
  {
    if(!position||comp_(key(position->value()),key(x->value()))){
      position=lower_bound(key(x->value())).get_node();
    }
    else if(comp_(key(x->value()),key(position->value()))){
      /* inconsistent rearrangement */
      throw_exception(
        archive::archive_exception(
          archive::archive_exception::other_exception));
    }
    else index_node_type::increment(position);

    if(position!=x){
      node_impl_type::rebalance_for_extract(
        x->impl(),header()->parent(),header()->left(),header()->right());
      node_impl_type::restore(
        x->impl(),position->impl(),header()->impl());
    }
  }
#endif /* serialization */

protected: /* for the benefit of AugmentPolicy::augmented_interface */
  key_from_value key;
  key_compare    comp_;

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_mfunc_templ reset
#endif
};

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
class ordered_index:
  public AugmentPolicy::template augmented_interface<
    ordered_index_impl<
      KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy
    >
  >::type
{
  typedef typename AugmentPolicy::template
    augmented_interface<
      ordered_index_impl<
        KeyFromValue,Compare,
        SuperMeta,TagList,Category,AugmentPolicy
      >
    >::type                                       super;
public:
  typedef typename super::ctor_args_list          ctor_args_list;
  typedef typename super::allocator_type          allocator_type;
  typedef typename super::iterator                iterator;

  /* construct/copy/destroy
   * Default and copy ctors are in the protected section as indices are
   * not supposed to be created on their own. No range ctor either.
   */

  ordered_index& operator=(const ordered_index& x)
  {
    this->final()=x.final();
    return *this;
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  ordered_index& operator=(
    std::initializer_list<BOOST_DEDUCED_TYPENAME super::value_type> list)
  {
    this->final()=list;
    return *this;
  }
#endif

protected:
  ordered_index(
    const ctor_args_list& args_list,const allocator_type& al):
    super(args_list,al){}

  ordered_index(const ordered_index& x):super(x){}

  ordered_index(const ordered_index& x,do_not_copy_elements_tag):
    super(x,do_not_copy_elements_tag()){}
};

/* comparison */

template<
  typename KeyFromValue1,typename Compare1,
  typename SuperMeta1,typename TagList1,typename Category1,
  typename AugmentPolicy1,
  typename KeyFromValue2,typename Compare2,
  typename SuperMeta2,typename TagList2,typename Category2,
  typename AugmentPolicy2
>
bool operator==(
  const ordered_index<
    KeyFromValue1,Compare1,SuperMeta1,TagList1,Category1,AugmentPolicy1>& x,
  const ordered_index<
    KeyFromValue2,Compare2,SuperMeta2,TagList2,Category2,AugmentPolicy2>& y)
{
  return x.size()==y.size()&&std::equal(x.begin(),x.end(),y.begin());
}

template<
  typename KeyFromValue1,typename Compare1,
  typename SuperMeta1,typename TagList1,typename Category1,
  typename AugmentPolicy1,
  typename KeyFromValue2,typename Compare2,
  typename SuperMeta2,typename TagList2,typename Category2,
  typename AugmentPolicy2
>
bool operator<(
  const ordered_index<
    KeyFromValue1,Compare1,SuperMeta1,TagList1,Category1,AugmentPolicy1>& x,
  const ordered_index<
    KeyFromValue2,Compare2,SuperMeta2,TagList2,Category2,AugmentPolicy2>& y)
{
  return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());
}

template<
  typename KeyFromValue1,typename Compare1,
  typename SuperMeta1,typename TagList1,typename Category1,
  typename AugmentPolicy1,
  typename KeyFromValue2,typename Compare2,
  typename SuperMeta2,typename TagList2,typename Category2,
  typename AugmentPolicy2
>
bool operator!=(
  const ordered_index<
    KeyFromValue1,Compare1,SuperMeta1,TagList1,Category1,AugmentPolicy1>& x,
  const ordered_index<
    KeyFromValue2,Compare2,SuperMeta2,TagList2,Category2,AugmentPolicy2>& y)
{
  return !(x==y);
}

template<
  typename KeyFromValue1,typename Compare1,
  typename SuperMeta1,typename TagList1,typename Category1,
  typename AugmentPolicy1,
  typename KeyFromValue2,typename Compare2,
  typename SuperMeta2,typename TagList2,typename Category2,
  typename AugmentPolicy2
>
bool operator>(
  const ordered_index<
    KeyFromValue1,Compare1,SuperMeta1,TagList1,Category1,AugmentPolicy1>& x,
  const ordered_index<
    KeyFromValue2,Compare2,SuperMeta2,TagList2,Category2,AugmentPolicy2>& y)
{
  return y<x;
}

template<
  typename KeyFromValue1,typename Compare1,
  typename SuperMeta1,typename TagList1,typename Category1,
  typename AugmentPolicy1,
  typename KeyFromValue2,typename Compare2,
  typename SuperMeta2,typename TagList2,typename Category2,
  typename AugmentPolicy2
>
bool operator>=(
  const ordered_index<
    KeyFromValue1,Compare1,SuperMeta1,TagList1,Category1,AugmentPolicy1>& x,
  const ordered_index<
    KeyFromValue2,Compare2,SuperMeta2,TagList2,Category2,AugmentPolicy2>& y)
{
  return !(x<y);
}

template<
  typename KeyFromValue1,typename Compare1,
  typename SuperMeta1,typename TagList1,typename Category1,
  typename AugmentPolicy1,
  typename KeyFromValue2,typename Compare2,
  typename SuperMeta2,typename TagList2,typename Category2,
  typename AugmentPolicy2
>
bool operator<=(
  const ordered_index<
    KeyFromValue1,Compare1,SuperMeta1,TagList1,Category1,AugmentPolicy1>& x,
  const ordered_index<
    KeyFromValue2,Compare2,SuperMeta2,TagList2,Category2,AugmentPolicy2>& y)
{
  return !(x>y);
}

/*  specialized algorithms */

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
void swap(
  ordered_index<
    KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& x,
  ordered_index<
    KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>& y)
{
  x.swap(y);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

/* Boost.Foreach compatibility */

template<
  typename KeyFromValue,typename Compare,
  typename SuperMeta,typename TagList,typename Category,typename AugmentPolicy
>
inline boost::mpl::true_* boost_foreach_is_noncopyable(
  boost::multi_index::detail::ordered_index<
    KeyFromValue,Compare,SuperMeta,TagList,Category,AugmentPolicy>*&,
  boost_foreach_argument_dependent_lookup_hack)
{
  return 0;
}

#undef BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT
#undef BOOST_MULTI_INDEX_ORD_INDEX_CHECK_INVARIANT_OF

#endif
