// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_MEMORY_SCOPED_ALLOCATOR_HPP
#define BOOST_CSBL_MEMORY_SCOPED_ALLOCATOR_HPP

#include <boost/thread/csbl/memory/config.hpp>

// 20.7.7, uses_allocator
#if defined BOOST_NO_CXX11_ALLOCATOR
#include <boost/container/scoped_allocator.hpp>

namespace boost
{
  namespace csbl
  {
    using ::boost::container::uses_allocator;
  }
}
#else
namespace boost
{
  namespace csbl
  {
    using ::std::uses_allocator;
  }
}
#endif // BOOST_NO_CXX11_POINTER_TRAITS

#endif // header
