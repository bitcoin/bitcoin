/*=============================================================================
    Copyright (c) 2013-2014 Damien Buhl

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_ADT_DETAIL_ADAPT_BASE_ATTR_FILLER_HPP
#define BOOST_FUSION_ADAPTED_ADT_DETAIL_ADAPT_BASE_ATTR_FILLER_HPP

#include <boost/config.hpp>

#include <boost/fusion/adapted/struct/detail/adapt_auto.hpp>
#include <boost/fusion/adapted/struct/detail/preprocessor/is_seq.hpp>

#include <boost/mpl/aux_/preprocessor/token_equal.hpp>

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/logical/or.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/rest_n.hpp>


#define BOOST_FUSION_ADAPT_ADT_WRAPPEDATTR_SIZE(ATTRIBUTE)                      \
  BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE)

#define BOOST_FUSION_ADAPT_ADT_WRAPPEDATTR(ATTRIBUTE)                           \
  BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE)

#if BOOST_PP_VARIADICS

#  define BOOST_FUSION_ADAPT_ADT_FILLER_0(...)                                  \
      BOOST_FUSION_ADAPT_ADT_FILLER(__VA_ARGS__)                                \
      BOOST_FUSION_ADAPT_ADT_FILLER_1

#  define BOOST_FUSION_ADAPT_ADT_FILLER_1(...)                                  \
      BOOST_FUSION_ADAPT_ADT_FILLER(__VA_ARGS__)                                \
      BOOST_FUSION_ADAPT_ADT_FILLER_0

#  define BOOST_FUSION_ADAPT_ADT_FILLER_0_END
#  define BOOST_FUSION_ADAPT_ADT_FILLER_1_END

// MSVC don't compile when using BOOST_PP_BITOR instead of BOOST_PP_OR.
#  define BOOST_FUSION_ADAPT_ADT_FILLER(...)                                    \
      BOOST_PP_IIF(                                                             \
          BOOST_PP_OR(                                                          \
              BOOST_MPL_PP_TOKEN_EQUAL(auto,                                    \
                  BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)),                      \
              BOOST_MPL_PP_TOKEN_EQUAL(auto,                                    \
                  BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__))),                     \
                                                                                \
          BOOST_FUSION_ADAPT_ADT_WRAP_ATTR(                                     \
              BOOST_PP_VARIADIC_ELEM(2, __VA_ARGS__),                           \
              BOOST_FUSION_WORKAROUND_VARIADIC_EMPTINESS_LAST_ELEM(__VA_ARGS__) \
          ),                                                                    \
          BOOST_FUSION_ADAPT_ADT_WRAP_ATTR(__VA_ARGS__))

#  define BOOST_FUSION_ADAPT_ADT_WRAP_ATTR(...)                                 \
      ((BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), (__VA_ARGS__)))

#  define BOOST_FUSION_WORKAROUND_VARIADIC_EMPTINESS_LAST_ELEM(...)             \
  BOOST_PP_SEQ_HEAD(BOOST_PP_SEQ_REST_N(                                        \
            BOOST_PP_DEC(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)),                  \
        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

#else // BOOST_PP_VARIADICS

#  define BOOST_FUSION_ADAPT_ADT_FILLER_0(A, B, C, D)                           \
      BOOST_FUSION_ADAPT_ADT_WRAP_ATTR(A,B,C,D)                                 \
      BOOST_FUSION_ADAPT_ADT_FILLER_1

#  define BOOST_FUSION_ADAPT_ADT_FILLER_1(A, B, C, D)                           \
      BOOST_FUSION_ADAPT_ADT_WRAP_ATTR(A,B,C,D)                                 \
      BOOST_FUSION_ADAPT_ADT_FILLER_0

#  define BOOST_FUSION_ADAPT_ADT_FILLER_0_END
#  define BOOST_FUSION_ADAPT_ADT_FILLER_1_END

#  define BOOST_FUSION_ADAPT_ADT_WRAP_ATTR(A, B, C, D)                          \
      BOOST_PP_IIF(BOOST_MPL_PP_TOKEN_EQUAL(auto, A),                           \
        ((2, (C,D))),                                                           \
        ((4, (A,B,C,D)))                                                        \
      )

#endif // BOOST_PP_VARIADICS

#endif
