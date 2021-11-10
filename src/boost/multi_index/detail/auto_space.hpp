/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_AUTO_SPACE_HPP
#define BOOST_MULTI_INDEX_DETAIL_AUTO_SPACE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/multi_index/detail/adl_swap.hpp>
#include <boost/multi_index/detail/allocator_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <memory>

namespace boost{

namespace multi_index{

namespace detail{

/* auto_space provides uninitialized space suitably to store
 * a given number of elements of a given type.
 */

/* NB: it is not clear whether using an allocator to handle
 * zero-sized arrays of elements is conformant or not. GCC 3.3.1
 * and prior fail here, other stdlibs handle the issue gracefully.
 * To be on the safe side, the case n==0 is given special treatment.
 * References:
 *   GCC Bugzilla, "standard allocator crashes when deallocating segment
 *    "of zero length", http://gcc.gnu.org/bugzilla/show_bug.cgi?id=14176
 *   C++ Standard Library Defect Report List (Revision 28), issue 199
 *     "What does allocate(0) return?",
 *     http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#199
 */

template<typename T,typename Allocator=std::allocator<T> >
struct auto_space:private noncopyable
{
  typedef typename rebind_alloc_for<
    Allocator,T>
  ::type                                   allocator;
  typedef allocator_traits<allocator>      alloc_traits;
  typedef typename alloc_traits::pointer   pointer;
  typedef typename alloc_traits::size_type size_type;

  explicit auto_space(const Allocator& al=Allocator(),size_type n=1):
  al_(al),n_(n),data_(n_?alloc_traits::allocate(al_,n_):pointer(0))
  {}

  ~auto_space(){if(n_)alloc_traits::deallocate(al_,data_,n_);}

  Allocator get_allocator()const{return al_;}

  pointer data()const{return data_;}

  void swap(auto_space& x)
  {
    swap(
      x,
      boost::integral_constant<
        bool,alloc_traits::propagate_on_container_swap::value>());
  }

  void swap(auto_space& x,boost::true_type /* swap_allocators */)
  {
    adl_swap(al_,x.al_);
    std::swap(n_,x.n_);
    std::swap(data_,x.data_);
  }
    
  void swap(auto_space& x,boost::false_type /* swap_allocators */)
  {
    std::swap(n_,x.n_);
    std::swap(data_,x.data_);
  }

private:
  allocator al_;
  size_type n_;
  pointer   data_;
};

template<typename T,typename Allocator>
void swap(auto_space<T,Allocator>& x,auto_space<T,Allocator>& y)
{
  x.swap(y);
}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
