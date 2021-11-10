#ifndef BOOST_METAPARSE_V1_CPP11_IMPL_ANY_OF_C_HPP
#define BOOST_METAPARSE_V1_CPP11_IMPL_ANY_OF_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp11/impl/or_c.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <char... Cs>
        struct any_of_c
        {
          typedef any_of_c type;

          template <class Chr>
          struct apply : or_c<(Chr::type::value == Cs)...> {};
        };
      }
    }
  }
}

#endif

