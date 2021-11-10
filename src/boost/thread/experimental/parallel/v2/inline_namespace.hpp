#ifndef BOOST_THREAD_EXPERIMENTAL_PARALLEL_V2_INLINE_NAMESPACE_HPP
#define BOOST_THREAD_EXPERIMENTAL_PARALLEL_V2_INLINE_NAMESPACE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/experimental/config/inline_namespace.hpp>

namespace boost {
namespace experimental {
namespace parallel {

  BOOST_THREAD_INLINE_NAMESPACE(v2) {}

#if defined(BOOST_NO_CXX11_INLINE_NAMESPACES)
  using namespace v2;
#endif

}
}
}
#endif
