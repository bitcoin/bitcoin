// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_MEMORY_POINTER_TRAITS_HPP
#define BOOST_CSBL_MEMORY_POINTER_TRAITS_HPP

#include <boost/thread/csbl/memory/config.hpp>

// 20.7.3, pointer traits
#if defined BOOST_NO_CXX11_ALLOCATOR
#include <boost/intrusive/pointer_traits.hpp>

namespace boost
{
  namespace csbl
  {
    using ::boost::intrusive::pointer_traits;
  }
}
#else
namespace boost
{
  namespace csbl
  {
    using ::std::pointer_traits;
  }
}
#endif // BOOST_NO_CXX11_ALLOCATOR

#endif // header
