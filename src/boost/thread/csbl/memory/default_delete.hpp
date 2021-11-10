// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_MEMORY_DEFAULT_DELETE_HPP
#define BOOST_CSBL_MEMORY_DEFAULT_DELETE_HPP

#include <boost/thread/csbl/memory/config.hpp>

// 20.8.1 class template unique_ptr:
// default_delete

#if defined BOOST_NO_CXX11_SMART_PTR
#include <boost/move/unique_ptr.hpp>

namespace boost
{
  namespace csbl
  {
    using ::boost::movelib::default_delete;
  }
}
#else
namespace boost
{
  namespace csbl
  {
    using ::std::default_delete;
  }
}
#endif // defined  BOOST_NO_CXX11_SMART_PTR

namespace boost
{
  using ::boost::csbl::default_delete;
}
#endif // header
