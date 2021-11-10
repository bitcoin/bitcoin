/* Copyright 2003-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_RND_INDEX_NODE_HPP
#define BOOST_MULTI_INDEX_DETAIL_RND_INDEX_NODE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/integer/common_factor_rt.hpp>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/multi_index/detail/raw_ptr.hpp>
#include <cstddef>
#include <functional>

namespace boost{

namespace multi_index{

namespace detail{

template<typename Allocator>
struct random_access_index_node_impl
{
  typedef typename rebind_alloc_for<
    Allocator,random_access_index_node_impl
  >::type                                             node_allocator;
  typedef allocator_traits<node_allocator>            node_alloc_traits;
  typedef typename node_alloc_traits::pointer         pointer;
  typedef typename node_alloc_traits::const_pointer   const_pointer;
  typedef typename node_alloc_traits::difference_type difference_type;
  typedef typename rebind_alloc_for<
    Allocator,pointer
  >::type                                             ptr_allocator;
  typedef allocator_traits<ptr_allocator>             ptr_alloc_traits;
  typedef typename ptr_alloc_traits::pointer          ptr_pointer;

  ptr_pointer& up(){return up_;}
  ptr_pointer  up()const{return up_;}

  /* interoperability with rnd_node_iterator */

  static void increment(pointer& x)
  {
    x=*(x->up()+1);
  }

  static void decrement(pointer& x)
  {
    x=*(x->up()-1);
  }

  static void advance(pointer& x,difference_type n)
  {
    x=*(x->up()+n);
  }

  static difference_type distance(pointer x,pointer y)
  {
    return static_cast<difference_type>(y->up()-x->up());
  }

  /* algorithmic stuff */

  static void relocate(ptr_pointer pos,ptr_pointer x)
  {
    pointer n=*x;
    if(x<pos){
      extract(x,pos);
      *(pos-1)=n;
      n->up()=pos-1;
    }
    else{
      while(x!=pos){
        *x=*(x-1);
        (*x)->up()=x;
        --x;
      }
      *pos=n;
      n->up()=pos;
    }
  };

  static void relocate(ptr_pointer pos,ptr_pointer first,ptr_pointer last)
  {
    ptr_pointer begin,middle,end;
    if(pos<first){
      begin=pos;
      middle=first;
      end=last;
    }
    else{
      begin=first;
      middle=last;
      end=pos;
    }

    std::ptrdiff_t n=end-begin;
    std::ptrdiff_t m=middle-begin;
    std::ptrdiff_t n_m=n-m;
    std::ptrdiff_t p=integer::gcd(n,m);

    for(std::ptrdiff_t i=0;i<p;++i){
      pointer tmp=begin[i];
      for(std::ptrdiff_t j=i,k;;){
        if(j<n_m)k=j+m;
        else     k=j-n_m;
        if(k==i){
          *(begin+j)=tmp;
          (*(begin+j))->up()=begin+j;
          break;
        }
        else{
          *(begin+j)=*(begin+k);
          (*(begin+j))->up()=begin+j;
        }

        if(k<n_m)j=k+m;
        else     j=k-n_m;
        if(j==i){
          *(begin+k)=tmp;
          (*(begin+k))->up()=begin+k;
          break;
        }
        else{
          *(begin+k)=*(begin+j);
          (*(begin+k))->up()=begin+k;
        }
      }
    }
  };

  static void extract(ptr_pointer x,ptr_pointer pend)
  {
    --pend;
    while(x!=pend){
      *x=*(x+1);
      (*x)->up()=x;
      ++x;
    }
  }

  static void transfer(
    ptr_pointer pbegin0,ptr_pointer pend0,ptr_pointer pbegin1)
  {
    while(pbegin0!=pend0){
      *pbegin1=*pbegin0++;
      (*pbegin1)->up()=pbegin1;
      ++pbegin1;
    }
  }

  static void reverse(ptr_pointer pbegin,ptr_pointer pend)
  {
    std::ptrdiff_t d=(pend-pbegin)/2;
    for(std::ptrdiff_t i=0;i<d;++i){
      std::swap(*pbegin,*--pend);
      (*pbegin)->up()=pbegin;
      (*pend)->up()=pend;
      ++pbegin;
    }
  }

private:
  ptr_pointer up_;
};

template<typename Super>
struct random_access_index_node_trampoline:
  random_access_index_node_impl<
    typename rebind_alloc_for<
      typename Super::allocator_type,
      char
    >::type
  >
{
  typedef random_access_index_node_impl<
    typename rebind_alloc_for<
      typename Super::allocator_type,
      char
    >::type
  > impl_type;
};

template<typename Super>
struct random_access_index_node:
  Super,random_access_index_node_trampoline<Super>
{
private:
  typedef random_access_index_node_trampoline<Super> trampoline;

public:
  typedef typename trampoline::impl_type         impl_type;
  typedef typename trampoline::pointer           impl_pointer;
  typedef typename trampoline::const_pointer     const_impl_pointer;
  typedef typename trampoline::difference_type   difference_type;
  typedef typename trampoline::ptr_pointer       impl_ptr_pointer;

  impl_ptr_pointer& up(){return trampoline::up();}
  impl_ptr_pointer  up()const{return trampoline::up();}

  impl_pointer impl()
  {
    return static_cast<impl_pointer>(
      static_cast<impl_type*>(static_cast<trampoline*>(this)));
  }

  const_impl_pointer impl()const
  {
    return static_cast<const_impl_pointer>(
      static_cast<const impl_type*>(static_cast<const trampoline*>(this)));
  }

  static random_access_index_node* from_impl(impl_pointer x)
  {
    return
      static_cast<random_access_index_node*>(
        static_cast<trampoline*>(
          raw_ptr<impl_type*>(x)));
  }

  static const random_access_index_node* from_impl(const_impl_pointer x)
  {
    return
      static_cast<const random_access_index_node*>(
        static_cast<const trampoline*>(
          raw_ptr<const impl_type*>(x)));
  }

  /* interoperability with rnd_node_iterator */

  static void increment(random_access_index_node*& x)
  {
    impl_pointer xi=x->impl();
    trampoline::increment(xi);
    x=from_impl(xi);
  }

  static void decrement(random_access_index_node*& x)
  {
    impl_pointer xi=x->impl();
    trampoline::decrement(xi);
    x=from_impl(xi);
  }

  static void advance(random_access_index_node*& x,difference_type n)
  {
    impl_pointer xi=x->impl();
    trampoline::advance(xi,n);
    x=from_impl(xi);
  }

  static difference_type distance(
    random_access_index_node* x,random_access_index_node* y)
  {
    return trampoline::distance(x->impl(),y->impl());
  }
};

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
