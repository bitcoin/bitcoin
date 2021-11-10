/* Multiply indexed container.
 *
 * Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_HPP
#define BOOST_MULTI_INDEX_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/core/addressof.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/multi_index_container_fwd.hpp>
#include <boost/multi_index/detail/access_specifier.hpp>
#include <boost/multi_index/detail/adl_swap.hpp>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/base_type.hpp>
#include <boost/multi_index/detail/do_not_copy_elements_tag.hpp>
#include <boost/multi_index/detail/converter.hpp>
#include <boost/multi_index/detail/header_holder.hpp>
#include <boost/multi_index/detail/has_tag.hpp>
#include <boost/multi_index/detail/no_duplicate_tags.hpp>
#include <boost/multi_index/detail/safe_mode.hpp>
#include <boost/multi_index/detail/scope_guard.hpp>
#include <boost/multi_index/detail/vartempl_support.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/base_from_member.hpp>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
#include <boost/multi_index/detail/archive_constructed.hpp>
#include <boost/multi_index/detail/serialization_version.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <boost/throw_exception.hpp> 
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
#include <boost/multi_index/detail/invariant_assert.hpp>
#define BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x)                              \
  detail::scope_guard BOOST_JOIN(check_invariant_,__LINE__)=                 \
    detail::make_obj_guard(x,&multi_index_container::check_invariant_);      \
  BOOST_JOIN(check_invariant_,__LINE__).touch();
#define BOOST_MULTI_INDEX_CHECK_INVARIANT                                    \
  BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(*this)
#else
#define BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x)
#define BOOST_MULTI_INDEX_CHECK_INVARIANT
#endif

namespace boost{

namespace multi_index{

namespace detail{

struct unequal_alloc_move_ctor_tag{};

} /* namespace multi_index::detail */

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
#pragma warning(push)
#pragma warning(disable:4522) /* spurious warning on multiple operator=()'s */
#endif

template<typename Value,typename IndexSpecifierList,typename Allocator>
class multi_index_container:
  private ::boost::base_from_member<
    typename detail::rebind_alloc_for<
      Allocator,
      typename detail::multi_index_node_type<
        Value,IndexSpecifierList,Allocator>::type
    >::type
  >,
  BOOST_MULTI_INDEX_PRIVATE_IF_MEMBER_TEMPLATE_FRIENDS detail::header_holder<
    typename detail::allocator_traits<
      typename detail::rebind_alloc_for<
        Allocator,
        typename detail::multi_index_node_type<
          Value,IndexSpecifierList,Allocator>::type
      >::type
    >::pointer,
    multi_index_container<Value,IndexSpecifierList,Allocator> >,
  public detail::multi_index_base_type<
    Value,IndexSpecifierList,Allocator>::type
{
#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
/* The "ISO C++ Template Parser" option in CW8.3 has a problem with the
 * lifetime of const references bound to temporaries --precisely what
 * scopeguards are.
 */

#pragma parse_mfunc_templ off
#endif

private:
  BOOST_COPYABLE_AND_MOVABLE(multi_index_container)

#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
  template <typename,typename,typename> friend class  detail::index_base;
  template <typename,typename>          friend struct detail::header_holder;
  template <typename,typename>          friend struct detail::converter;
#endif

  typedef typename detail::multi_index_base_type<
      Value,IndexSpecifierList,Allocator>::type    super;
  typedef typename detail::rebind_alloc_for<
    Allocator,
    typename super::index_node_type
  >::type                                          node_allocator;
  typedef detail::allocator_traits<node_allocator> node_alloc_traits;
  typedef typename node_alloc_traits::pointer      node_pointer;
  typedef ::boost::base_from_member<
    node_allocator>                                bfm_allocator;
  typedef detail::header_holder<
    node_pointer,
    multi_index_container>                         bfm_header;

public:
  /* All types are inherited from super, a few are explicitly
   * brought forward here to save us some typename's.
   */

  typedef typename super::ctor_args_list           ctor_args_list;
  typedef IndexSpecifierList                       index_specifier_type_list;
 
  typedef typename super::index_type_list          index_type_list;

  typedef typename super::iterator_type_list       iterator_type_list;
  typedef typename super::const_iterator_type_list const_iterator_type_list;
  typedef typename super::value_type               value_type;
  typedef typename super::final_allocator_type     allocator_type;
  typedef typename super::size_type                size_type;
  typedef typename super::iterator                 iterator;
  typedef typename super::const_iterator           const_iterator;

  BOOST_STATIC_ASSERT(
    detail::no_duplicate_tags_in_index_list<index_type_list>::value);

  /* global project() needs to see this publicly */

  typedef typename super::final_node_type         final_node_type;

  /* construct/copy/destroy */

  multi_index_container():
    bfm_allocator(allocator_type()),
    super(ctor_args_list(),bfm_allocator::member),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }

  explicit multi_index_container(
    const ctor_args_list& args_list,

#if BOOST_WORKAROUND(__IBMCPP__,<=600)
    /* VisualAge seems to have an ETI issue with the default value for
     * argument al.
     */

    const allocator_type& al=
      typename mpl::identity<multi_index_container>::type::
        allocator_type()):
#else
    const allocator_type& al=allocator_type()):
#endif

    bfm_allocator(al),
    super(args_list,bfm_allocator::member),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }

  explicit multi_index_container(const allocator_type& al):
    bfm_allocator(al),
    super(ctor_args_list(),bfm_allocator::member),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }
  
  template<typename InputIterator>
  multi_index_container(
    InputIterator first,InputIterator last,

#if BOOST_WORKAROUND(__IBMCPP__,<=600)
    /* VisualAge seems to have an ETI issue with the default values
     * for arguments args_list and al.
     */

    const ctor_args_list& args_list=
      typename mpl::identity<multi_index_container>::type::
        ctor_args_list(),
    const allocator_type& al=
      typename mpl::identity<multi_index_container>::type::
        allocator_type()):
#else
    const ctor_args_list& args_list=ctor_args_list(),
    const allocator_type& al=allocator_type()):
#endif

    bfm_allocator(al),
    super(args_list,bfm_allocator::member),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
    BOOST_TRY{
      iterator hint=super::end();
      for(;first!=last;++first){
        hint=super::make_iterator(
          insert_ref_(*first,hint.get_node()).first);
        ++hint;
      }
    }
    BOOST_CATCH(...){
      clear_();
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  multi_index_container(
    std::initializer_list<Value> list,
    const ctor_args_list& args_list=ctor_args_list(),
    const allocator_type& al=allocator_type()):
    bfm_allocator(al),
    super(args_list,bfm_allocator::member),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
    BOOST_TRY{
      typedef const Value* init_iterator;

      iterator hint=super::end();
      for(init_iterator first=list.begin(),last=list.end();
          first!=last;++first){
        hint=super::make_iterator(insert_(*first,hint.get_node()).first);
        ++hint;
      }
    }
    BOOST_CATCH(...){
      clear_();
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }
#endif

  multi_index_container(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x):
    bfm_allocator(
      node_alloc_traits::select_on_container_copy_construction(
        x.bfm_allocator::member)),
    bfm_header(),
    super(x),
    node_count(0)
  {
    copy_construct_from(x);
  }

  multi_index_container(BOOST_RV_REF(multi_index_container) x):
    bfm_allocator(boost::move(x.bfm_allocator::member)),
    bfm_header(),
    super(x,detail::do_not_copy_elements_tag()),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
    BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x);
    swap_elements_(x);
  }

  multi_index_container(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x,
    const allocator_type& al):
    bfm_allocator(al),
    bfm_header(),
    super(x),
    node_count(0)
  {
    copy_construct_from(x);
  }

  multi_index_container(
    BOOST_RV_REF(multi_index_container) x,const allocator_type& al):
    bfm_allocator(al),
    bfm_header(),
    super(x,detail::do_not_copy_elements_tag()),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
    BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x);

    if(al==x.get_allocator()){
      swap_elements_(x);
    }
    else{
      multi_index_container y(x,al,detail::unequal_alloc_move_ctor_tag());
      swap_elements_(y);
    }
  }

  ~multi_index_container()
  {
    delete_all_nodes_();
  }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  /* As per http://www.boost.org/doc/html/move/emulation_limitations.html
   * #move.emulation_limitations.assignment_operator
   */

  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    multi_index_container y(
      x,
      node_alloc_traits::propagate_on_container_copy_assignment::value?
        x.get_allocator():this->get_allocator());
    swap_(y,boost::true_type() /* swap_allocators */);
    return *this;
  }
#endif

  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    BOOST_COPY_ASSIGN_REF(multi_index_container) x)
  {
    multi_index_container y(
      x,
      node_alloc_traits::propagate_on_container_copy_assignment::value?
        x.get_allocator():this->get_allocator());
    swap_(y,boost::true_type() /* swap_allocators */);
    return *this;
  }

  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    BOOST_RV_REF(multi_index_container) x)
  {
#include <boost/multi_index/detail/define_if_constexpr_macro.hpp>

    BOOST_MULTI_INDEX_IF_CONSTEXPR(
      node_alloc_traits::propagate_on_container_move_assignment::value){
      swap_(x,boost::true_type() /* swap_allocators */);
    }
    else if(this->get_allocator()==x.get_allocator()){
      swap_(x,boost::false_type() /* swap_allocators */);
    }
    else{
      multi_index_container y(boost::move(x),this->get_allocator());
      swap_(y,boost::false_type() /* swap_allocators */);
    }
    return *this;

#include <boost/multi_index/detail/undef_if_constexpr_macro.hpp>
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  multi_index_container<Value,IndexSpecifierList,Allocator>& operator=(
    std::initializer_list<Value> list)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
    typedef const Value* init_iterator;

    multi_index_container x(*this,detail::do_not_copy_elements_tag());    
    iterator hint=x.end();
    for(init_iterator first=list.begin(),last=list.end();
        first!=last;++first){
      hint=x.make_iterator(x.insert_(*first,hint.get_node()).first);
      ++hint;
    }
    x.swap_elements_(*this);
    return*this;
  }
#endif

  allocator_type get_allocator()const BOOST_NOEXCEPT
  {
    return allocator_type(bfm_allocator::member);
  }

  /* retrieval of indices by number */

#if !defined(BOOST_NO_MEMBER_TEMPLATES)
  template<int N>
  struct nth_index
  {
    BOOST_STATIC_ASSERT(N>=0&&N<mpl::size<index_type_list>::type::value);
    typedef typename mpl::at_c<index_type_list,N>::type type;
  };

  template<int N>
  typename nth_index<N>::type& get()BOOST_NOEXCEPT
  {
    BOOST_STATIC_ASSERT(N>=0&&N<mpl::size<index_type_list>::type::value);
    return *this;
  }

  template<int N>
  const typename nth_index<N>::type& get()const BOOST_NOEXCEPT
  {
    BOOST_STATIC_ASSERT(N>=0&&N<mpl::size<index_type_list>::type::value);
    return *this;
  }
#endif

  /* retrieval of indices by tag */

#if !defined(BOOST_NO_MEMBER_TEMPLATES)
  template<typename Tag>
  struct index
  {
    typedef typename mpl::find_if<
      index_type_list,
      detail::has_tag<Tag>
    >::type                                    iter;

    BOOST_STATIC_CONSTANT(
      bool,index_found=!(is_same<iter,typename mpl::end<index_type_list>::type >::value));
    BOOST_STATIC_ASSERT(index_found);

    typedef typename mpl::deref<iter>::type    type;
  };

  template<typename Tag>
  typename index<Tag>::type& get()BOOST_NOEXCEPT
  {
    return *this;
  }

  template<typename Tag>
  const typename index<Tag>::type& get()const BOOST_NOEXCEPT
  {
    return *this;
  }
#endif

  /* projection of iterators by number */

#if !defined(BOOST_NO_MEMBER_TEMPLATES)
  template<int N>
  struct nth_index_iterator
  {
    typedef typename nth_index<N>::type::iterator type;
  };

  template<int N>
  struct nth_index_const_iterator
  {
    typedef typename nth_index<N>::type::const_iterator type;
  };

  template<int N,typename IteratorType>
  typename nth_index_iterator<N>::type project(IteratorType it)
  {
    typedef typename nth_index<N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT(
      (mpl::contains<iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<typename IteratorType::container_type&>(*this));

    return index_type::make_iterator(
      static_cast<final_node_type*>(it.get_node()));
  }

  template<int N,typename IteratorType>
  typename nth_index_const_iterator<N>::type project(IteratorType it)const
  {
    typedef typename nth_index<N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT((
      mpl::contains<iterator_type_list,IteratorType>::value||
      mpl::contains<const_iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<const typename IteratorType::container_type&>(*this));
    return index_type::make_iterator(
      static_cast<final_node_type*>(it.get_node()));
  }
#endif

  /* projection of iterators by tag */

#if !defined(BOOST_NO_MEMBER_TEMPLATES)
  template<typename Tag>
  struct index_iterator
  {
    typedef typename index<Tag>::type::iterator type;
  };

  template<typename Tag>
  struct index_const_iterator
  {
    typedef typename index<Tag>::type::const_iterator type;
  };

  template<typename Tag,typename IteratorType>
  typename index_iterator<Tag>::type project(IteratorType it)
  {
    typedef typename index<Tag>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT(
      (mpl::contains<iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<typename IteratorType::container_type&>(*this));
    return index_type::make_iterator(
      static_cast<final_node_type*>(it.get_node()));
  }

  template<typename Tag,typename IteratorType>
  typename index_const_iterator<Tag>::type project(IteratorType it)const
  {
    typedef typename index<Tag>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* fails in Sun C++ 5.7 */
    BOOST_STATIC_ASSERT((
      mpl::contains<iterator_type_list,IteratorType>::value||
      mpl::contains<const_iterator_type_list,IteratorType>::value));
#endif

    BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);
    BOOST_MULTI_INDEX_CHECK_IS_OWNER(
      it,static_cast<const typename IteratorType::container_type&>(*this));
    return index_type::make_iterator(
      static_cast<final_node_type*>(it.get_node()));
  }
#endif

BOOST_MULTI_INDEX_PROTECTED_IF_MEMBER_TEMPLATE_FRIENDS:
  typedef typename super::final_node_handle_type final_node_handle_type;
  typedef typename super::copy_map_type          copy_map_type;

  multi_index_container(
    multi_index_container<Value,IndexSpecifierList,Allocator>& x,
    const allocator_type& al,
    detail::unequal_alloc_move_ctor_tag):
    bfm_allocator(al),
    bfm_header(),
    super(x),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT_OF(x);
    BOOST_TRY{
      copy_map_type map(bfm_allocator::member,x.size(),x.header(),header());
      for(const_iterator it=x.begin(),it_end=x.end();it!=it_end;++it){
        map.move_clone(it.get_node());
      }
      super::copy_(x,map);
      map.release();
      node_count=x.size();
      x.clear();
    }
    BOOST_CATCH(...){
      x.clear();
      BOOST_RETHROW;
    }
    BOOST_CATCH_END

    /* Not until this point are the indices required to be consistent,
     * hence the position of the invariant checker.
     */

    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  multi_index_container(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x,
    detail::do_not_copy_elements_tag):
    bfm_allocator(x.bfm_allocator::member),
    bfm_header(),
    super(x,detail::do_not_copy_elements_tag()),
    node_count(0)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }
#endif

  void copy_construct_from(
    const multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    copy_map_type map(bfm_allocator::member,x.size(),x.header(),header());
    for(const_iterator it=x.begin(),it_end=x.end();it!=it_end;++it){
      map.copy_clone(it.get_node());
    }
    super::copy_(x,map);
    map.release();
    node_count=x.size();

    /* Not until this point are the indices required to be consistent,
     * hence the position of the invariant checker.
     */

    BOOST_MULTI_INDEX_CHECK_INVARIANT;
  }

  final_node_type* header()const
  {
    return &*bfm_header::member;
  }

  final_node_type* allocate_node()
  {
    return &*node_alloc_traits::allocate(bfm_allocator::member,1);
  }

  void deallocate_node(final_node_type* x)
  {
    node_alloc_traits::deallocate(
      bfm_allocator::member,static_cast<node_pointer>(x),1);
  }

  void construct_value(final_node_type* x,const Value& v)
  {
    node_alloc_traits::construct(
      bfm_allocator::member,boost::addressof(x->value()),v);
  }

  void construct_value(final_node_type* x,BOOST_RV_REF(Value) v)
  {
    node_alloc_traits::construct(
      bfm_allocator::member,boost::addressof(x->value()),boost::move(v));
  }

  BOOST_MULTI_INDEX_OVERLOADS_TO_VARTEMPL_EXTRA_ARG(
    void,construct_value,vartempl_construct_value_impl,final_node_type*,x)

  void destroy_value(final_node_type* x)
  {
    node_alloc_traits::destroy(
      bfm_allocator::member,boost::addressof(x->value()));
  }

  bool empty_()const
  {
    return node_count==0;
  }

  size_type size_()const
  {
    return node_count;
  }

  size_type max_size_()const
  {
    return static_cast<size_type>(-1);
  }

  template<typename Variant>
  std::pair<final_node_type*,bool> insert_(const Value& v,Variant variant)
  {
    final_node_type* x=0;
    final_node_type* res=super::insert_(v,x,variant);
    if(res==x){
      ++node_count;
      return std::pair<final_node_type*,bool>(res,true);
    }
    else{
      return std::pair<final_node_type*,bool>(res,false);
    }
  }

  std::pair<final_node_type*,bool> insert_(const Value& v)
  {
    return insert_(v,detail::lvalue_tag());
  }

  std::pair<final_node_type*,bool> insert_rv_(const Value& v)
  {
    return insert_(v,detail::rvalue_tag());
  }

  template<typename T>
  std::pair<final_node_type*,bool> insert_ref_(T& t)
  {
    final_node_type* x=allocate_node();
    BOOST_TRY{
      construct_value(x,t);
      BOOST_TRY{
        final_node_type* res=super::insert_(
          x->value(),x,detail::emplaced_tag());
        if(res==x){
          ++node_count;
          return std::pair<final_node_type*,bool>(res,true);
        }
        else{
          delete_node_(x);
          return std::pair<final_node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        destroy_value(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  std::pair<final_node_type*,bool> insert_ref_(const value_type& x)
  {
    return insert_(x);
  }

  std::pair<final_node_type*,bool> insert_ref_(value_type& x)
  {
    return insert_(x);
  }

  std::pair<final_node_type*,bool> insert_nh_(final_node_handle_type& nh)
  {
    if(!nh)return std::pair<final_node_type*,bool>(header(),false);
    else{
      final_node_type* x=nh.node;
      final_node_type* res=super::insert_(
        x->value(),x,detail::emplaced_tag());
      if(res==x){
        nh.release_node();
        ++node_count;
        return std::pair<final_node_type*,bool>(res,true);
      }
      else return std::pair<final_node_type*,bool>(res,false);
    }
  }

  template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  std::pair<final_node_type*,bool> emplace_(
    BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    final_node_type* x=allocate_node();
    BOOST_TRY{
      construct_value(x,BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
      BOOST_TRY{
        final_node_type* res=super::insert_(
          x->value(),x,detail::emplaced_tag());
        if(res==x){
          ++node_count;
          return std::pair<final_node_type*,bool>(res,true);
        }
        else{
          delete_node_(x);
          return std::pair<final_node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        destroy_value(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  template<typename Variant>
  std::pair<final_node_type*,bool> insert_(
    const Value& v,final_node_type* position,Variant variant)
  {
    final_node_type* x=0;
    final_node_type* res=super::insert_(v,position,x,variant);
    if(res==x){
      ++node_count;
      return std::pair<final_node_type*,bool>(res,true);
    }
    else{
      return std::pair<final_node_type*,bool>(res,false);
    }
  }

  std::pair<final_node_type*,bool> insert_(
    const Value& v,final_node_type* position)
  {
    return insert_(v,position,detail::lvalue_tag());
  }

  std::pair<final_node_type*,bool> insert_rv_(
    const Value& v,final_node_type* position)
  {
    return insert_(v,position,detail::rvalue_tag());
  }

  template<typename T>
  std::pair<final_node_type*,bool> insert_ref_(
    T& t,final_node_type* position)
  {
    final_node_type* x=allocate_node();
    BOOST_TRY{
      construct_value(x,t);
      BOOST_TRY{
        final_node_type* res=super::insert_(
          x->value(),position,x,detail::emplaced_tag());
        if(res==x){
          ++node_count;
          return std::pair<final_node_type*,bool>(res,true);
        }
        else{
          delete_node_(x);
          return std::pair<final_node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        destroy_value(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  std::pair<final_node_type*,bool> insert_ref_(
    const value_type& x,final_node_type* position)
  {
    return insert_(x,position);
  }

  std::pair<final_node_type*,bool> insert_ref_(
    value_type& x,final_node_type* position)
  {
    return insert_(x,position);
  }

  std::pair<final_node_type*,bool> insert_nh_(
    final_node_handle_type& nh,final_node_type* position)
  {
    if(!nh)return std::pair<final_node_type*,bool>(header(),false);
    else{
      final_node_type* x=nh.node;
      final_node_type* res=super::insert_(
        x->value(),position,x,detail::emplaced_tag());
      if(res==x){
        nh.release_node();
        ++node_count;
        return std::pair<final_node_type*,bool>(res,true);
      }
      else return std::pair<final_node_type*,bool>(res,false);
    }
  }

  template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  std::pair<final_node_type*,bool> emplace_hint_(
    final_node_type* position,
    BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    final_node_type* x=allocate_node();
    BOOST_TRY{
      construct_value(x,BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
      BOOST_TRY{
        final_node_type* res=super::insert_(
          x->value(),position,x,detail::emplaced_tag());
        if(res==x){
          ++node_count;
          return std::pair<final_node_type*,bool>(res,true);
        }
        else{
          delete_node_(x);
          return std::pair<final_node_type*,bool>(res,false);
        }
      }
      BOOST_CATCH(...){
        destroy_value(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH(...){
      deallocate_node(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  final_node_handle_type extract_(final_node_type* x)
  {
    --node_count;
    super::extract_(x);
    return final_node_handle_type(x,get_allocator());
  }

  void erase_(final_node_type* x)
  {
    --node_count;
    super::extract_(x);
    delete_node_(x);
  }

  void delete_node_(final_node_type* x)
  {
    destroy_value(x);
    deallocate_node(x);
  }

  void delete_all_nodes_()
  {
    super::delete_all_nodes_();
  }

  void clear_()
  {
    delete_all_nodes_();
    super::clear_();
    node_count=0;
  }

  void swap_(multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    swap_(
      x,
      boost::integral_constant<
        bool,node_alloc_traits::propagate_on_container_swap::value>());
  }

  void swap_(
    multi_index_container<Value,IndexSpecifierList,Allocator>& x,
    boost::true_type swap_allocators)
  {
    detail::adl_swap(bfm_allocator::member,x.bfm_allocator::member);
    std::swap(bfm_header::member,x.bfm_header::member);
    super::swap_(x,swap_allocators);
    std::swap(node_count,x.node_count);
  }

  void swap_(
    multi_index_container<Value,IndexSpecifierList,Allocator>& x,
    boost::false_type swap_allocators)
  {
    std::swap(bfm_header::member,x.bfm_header::member);
    super::swap_(x,swap_allocators);
    std::swap(node_count,x.node_count);
  }

  void swap_elements_(
    multi_index_container<Value,IndexSpecifierList,Allocator>& x)
  {
    std::swap(bfm_header::member,x.bfm_header::member);
    super::swap_elements_(x);
    std::swap(node_count,x.node_count);
  }

  bool replace_(const Value& k,final_node_type* x)
  {
    return super::replace_(k,x,detail::lvalue_tag());
  }

  bool replace_rv_(const Value& k,final_node_type* x)
  {
    return super::replace_(k,x,detail::rvalue_tag());
  }

  template<typename Modifier>
  bool modify_(Modifier& mod,final_node_type* x)
  {
    BOOST_TRY{
      mod(const_cast<value_type&>(x->value()));
    }
    BOOST_CATCH(...){
      this->erase_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END

    BOOST_TRY{
      if(!super::modify_(x)){
        delete_node_(x);
        --node_count;
        return false;
      }
      else return true;
    }
    BOOST_CATCH(...){
      delete_node_(x);
      --node_count;
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

  template<typename Modifier,typename Rollback>
  bool modify_(Modifier& mod,Rollback& back_,final_node_type* x)
  {
    BOOST_TRY{
      mod(const_cast<value_type&>(x->value()));
    }
    BOOST_CATCH(...){
      this->erase_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END

    bool b;
    BOOST_TRY{
      b=super::modify_rollback_(x);
    }
    BOOST_CATCH(...){
      BOOST_TRY{
        back_(const_cast<value_type&>(x->value()));
        if(!super::check_rollback_(x))this->erase_(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH(...){
        this->erase_(x);
        BOOST_RETHROW;
      }
      BOOST_CATCH_END
    }
    BOOST_CATCH_END

    BOOST_TRY{
      if(!b){
        back_(const_cast<value_type&>(x->value()));
        if(!super::check_rollback_(x))this->erase_(x);
        return false;
      }
      else return true;
    }
    BOOST_CATCH(...){
      this->erase_(x);
      BOOST_RETHROW;
    }
    BOOST_CATCH_END
  }

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
  /* serialization */

  friend class boost::serialization::access;

  BOOST_SERIALIZATION_SPLIT_MEMBER()

  typedef typename super::index_saver_type        index_saver_type;
  typedef typename super::index_loader_type       index_loader_type;

  template<class Archive>
  void save(Archive& ar,const unsigned int version)const
  {
    const serialization::collection_size_type       s(size_());
    const detail::serialization_version<value_type> value_version;
    ar<<serialization::make_nvp("count",s);
    ar<<serialization::make_nvp("value_version",value_version);

    index_saver_type sm(bfm_allocator::member,s);

    for(iterator it=super::begin(),it_end=super::end();it!=it_end;++it){
      serialization::save_construct_data_adl(
        ar,boost::addressof(*it),value_version);
      ar<<serialization::make_nvp("item",*it);
      sm.add(it.get_node(),ar,version);
    }
    sm.add_track(header(),ar,version);

    super::save_(ar,version,sm);
  }

  template<class Archive>
  void load(Archive& ar,const unsigned int version)
  {
    BOOST_MULTI_INDEX_CHECK_INVARIANT;

    clear_(); 
    serialization::collection_size_type       s;
    detail::serialization_version<value_type> value_version;
    if(version<1){
      std::size_t sz;
      ar>>serialization::make_nvp("count",sz);
      s=static_cast<serialization::collection_size_type>(sz);
    }
    else{
      ar>>serialization::make_nvp("count",s);
    }
    if(version<2){
      value_version=0;
    }
    else{
      ar>>serialization::make_nvp("value_version",value_version);
    }

    index_loader_type lm(bfm_allocator::member,s);

    for(std::size_t n=0;n<s;++n){
      detail::archive_constructed<Value> value("item",ar,value_version);
      std::pair<final_node_type*,bool> p=insert_rv_(
        value.get(),super::end().get_node());
      if(!p.second)throw_exception(
        archive::archive_exception(
          archive::archive_exception::other_exception));
      ar.reset_object_address(
        boost::addressof(p.first->value()),boost::addressof(value.get()));
      lm.add(p.first,ar,version);
    }
    lm.add_track(header(),ar,version);

    super::load_(ar,version,lm);
  }
#endif

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
  /* invariant stuff */

  bool invariant_()const
  {
    return super::invariant_();
  }

  void check_invariant_()const
  {
    BOOST_MULTI_INDEX_INVARIANT_ASSERT(invariant_());
  }
#endif

private:
  template<BOOST_MULTI_INDEX_TEMPLATE_PARAM_PACK>
  void vartempl_construct_value_impl(
    final_node_type* x,BOOST_MULTI_INDEX_FUNCTION_PARAM_PACK)
  {
    node_alloc_traits::construct(
      bfm_allocator::member,boost::addressof(x->value()),
      BOOST_MULTI_INDEX_FORWARD_PARAM_PACK);
  }

  size_type node_count;

#if defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)&&\
    BOOST_WORKAROUND(__MWERKS__,<=0x3003)
#pragma parse_mfunc_templ reset
#endif
};

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1500))
#pragma warning(pop) /* C4522 */
#endif

/* retrieval of indices by number */

template<typename MultiIndexContainer,int N>
struct nth_index
{
  BOOST_STATIC_CONSTANT(
    int,
    M=mpl::size<typename MultiIndexContainer::index_type_list>::type::value);
  BOOST_STATIC_ASSERT(N>=0&&N<M);
  typedef typename mpl::at_c<
    typename MultiIndexContainer::index_type_list,N>::type type;
};

template<int N,typename Value,typename IndexSpecifierList,typename Allocator>
typename nth_index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type&
get(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>    multi_index_type;
  typedef typename nth_index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    N
  >::type                                  index_type;

  BOOST_STATIC_ASSERT(N>=0&&
    N<
    mpl::size<
      BOOST_DEDUCED_TYPENAME multi_index_type::index_type_list
    >::type::value);

  return detail::converter<multi_index_type,index_type>::index(m);
}

template<int N,typename Value,typename IndexSpecifierList,typename Allocator>
const typename nth_index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type&
get(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m
)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>    multi_index_type;
  typedef typename nth_index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    N
  >::type                                  index_type;

  BOOST_STATIC_ASSERT(N>=0&&
    N<
    mpl::size<
      BOOST_DEDUCED_TYPENAME multi_index_type::index_type_list
    >::type::value);

  return detail::converter<multi_index_type,index_type>::index(m);
}

/* retrieval of indices by tag */

template<typename MultiIndexContainer,typename Tag>
struct index
{
  typedef typename MultiIndexContainer::index_type_list index_type_list;

  typedef typename mpl::find_if<
    index_type_list,
    detail::has_tag<Tag>
  >::type                                      iter;

  BOOST_STATIC_CONSTANT(
    bool,index_found=!(is_same<iter,typename mpl::end<index_type_list>::type >::value));
  BOOST_STATIC_ASSERT(index_found);

  typedef typename mpl::deref<iter>::type       type;
};

template<
  typename Tag,typename Value,typename IndexSpecifierList,typename Allocator
>
typename ::boost::multi_index::index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type&
get(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::boost::multi_index::index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    Tag
  >::type                                       index_type;

  return detail::converter<multi_index_type,index_type>::index(m);
}

template<
  typename Tag,typename Value,typename IndexSpecifierList,typename Allocator
>
const typename ::boost::multi_index::index<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type&
get(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m
)BOOST_NOEXCEPT
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::boost::multi_index::index<
    multi_index_container<
      Value,IndexSpecifierList,Allocator>,
    Tag
  >::type                                       index_type;

  return detail::converter<multi_index_type,index_type>::index(m);
}

/* projection of iterators by number */

template<typename MultiIndexContainer,int N>
struct nth_index_iterator
{
  typedef typename nth_index<MultiIndexContainer,N>::type::iterator type;
};

template<typename MultiIndexContainer,int N>
struct nth_index_const_iterator
{
  typedef typename nth_index<MultiIndexContainer,N>::type::const_iterator type;
};

template<
  int N,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename nth_index_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type
project(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>                multi_index_type;
  typedef typename nth_index<multi_index_type,N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::iterator(
    m,static_cast<typename multi_index_type::final_node_type*>(it.get_node()));
}

template<
  int N,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename nth_index_const_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,N>::type
project(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>                multi_index_type;
  typedef typename nth_index<multi_index_type,N>::type index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value||
    mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::const_iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::const_iterator(
    m,static_cast<typename multi_index_type::final_node_type*>(it.get_node()));
}

/* projection of iterators by tag */

template<typename MultiIndexContainer,typename Tag>
struct index_iterator
{
  typedef typename ::boost::multi_index::index<
    MultiIndexContainer,Tag>::type::iterator    type;
};

template<typename MultiIndexContainer,typename Tag>
struct index_const_iterator
{
  typedef typename ::boost::multi_index::index<
    MultiIndexContainer,Tag>::type::const_iterator type;
};

template<
  typename Tag,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename index_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type
project(
  multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::boost::multi_index::index<
    multi_index_type,Tag>::type                 index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::iterator(
    m,static_cast<typename multi_index_type::final_node_type*>(it.get_node()));
}

template<
  typename Tag,typename IteratorType,
  typename Value,typename IndexSpecifierList,typename Allocator>
typename index_const_iterator<
  multi_index_container<Value,IndexSpecifierList,Allocator>,Tag>::type
project(
  const multi_index_container<Value,IndexSpecifierList,Allocator>& m,
  IteratorType it)
{
  typedef multi_index_container<
    Value,IndexSpecifierList,Allocator>         multi_index_type;
  typedef typename ::boost::multi_index::index<
    multi_index_type,Tag>::type                 index_type;

#if !defined(__SUNPRO_CC)||!(__SUNPRO_CC<0x580) /* Sun C++ 5.7 fails */
  BOOST_STATIC_ASSERT((
    mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::iterator_type_list,
      IteratorType>::value||
    mpl::contains<
      BOOST_DEDUCED_TYPENAME multi_index_type::const_iterator_type_list,
      IteratorType>::value));
#endif

  BOOST_MULTI_INDEX_CHECK_VALID_ITERATOR(it);

#if defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
  typedef detail::converter<
    multi_index_type,
    BOOST_DEDUCED_TYPENAME IteratorType::container_type> converter;
  BOOST_MULTI_INDEX_CHECK_IS_OWNER(it,converter::index(m));
#endif

  return detail::converter<multi_index_type,index_type>::const_iterator(
    m,static_cast<typename multi_index_type::final_node_type*>(it.get_node()));
}

/* Comparison. Simple forward to first index. */

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator==(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)==get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator<(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)<get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator!=(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)!=get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator>(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)>get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator>=(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)>=get<0>(y);
}

template<
  typename Value1,typename IndexSpecifierList1,typename Allocator1,
  typename Value2,typename IndexSpecifierList2,typename Allocator2
>
bool operator<=(
  const multi_index_container<Value1,IndexSpecifierList1,Allocator1>& x,
  const multi_index_container<Value2,IndexSpecifierList2,Allocator2>& y)
{
  return get<0>(x)<=get<0>(y);
}

/*  specialized algorithms */

template<typename Value,typename IndexSpecifierList,typename Allocator>
void swap(
  multi_index_container<Value,IndexSpecifierList,Allocator>& x,
  multi_index_container<Value,IndexSpecifierList,Allocator>& y)
{
  x.swap(y);
}

} /* namespace multi_index */

#if !defined(BOOST_MULTI_INDEX_DISABLE_SERIALIZATION)
/* class version = 1 : we now serialize the size through
 * boost::serialization::collection_size_type.
 * class version = 2 : proper use of {save|load}_construct_data.
 */

namespace serialization {
template<typename Value,typename IndexSpecifierList,typename Allocator>
struct version<
  boost::multi_index_container<Value,IndexSpecifierList,Allocator>
>
{
  BOOST_STATIC_CONSTANT(int,value=2);
};
} /* namespace serialization */
#endif

/* Associated global functions are promoted to namespace boost, except
 * comparison operators and swap, which are meant to be Koenig looked-up.
 */

using multi_index::get;
using multi_index::project;

} /* namespace boost */

#undef BOOST_MULTI_INDEX_CHECK_INVARIANT
#undef BOOST_MULTI_INDEX_CHECK_INVARIANT_OF

#endif
