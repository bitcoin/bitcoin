#ifndef BOOST_METAPARSE_V1_RETURN__HPP
#define BOOST_METAPARSE_V1_RETURN__HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/get_result.hpp>


#include <iostream>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class C>
      struct return_
      {
        typedef return_ type;
        
        template <class S, class Pos>
        struct apply : accept<C, S, Pos> {};
      };
    }
  }
}

#endif

