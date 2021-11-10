/* Copyright 2003-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_RND_INDEX_LOADER_HPP
#define BOOST_MULTI_INDEX_DETAIL_RND_INDEX_LOADER_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/auto_space.hpp>
#include <boost/multi_index/detail/rnd_index_ptr_array.hpp>
#include <boost/noncopyable.hpp>

namespace boost{

namespace multi_index{

namespace detail{

/* This class implements a serialization rearranger for random access
 * indices. In order to achieve O(n) performance, the following strategy
 * is followed: the nodes of the index are handled as if in a bidirectional
 * list, where the next pointers are stored in the original
 * random_access_index_ptr_array and the prev pointers are stored in
 * an auxiliary array. Rearranging of nodes in such a bidirectional list
 * is constant time. Once all the arrangements are performed (on destruction
 * time) the list is traversed in reverse order and
 * pointers are swapped and set accordingly so that they recover its
 * original semantics ( *(node->up())==node ) while retaining the
 * new order.
 */

template<typename Allocator>
class random_access_index_loader_base:private noncopyable
{
protected:
  typedef random_access_index_node_impl<
    typename rebind_alloc_for<
      Allocator,
      char
    >::type
  >                                                 node_impl_type;
  typedef typename node_impl_type::pointer          node_impl_pointer;
  typedef random_access_index_ptr_array<Allocator>  ptr_array;

  random_access_index_loader_base(const Allocator& al_,ptr_array& ptrs_):
    al(al_),
    ptrs(ptrs_),
    header(*ptrs.end()),
    prev_spc(al,0),
    preprocessed(false)
  {}

  ~random_access_index_loader_base()
  {
    if(preprocessed)
    {
      node_impl_pointer n=header;
      next(n)=n;

      for(size_type i=ptrs.size();i--;){
        n=prev(n);
        size_type d=position(n);
        if(d!=i){
          node_impl_pointer m=prev(next_at(i));
          std::swap(m->up(),n->up());
          next_at(d)=next_at(i);
          std::swap(prev_at(d),prev_at(i));
        }
        next(n)=n;
      }
    }
  }

  void rearrange(node_impl_pointer position_,node_impl_pointer x)
  {
    preprocess(); /* only incur this penalty if rearrange() is ever called */
    if(position_==node_impl_pointer(0))position_=header;
    next(prev(x))=next(x);
    prev(next(x))=prev(x);
    prev(x)=position_;
    next(x)=next(position_);
    next(prev(x))=prev(next(x))=x;
  }

private:
  typedef allocator_traits<Allocator>      alloc_traits;
  typedef typename alloc_traits::size_type size_type;

  void preprocess()
  {
    if(!preprocessed){
      /* get space for the auxiliary prev array */
      auto_space<node_impl_pointer,Allocator> tmp(al,ptrs.size()+1);
      prev_spc.swap(tmp);

      /* prev_spc elements point to the prev nodes */
      std::rotate_copy(
        &*ptrs.begin(),&*ptrs.end(),&*ptrs.end()+1,&*prev_spc.data());

      /* ptrs elements point to the next nodes */
      std::rotate(&*ptrs.begin(),&*ptrs.begin()+1,&*ptrs.end()+1);

      preprocessed=true;
    }
  }

  size_type position(node_impl_pointer x)const
  {
    return (size_type)(x->up()-ptrs.begin());
  }

  node_impl_pointer& next_at(size_type n)const
  {
    return *ptrs.at(n);
  }

  node_impl_pointer& prev_at(size_type n)const
  {
    return *(prev_spc.data()+n);
  }

  node_impl_pointer& next(node_impl_pointer x)const
  {
    return *(x->up());
  }

  node_impl_pointer& prev(node_impl_pointer x)const
  {
    return prev_at(position(x));
  }

  Allocator                               al;
  ptr_array&                              ptrs;
  node_impl_pointer                       header;
  auto_space<node_impl_pointer,Allocator> prev_spc;
  bool                                    preprocessed;
};

template<typename Node,typename Allocator>
class random_access_index_loader:
  private random_access_index_loader_base<Allocator>
{
  typedef random_access_index_loader_base<Allocator> super;
  typedef typename super::node_impl_pointer          node_impl_pointer;
  typedef typename super::ptr_array                  ptr_array;

public:
  random_access_index_loader(const Allocator& al_,ptr_array& ptrs_):
    super(al_,ptrs_)
  {}

  void rearrange(Node* position_,Node *x)
  {
    super::rearrange(
      position_?position_->impl():node_impl_pointer(0),x->impl());
  }
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
