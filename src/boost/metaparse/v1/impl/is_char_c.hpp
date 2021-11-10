#ifndef BOOST_METAPARSE_V1_IMPL_IS_CHAR_C_HPP
#define BOOST_METAPARSE_V1_IMPL_IS_CHAR_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/bool.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <char C>
        struct is_char_c
        {
          typedef is_char_c type;

          template <class Ch>
          struct apply : boost::mpl::bool_<Ch::type::value == C> {};
        };
      }
    }
  }
}

#endif

