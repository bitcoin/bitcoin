/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_ANY_COLLECTION_HPP
#define BOOST_POLY_COLLECTION_ANY_COLLECTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/poly_collection/any_collection_fwd.hpp>
#include <boost/poly_collection/detail/any_model.hpp>
#include <boost/poly_collection/detail/poly_collection.hpp>
#include <utility>

namespace boost{

namespace poly_collection{

template<typename Concept,typename Allocator>
class any_collection:
 public common_impl::poly_collection<detail::any_model<Concept>,Allocator>
{
  using base_type=common_impl::poly_collection<
    detail::any_model<Concept>,Allocator>;

  base_type&       base()noexcept{return *this;}
  const base_type& base()const noexcept{return *this;}

public:
  using base_type::base_type;

  any_collection()=default;
  any_collection(const any_collection& x)=default;
  any_collection(any_collection&& x)=default;
  any_collection& operator=(const any_collection& x)=default;
  any_collection& operator=(any_collection&& x)=default;
 
  template<typename C,typename A>
  friend bool operator==(
    const any_collection<C,A>&,const any_collection<C,A>&);
};

template<typename Concept,typename Allocator>
bool operator==(
  const any_collection<Concept,Allocator>& x,
  const any_collection<Concept,Allocator>& y)
{
  return x.base()==y.base();
}

template<typename Concept,typename Allocator>
bool operator!=(
  const any_collection<Concept,Allocator>& x,
  const any_collection<Concept,Allocator>& y)
{
 return !(x==y);
}

template<typename Concept,typename Allocator>
void swap(
  any_collection<Concept,Allocator>& x,any_collection<Concept,Allocator>& y)
{
  x.swap(y);
}

} /* namespace  */

using poly_collection::any_collection;

} /* namespace boost */

#endif
