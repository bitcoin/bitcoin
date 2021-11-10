#ifndef BOOST_METAPARSE_V1_FAIL_HPP
#define BOOST_METAPARSE_V1_FAIL_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/reject.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class Msg>
      struct fail
      {
        typedef fail type;
        
        template <class S, class Pos>
        struct apply : reject<Msg, Pos> {};
      };
    }
  }
}

#endif

