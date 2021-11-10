#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_POP_FRONT_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_POP_FRONT_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp98/fwd/string.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class S>
        struct pop_front;

        #ifdef BOOST_METAPARSE_POP_FRONT
        #  error BOOST_METAPARSE_POP_FRONT already defined
        #endif
        #define BOOST_METAPARSE_POP_FRONT(z, n, unused) \
          BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) BOOST_PP_CAT(C, n)

        template < \
          BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, int C) \
        >
        struct
          pop_front<
            string<BOOST_PP_ENUM_PARAMS(BOOST_METAPARSE_LIMIT_STRING_SIZE, C)>
          > :
          string<
            BOOST_PP_REPEAT_FROM_TO(
              1,
              BOOST_METAPARSE_LIMIT_STRING_SIZE,
              BOOST_METAPARSE_POP_FRONT,
              ~
            ),
            BOOST_NO_CHAR
          >
        {};

        #undef BOOST_METAPARSE_POP_FRONT
      }
    }
  }
}

#endif

