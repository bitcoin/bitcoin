/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_NODE_HANDLE_HPP
#define BOOST_MULTI_INDEX_DETAIL_NODE_HANDLE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/core/addressof.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/multi_index_container_fwd.hpp>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp> 
#include <new>

namespace boost{

namespace multi_index{

namespace detail{

/* Node handle template class following [container.node] specs.
 */

#include <boost/multi_index/detail/define_if_constexpr_macro.hpp>

template<typename Node,typename Allocator>
class node_handle
{
public:
  typedef typename Node::value_type        value_type;
  typedef Allocator                        allocator_type;

private:
  typedef allocator_traits<allocator_type> alloc_traits;

public:
  node_handle()BOOST_NOEXCEPT:node(0){}

  node_handle(BOOST_RV_REF(node_handle) x)BOOST_NOEXCEPT:node(x.node)
  {
    if(!x.empty()){
      move_construct_allocator(boost::move(x));
      x.destroy_allocator();
      x.node=0;
    }
  }

  ~node_handle()
  {
    if(!empty()){
      delete_node();
      destroy_allocator();
    }
  }

  node_handle& operator=(BOOST_RV_REF(node_handle) x)
  {
    if(this!=&x){
      if(!empty()){
        delete_node();
        if(!x.empty()){
          BOOST_MULTI_INDEX_IF_CONSTEXPR(
            alloc_traits::propagate_on_container_move_assignment::value){
            move_assign_allocator(boost::move(x));
          }
          x.destroy_allocator();
        }
        else{
          destroy_allocator();
        }
      }
      else if(!x.empty()){
        move_construct_allocator(boost::move(x));
        x.destroy_allocator();
      }
      node=x.node;
      x.node=0;
    }
    return *this;
  }

  value_type& value()const{return node->value();}
  allocator_type get_allocator()const{return *allocator_ptr();}

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
  explicit
#endif
  operator bool()const BOOST_NOEXCEPT{return (node!=0);}

#if BOOST_WORKAROUND(BOOST_GCC_VERSION,>=70000)&&__cplusplus<201103L
  /* https://github.com/boostorg/config/issues/336 */
#else
  BOOST_ATTRIBUTE_NODISCARD 
#endif
  bool empty()const BOOST_NOEXCEPT{return (node==0);}

  void swap(node_handle& x)
    BOOST_NOEXCEPT_IF(
      alloc_traits::propagate_on_container_swap::value||
      alloc_traits::is_always_equal::value)
  {
    if(!empty()){
      if(!x.empty()){
        BOOST_MULTI_INDEX_IF_CONSTEXPR(
          alloc_traits::propagate_on_container_swap::value){
          using std::swap;
          swap(*allocator_ptr(),*x.allocator_ptr());
        }
      }
      else{
        x.move_construct_allocator(boost::move(*this));
        destroy_allocator();
      }
    }
    else if(!x.empty()){
      move_construct_allocator(boost::move(x));
      x.destroy_allocator();
    }
    std::swap(node,x.node);
  }

  friend void swap(node_handle& x,node_handle& y)
    BOOST_NOEXCEPT_IF(noexcept(x.swap(y)))
  {
    x.swap(y);
  }

private:
  BOOST_MOVABLE_BUT_NOT_COPYABLE(node_handle)

  template <typename,typename,typename>
  friend class boost::multi_index::multi_index_container;

  node_handle(Node* node_,const allocator_type& al):node(node_)
  {
    ::new (static_cast<void*>(allocator_ptr())) allocator_type(al);
  }

  void release_node()
  {
    if(!empty()){
      node=0;
      destroy_allocator();
    }
  }

#include <boost/multi_index/detail/ignore_wstrict_aliasing.hpp>

  const allocator_type* allocator_ptr()const
  {
    return reinterpret_cast<const allocator_type*>(&space);
  }

  allocator_type* allocator_ptr()
  {
    return reinterpret_cast<allocator_type*>(&space);
  }

#include <boost/multi_index/detail/restore_wstrict_aliasing.hpp>

  void move_construct_allocator(BOOST_RV_REF(node_handle) x)
  {
    ::new (static_cast<void*>(allocator_ptr()))
      allocator_type(boost::move(*x.allocator_ptr()));
  }

  void move_assign_allocator(BOOST_RV_REF(node_handle) x)
  {
    *allocator_ptr()=boost::move(*x.allocator_ptr());
  }

  void destroy_allocator(){allocator_ptr()->~allocator_type();}

  void delete_node()
  {
    typedef typename rebind_alloc_for<
      allocator_type,Node
    >::type                                          node_allocator;
    typedef detail::allocator_traits<node_allocator> node_alloc_traits;
    typedef typename node_alloc_traits::pointer      node_pointer;

    alloc_traits::destroy(*allocator_ptr(),boost::addressof(node->value()));
    node_allocator nal(*allocator_ptr());
    node_alloc_traits::deallocate(nal,static_cast<node_pointer>(node),1);
  }

  Node*                                 node;
  typename aligned_storage<
    sizeof(allocator_type),
    alignment_of<allocator_type>::value
  >::type                               space;
};

#include <boost/multi_index/detail/undef_if_constexpr_macro.hpp>

/* node handle insert return type template class following
 * [container.insert.return] specs.
 */

template<typename Iterator,typename NodeHandle>
struct insert_return_type
{
  insert_return_type(
    Iterator position_,bool inserted_,BOOST_RV_REF(NodeHandle) node_):
    position(position_),inserted(inserted_),node(boost::move(node_)){}
  insert_return_type(BOOST_RV_REF(insert_return_type) x):
    position(x.position),inserted(x.inserted),node(boost::move(x.node)){}

  insert_return_type& operator=(BOOST_RV_REF(insert_return_type) x)
  {
    position=x.position;
    inserted=x.inserted;
    node=boost::move(x.node);
    return *this;
  }

  Iterator   position;
  bool       inserted;
  NodeHandle node;

private:
  BOOST_MOVABLE_BUT_NOT_COPYABLE(insert_return_type)
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
