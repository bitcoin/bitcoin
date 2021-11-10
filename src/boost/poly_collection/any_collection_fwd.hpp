/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_ANY_COLLECTION_FWD_HPP
#define BOOST_POLY_COLLECTION_ANY_COLLECTION_FWD_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <memory>

namespace boost{

namespace poly_collection{

namespace detail{
template<typename Concept> struct any_model;
}

template<typename Concept>
using any_collection_value_type=
  typename detail::any_model<Concept>::value_type;

template<
  typename Concept,
  typename Allocator=std::allocator<any_collection_value_type<Concept>>
>
class any_collection;

template<typename Concept,typename Allocator>
bool operator==(
  const any_collection<Concept,Allocator>& x,
  const any_collection<Concept,Allocator>& y);

template<typename Concept,typename Allocator>
bool operator!=(
  const any_collection<Concept,Allocator>& x,
  const any_collection<Concept,Allocator>& y);

template<typename Concept,typename Allocator>
void swap(
  any_collection<Concept,Allocator>& x,any_collection<Concept,Allocator>& y);

} /* namespace poly_collection */

using poly_collection::any_collection;

} /* namespace boost */

#endif
