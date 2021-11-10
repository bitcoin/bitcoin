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
# ifndef BOOST_PREPROCESSOR_LIST_REVERSE_HPP
# define BOOST_PREPROCESSOR_LIST_REVERSE_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/list/fold_left.hpp>
#
# /* BOOST_PP_LIST_REVERSE */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_REVERSE(list) BOOST_PP_LIST_FOLD_LEFT(BOOST_PP_LIST_REVERSE_O, BOOST_PP_NIL, list)
# else
#    define BOOST_PP_LIST_REVERSE(list) BOOST_PP_LIST_REVERSE_I(list)
#    define BOOST_PP_LIST_REVERSE_I(list) BOOST_PP_LIST_FOLD_LEFT(BOOST_PP_LIST_REVERSE_O, BOOST_PP_NIL, list)
# endif
#
# define BOOST_PP_LIST_REVERSE_O(d, s, x) (x, s)
#
# /* BOOST_PP_LIST_REVERSE_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_REVERSE_D(d, list) BOOST_PP_LIST_FOLD_LEFT_ ## d(BOOST_PP_LIST_REVERSE_O, BOOST_PP_NIL, list)
# else
#    define BOOST_PP_LIST_REVERSE_D(d, list) BOOST_PP_LIST_REVERSE_D_I(d, list)
#    define BOOST_PP_LIST_REVERSE_D_I(d, list) BOOST_PP_LIST_FOLD_LEFT_ ## d(BOOST_PP_LIST_REVERSE_O, BOOST_PP_NIL, list)
# endif
#
# else
#
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/facilities/identity.hpp>
# include <boost/preprocessor/list/adt.hpp>
# include <boost/preprocessor/list/fold_left.hpp>
#
# /* BOOST_PP_LIST_REVERSE */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_REVERSE(list) BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(list),BOOST_PP_LIST_REVERSE_CONS,BOOST_PP_IDENTITY_N(BOOST_PP_NIL,1))(list)
# else
#    define BOOST_PP_LIST_REVERSE(list) BOOST_PP_LIST_REVERSE_I(list)
#    define BOOST_PP_LIST_REVERSE_I(list) BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(list),BOOST_PP_LIST_REVERSE_CONS,BOOST_PP_IDENTITY_N(BOOST_PP_NIL,1))(list)
# endif
#
# define BOOST_PP_LIST_REVERSE_O(d, s, x) (x, s)
#
# /* BOOST_PP_LIST_REVERSE_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_REVERSE_D(d, list) BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(list),BOOST_PP_LIST_REVERSE_CONS_D,BOOST_PP_IDENTITY_N(BOOST_PP_NIL,2))(d,list)
# else
#    define BOOST_PP_LIST_REVERSE_D(d, list) BOOST_PP_LIST_REVERSE_D_I(d, list)
#    define BOOST_PP_LIST_REVERSE_D_I(d, list) BOOST_PP_IIF(BOOST_PP_LIST_IS_CONS(list),BOOST_PP_LIST_REVERSE_CONS_D,BOOST_PP_IDENTITY_N(BOOST_PP_NIL,2))(d,list)
# endif
#
# define BOOST_PP_LIST_REVERSE_CONS(list) BOOST_PP_LIST_FOLD_LEFT(BOOST_PP_LIST_REVERSE_O, (BOOST_PP_LIST_FIRST(list),BOOST_PP_NIL), BOOST_PP_LIST_REST(list))
# define BOOST_PP_LIST_REVERSE_CONS_D(d, list) BOOST_PP_LIST_FOLD_LEFT_ ## d(BOOST_PP_LIST_REVERSE_O, (BOOST_PP_LIST_FIRST(list),BOOST_PP_NIL), BOOST_PP_LIST_REST(list))
#
# endif
#
# endif
