/* Copyright 2016-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_BASE_MODEL_HPP
#define BOOST_POLY_COLLECTION_DETAIL_BASE_MODEL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/core/addressof.hpp>
#include <boost/poly_collection/detail/is_final.hpp>
#include <boost/poly_collection/detail/packed_segment.hpp>
#include <boost/poly_collection/detail/stride_iterator.hpp>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace boost{

namespace poly_collection{

namespace detail{

/* model for base_collection */

template<typename Base>
struct base_model
{
  using value_type=Base;
  template<typename Derived>
  using is_implementation=std::is_base_of<Base,Derived>;
  template<typename T>
  using is_terminal=is_final<T>; //TODO: should we say !is_polymorhpic||is_final?

private:
  template<typename T>
  using enable_if_not_terminal=
    typename std::enable_if<!is_terminal<T>::value>::type*;
  template<typename T>
  using enable_if_terminal=
    typename std::enable_if<is_terminal<T>::value>::type*;

public:
  template<typename T,enable_if_not_terminal<T> =nullptr>
  static const std::type_info& subtypeid(const T& x){return typeid(x);}

  template<typename T,enable_if_terminal<T> =nullptr>
  static const std::type_info& subtypeid(const T&){return typeid(T);}

  template<typename T,enable_if_not_terminal<T> =nullptr>
  static void* subaddress(T& x)
  {
    return dynamic_cast<void*>(boost::addressof(x));
  }

  template<typename T,enable_if_not_terminal<T> =nullptr>
  static const void* subaddress(const T& x)
  {
    return dynamic_cast<const void*>(boost::addressof(x));
  }

  template<typename T,enable_if_terminal<T> =nullptr>
  static void* subaddress(T& x){return boost::addressof(x);}

  template<typename T,enable_if_terminal<T> =nullptr>
  static const void* subaddress(const T& x){return boost::addressof(x);}

  using base_iterator=stride_iterator<Base>;
  using const_base_iterator=stride_iterator<const Base>;
  using base_sentinel=Base*;
  using const_base_sentinel=const Base*;
  template<typename Derived>
  using iterator=Derived*;
  template<typename Derived>
  using const_iterator=const Derived*;
  template<typename Allocator>
  using segment_backend=detail::segment_backend<base_model,Allocator>;
  template<typename Derived,typename Allocator>
  using segment_backend_implementation=
    packed_segment<base_model,Derived,Allocator>;

  static base_iterator nonconst_iterator(const_base_iterator it)
  {
    return {
      const_cast<value_type*>(static_cast<const value_type*>(it)),
      it.stride()
    };
  }

  template<typename T>
  static iterator<T> nonconst_iterator(const_iterator<T> it)
  {
    return const_cast<iterator<T>>(it);
  }

private:
  template<typename,typename,typename>
  friend class packed_segment;

  template<typename Derived>
  static const Base* value_ptr(const Derived* p)noexcept
  {
    return p;
  }
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
