/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_DETAIL_ADL_SWAP_HPP
#define BOOST_MULTI_INDEX_DETAIL_ADL_SWAP_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>

namespace boost{

namespace multi_index{

namespace detail{

template<typename T>
void adl_swap(T& x,T& y)
{

#if !defined(BOOST_FUNCTION_SCOPE_USING_DECLARATION_BREAKS_ADL)
  using std::swap;
  swap(x,y);
#else
  std::swap(x,y);
#endif

}

} /* namespace multi_index::detail */

} /* namespace multi_index */

} /* namespace boost */

#endif
