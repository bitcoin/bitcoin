/*=============================================================================
    Copyright (c) 2013-2014 Damien Buhl

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DETAIL_ADAPT_BASE_ASSOC_ATTR_FILLER_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DETAIL_ADAPT_BASE_ASSOC_ATTR_FILLER_HPP

#include <boost/config.hpp>

#include <boost/fusion/adapted/struct/detail/adapt_auto.hpp>
#include <boost/fusion/adapted/struct/detail/adapt_base_attr_filler.hpp>

#include <boost/mpl/aux_/preprocessor/token_equal.hpp>

#include <boost/preprocessor/config/config.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/elem.hpp>

#if BOOST_PP_VARIADICS

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0(...)                           \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAP_ATTR(__VA_ARGS__)                      \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1(...)                           \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAP_ATTR(__VA_ARGS__)                      \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAP_ATTR(...)                          \
  BOOST_PP_IIF(                                                                 \
      BOOST_MPL_PP_TOKEN_EQUAL(auto, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)),   \
      ((2,                                                                      \
          (BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__),                              \
           BOOST_PP_VARIADIC_ELEM(2, __VA_ARGS__)))),                           \
      ((BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),                                    \
          (__VA_ARGS__)))                                                       \
    )

#else // BOOST_PP_VARIADICS


#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0(X, Y, Z)                       \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAP_ATTR(X, Y, Z)                          \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1(X, Y, Z)                       \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAP_ATTR(X, Y, Z)                          \
    BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAP_ATTR(X, Y, Z)                      \
    BOOST_PP_IIF(BOOST_MPL_PP_TOKEN_EQUAL(auto, X),                             \
      ((2, (Y,Z))),                                                             \
      ((3, (X,Y,Z)))                                                            \
    )

#endif // BOOST_PP_VARIADICS

#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_0_END
#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_FILLER_1_END


#define BOOST_FUSION_ADAPT_ASSOC_STRUCT_WRAPPEDATTR_GET_KEY(ATTRIBUTE)          \
    BOOST_PP_TUPLE_ELEM(                                                        \
        BOOST_FUSION_ADAPT_STRUCT_WRAPPEDATTR_SIZE(ATTRIBUTE),                  \
        BOOST_PP_DEC(BOOST_FUSION_ADAPT_STRUCT_WRAPPEDATTR_SIZE(ATTRIBUTE)),    \
        BOOST_FUSION_ADAPT_STRUCT_WRAPPEDATTR(ATTRIBUTE))

#endif
