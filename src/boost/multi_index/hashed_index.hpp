/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_HASHED_INDEX_HPP
#define BOOST_MULTI_INDEX_HASHED_INDEX_HPP

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
#include <boost/limits.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/multi_index/detail/access_specifier.hpp>
#include <boost/multi_index/detail/adl_swap.hpp>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/auto_space.hpp>
#include <boost/multi_index/detail/bucket_array.hpp>
#include <boost/multi_index/detail/do_not_copy_elements_tag.hpp>
#include <boost/multi_index/detail/hash_index_iterator.hpp>
#include <boost/multi_index/detail/index_node_base.hpp>
#include <boost/multi_index/detail/modify_key_adaptor.hpp>
#include <boost/multi_index/detail/node_handle.hpp>
#include <boost/multi_index/detail/promotes_arg.hpp>
#include <boost/multi_index/detail/safe_mode.hpp>
#include <boost/multi_index/detail/scope_guard.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/serialization/nvp.hpp>
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
#define BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT_OF(x)                 \
  detail::scope_guard BOOST_JOIN(check_invariant_,__LINE__)=                 \
    detail::make_obj_guard(x,&hashed_index::check_invariant_);               \
  BOOST_JOIN(check_invariant_,__LINE__).touch();
#define BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT                       \
  BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT_OF(*this)
#else
#define BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT_OF(x)
#define BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT
#endif

namespace boost{

namespace multi_index{

namespace detail{

/* hashed_index adds a layer of hashed indexing to a given Super */

/* Most of the implementation of unique and non-unique indices is
 * shared. We tell from one another on instantiation time by using
 * Category tags defined in hash_index_node.hpp.
 */

template<
  typename KeyFromValue,typename Hash,typename Pred,
  typename SuperMeta,typename TagList,typename Category
>
class hashed_index:
  BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS SuperMeta::type

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  ,public safe_mode::safe_container<
    hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category> >
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

  typedef typename SuperMeta::type               super;

protected:
  typedef hashed_index_node<
    typename super::index_node_type>             index_node_type;

private:
  typedef typename index_node_type::
    template node_alg<Category>::type            node_alg;
  typedef typename index_node_type::impl_type    node_impl_type;
  typedef typename node_impl_type::pointer       node_impl_pointer;
  typedef typename node_impl_type::base_pointer  node_impl_base_pointer;
  typedef bucket_array<
    typename super::final_allocator_type>        bucket_array_type;

public:
  /* types */

  typedef typename KeyFromValue::result_type     key_type;
  typedef typename index_node_type::value_type   value_type;
  typedef KeyFromValue                           key_from_value;
  typedef Hash                                   hasher;
  typedef Pred                                   key_equal;
  typedef typename super::final_allocator_type   allocator_type;

private:
  typedef allocator_traits<allocator_type>       alloc_traits;

public:
  typedef typename alloc_traits::pointer         pointer;
  typedef typename alloc_traits::const_pointer   const_pointer;
  typedef value_type&                            reference;
  typedef const value_type&                      const_reference;
  typedef typename alloc_traits::size_type       size_type;
  typedef typename alloc_traits::difference_type difference_type;
  typedef tuple<size_type,
    key_from_value,hasher,key_equal>             ctor_args;

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef safe_mode::safe_iterator<
    hashed_index_iterator<
      index_node_type,bucket_array_type,
      Category,
      hashed_index_global_iterator_tag>,
    hashed_index>                                iterator;
#else
  typedef hashed_index_iterator<
    index_node_type,bucket_array_type,
    Category,hashed_index_global_iterator_tag>   iterator;
#endif

  typedef iterator                               const_iterator;

  typedef hashed_index_iterator<
    index_node_type,bucket_array_type,
    Category,hashed_index_local_iterator_tag>    local_iterator;
  typedef local_iterator                         const_local_iterator;

  typedef typename super::final_node_handle_type node_type;
  typedef detail::insert_return_type<
    iterator,node_type>                          insert_return_type;
  typedef TagList                                tag_list;

protected:
  typedef typename super::final_node_type     final_node_type;
  typedef tuples::cons<
    ctor_args, 
    typename super::ctor_args_list>           ctor_args_list;
  typedef typename mpl::push_front<
    typename super::index_type_list,
    hashed_index>::type                       index_type_list;
  typedef typename mpl::push_front<
    typename super::iterator_type_list,
    iterator>::type                           iterator_type_list;
  typedef typename mpl::push_front<
    typename super::const_iterator_type_list,
    const_iterator>::type                     const_iterator_type_list;
  typedef typename super::copy_map_type       copy_map_type;

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  typedef typename super::index_saver_type    index_saver_type;
  typedef typename super::index_loader_type   index_loader_type;
#endif

private:
#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef safe_mode::safe_container<
    hashed_index>                             safe_super;
#endif

  typedef typename call_traits<value_type>::param_type value_param_type;
  typedef typename call_traits<
    key_type>::param_type                              key_param_type;

  /* Needed to avoid commas in BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL
   * expansion.
   */

  typedef std::pair<iterator,bool>                     emplace_return_type;

public:

  /* construct/destroy/copy
   * Default and copy ctors are in the protected section as indices are
   * not supposed to be created on their own. No range ctor either.
   */

  hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& operator=(
    const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x)
  {
    this->final()=x.final();
    return *this;
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& operator=(
    std::initializer_list<value_type> list)
  {
    this->final()=list;
    return *this;
  }
#endif

  allocator_type get_allocator()const BOOST_NOEXCEPT
  {
    return this->final().get_allocator();
  }

  /* size and capacity */

  bool      empty()const BOOST_NOEXCEPT{return this->final_empty_();}
  size_type size()const BOOST_NOEXCEPT{return this->final_size_();}
  size_type max_size()const BOOST_NOEXCEPT{return this->final_max_size_();}

  /* iterators */

  iterator begin()BOOST_NOEXCEPT
  {
    return make_iterator(
      index_node_type::from_impl(header()->next()->prior()));
  
  }
  const_iterator begin()const BOOST_NOEXCEPT
  {
    return make_iterator(
      index_node_type::from_impl(header()->next()->prior()));
  }

  iterator       end()BOOST_NOEXCEPT{return make_iterator(header());}
  const_iterator end()const BOOST_NOEXCEPT{return make_iterator(header());}
  const_iterator cbegin()const BOOST_NOEXCEPT{return begin();}
  const_iterator cend()const BOOST_NOEXCEPT{return end();}

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

  /* modifiers */

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL(
    emplace_return_type,emplace,emplace_impl)

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG(
    iterator,emplace_hint,emplace_hint_impl,iterator,position)

  std::pair<iterator,bool> insert(const value_type& x)
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_(x);
    return std::pair<iterator,bool>(make_iterator(p.first),p.second);
  }

  std::pair<iterator,bool> insert(BOOST_RV_REF(value_type) x)
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_rv_(x);
    return std::pair<iterator,bool>(make_iterator(p.first),p.second);
  }

  iterator insert(iterator position,const value_type& x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_(
      x,static_cast<final_node_type*>(position.get_node()));
    return make_iterator(p.first);
  }
    
  iterator insert(iterator position,BOOST_RV_REF(value_type) x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_rv_(
      x,static_cast<final_node_type*>(position.get_node()));
    return make_iterator(p.first);
  }

  template<typename InputIterator>
  void insert(InputIterator first,InputIterator last)
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    for(;first!=last;++first)this->final_insert_ref_(*first);
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
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_nh_(nh);
    return insert_return_type(make_iterator(p.first),p.second,boost::move(nh));
  }

  iterator insert(const_iterator position,BOOST_RV_REF(node_type) nh)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    if(nh)BOOST_MULTI_INDEX_CHECK_EQUAL_ALLOCATORS(*this,nh);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool> p=this->final_insert_nh_(
      nh,static_cast<final_node_type*>(position.get_node()));
    return make_iterator(p.first);
  }

  node_type extract(const_iterator position)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    return this->final_extract_(
      static_cast<final_node_type*>(position.get_node()));
  }

  node_type extract(key_param_type x)
  {
    iterator position=find(x);
    if(position==end())return node_type();
    else return extract(position);
  }

  iterator erase(iterator position)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    this->final_erase_(static_cast<final_node_type*>(position++.get_node()));
    return position;
  }

  size_type erase(key_param_type k)
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;

    std::size_t buc=buckets.position(hash_(k));
    for(node_impl_pointer x=buckets.at(buc)->prior();
        x!=node_impl_pointer(0);x=node_alg::next_to_inspect(x)){
      if(eq_(k,key(index_node_type::from_impl(x)->value()))){
        node_impl_pointer y=end_of_range(x);
        size_type         s=0;
        do{
          node_impl_pointer z=node_alg::after(x);
          this->final_erase_(
            static_cast<final_node_type*>(index_node_type::from_impl(x)));
          x=z;
          ++s;
        }while(x!=y);
        return s;
      }
    }
    return 0;
  }

  iterator erase(iterator first,iterator last)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(first);
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(last);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(first,*this);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(last,*this);
    BOOST_MULTI_INDEX_CHECK_VALID_RANGE(first,last);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
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
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    return this->final_replace_(
      x,static_cast<final_node_type*>(position.get_node()));
  }

  bool replace(iterator position,BOOST_RV_REF(value_type) x)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    return this->final_replace_rv_(
      x,static_cast<final_node_type*>(position.get_node()));
  }

  template<typename Modifier>
  bool modify(iterator position,Modifier mod)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;

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
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;

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
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    return modify(
      position,modify_key_adaptor<Modifier,value_type,KeyFromValue>(mod,key));
  }

  template<typename Modifier,typename Rollback>
  bool modify_key(iterator position,Modifier mod,Rollback back_)
  {
    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_DEREFERENCEABLE_ITERATOR(position);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(position,*this);
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    return modify(
      position,
      modify_key_adaptor<Modifier,value_type,KeyFromValue>(mod,key),
      modify_key_adaptor<Rollback,value_type,KeyFromValue>(back_,key));
  }

  void clear()BOOST_NOEXCEPT
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    this->final_clear_();
  }

  void swap(hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x)
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT_OF(x);
    this->final_swap_(x.final());
  }

  /* observers */

  key_from_value key_extractor()const{return key;}
  hasher         hash_function()const{return hash_;}
  key_equal      key_eq()const{return eq_;}
  
  /* lookup */

  /* Internally, these ops rely on const_iterator being the same
   * type as iterator.
   */

  /* Implementation note: When CompatibleKey is consistently promoted to
   * KeyFromValue::result_type for equality comparison, the promotion is made
   * once in advance to increase efficiency.
   */

  template<typename CompatibleKey>
  iterator find(const CompatibleKey& k)const
  {
    return find(k,hash_,eq_);
  }

  template<
    typename CompatibleKey,typename CompatibleHash,typename CompatiblePred
  >
  iterator find(
    const CompatibleKey& k,
    const CompatibleHash& hash,const CompatiblePred& eq)const
  {
    return find(
      k,hash,eq,promotes_1st_arg<CompatiblePred,CompatibleKey,key_type>());
  }

  template<typename CompatibleKey>
  size_type count(const CompatibleKey& k)const
  {
    return count(k,hash_,eq_);
  }

  template<
    typename CompatibleKey,typename CompatibleHash,typename CompatiblePred
  >
  size_type count(
    const CompatibleKey& k,
    const CompatibleHash& hash,const CompatiblePred& eq)const
  {
    return count(
      k,hash,eq,promotes_1st_arg<CompatiblePred,CompatibleKey,key_type>());
  }

  template<typename CompatibleKey>
  std::pair<iterator,iterator> equal_range(const CompatibleKey& k)const
  {
    return equal_range(k,hash_,eq_);
  }

  template<
    typename CompatibleKey,typename CompatibleHash,typename CompatiblePred
  >
  std::pair<iterator,iterator> equal_range(
    const CompatibleKey& k,
    const CompatibleHash& hash,const CompatiblePred& eq)const
  {
    return equal_range(
      k,hash,eq,promotes_1st_arg<CompatiblePred,CompatibleKey,key_type>());
  }

  /* bucket interface */

  size_type bucket_count()const BOOST_NOEXCEPT
  {
    return static_cast<size_type>(buckets.size());
  }

  size_type max_bucket_count()const BOOST_NOEXCEPT{return static_cast<size_type>(-1);}

  size_type bucket_size(size_type n)const
  {
    size_type res=0;
    for(node_impl_pointer x=buckets.at(n)->prior();
        x!=node_impl_pointer(0);x=node_alg::after_local(x)){
      ++res;
    }
    return res;
  }

  size_type bucket(key_param_type k)const
  {
    return static_cast<size_type>(buckets.position(hash_(k)));
  }

  local_iterator begin(size_type n)
  {
    return const_cast<const hashed_index*>(this)->begin(n);
  }

  const_local_iterator begin(size_type n)const
  {
    node_impl_pointer x=buckets.at(n)->prior();
    if(x==node_impl_pointer(0))return end(n);
    return make_local_iterator(index_node_type::from_impl(x));
  }

  local_iterator end(size_type n)
  {
    return const_cast<const hashed_index*>(this)->end(n);
  }

  const_local_iterator end(size_type)const
  {
    return make_local_iterator(0);
  }

  const_local_iterator cbegin(size_type n)const{return begin(n);}
  const_local_iterator cend(size_type n)const{return end(n);}

  local_iterator local_iterator_to(const value_type& x)
  {
    return make_local_iterator(
      node_from_value<index_node_type>(boost::addressof(x)));
  }

  const_local_iterator local_iterator_to(const value_type& x)const
  {
    return make_local_iterator(
      node_from_value<index_node_type>(boost::addressof(x)));
  }

  /* hash policy */

  float load_factor()const BOOST_NOEXCEPT
    {return static_cast<float>(size())/bucket_count();}
  float max_load_factor()const BOOST_NOEXCEPT{return mlf;}
  void  max_load_factor(float z){mlf=z;calculate_max_load();}

  void rehash(size_type n)
  {
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    if(size()<=max_load&&n<=bucket_count())return;

    size_type bc =(std::numeric_limits<size_type>::max)();
    float     fbc=1.0f+static_cast<float>(size())/mlf;
    if(bc>fbc){
      bc=static_cast<size_type>(fbc);
      if(bc<n)bc=n;
    }
    unchecked_rehash(bc);
  }

  void reserve(size_type n)
  {
    rehash(static_cast<size_type>(std::ceil(static_cast<float>(n)/mlf)));
  }

BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS:
  hashed_index(const ctor_args_list& args_list,const allocator_type& al):
    super(args_list.get_tail(),al),
    key(tuples::get<1>(args_list.get_head())),
    hash_(tuples::get<2>(args_list.get_head())),
    eq_(tuples::get<3>(args_list.get_head())),
    buckets(al,header()->impl(),tuples::get<0>(args_list.get_head())),
    mlf(1.0f)
  {
    calculate_max_load();
  }

  hashed_index(
    const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x):
    super(x),

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super(),
#endif

    key(x.key),
    hash_(x.hash_),
    eq_(x.eq_),
    buckets(x.get_allocator(),header()->impl(),x.buckets.size()),
    mlf(x.mlf),
    max_load(x.max_load)
  {
    /* Copy ctor just takes the internal configuration objects from x. The rest
     * is done in subsequent call to copy_().
     */
  }

  hashed_index(
    const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
    do_not_copy_elements_tag):
    super(x,do_not_copy_elements_tag()),

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super(),
#endif

    key(x.key),
    hash_(x.hash_),
    eq_(x.eq_),
    buckets(x.get_allocator(),header()->impl(),0),
    mlf(1.0f)
  {
     calculate_max_load();
  }

  ~hashed_index()
  {
    /* the container is guaranteed to be empty by now */
  }

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  iterator make_iterator(index_node_type* node)
  {
    return iterator(node,this);
  }

  const_iterator make_iterator(index_node_type* node)const
  {
    return const_iterator(node,const_cast<hashed_index*>(this));
  }
#else
  iterator make_iterator(index_node_type* node)
  {
    return iterator(node);
  }

  const_iterator make_iterator(index_node_type* node)const
  {
    return const_iterator(node);
  }
#endif

  local_iterator make_local_iterator(index_node_type* node)
  {
    return local_iterator(node);
  }

  const_local_iterator make_local_iterator(index_node_type* node)const
  {
    return const_local_iterator(node);
  }

  void copy_(
    const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
    const copy_map_type& map)
  {
    copy_(x,map,Category());
  }

  void copy_(
    const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
    const copy_map_type& map,hashed_unique_tag)
  {
    if(x.size()!=0){
      node_impl_pointer end_org=x.header()->impl(),
                        org=end_org,
                        cpy=header()->impl();
      do{
        node_impl_pointer prev_org=org->prior(),
                          prev_cpy=
          static_cast<index_node_type*>(map.find(static_cast<final_node_type*>(
            index_node_type::from_impl(prev_org))))->impl();
        cpy->prior()=prev_cpy;
        if(node_alg::is_first_of_bucket(org)){
          node_impl_base_pointer buc_org=prev_org->next(),
                                 buc_cpy=
            buckets.begin()+(buc_org-x.buckets.begin());
          prev_cpy->next()=buc_cpy;
          buc_cpy->prior()=cpy;
        }
        else{
          prev_cpy->next()=node_impl_type::base_pointer_from(cpy);
        }
        org=prev_org;
        cpy=prev_cpy;
      }while(org!=end_org);
    }

    super::copy_(x,map);
  }
  
  void copy_(
    const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
    const copy_map_type& map,hashed_non_unique_tag)
  {
    if(x.size()!=0){
      node_impl_pointer end_org=x.header()->impl(),
                        org=end_org,
                        cpy=header()->impl();
      do{
        node_impl_pointer next_org=node_alg::after(org),
                          next_cpy=
          static_cast<index_node_type*>(map.find(static_cast<final_node_type*>(
            index_node_type::from_impl(next_org))))->impl();
        if(node_alg::is_first_of_bucket(next_org)){
          node_impl_base_pointer buc_org=org->next(),
                                 buc_cpy=
            buckets.begin()+(buc_org-x.buckets.begin());
          cpy->next()=buc_cpy;
          buc_cpy->prior()=next_cpy;
          next_cpy->prior()=cpy;
        }
        else{
          if(org->next()==node_impl_type::base_pointer_from(next_org)){
            cpy->next()=node_impl_type::base_pointer_from(next_cpy);
          }
          else{
            cpy->next()=
              node_impl_type::base_pointer_from(
                static_cast<index_node_type*>(
                  map.find(static_cast<final_node_type*>(
                    index_node_type::from_impl(
                      node_impl_type::pointer_from(org->next())))))->impl());
          }

          if(next_org->prior()!=org){
            next_cpy->prior()=
              static_cast<index_node_type*>(
                map.find(static_cast<final_node_type*>(
                  index_node_type::from_impl(next_org->prior()))))->impl();
          }
          else{
            next_cpy->prior()=cpy;
          }
        }
        org=next_org;
        cpy=next_cpy;
      }while(org!=end_org);
    }

    super::copy_(x,map);
  }

  template<typename Variant>
  final_node_type* insert_(
    value_param_type v,final_node_type*& x,Variant variant)
  {
    reserve_for_insert(size()+1);

    std::size_t buc=find_bucket(v);
    link_info   pos(buckets.at(buc));
    if(!link_point(v,pos)){
      return static_cast<final_node_type*>(
        index_node_type::from_impl(node_impl_type::pointer_from(pos)));
    }

    final_node_type* res=super::insert_(v,x,variant);
    if(res==x)link(static_cast<index_node_type*>(x),pos);
    return res;
  }

  template<typename Variant>
  final_node_type* insert_(
    value_param_type v,index_node_type* position,
    final_node_type*& x,Variant variant)
  {
    reserve_for_insert(size()+1);

    std::size_t buc=find_bucket(v);
    link_info   pos(buckets.at(buc));
    if(!link_point(v,pos)){
      return static_cast<final_node_type*>(
        index_node_type::from_impl(node_impl_type::pointer_from(pos)));
    }

    final_node_type* res=super::insert_(v,position,x,variant);
    if(res==x)link(static_cast<index_node_type*>(x),pos);
    return res;
  }

  void extract_(index_node_type* x)
  {
    unlink(x);
    super::extract_(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    detach_iterators(x);
#endif
  }

  void delete_all_nodes_()
  {
    delete_all_nodes_(Category());
  }

  void delete_all_nodes_(hashed_unique_tag)
  {
    for(node_impl_pointer x_end=header()->impl(),x=x_end->prior();x!=x_end;){
      node_impl_pointer y=x->prior();
      this->final_delete_node_(
        static_cast<final_node_type*>(index_node_type::from_impl(x)));
      x=y;
    }
  }

  void delete_all_nodes_(hashed_non_unique_tag)
  {
    for(node_impl_pointer x_end=header()->impl(),x=x_end->prior();x!=x_end;){
      node_impl_pointer y=x->prior();
      if(y->next()!=node_impl_type::base_pointer_from(x)&&
         y->next()->prior()!=x){ /* n-1 of group */
        /* Make the second node prior() pointer back-linked so that it won't
         * refer to a deleted node when the time for its own destruction comes.
         */

        node_impl_pointer first=node_impl_type::pointer_from(y->next());
        first->next()->prior()=first;
      }
      this->final_delete_node_(
        static_cast<final_node_type*>(index_node_type::from_impl(x)));
      x=y;
    }
  }

  void clear_()
  {
    super::clear_();
    buckets.clear(header()->impl());

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super::detach_dereferenceable_iterators();
#endif
  }

  template<typename BoolConstant>
  void swap_(
    hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
    BoolConstant swap_allocators)
  {
    adl_swap(key,x.key);
    adl_swap(hash_,x.hash_);
    adl_swap(eq_,x.eq_);
    buckets.swap(x.buckets,swap_allocators);
    std::swap(mlf,x.mlf);
    std::swap(max_load,x.max_load);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super::swap(x);
#endif

    super::swap_(x,swap_allocators);
  }

  void swap_elements_(
    hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x)
  {
    buckets.swap(x.buckets);
    std::swap(mlf,x.mlf);
    std::swap(max_load,x.max_load);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
    safe_super::swap(x);
#endif

    super::swap_elements_(x);
  }

  template<typename Variant>
  bool replace_(value_param_type v,index_node_type* x,Variant variant)
  {
    if(eq_(key(v),key(x->value()))){
      return super::replace_(v,x,variant);
    }
      
    unlink_undo undo;
    unlink(x,undo);

    BOOST_TRY{
      std::size_t  buc=find_bucket(v);
      link_info    pos(buckets.at(buc));
      if(link_point(v,pos)&&super::replace_(v,x,variant)){
        link(x,pos);
        return true;
      }
      undo();
      return false;
    }
    BOOST_CATCH(...){
      undo();
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  bool modify_(index_node_type* x)
  {
    std::size_t buc;
    bool        b; 
    BOOST_TRY{
      buc=find_bucket(x->value());
      b=in_place(x->impl(),key(x->value()),buc);
    }
    BOOST_CATCH(...){
      extract_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
    if(!b){
      unlink(x);
      BOOST_TRY{
        link_info pos(buckets.at(buc));
        if(!link_point(x->value(),pos)){
          super::extract_(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
          detach_iterators(x);
#endif
          return false;
        }
        link(x,pos);
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
        unlink(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
        detach_iterators(x);
#endif
        return false;
      }
      else return true;
    }
    BOOST_CATCH(...){
      unlink(x);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
      detach_iterators(x);
#endif

      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  bool modify_rollback_(index_node_type* x)
  {
    std::size_t buc=find_bucket(x->value());
    if(in_place(x->impl(),key(x->value()),buc)){
      return super::modify_rollback_(x);
    }

    unlink_undo undo;
    unlink(x,undo);

    BOOST_TRY{
      link_info pos(buckets.at(buc));
      if(link_point(x->value(),pos)&&super::modify_rollback_(x)){
        link(x,pos);
        return true;
      }
      undo();
      return false;
    }
    BOOST_CATCH(...){
      undo();
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  bool check_rollback_(index_node_type* x)const
  {
    std::size_t buc=find_bucket(x->value());
    return in_place(x->impl(),key(x->value()),buc)&&super::check_rollback_(x);
  }

  /* comparison */

#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
  /* defect macro refers to class, not function, templates, but anyway */

  template<typename K,typename H,typename P,typename S,typename T,typename C>
  friend bool operator==(
    const hashed_index<K,H,P,S,T,C>&,const hashed_index<K,H,P,S,T,C>& y);
#endif

  bool equals(const hashed_index& x)const{return equals(x,Category());}

  bool equals(const hashed_index& x,hashed_unique_tag)const
  {
    if(size()!=x.size())return false;
    for(const_iterator it=begin(),it_end=end(),it2_end=x.end();
        it!=it_end;++it){
      const_iterator it2=x.find(key(*it));
      if(it2==it2_end||!(*it==*it2))return false;
    }
    return true;
  }

  bool equals(const hashed_index& x,hashed_non_unique_tag)const
  {
    if(size()!=x.size())return false;
    for(const_iterator it=begin(),it_end=end();it!=it_end;){
      const_iterator it2,it2_last;
      boost::tie(it2,it2_last)=x.equal_range(key(*it));
      if(it2==it2_last)return false;

      const_iterator it_last=make_iterator(
        index_node_type::from_impl(end_of_range(it.get_node()->impl())));
      if(std::distance(it,it_last)!=std::distance(it2,it2_last))return false;

      /* From is_permutation code in
       * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3068.pdf
       */

      for(;it!=it_last;++it,++it2){
        if(!(*it==*it2))break;
      }
      if(it!=it_last){
        for(const_iterator scan=it;scan!=it_last;++scan){
          if(std::find(it,scan,*scan)!=scan)continue;
          difference_type matches=std::count(it2,it2_last,*scan);
          if(matches==0||matches!=std::count(scan,it_last,*scan))return false;
        }
        it=it_last;
      }
    }
    return true;
  }

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  /* serialization */

  template<typename Archive>
  void save_(
    Archive& ar,const unsigned int version,const index_saver_type& sm)const
  {
    ar<<serialization::make_nvp("position",buckets);
    super::save_(ar,version,sm);
  }

  template<typename Archive>
  void load_(Archive& ar,const unsigned int version,const index_loader_type& lm)
  {
    ar>>serialization::make_nvp("position",buckets);
    super::load_(ar,version,lm);
  }
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
  /* invariant stuff */

  bool invariant_()const
  {
    if(size()==0||begin()==end()){
      if(size()!=0||begin()!=end())return false;
    }
    else{
      size_type s0=0;
      for(const_iterator it=begin(),it_end=end();it!=it_end;++it,++s0){}
      if(s0!=size())return false;

      size_type s1=0;
      for(size_type buc=0;buc<bucket_count();++buc){
        size_type ss1=0;
        for(const_local_iterator it=begin(buc),it_end=end(buc);
            it!=it_end;++it,++ss1){
          if(find_bucket(*it)!=buc)return false;
        }
        if(ss1!=bucket_size(buc))return false;
        s1+=ss1;
      }
      if(s1!=size())return false;
    }

    return super::invariant_();
  }

  /* This forwarding function eases things for the boost::mem_fn construct
   * in BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT. Actually,
   * final_check_invariant is already an inherited member function of index.
   */
  void check_invariant_()const{this->final_check_invariant_();}
#endif

private:
  index_node_type* header()const{return this->final_header();}

  std::size_t find_bucket(value_param_type v)const
  {
    return bucket(key(v));
  }

  struct link_info_non_unique
  {
    link_info_non_unique(node_impl_base_pointer pos):
      first(pos),last(node_impl_base_pointer(0)){}

    operator const node_impl_base_pointer&()const{return this->first;}

    node_impl_base_pointer first,last;
  };

  typedef typename mpl::if_<
    is_same<Category,hashed_unique_tag>,
    node_impl_base_pointer,
    link_info_non_unique
  >::type                                link_info;

  bool link_point(value_param_type v,link_info& pos)
  {
    return link_point(v,pos,Category());
  }

  bool link_point(
    value_param_type v,node_impl_base_pointer& pos,hashed_unique_tag)
  {
    for(node_impl_pointer x=pos->prior();x!=node_impl_pointer(0);
        x=node_alg::after_local(x)){
      if(eq_(key(v),key(index_node_type::from_impl(x)->value()))){
        pos=node_impl_type::base_pointer_from(x);
        return false;
      }
    }
    return true;
  }

  bool link_point(
    value_param_type v,link_info_non_unique& pos,hashed_non_unique_tag)
  {
    for(node_impl_pointer x=pos.first->prior();x!=node_impl_pointer(0);
        x=node_alg::next_to_inspect(x)){
      if(eq_(key(v),key(index_node_type::from_impl(x)->value()))){
        pos.first=node_impl_type::base_pointer_from(x);
        pos.last=node_impl_type::base_pointer_from(last_of_range(x));
        return true;
      }
    }
    return true;
  }

  node_impl_pointer last_of_range(node_impl_pointer x)const
  {
    return last_of_range(x,Category());
  }

  node_impl_pointer last_of_range(node_impl_pointer x,hashed_unique_tag)const
  {
    return x;
  }

  node_impl_pointer last_of_range(
    node_impl_pointer x,hashed_non_unique_tag)const
  {
    node_impl_base_pointer y=x->next();
    node_impl_pointer      z=y->prior();
    if(z==x){                      /* range of size 1 or 2 */
      node_impl_pointer yy=node_impl_type::pointer_from(y);
      return
        eq_(
          key(index_node_type::from_impl(x)->value()),
          key(index_node_type::from_impl(yy)->value()))?yy:x;
    }
    else if(z->prior()==x)               /* last of bucket */
      return x;
    else                                /* group of size>2 */        
      return z;
  }

  node_impl_pointer end_of_range(node_impl_pointer x)const
  {
    return end_of_range(x,Category());
  }

  node_impl_pointer end_of_range(node_impl_pointer x,hashed_unique_tag)const
  {
    return node_alg::after(last_of_range(x));
  }

  node_impl_pointer end_of_range(
    node_impl_pointer x,hashed_non_unique_tag)const
  {
    node_impl_base_pointer y=x->next();
    node_impl_pointer      z=y->prior();
    if(z==x){                      /* range of size 1 or 2 */
      node_impl_pointer yy=node_impl_type::pointer_from(y);
      if(!eq_(
           key(index_node_type::from_impl(x)->value()),
           key(index_node_type::from_impl(yy)->value())))yy=x;
      return yy->next()->prior()==yy?
               node_impl_type::pointer_from(yy->next()):
               yy->next()->prior();
    }
    else if(z->prior()==x)               /* last of bucket */
      return z;
    else                                /* group of size>2 */        
      return z->next()->prior()==z?
               node_impl_type::pointer_from(z->next()):
               z->next()->prior();
  }

  void link(index_node_type* x,const link_info& pos)
  {
    link(x,pos,Category());
  }

  void link(index_node_type* x,node_impl_base_pointer pos,hashed_unique_tag)
  {
    node_alg::link(x->impl(),pos,header()->impl());
  }

  void link(
    index_node_type* x,const link_info_non_unique& pos,hashed_non_unique_tag)
  {
    if(pos.last==node_impl_base_pointer(0)){
      node_alg::link(x->impl(),pos.first,header()->impl());
    }
    else{
      node_alg::link(
        x->impl(),
        node_impl_type::pointer_from(pos.first),
        node_impl_type::pointer_from(pos.last));
    }
  }

  void unlink(index_node_type* x)
  {
    node_alg::unlink(x->impl());
  }

  typedef typename node_alg::unlink_undo unlink_undo;

  void unlink(index_node_type* x,unlink_undo& undo)
  {
    node_alg::unlink(x->impl(),undo);
  }

  void calculate_max_load()
  {
    float fml=mlf*static_cast<float>(bucket_count());
    max_load=(std::numeric_limits<size_type>::max)();
    if(max_load>fml)max_load=static_cast<size_type>(fml);
  }

  void reserve_for_insert(size_type n)
  {
    if(n>max_load){
      size_type bc =(std::numeric_limits<size_type>::max)();
      float     fbc=1.0f+static_cast<float>(n)/mlf;
      if(bc>fbc)bc =static_cast<size_type>(fbc);
      unchecked_rehash(bc);
    }
  }

  void unchecked_rehash(size_type n){unchecked_rehash(n,Category());}

  void unchecked_rehash(size_type n,hashed_unique_tag)
  {
    node_impl_type    cpy_end_node;
    node_impl_pointer cpy_end=node_impl_pointer(&cpy_end_node),
                      end_=header()->impl();
    bucket_array_type buckets_cpy(get_allocator(),cpy_end,n);

    if(size()!=0){
      auto_space<
        std::size_t,allocator_type>       hashes(get_allocator(),size());
      auto_space<
        node_impl_pointer,allocator_type> node_ptrs(get_allocator(),size());
      std::size_t                         i=0,size_=size();
      bool                                within_bucket=false;
      BOOST_TRY{
        for(;i!=size_;++i){
          node_impl_pointer x=end_->prior();

          /* only this can possibly throw */
          std::size_t h=hash_(key(index_node_type::from_impl(x)->value()));

          hashes.data()[i]=h;
          node_ptrs.data()[i]=x;
          within_bucket=!node_alg::unlink_last(end_);
          node_alg::link(x,buckets_cpy.at(buckets_cpy.position(h)),cpy_end);
        }
      }
      BOOST_CATCH(...){
        if(i!=0){
          std::size_t prev_buc=buckets.position(hashes.data()[i-1]);
          if(!within_bucket)prev_buc=~prev_buc;

          for(std::size_t j=i;j--;){
            std::size_t       buc=buckets.position(hashes.data()[j]);
            node_impl_pointer x=node_ptrs.data()[j];
            if(buc==prev_buc)node_alg::append(x,end_);
            else node_alg::link(x,buckets.at(buc),end_);
            prev_buc=buc;
          }
        }
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }

    end_->prior()=cpy_end->prior()!=cpy_end?cpy_end->prior():end_;
    end_->next()=cpy_end->next();
    end_->prior()->next()->prior()=end_->next()->prior()->prior()=end_;
    buckets.swap(buckets_cpy);
    calculate_max_load();
  }

  void unchecked_rehash(size_type n,hashed_non_unique_tag)
  {
    node_impl_type    cpy_end_node;
    node_impl_pointer cpy_end=node_impl_pointer(&cpy_end_node),
                      end_=header()->impl();
    bucket_array_type buckets_cpy(get_allocator(),cpy_end,n);

    if(size()!=0){
      auto_space<
        std::size_t,allocator_type>       hashes(get_allocator(),size());
      auto_space<
        node_impl_pointer,allocator_type> node_ptrs(get_allocator(),size());
      std::size_t                         i=0;
      bool                                within_bucket=false;
      BOOST_TRY{
        for(;;++i){
          node_impl_pointer x=end_->prior();
          if(x==end_)break;

          /* only this can possibly throw */
          std::size_t h=hash_(key(index_node_type::from_impl(x)->value()));

          hashes.data()[i]=h;
          node_ptrs.data()[i]=x;
          std::pair<node_impl_pointer,bool> p=
            node_alg::unlink_last_group(end_);
          node_alg::link_range(
            p.first,x,buckets_cpy.at(buckets_cpy.position(h)),cpy_end);
          within_bucket=!(p.second);
        }
      }
      BOOST_CATCH(...){
        if(i!=0){
          std::size_t prev_buc=buckets.position(hashes.data()[i-1]);
          if(!within_bucket)prev_buc=~prev_buc;

          for(std::size_t j=i;j--;){
            std::size_t       buc=buckets.position(hashes.data()[j]);
            node_impl_pointer x=node_ptrs.data()[j],
                              y=
              x->prior()->next()!=node_impl_type::base_pointer_from(x)&&
              x->prior()->next()->prior()!=x?
                node_impl_type::pointer_from(x->prior()->next()):x;
            node_alg::unlink_range(y,x);
            if(buc==prev_buc)node_alg::append_range(y,x,end_);
            else node_alg::link_range(y,x,buckets.at(buc),end_);
            prev_buc=buc;
          }
        }
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }

    end_->prior()=cpy_end->prior()!=cpy_end?cpy_end->prior():end_;
    end_->next()=cpy_end->next();
    end_->prior()->next()->prior()=end_->next()->prior()->prior()=end_;
    buckets.swap(buckets_cpy);
    calculate_max_load();
  }

  bool in_place(node_impl_pointer x,key_param_type k,std::size_t buc)const
  {
    return in_place(x,k,buc,Category());
  }

  bool in_place(
    node_impl_pointer x,key_param_type k,std::size_t buc,
    hashed_unique_tag)const
  {
    bool found=false;
    for(node_impl_pointer y=buckets.at(buc)->prior();
        y!=node_impl_pointer(0);y=node_alg::after_local(y)){
      if(y==x)found=true;
      else if(eq_(k,key(index_node_type::from_impl(y)->value())))return false;
    }
    return found;
  }

  bool in_place(
    node_impl_pointer x,key_param_type k,std::size_t buc,
    hashed_non_unique_tag)const
  {
    bool found=false;
    int  range_size=0;
    for(node_impl_pointer y=buckets.at(buc)->prior();y!=node_impl_pointer(0);){
      if(node_alg::is_first_of_group(y)){ /* group of 3 or more */
        if(y==x){
          /* in place <-> equal to some other member of the group */
          return eq_(
            k,
            key(index_node_type::from_impl(
              node_impl_type::pointer_from(y->next()))->value()));
        }
        else{
          node_impl_pointer z=
            node_alg::after_local(y->next()->prior()); /* end of range */
          if(eq_(k,key(index_node_type::from_impl(y)->value()))){
            if(found)return false; /* x lies outside */
            do{
              if(y==x)return true;
              y=node_alg::after_local(y);
            }while(y!=z);
            return false; /* x not found */
          }
          else{
            if(range_size==1&&!found)return false;
            if(range_size==2)return found;
            range_size=0;
            y=z; /* skip range (and potentially x, too, which is fine) */
          }
        }
      }
      else{ /* group of 1 or 2 */
        if(y==x){
          if(range_size==1)return true;
          range_size=1;
          found=true;
        }
        else if(eq_(k,key(index_node_type::from_impl(y)->value()))){
          if(range_size==0&&found)return false;
          if(range_size==1&&!found)return false;
          if(range_size==2)return false;
          ++range_size;
        }
        else{
          if(range_size==1&&!found)return false;
          if(range_size==2)return found;
          range_size=0;
        }
        y=node_alg::after_local(y);
      }
    }
    return found;
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
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
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
    BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT;
    std::pair<final_node_type*,bool>p=
      this->final_emplace_hint_(
        static_cast<final_node_type*>(position.get_node()),
        BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
    return make_iterator(p.first);
  }

  template<
    typename CompatibleHash,typename CompatiblePred
  >
  iterator find(
    const key_type& k,
    const CompatibleHash& hash,const CompatiblePred& eq,mpl::true_)const
  {
    return find(k,hash,eq,mpl::false_());
  }

  template<
    typename CompatibleKey,typename CompatibleHash,typename CompatiblePred
  >
  iterator find(
    const CompatibleKey& k,
    const CompatibleHash& hash,const CompatiblePred& eq,mpl::false_)const
  {
    std::size_t buc=buckets.position(hash(k));
    for(node_impl_pointer x=buckets.at(buc)->prior();
        x!=node_impl_pointer(0);x=node_alg::next_to_inspect(x)){
      if(eq(k,key(index_node_type::from_impl(x)->value()))){
        return make_iterator(index_node_type::from_impl(x));
      }
    }
    return end();
  }

  template<
    typename CompatibleHash,typename CompatiblePred
  >
  size_type count(
    const key_type& k,
    const CompatibleHash& hash,const CompatiblePred& eq,mpl::true_)const
  {
    return count(k,hash,eq,mpl::false_());
  }

  template<
    typename CompatibleKey,typename CompatibleHash,typename CompatiblePred
  >
  size_type count(
    const CompatibleKey& k,
    const CompatibleHash& hash,const CompatiblePred& eq,mpl::false_)const
  {
    std::size_t buc=buckets.position(hash(k));
    for(node_impl_pointer x=buckets.at(buc)->prior();
        x!=node_impl_pointer(0);x=node_alg::next_to_inspect(x)){
      if(eq(k,key(index_node_type::from_impl(x)->value()))){
        size_type         res=0;
        node_impl_pointer y=end_of_range(x);
        do{
          ++res;
          x=node_alg::after(x);
        }while(x!=y);
        return res;
      }
    }
    return 0;
  }

  template<
    typename CompatibleHash,typename CompatiblePred
  >
  std::pair<iterator,iterator> equal_range(
    const key_type& k,
    const CompatibleHash& hash,const CompatiblePred& eq,mpl::true_)const
  {
    return equal_range(k,hash,eq,mpl::false_());
  }

  template<
    typename CompatibleKey,typename CompatibleHash,typename CompatiblePred
  >
  std::pair<iterator,iterator> equal_range(
    const CompatibleKey& k,
    const CompatibleHash& hash,const CompatiblePred& eq,mpl::false_)const
  {
    std::size_t buc=buckets.position(hash(k));
    for(node_impl_pointer x=buckets.at(buc)->prior();
        x!=node_impl_pointer(0);x=node_alg::next_to_inspect(x)){
      if(eq(k,key(index_node_type::from_impl(x)->value()))){
        return std::pair<iterator,iterator>(
          make_iterator(index_node_type::from_impl(x)),
          make_iterator(index_node_type::from_impl(end_of_range(x))));
      }
    }
    return std::pair<iterator,iterator>(end(),end());
  }

  key_from_value               key;
  hasher                       hash_;
  key_equal                    eq_;
  bucket_array_type            buckets;
  float                        mlf;
  size_type                    max_load;
      
#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_mfunc_templ reset
#endif
};

/* comparison */

template<
  typename KeyFromValue,typename Hash,typename Pred,
  typename SuperMeta,typename TagList,typename Category
>
bool operator==(
  const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
  const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& y)
{
  return x.equals(y);
}

template<
  typename KeyFromValue,typename Hash,typename Pred,
  typename SuperMeta,typename TagList,typename Category
>
bool operator!=(
  const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
  const hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& y)
{
  return !(x==y);
}

/*  specialized algorithms */

template<
  typename KeyFromValue,typename Hash,typename Pred,
  typename SuperMeta,typename TagList,typename Category
>
void swap(
  hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& x,
  hashed_index<KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>& y)
{
  x.swap(y);
}

} /* namespace multi_index::detail */

/* hashed index specifiers */

template<typename Arg1,typename Arg2,typename Arg3,typename Arg4>
struct hashed_unique
{
  typedef typename detail::hashed_index_args<
    Arg1,Arg2,Arg3,Arg4>                           index_args;
  typedef typename index_args::tag_list_type::type tag_list_type;
  typedef typename index_args::key_from_value_type key_from_value_type;
  typedef typename index_args::hash_type           hash_type;
  typedef typename index_args::pred_type           pred_type;

  template<typename Super>
  struct node_class
  {
    typedef detail::hashed_index_node<Super> type;
  };

  template<typename SuperMeta>
  struct index_class
  {
    typedef detail::hashed_index<
      key_from_value_type,hash_type,pred_type,
      SuperMeta,tag_list_type,detail::hashed_unique_tag> type;
  };
};

template<typename Arg1,typename Arg2,typename Arg3,typename Arg4>
struct hashed_non_unique
{
  typedef typename detail::hashed_index_args<
    Arg1,Arg2,Arg3,Arg4>                           index_args;
  typedef typename index_args::tag_list_type::type tag_list_type;
  typedef typename index_args::key_from_value_type key_from_value_type;
  typedef typename index_args::hash_type           hash_type;
  typedef typename index_args::pred_type           pred_type;

  template<typename Super>
  struct node_class
  {
    typedef detail::hashed_index_node<Super> type;
  };

  template<typename SuperMeta>
  struct index_class
  {
    typedef detail::hashed_index<
      key_from_value_type,hash_type,pred_type,
      SuperMeta,tag_list_type,detail::hashed_non_unique_tag> type;
  };
};

} /* namespace multi_index */

} /* namespace boost */

/* Boost.Foreach compatibility */

template<
  typename KeyFromValue,typename Hash,typename Pred,
  typename SuperMeta,typename TagList,typename Category
>
inline boost::mpl::true_* boost_foreach_is_noncopyable(
  boost::multi_index::detail::hashed_index<
    KeyFromValue,Hash,Pred,SuperMeta,TagList,Category>*&,
  boost_foreach_argument_dependent_lookup_hack)
{
  return 0;
}

#undef BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT
#undef BOOST_MULTI_INDEX_HASHED_INDEX_CHECK_INVARIANT_OF

#endif
