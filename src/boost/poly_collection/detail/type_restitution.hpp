/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_TYPE_RESTITUTION_HPP
#define BOOST_POLY_COLLECTION_DETAIL_TYPE_RESTITUTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/poly_collection/detail/functional.hpp>
#include <boost/poly_collection/detail/iterator_traits.hpp>
#include <typeinfo>
#include <utility>

namespace boost{

namespace poly_collection{

namespace detail{

/* Given types Ts..., a const std::type_info& info and a local_base_iterator
 * it, we denote by restitute<Ts...>(info,it):
 *   - a local_iterator<Ti> from it, if info==typeid(Ti) for some Ti in Ts...
 *   - it otherwise.
 *
 * Using this notation, restitute_range<Ts...>(f,args...)(s) resolves to
 * f(restitute<Ts...>(info,begin),restitute<Ts...>(info,end),args...) where
 * info=s.type_info(), begin=s.begin(), end=s.end().
 */

template<typename F,typename... Ts>
struct restitute_range_class;
       
template<typename F,typename T,typename... Ts>
struct restitute_range_class<F,T,Ts...>:
  restitute_range_class<F,Ts...>
{
  using super=restitute_range_class<F,Ts...>;
  using super::super;
  
  template<typename SegmentInfo>
  auto operator()(SegmentInfo&& s)
    ->decltype(std::declval<F>()(s.begin(),s.end()))
  {
    using traits=iterator_traits<decltype(s.begin())>;
    using local_iterator=typename traits::template local_iterator<T>;

    if(s.type_info()==typeid(T))
      return (this->f)(
        local_iterator{s.begin()},local_iterator{s.end()});
    else
      return super::operator()(std::forward<SegmentInfo>(s));
  }
};

template<typename F>
struct restitute_range_class<F>
{
  restitute_range_class(const F& f):f(f){}
  
  template<typename SegmentInfo>
  auto operator()(SegmentInfo&& s)
    ->decltype(std::declval<F>()(s.begin(),s.end()))
  {
    return f(s.begin(),s.end());
  }

  F f;
};

template<typename... Ts,typename F,typename... Args>
auto restitute_range(const F& f,Args&&... args)
  ->restitute_range_class<
    decltype(tail_closure(f,std::forward<Args>(args)...)),
    Ts...
  >
{
  return tail_closure(f,std::forward<Args>(args)...);
}

/* restitute_iterator<Ts...>(f,args2...)(index,it,args1...) resolves to
 * f(restitute<Ts...>(index,it),args1...,args2...).
 */

template<typename F,typename... Ts>
struct restitute_iterator_class;
       
template<typename F,typename T,typename... Ts>
struct restitute_iterator_class<F,T,Ts...>:
  restitute_iterator_class<F,Ts...>
{
  using super=restitute_iterator_class<F,Ts...>;
  using super::super;
  
  template<typename Iterator,typename... Args>
  auto operator()(
    const std::type_info& info,Iterator&& it,Args&&... args)
    ->decltype(
      std::declval<F>()
        (std::forward<Iterator>(it),std::forward<Args>(args)...))
  {
    using traits=iterator_traits<typename std::decay<Iterator>::type>;
    using local_iterator=typename traits::template local_iterator<T>;

    if(info==typeid(T))
      return (this->f)(
        local_iterator{it},std::forward<Args>(args)...);
    else
      return super::operator()(
        info,std::forward<Iterator>(it),std::forward<Args>(args)...);
  }
};

template<typename F>
struct restitute_iterator_class<F>
{
  restitute_iterator_class(const F& f):f(f){}
  
  template<typename Iterator,typename... Args>
  auto operator()(
    const std::type_info&,Iterator&& it,Args&&... args)
    ->decltype(
      std::declval<F>()
        (std::forward<Iterator>(it),std::forward<Args>(args)...))
  {
    return f(std::forward<Iterator>(it),std::forward<Args>(args)...);
  }

  F f;
};

template<typename... Ts,typename F,typename... Args>
auto restitute_iterator(const F& f,Args&&... args)
  ->restitute_iterator_class<
    decltype(tail_closure(f,std::forward<Args>(args)...)),
    Ts...
  >
{
  return tail_closure(f,std::forward<Args>(args)...);
}

/* binary_restitute_iterator<Ts...>(f,args...)(index1,it1,index2,it2) resolves
 * to f(restitute<Ts...>(index1,it1),restitute<Ts...>(index2,it2),args...).
 */

template<typename F,typename... Ts>
struct binary_restitute_iterator_class
{
  binary_restitute_iterator_class(const F& f):f(f){}

  template<typename Iterator1,typename Iterator2>
  auto operator()(
    const std::type_info& info1,Iterator1&& it1,
    const std::type_info& info2,Iterator2&& it2)
    ->decltype(
      std::declval<F>()
        (std::forward<Iterator1>(it1),std::forward<Iterator2>(it2)))
  {
    return restitute_iterator<Ts...>(*this)(
      info2,std::forward<Iterator2>(it2),info1,std::forward<Iterator1>(it1));
  }

  template<typename Iterator2,typename Iterator1>
  auto operator()(
    Iterator2&& it2,const std::type_info& info1,Iterator1&& it1)
    ->decltype(
      std::declval<F>()
        (std::forward<Iterator1>(it1),std::forward<Iterator2>(it2)))
  {
    return restitute_iterator<Ts...>(f)(
      info1,std::forward<Iterator1>(it1),std::forward<Iterator2>(it2));
  }
  
  F f;
};

template<typename... Ts,typename F,typename... Args>
auto binary_restitute_iterator(const F& f,Args&&... args)
  ->binary_restitute_iterator_class<
    decltype(tail_closure(f,std::forward<Args>(args)...)),
    Ts...
  >
{
  return tail_closure(f,std::forward<Args>(args)...);
}

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
