# /* Copyright (C) 2001
#  * Housemarque Oy
#  * http://www.housemarque.com
#  *
#  * Distributed under the Boost Software License, Version 1.0. (See
#  * accompanying file LICENSE_1_0.txt or copy at
#  * http://www.boost.org/LICENSE_1_0.txt)
#  */
#
# /* Revised by Paul Mensonides (2002) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_LIST_SIZE_HPP
# define BOOST_PREPROCESSOR_LIST_SIZE_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/list/adt.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_LIST_SIZE */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE(list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (0, list)))
# else
#    define BOOST_PP_LIST_SIZE(list) BOOST_PP_LIST_SIZE_I(list)
#    define BOOST_PP_LIST_SIZE_I(list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (0, list)))
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_P(d, rl) BOOST_PP_LIST_IS_CONS(BOOST_PP_TUPLE_ELEM(2, 1, rl))
# else
#    define BOOST_PP_LIST_SIZE_P(d, rl) BOOST_PP_LIST_SIZE_P_I(BOOST_PP_TUPLE_REM_2 rl)
#    define BOOST_PP_LIST_SIZE_P_I(im) BOOST_PP_LIST_SIZE_P_II(im)
#    define BOOST_PP_LIST_SIZE_P_II(r, l) BOOST_PP_LIST_IS_CONS(l)
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_O(d, rl) (BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, rl)), BOOST_PP_LIST_REST(BOOST_PP_TUPLE_ELEM(2, 1, rl)))
# else
#    define BOOST_PP_LIST_SIZE_O(d, rl) BOOST_PP_LIST_SIZE_O_I(BOOST_PP_TUPLE_REM_2 rl)
#    define BOOST_PP_LIST_SIZE_O_I(im) BOOST_PP_LIST_SIZE_O_II(im)
#    define BOOST_PP_LIST_SIZE_O_II(r, l) (BOOST_PP_INC(r), BOOST_PP_LIST_REST(l))
# endif
#
# /* BOOST_PP_LIST_SIZE_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_D(d, list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (0, list)))
# else
#    define BOOST_PP_LIST_SIZE_D(d, list) BOOST_PP_LIST_SIZE_D_I(d, list)
#    define BOOST_PP_LIST_SIZE_D_I(d, list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (0, list)))
# endif
#
# else
#
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/facilities/identity.hpp>
# include <boost/preprocessor/list/adt.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_LIST_SIZE */
#
#    define BOOST_PP_LIST_SIZE(list) BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(list),BOOST_PP_LIST_SIZE_S,BOOST_PP_IDENTITY_N(0,1))(list)
#    define BOOST_PP_LIST_SIZE_S(list) BOOST_PP_LIST_SIZE_DO(BOOST_PP_LIST_REST(list))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_DO(list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (1, list)))
# else
#    define BOOST_PP_LIST_SIZE_DO(list) BOOST_PP_LIST_SIZE_I(list)
#    define BOOST_PP_LIST_SIZE_I(list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (1, list)))
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_P(d, rl) BOOST_PP_LIST_IS_CONS(BOOST_PP_TUPLE_ELEM(2, 1, rl))
# else
#    define BOOST_PP_LIST_SIZE_P(d, rl) BOOST_PP_LIST_SIZE_P_I(BOOST_PP_TUPLE_REM_2 rl)
#    define BOOST_PP_LIST_SIZE_P_I(im) BOOST_PP_LIST_SIZE_P_II(im)
#    define BOOST_PP_LIST_SIZE_P_II(r, l) BOOST_PP_LIST_IS_CONS(l)
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_O(d, rl) (BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, rl)), BOOST_PP_LIST_REST(BOOST_PP_TUPLE_ELEM(2, 1, rl)))
# else
#    define BOOST_PP_LIST_SIZE_O(d, rl) BOOST_PP_LIST_SIZE_O_I(BOOST_PP_TUPLE_REM_2 rl)
#    define BOOST_PP_LIST_SIZE_O_I(im) BOOST_PP_LIST_SIZE_O_II(im)
#    define BOOST_PP_LIST_SIZE_O_II(r, l) (BOOST_PP_INC(r), BOOST_PP_LIST_REST(l))
# endif
#
# /* BOOST_PP_LIST_SIZE_D */
#
#    define BOOST_PP_LIST_SIZE_D(d, list) BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(list),BOOST_PP_LIST_SIZE_S_D,BOOST_PP_IDENTITY_N(0,2))(d,list)
#    define BOOST_PP_LIST_SIZE_S_D(d,list) BOOST_PP_LIST_SIZE_DO_D(d,BOOST_PP_LIST_REST(list))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_SIZE_DO_D(d, list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (1, list)))
# else
#    define BOOST_PP_LIST_SIZE_DO_D(d, list) BOOST_PP_LIST_SIZE_D_I(d, list)
#    define BOOST_PP_LIST_SIZE_D_I(d, list) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_LIST_SIZE_P, BOOST_PP_LIST_SIZE_O, (1, list)))
# endif
#
# endif
#
# endif
