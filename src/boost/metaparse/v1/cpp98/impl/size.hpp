#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_SIZE_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_SIZE_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp98/fwd/string.hpp>

#include <boost/mpl/int.hpp>

#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class S>
        struct size;

        #ifdef BOOST_METAPARSE_STRING_CASE
        #  error BOOST_METAPARSE_STRING_CASE
        #endif
        #define BOOST_METAPARSE_STRING_CASE(z, n, unused) \
          template <BOOST_PP_ENUM_PARAMS(n, int C)> \
          struct \
            size< \
              string< \
                BOOST_PP_ENUM_PARAMS(n, C) BOOST_PP_COMMA_IF(n) \
                BOOST_PP_ENUM( \
                  BOOST_PP_SUB(BOOST_METAPARSE_LIMIT_STRING_SIZE, n), \
                  BOOST_NO_CHAR BOOST_PP_TUPLE_EAT(3), \
                  ~ \
                ) \
              > \
            > : \
            boost::mpl::int_<n> \
          {};
  
        BOOST_PP_REPEAT(
          BOOST_METAPARSE_LIMIT_STRING_SIZE,
          BOOST_METAPARSE_STRING_CASE,
          ~
        )
  
        #undef BOOST_METAPARSE_STRING_CASE
      }
    }
  }
}

#endif

