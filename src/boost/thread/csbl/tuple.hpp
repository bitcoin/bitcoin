// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_TUPLE_HPP
#define BOOST_CSBL_TUPLE_HPP

#include <boost/config.hpp>

#if defined BOOST_THREAD_USES_BOOST_TUPLE || defined BOOST_NO_CXX11_HDR_TUPLE || defined BOOST_NO_CXX11_RVALUE_REFERENCES
#include <boost/tuple/tuple.hpp>
#ifndef BOOST_THREAD_USES_BOOST_TUPLE
#define BOOST_THREAD_USES_BOOST_TUPLE
#endif

#else
#include <tuple>
#endif

namespace boost
{
  namespace csbl
  {
#if defined BOOST_THREAD_USES_BOOST_TUPLE
    using ::boost::tuple;
    using ::boost::get;
    using ::boost::make_tuple;
    //using ::boost::tuple_size;
#else
    // 20.4.2, class template tuple:
    using ::std::tuple;
    using ::std::get;
    using ::std::make_tuple;
    using ::std::tuple_size;
    // 20.4.2.4, tuple creation functions:
    // 20.4.2.5, tuple helper classes:
    // 20.4.2.6, element access:
    // 20.4.2.7, relational operators:
    // 20.4.2.8, allocator-related traits
    // 20.4.2.9, specialized algorithms:
#endif

  }
}
#endif // header
