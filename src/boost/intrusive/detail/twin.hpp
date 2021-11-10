/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2014-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_TWIN_HPP
#define BOOST_INTRUSIVE_DETAIL_TWIN_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

//A tiny utility to avoid pulling std::pair / utility for
//very simple algorithms/types

namespace boost {
namespace intrusive {

template <class T>
struct twin
{
   typedef T type;
   twin()
      : first(), second()
   {}

   twin(const type &f, const type &s)
      : first(f), second(s)
   {}

   T first;
   T second;
};

}  //namespace intrusive{
}  //namespace boost{

#endif //BOOST_INTRUSIVE_DETAIL_TWIN_HPP
