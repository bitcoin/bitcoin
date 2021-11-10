// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_MEMORY_ALLOCATOR_TRAITS_HPP
#define BOOST_CSBL_MEMORY_ALLOCATOR_TRAITS_HPP

#include <boost/thread/csbl/memory/config.hpp>

// 20.7.8, allocator traits
#if defined BOOST_NO_CXX11_ALLOCATOR
#include <boost/container/allocator_traits.hpp>

namespace boost
{
  namespace csbl
  {
    using ::boost::container::allocator_traits;
  }
}
#else
namespace boost
{
  namespace csbl
  {
    using ::std::allocator_traits;
  }
}
#endif // BOOST_NO_CXX11_POINTER_TRAITS

#endif // header
