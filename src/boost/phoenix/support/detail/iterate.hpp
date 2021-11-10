/*==============================================================================
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !BOOST_PP_IS_ITERATING

#include <boost/preprocessor/tuple/elem.hpp>

#ifndef BOOST_PHOENIX_ITERATION_PARAMS
#error "BOOST_PHOENIX_ITERATION_PARAMS not defined"
#endif

#define BOOST_PHOENIX_ITERATION_PARAMS_SIZE                                     \
    BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PHOENIX_ITERATION_PARAMS)                   \
/**/

#define BOOST_PHOENIX_ITERATION_TUPLE                                           \
    BOOST_PP_TUPLE_ELEM(2, 1, BOOST_PHOENIX_ITERATION_PARAMS)                   \
/**/

#define BOOST_PHOENIX_ITERATION_START                                           \
    BOOST_PP_TUPLE_ELEM(                                                        \
        BOOST_PHOENIX_ITERATION_PARAMS_SIZE, 0, BOOST_PHOENIX_ITERATION_TUPLE)  \
/**/

#define BOOST_PHOENIX_ITERATION_END                                             \
    BOOST_PP_TUPLE_ELEM(                                                        \
        BOOST_PHOENIX_ITERATION_PARAMS_SIZE, 1, BOOST_PHOENIX_ITERATION_TUPLE)  \
/**/


#define BOOST_PHOENIX_ITERATION_FILE()                                          \
    BOOST_PP_TUPLE_ELEM(                                                        \
        BOOST_PHOENIX_ITERATION_PARAMS_SIZE, 2, BOOST_PHOENIX_ITERATION_TUPLE)  \
/**/

#if BOOST_PHOENIX_ITERATION_PARAMS_SIZE == 3

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (3, (BOOST_PHOENIX_ITERATION_START, BOOST_PHOENIX_ITERATION_END,            \
    <boost/phoenix/support/detail/iterate.hpp>))                                \
/**/

#else

#define BOOST_PHOENIX_ITERATION_FLAG                                            \
    BOOST_PP_TUPLE_ELEM(                                                        \
        BOOST_PHOENIX_ITERATION_PARAMS_SIZE, 3, BOOST_PHOENIX_ITERATION_TUPLE)  \
/**/

#define BOOST_PP_ITERATION_PARAMS_1                                             \
    (4, (BOOST_PHOENIX_ITERATION_START, BOOST_PHOENIX_ITERATION_END,            \
    <boost/phoenix/support/detail/iterate.hpp>,                                 \
    BOOST_PHOENIX_ITERATION_FLAG))                                              \
/**/

#endif

#include BOOST_PP_ITERATE()

#undef BOOST_PHOENIX_ITERATION_PARAMS_SIZE
#undef BOOST_PHOENIX_ITERATION_TUPLE
#undef BOOST_PHOENIX_ITERATION_START
#undef BOOST_PHOENIX_ITERATION_END
#undef BOOST_PHOENIX_ITERATION_PARAMS

#else

#undef BOOST_PHOENIX_IS_ITERATING
#define BOOST_PHOENIX_IS_ITERATING 1

#include <boost/phoenix/support/detail/iterate_undef.hpp>
#include <boost/phoenix/support/detail/iterate_define.hpp>


#include BOOST_PHOENIX_ITERATION_FILE()

#undef BOOST_PHOENIX_ITERATION

#undef BOOST_PHOENIX_IS_ITERATING
#define BOOST_PHOENIX_IS_ITERATING 0

#include <boost/phoenix/support/detail/iterate_undef.hpp>
#include <boost/phoenix/support/detail/iterate_define.hpp>

#endif
