/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_RND_INDEX_PTR_ARRAY_HPP
#define BOOST_MULTI_INDEX_DETAIL_RND_INDEX_PTR_ARRAY_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/auto_space.hpp>
#include <boost/multi_index/detail/rnd_index_node.hpp>
#include <boost/noncopyable.hpp>

namespace boost{

namespace multi_index{

namespace detail{

/* pointer structure for use by random access indices */

template<typename Allocator>
class random_access_index_ptr_array:private noncopyable
{
  typedef random_access_index_node_impl<
    typename rebind_alloc_for<
      Allocator,
      char
    >::type
  >                                         node_impl_type;

public:
  typedef typename node_impl_type::pointer  value_type;
  typedef typename rebind_alloc_for<
    Allocator,value_type
  >::type                                   value_allocator;
  typedef allocator_traits<value_allocator> alloc_traits;
  typedef typename alloc_traits::pointer    pointer;
  typedef typename alloc_traits::size_type  size_type;

  random_access_index_ptr_array(
    const Allocator& al,value_type end_,size_type sz):
    size_(sz),
    capacity_(sz),
    spc(al,capacity_+1)
  {
    *end()=end_;
    end_->up()=end();
  }

  size_type size()const{return size_;}
  size_type capacity()const{return capacity_;}

  void room_for_one()
  {
    if(size_==capacity_){
      reserve(capacity_<=10?15:capacity_+capacity_/2);
    }
  }

  void reserve(size_type c)
  {
    if(c>capacity_)set_capacity(c);
  }

  void shrink_to_fit()
  {
    if(capacity_>size_)set_capacity(size_);
  }

  pointer begin()const{return ptrs();}
  pointer end()const{return ptrs()+size_;}
  pointer at(size_type n)const{return ptrs()+n;}

  void push_back(value_type x)
  {
    *(end()+1)=*end();
    (*(end()+1))->up()=end()+1;
    *end()=x;
    (*end())->up()=end();
    ++size_;
  }

  void erase(value_type x)
  {
    node_impl_type::extract(x->up(),end()+1);
    --size_;
  }

  void clear()
  {
    *begin()=*end();
    (*begin())->up()=begin();
    size_=0;
  }

  void swap(random_access_index_ptr_array& x)
  {
    std::swap(size_,x.size_);
    std::swap(capacity_,x.capacity_);
    spc.swap(x.spc);
  }

  template<typename BoolConstant>
  void swap(random_access_index_ptr_array& x,BoolConstant swap_allocators)
  {
    std::swap(size_,x.size_);
    std::swap(capacity_,x.capacity_);
    spc.swap(x.spc,swap_allocators);
  }

private:
  size_type                        size_;
  size_type                        capacity_;
  auto_space<value_type,Allocator> spc;

  pointer ptrs()const
  {
    return spc.data();
  }

  void set_capacity(size_type c)
  {
    auto_space<value_type,Allocator> spc1(spc.get_allocator(),c+1);
    node_impl_type::transfer(begin(),end()+1,spc1.data());
    spc.swap(spc1);
    capacity_=c;
  }
};

template<typename Allocator>
void swap(
  random_access_index_ptr_array<Allocator>& x,
  random_access_index_ptr_array<Allocator>& y)
{
  x.swap(y);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
