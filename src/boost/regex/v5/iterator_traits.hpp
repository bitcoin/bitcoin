/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         iterator_traits.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares iterator traits workarounds.
  */

#ifndef BOOST_REGEX_V5_ITERATOR_TRAITS_HPP
#define BOOST_REGEX_V5_ITERATOR_TRAITS_HPP

namespace boost{
namespace BOOST_REGEX_DETAIL_NS{

template <class T>
struct regex_iterator_traits : public std::iterator_traits<T> {};

} // namespace BOOST_REGEX_DETAIL_NS
} // namespace boost

#endif

