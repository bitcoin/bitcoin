#ifndef BOOST_METAPARSE_V1_ITERATE_HPP
#define BOOST_METAPARSE_V1_ITERATE_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/iterate_c.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P, class N>
      struct iterate : iterate_c<P, N::type::value> {};
    }
  }
}

#endif

