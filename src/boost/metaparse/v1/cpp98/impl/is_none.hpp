#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_IS_NONE_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_IS_NONE_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/limit_one_char_except_size.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/bool.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
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
        template <class Stub = int>
        struct is_none0
        {
          template <class C>
          struct apply : boost::mpl::true_ {};
        };

        #ifdef BOOST_METAPARSE_DEFINE_IS_NONE
        #  error BOOST_METAPARSE_DEFINE_IS_NONE already defined
        #endif
        #define BOOST_METAPARSE_DEFINE_IS_NONE(z, n, unused) \
          template <BOOST_PP_ENUM_PARAMS(n, class T)> \
          struct BOOST_PP_CAT(is_none, n) \
          { \
            template <class C> \
            struct apply : \
              boost::mpl::eval_if< \
                boost::mpl::bool_< \
                  C::type::value \
                  == BOOST_PP_CAT(T, BOOST_PP_DEC(n))::type::value \
                >, \
                boost::mpl::false_, \
                typename BOOST_PP_CAT(is_none, BOOST_PP_DEC(n))< \
                  BOOST_PP_ENUM_PARAMS(BOOST_PP_DEC(n), T) \
                >::template apply<C> \
              > \
            {}; \
          };

        BOOST_PP_REPEAT_FROM_TO(
          1,
          BOOST_METAPARSE_LIMIT_ONE_CHAR_EXCEPT_SIZE,
          BOOST_METAPARSE_DEFINE_IS_NONE,
          ~
        )

        #undef BOOST_METAPARSE_DEFINE_IS_NONE
      }
    }
  }
}

#endif

