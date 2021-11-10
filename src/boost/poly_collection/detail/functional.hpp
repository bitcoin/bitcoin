/* Copyright 2016-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_FUNCTIONAL_HPP
#define BOOST_POLY_COLLECTION_DETAIL_FUNCTIONAL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/mp11/integer_sequence.hpp>
#include <tuple>
#include <utility>

/* Assorted functional utilities. Much of this would be almost trivial with
 * C++14 generic lambdas. 
 */

#if BOOST_WORKAROUND(BOOST_MSVC,>=1910)
/* https://lists.boost.org/Archives/boost/2017/06/235687.php */

#define BOOST_POLY_COLLECTION_DEFINE_OVERLOAD_SET(name,f) \
struct name                                               \
{                                                         \
  template<typename... Args>                              \
  auto operator()(Args&&... args)const                    \
  {                                                       \
    return f(std::forward<Args>(args)...);                \
  }                                                       \
};
#else
#define BOOST_POLY_COLLECTION_DEFINE_OVERLOAD_SET(name,f) \
struct name                                               \
{                                                         \
  template<typename... Args>                              \
  auto operator()(Args&&... args)const->                  \
    decltype(f(std::forward<Args>(args)...))              \
  {                                                       \
    return f(std::forward<Args>(args)...);                \
  }                                                       \
};
#endif

namespace boost{

namespace poly_collection{

namespace detail{

template<typename F,typename... TailArgs>
struct tail_closure_class
{
  tail_closure_class(const F& f,std::tuple<TailArgs...> t):f(f),t(t){}

  template<typename... Args>
  using return_type=decltype(
    std::declval<F>()(std::declval<Args>()...,std::declval<TailArgs>()...));

  template<typename... Args,std::size_t... I>
  return_type<Args&&...> call(mp11::index_sequence<I...>,Args&&... args)
  {
    return f(std::forward<Args>(args)...,std::get<I>(t)...);
  }

  template<typename... Args>
  return_type<Args&&...> operator()(Args&&... args)
  {
    return call(
      mp11::make_index_sequence<sizeof...(TailArgs)>{},
      std::forward<Args>(args)...);
  }
  
  F                       f;
  std::tuple<TailArgs...> t; 
};

template<typename F,typename... Args>
tail_closure_class<F,Args&&...> tail_closure(const F& f,Args&&... args)
{
  return {f,std::forward_as_tuple(std::forward<Args>(args)...)};
}

template<typename F,typename... HeadArgs>
struct head_closure_class
{
  head_closure_class(const F& f,std::tuple<HeadArgs...> t):f(f),t(t){}

  template<typename... Args>
  using return_type=decltype(
    std::declval<F>()(std::declval<HeadArgs>()...,std::declval<Args>()...));

  template<typename... Args,std::size_t... I>
  return_type<Args&&...> call(mp11::index_sequence<I...>,Args&&... args)
  {
    return f(std::get<I>(t)...,std::forward<Args>(args)...);
  }

  template<typename... Args>
  return_type<Args&&...> operator()(Args&&... args)
  {
    return call(
      mp11::make_index_sequence<sizeof...(HeadArgs)>{},
      std::forward<Args>(args)...);
  }
  
  F                       f;
  std::tuple<HeadArgs...> t; 
};

template<typename F,typename... Args>
head_closure_class<F,Args&&...> head_closure(const F& f,Args&&... args)
{
  return {f,std::forward_as_tuple(std::forward<Args>(args)...)};
}

template<typename ReturnType,typename F>
struct cast_return_class
{
  cast_return_class(const F& f):f(f){}

  template<typename... Args>
  ReturnType operator()(Args&&... args)const
  {
    return static_cast<ReturnType>(f(std::forward<Args>(args)...));
  }

  F f;
};

template<typename ReturnType,typename F>
cast_return_class<ReturnType,F> cast_return(const F& f)
{
  return {f};
}

template<typename F>
struct deref_to_class
{
  deref_to_class(const F& f):f(f){}

  template<typename... Args>
  auto operator()(Args&&... args)->decltype(std::declval<F>()(*args...))
  {
    return f(*args...);
  }

  F f;
};

template<typename F>
deref_to_class<F> deref_to(const F& f)
{
  return {f};
}

template<typename F>
struct deref_1st_to_class
{
  deref_1st_to_class(const F& f):f(f){}

  template<typename Arg,typename... Args>
  auto operator()(Arg&& arg,Args&&... args)
    ->decltype(std::declval<F>()(*arg,std::forward<Args>(args)...))
  {
    return f(*arg,std::forward<Args>(args)...);
  }

  F f;
};

template<typename F>
deref_1st_to_class<F> deref_1st_to(const F& f)
{
  return {f};
}

struct transparent_equal_to
{
  template<typename T,typename U>
  auto operator()(T&& x,U&& y)const
    noexcept(noexcept(std::forward<T>(x)==std::forward<U>(y)))
    ->decltype(std::forward<T>(x)==std::forward<U>(y))
  {
    return std::forward<T>(x)==std::forward<U>(y);
  }
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
