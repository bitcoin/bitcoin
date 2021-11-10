/* Copyright 2016 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_AUTO_ITERATOR_HPP
#define BOOST_POLY_COLLECTION_DETAIL_AUTO_ITERATOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/iterator/iterator_adaptor.hpp>

namespace boost{

namespace poly_collection{

namespace detail{

/* auto_iterator<Iterator> (for want of a better name) behaves like Iterator
 * save for the fact that it derefs to Iterator& rather than
 * Iterator::reference. This is useful to "lift" std algorithms so that
 * user-defined predicates are passed iterators that can then be dereferenced
 * internally.
 */

template<typename Iterator>
class auto_iterator:
  public boost::iterator_adaptor<auto_iterator<Iterator>,Iterator,Iterator>
{
public:
  auto_iterator()=default;
  auto_iterator(const Iterator& it):auto_iterator::iterator_adaptor_{it}{}
  auto_iterator(const auto_iterator&)=default;
  auto_iterator& operator=(const auto_iterator&)=default;

private:
  friend class boost::iterator_core_access;

  Iterator& dereference()const noexcept
  {
    return const_cast<auto_iterator*>(this)->base_reference();
  }
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
