/* Copyright 2016-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_SEGMENT_HPP
#define BOOST_POLY_COLLECTION_DETAIL_SEGMENT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace boost{

namespace poly_collection{

namespace detail{

/* segment<Model,Allocator> encapsulates implementations of
 * Model::segment_backend virtual interface under a value-semantics type for
 * use by poly_collection. The techique is described by Sean Parent at slides
 * 157-205 of
 * https://github.com/sean-parent/sean-parent.github.com/wiki/
 *   presentations/2013-09-11-cpp-seasoning/cpp-seasoning.pdf
 * with one twist: when the type of the implementation can be known at compile
 * time, a downcast is done and non-virtual member functions (named with a nv_
 * prefix) are used: this increases the performance of some operations.
 */

template<typename Model,typename Allocator>
class segment
{
public:
  using value_type=typename Model::value_type;
  using allocator_type=Allocator; /* needed for uses-allocator construction */
  using base_iterator=typename Model::base_iterator;
  using const_base_iterator=typename Model::const_base_iterator;
  using base_sentinel=typename Model::base_sentinel;
  using const_base_sentinel=typename Model::const_base_sentinel;
  template<typename T>
  using iterator=typename Model::template iterator<T>;
  template<typename T>
  using const_iterator=typename Model::template const_iterator<T>;

  template<typename T>
  static segment make(const allocator_type& al)
  {
    return segment_backend_implementation<T>::make(al);
  }

  /* clones the implementation of x with no elements */

  static segment make_from_prototype(const segment& x,const allocator_type& al)
  {
    return {from_prototype{},x,al};
  }

  segment(const segment& x):
    pimpl{x.impl().copy()}{set_sentinel();}
  segment(segment&& x)=default;
  segment(const segment& x,const allocator_type& al):
    pimpl{x.impl().copy(al)}{set_sentinel();}

  /* TODO: try ptr-level move before impl().move() */
  segment(segment&& x,const allocator_type& al):
    pimpl{x.impl().move(al)}{set_sentinel();}

  segment& operator=(const segment& x)
  {
    pimpl=allocator_traits::propagate_on_container_copy_assignment::value?
      x.impl().copy():x.impl().copy(impl().get_allocator());
    set_sentinel();
    return *this;
  }
  
  segment& operator=(segment&& x)
  {
    pimpl=x.impl().move(
      allocator_traits::propagate_on_container_move_assignment::value?
      x.impl().get_allocator():impl().get_allocator());
    set_sentinel();
    return *this;
  }

  friend bool operator==(const segment& x,const segment& y)
  {
    if(typeid(*(x.pimpl))!=typeid(*(y.pimpl)))return false;
    else return x.impl().equal(y.impl());
  }

  friend bool operator!=(const segment& x,const segment& y){return !(x==y);}

  base_iterator        begin()const noexcept{return impl().begin();}
  template<typename U>
  base_iterator        begin()const noexcept{return impl<U>().nv_begin();}
  base_iterator        end()const noexcept{return impl().end();}
  template<typename U>
  base_iterator        end()const noexcept{return impl<U>().nv_end();}
  base_sentinel        sentinel()const noexcept{return snt;}
  bool                 empty()const noexcept{return impl().empty();}
  template<typename U>
  bool                 empty()const noexcept{return impl<U>().nv_empty();}
  std::size_t          size()const noexcept{return impl().size();}
  template<typename U>
  std::size_t          size()const noexcept{return impl<U>().nv_size();}
  std::size_t          max_size()const noexcept{return impl().max_size();}
  template<typename U>
  std::size_t          max_size()const noexcept
                         {return impl<U>().nv_max_size();}
  void                 reserve(std::size_t n){filter(impl().reserve(n));}
  template<typename U>
  void                 reserve(std::size_t n){filter(impl<U>().nv_reserve(n));}
  std::size_t          capacity()const noexcept{return impl().capacity();}
  template<typename U>
  std::size_t          capacity()const noexcept
                         {return impl<U>().nv_capacity();}
  void                 shrink_to_fit(){filter(impl().shrink_to_fit());}
  template<typename U>
  void                 shrink_to_fit(){filter(impl<U>().nv_shrink_to_fit());}

  template<typename U,typename Iterator,typename... Args>
  base_iterator emplace(Iterator it,Args&&... args)
  {
    return filter(impl<U>().nv_emplace(it,std::forward<Args>(args)...));
  }

  template<typename U,typename... Args>
  base_iterator emplace_back(Args&&... args)
  {
    return filter(impl<U>().nv_emplace_back(std::forward<Args>(args)...));
  }

  template<typename T>
  base_iterator push_back(const T& x)
  {
    return filter(impl().push_back(subaddress(x)));
  }

  template<
    typename T,
    typename std::enable_if<
      !std::is_lvalue_reference<T>::value&&!std::is_const<T>::value
    >::type* =nullptr
  >
  base_iterator push_back(T&& x)
  {
    return filter(impl().push_back_move(subaddress(x)));
  }

  template<typename U>
  base_iterator push_back_terminal(U&& x)
  {
    return filter(
      impl<typename std::decay<U>::type>().nv_push_back(std::forward<U>(x)));
  }

  template<typename T>
  base_iterator insert(const_base_iterator it,const T& x)
  {
    return filter(impl().insert(it,subaddress(x)));
  }

  template<typename U,typename T>
  base_iterator insert(const_iterator<U> it,const T& x)
  {
    return filter(
      impl<U>().nv_insert(it,*static_cast<const U*>(subaddress(x))));
  }

  template<
    typename T,
    typename std::enable_if<
      !std::is_lvalue_reference<T>::value&&!std::is_const<T>::value
    >::type* =nullptr
  >
  base_iterator insert(const_base_iterator it,T&& x)
  {
    return filter(impl().insert_move(it,subaddress(x)));
  }

  template<
    typename U,typename T,
    typename std::enable_if<
      !std::is_lvalue_reference<T>::value&&!std::is_const<T>::value
    >::type* =nullptr
  >
  base_iterator insert(const_iterator<U> it,T&& x)
  {
    return filter(
      impl<U>().nv_insert(it,std::move(*static_cast<U*>(subaddress(x)))));
  }

  template<typename InputIterator>
  base_iterator insert(InputIterator first,InputIterator last)
  {
    return filter(
      impl<typename std::iterator_traits<InputIterator>::value_type>().
        nv_insert(first,last));
  }

  template<typename InputIterator>
  base_iterator insert(
    const_base_iterator it,InputIterator first,InputIterator last)
  {
    return insert(
      const_iterator<
        typename std::iterator_traits<InputIterator>::value_type>(it),
      first,last);
  }

  template<typename U,typename InputIterator>
  base_iterator insert(
    const_iterator<U> it,InputIterator first,InputIterator last)
  {
    return filter(impl<U>().nv_insert(it,first,last));
  }

  base_iterator erase(const_base_iterator it)
  {
    return filter(impl().erase(it));
  }

  template<typename U>
  base_iterator erase(const_iterator<U> it)
  {
    return filter(impl<U>().nv_erase(it));
  }

  base_iterator erase(const_base_iterator f,const_base_iterator l)
  {
    return filter(impl().erase(f,l));
  }

  template<typename U>
  base_iterator erase(const_iterator<U> f,const_iterator<U> l)
  {
    return filter(impl<U>().nv_erase(f,l));
  }

  template<typename Iterator>
  base_iterator erase_till_end(Iterator f)
  {
    return filter(impl().erase_till_end(f));
  }

  template<typename Iterator>
  base_iterator erase_from_begin(Iterator l)
  {
    return filter(impl().erase_from_begin(l));
  }
  
  void                 clear()noexcept{filter(impl().clear());}
  template<typename U>
  void                 clear()noexcept{filter(impl<U>().nv_clear());}

private:
  using allocator_traits=std::allocator_traits<Allocator>;
  using segment_backend=typename Model::template segment_backend<Allocator>;
  template<typename Concrete>
  using segment_backend_implementation=typename Model::
    template segment_backend_implementation<Concrete,Allocator>;
  using segment_backend_unique_ptr=
    typename segment_backend::segment_backend_unique_ptr;
  using range=typename segment_backend::range;

  struct from_prototype{};

  segment(segment_backend_unique_ptr&& pimpl):
    pimpl{std::move(pimpl)}{set_sentinel();}
  segment(from_prototype,const segment& x,const allocator_type& al):
    pimpl{x.impl().empty_copy(al)}{set_sentinel();}

  segment_backend&       impl()noexcept{return *pimpl;}
  const segment_backend& impl()const noexcept{return *pimpl;}

  template<typename Concrete>
  segment_backend_implementation<Concrete>& impl()noexcept
  {
    return static_cast<segment_backend_implementation<Concrete>&>(impl());
  }

  template<typename Concrete>
  const segment_backend_implementation<Concrete>& impl()const noexcept
  {
    return
      static_cast<const segment_backend_implementation<Concrete>&>(impl());
  }

  template<typename T>
  static void*         subaddress(T& x){return Model::subaddress(x);}
  template<typename T>
  static const void*   subaddress(const T& x){return Model::subaddress(x);}

  void          set_sentinel(){filter(impl().end());}
  void          filter(base_sentinel x){snt=x;}
  base_iterator filter(const range& x){snt=x.second;return x.first;}

  segment_backend_unique_ptr pimpl;
  base_sentinel              snt;
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
