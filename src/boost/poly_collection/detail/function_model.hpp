/* Copyright 2016-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_FUNCTION_MODEL_HPP
#define BOOST_POLY_COLLECTION_DETAIL_FUNCTION_MODEL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/core/addressof.hpp>
#include <boost/poly_collection/detail/callable_wrapper.hpp>
#include <boost/poly_collection/detail/callable_wrapper_iterator.hpp>
#include <boost/poly_collection/detail/is_invocable.hpp>
#include <boost/poly_collection/detail/segment_backend.hpp>
#include <boost/poly_collection/detail/split_segment.hpp>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace boost{

namespace poly_collection{

namespace detail{

/* model for function_collection */

template<typename Signature>
struct function_model;

/* is_terminal defined out-class to allow for partial specialization */

template<typename T>
struct function_model_is_terminal:std::true_type{};

template<typename Signature>
struct function_model_is_terminal<callable_wrapper<Signature>>:
  std::false_type{};

template<typename R,typename... Args>
struct function_model<R(Args...)>
{
  using value_type=callable_wrapper<R(Args...)>;

  template<typename Callable>
  using is_implementation=is_invocable_r<R,Callable&,Args...>;

  template<typename T>
  using is_terminal=function_model_is_terminal<T>;

  template<typename T>
  static const std::type_info& subtypeid(const T&){return typeid(T);}

  template<typename Signature>
  static const std::type_info& subtypeid(
    const callable_wrapper<Signature>& f)
  {
    return f.target_type();
  }

  template<typename T>
  static void* subaddress(T& x){return boost::addressof(x);}

  template<typename T>
  static const void* subaddress(const T& x){return boost::addressof(x);}

  template<typename Signature>
  static void* subaddress(callable_wrapper<Signature>& f)
  {
    return f.data();
  }
  
  template<typename Signature>
  static const void* subaddress(const callable_wrapper<Signature>& f)
  {
    return f.data();
  }

  using base_iterator=callable_wrapper_iterator<value_type>;
  using const_base_iterator=callable_wrapper_iterator<const value_type>;
  using base_sentinel=value_type*;
  using const_base_sentinel=const value_type*;
  template<typename Callable>
  using iterator=Callable*;
  template<typename Callable>
  using const_iterator=const Callable*;
  template<typename Allocator>
  using segment_backend=detail::segment_backend<function_model,Allocator>;
  template<typename Callable,typename Allocator>
  using segment_backend_implementation=
    split_segment<function_model,Callable,Allocator>;

  static base_iterator nonconst_iterator(const_base_iterator it)
  {
    return base_iterator{
      const_cast<value_type*>(static_cast<const value_type*>(it))};
  }

  template<typename T>
  static iterator<T> nonconst_iterator(const_iterator<T> it)
  {
    return const_cast<iterator<T>>(it);
  }

private:
  template<typename,typename,typename>
  friend class split_segment;

  template<typename Callable>
  static value_type make_value_type(Callable& x){return value_type{x};}
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
