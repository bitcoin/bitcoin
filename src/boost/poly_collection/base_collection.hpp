/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_BASE_COLLECTION_HPP
#define BOOST_POLY_COLLECTION_BASE_COLLECTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/poly_collection/base_collection_fwd.hpp>
#include <boost/poly_collection/detail/base_model.hpp>
#include <boost/poly_collection/detail/poly_collection.hpp>
#include <utility>

namespace boost{

namespace poly_collection{

template<typename Base,typename Allocator>
class base_collection:
 public common_impl::poly_collection<detail::base_model<Base>,Allocator>
{
  using base_type=common_impl::poly_collection<
    detail::base_model<Base>,Allocator>;

  base_type&       base()noexcept{return *this;}
  const base_type& base()const noexcept{return *this;}

public:
  using base_type::base_type;

  base_collection()=default;
  base_collection(const base_collection& x)=default;
  base_collection(base_collection&& x)=default;
  base_collection& operator=(const base_collection& x)=default;
  base_collection& operator=(base_collection&& x)=default;
   
  template<typename B,typename A>
  friend bool operator==(
    const base_collection<B,A>&,const base_collection<B,A>&);
};

template<typename Base,typename Allocator>
bool operator==(
  const base_collection<Base,Allocator>& x,
  const base_collection<Base,Allocator>& y)
{
  return x.base()==y.base();
}

template<typename Base,typename Allocator>
bool operator!=(
  const base_collection<Base,Allocator>& x,
  const base_collection<Base,Allocator>& y)
{
 return !(x==y);
}

template<typename Base,typename Allocator>
void swap(
  base_collection<Base,Allocator>& x,base_collection<Base,Allocator>& y)
{
  x.swap(y);
}

} /* namespace  */

using poly_collection::base_collection;

} /* namespace boost */

#endif
