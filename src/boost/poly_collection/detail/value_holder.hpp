/* Copyright 2016-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_VALUE_HOLDER_HPP
#define BOOST_POLY_COLLECTION_DETAIL_VALUE_HOLDER_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/core/addressof.hpp>
#include <boost/poly_collection/detail/is_constructible.hpp>
#include <boost/poly_collection/detail/is_equality_comparable.hpp>
#include <boost/poly_collection/detail/is_nothrow_eq_comparable.hpp>
#include <boost/poly_collection/exception.hpp>
#include <new>
#include <memory>
#include <type_traits>
#include <utility>

namespace boost{

namespace poly_collection{

namespace detail{

/* Segments of a poly_collection maintain vectors of value_holder<T>
 * rather than directly T. This serves several purposes:
 *  - value_holder<T> is copy constructible and equality comparable even if T
 *    is not: executing the corresponding op results in a reporting exception
 *    being thrown. This allows the segment to offer its full virtual
 *    interface regardless of the properties of the concrete class contained.
 *  - value_holder<T> emulates move assignment when T is not move assignable
 *    (nothrow move constructibility required); this happens most notably with
 *    lambda functions, whose assignment operator is deleted by standard
 *    mandate [expr.prim.lambda]/20 even if the compiler generated one would
 *    work (capture by value).
 *  - value_holder ctors accept a first allocator arg passed by
 *    boost::poly_collection::detail::allocator_adaptor, for purposes
 *    explained there.
 *
 * A pointer to value_holder_base<T> can be reinterpret_cast'ed to T*.
 * Emplacing is explicitly signalled with value_holder_emplacing_ctor to
 * protect us from greedy T's constructible from anything (like
 * boost::type_erasure::any).
 */

struct value_holder_emplacing_ctor_t{};
constexpr value_holder_emplacing_ctor_t value_holder_emplacing_ctor=
  value_holder_emplacing_ctor_t();

template<typename T>
class value_holder_base
{
protected:
  typename std::aligned_storage<sizeof(T),alignof(T)>::type s;
};

template<typename T>
class value_holder:public value_holder_base<T>
{
  template<typename U>
  using enable_if_not_emplacing_ctor_t=typename std::enable_if<
    !std::is_same<
      typename std::decay<U>::type,value_holder_emplacing_ctor_t
    >::value
  >::type*;

  using is_nothrow_move_constructible=std::is_nothrow_move_constructible<T>;
  using is_copy_constructible=std::is_copy_constructible<T>;
  using is_nothrow_copy_constructible=std::is_nothrow_copy_constructible<T>;
  using is_move_assignable=std::is_move_assignable<T>;
  using is_nothrow_move_assignable=std::is_nothrow_move_assignable<T>;
  using is_equality_comparable=detail::is_equality_comparable<T>;
  using is_nothrow_equality_comparable=
    detail::is_nothrow_equality_comparable<T>;

  T*       data()noexcept{return reinterpret_cast<T*>(&this->s);}
  const T* data()const noexcept
                {return reinterpret_cast<const T*>(&this->s);}

  T&       value()noexcept{return *static_cast<T*>(data());}
  const T& value()const noexcept{return *static_cast<const T*>(data());}

public:
  template<
    typename Allocator,
    enable_if_not_emplacing_ctor_t<Allocator> =nullptr
  >
  value_holder(Allocator& al,const T& x)
    noexcept(is_nothrow_copy_constructible::value)
    {copy(al,x);}
  template<
    typename Allocator,
    enable_if_not_emplacing_ctor_t<Allocator> =nullptr
  >
  value_holder(Allocator& al,T&& x)
    noexcept(is_nothrow_move_constructible::value)
    {std::allocator_traits<Allocator>::construct(al,data(),std::move(x));}
  template<
    typename Allocator,typename... Args,
    enable_if_not_emplacing_ctor_t<Allocator> =nullptr
  >
  value_holder(Allocator& al,value_holder_emplacing_ctor_t,Args&&... args)
    {std::allocator_traits<Allocator>::construct(
      al,data(),std::forward<Args>(args)...);}
  template<
    typename Allocator,
    enable_if_not_emplacing_ctor_t<Allocator> =nullptr
  >
  value_holder(Allocator& al,const value_holder& x)
    noexcept(is_nothrow_copy_constructible::value)
    {copy(al,x.value());}
  template<
    typename Allocator,
    enable_if_not_emplacing_ctor_t<Allocator> =nullptr
  >
  value_holder(Allocator& al,value_holder&& x)
    noexcept(is_nothrow_move_constructible::value)
    {std::allocator_traits<Allocator>::construct(
      al,data(),std::move(x.value()));}

  /* stdlib implementations in current use are notoriously lacking at
   * complying with [container.requirements.general]/3, so we keep the
   * following to make their life easier.
   */

  value_holder(const T& x)
    noexcept(is_nothrow_copy_constructible::value)
    {copy(x);}
  value_holder(T&& x)
    noexcept(is_nothrow_move_constructible::value)
    {::new ((void*)data()) T(std::move(x));}
  template<typename... Args>
  value_holder(value_holder_emplacing_ctor_t,Args&&... args)
    {::new ((void*)data()) T(std::forward<Args>(args)...);}
  value_holder(const value_holder& x)
    noexcept(is_nothrow_copy_constructible::value)
    {copy(x.value());}
  value_holder(value_holder&& x)
    noexcept(is_nothrow_move_constructible::value)
    {::new ((void*)data()) T(std::move(x.value()));}
 
  value_holder& operator=(const value_holder& x)=delete;
  value_holder& operator=(value_holder&& x)
    noexcept(is_nothrow_move_assignable::value||!is_move_assignable::value)
    /* if 2nd clause: nothrow move constructibility required */
  {
    move_assign(std::move(x.value()));
    return *this;
  }

  ~value_holder()noexcept{value().~T();}

  friend bool operator==(const value_holder& x,const value_holder& y)
    noexcept(is_nothrow_equality_comparable::value)
  {
    return x.equal(y.value());
  }

private:
  template<typename Allocator>
  void copy(Allocator& al,const T& x){copy(al,x,is_copy_constructible{});}

  template<typename Allocator>
  void copy(Allocator& al,const T& x,std::true_type)
  {
    std::allocator_traits<Allocator>::construct(al,data(),x);
  }

  template<typename Allocator>
  void copy(Allocator&,const T&,std::false_type)
  {
    throw not_copy_constructible{typeid(T)};
  }

  void copy(const T& x){copy(x,is_copy_constructible{});}

  void copy(const T& x,std::true_type)
  {
    ::new (data()) T(x);
  }

  void copy(const T&,std::false_type)
  {
    throw not_copy_constructible{typeid(T)};
  }

  void move_assign(T&& x){move_assign(std::move(x),is_move_assignable{});}

  void move_assign(T&& x,std::true_type)
  {
    value()=std::move(x);    
  }

  void move_assign(T&& x,std::false_type)
  {
    /* emulated assignment */

    static_assert(is_nothrow_move_constructible::value,
      "type should be move assignable or nothrow move constructible");

    if(data()!=boost::addressof(x)){
      value().~T();
      ::new (data()) T(std::move(x));
    }
  }

  bool equal(const T& x)const{return equal(x,is_equality_comparable{});}

  bool equal(const T& x,std::true_type)const
  {
    return value()==x;
  }

  bool equal(const T&,std::false_type)const
  {
    throw not_equality_comparable{typeid(T)};
  }
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
