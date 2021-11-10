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
# ifndef BOOST_PREPROCESSOR_LIST_ENUM_HPP
# define BOOST_PREPROCESSOR_LIST_ENUM_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/list/adt.hpp>
# include <boost/preprocessor/list/for_each_i.hpp>
# include <boost/preprocessor/punctuation/comma_if.hpp>
# include <boost/preprocessor/variadic/has_opt.hpp>
#
# /* BOOST_PP_LIST_ENUM */
#
# if BOOST_PP_VARIADIC_HAS_OPT()
#    if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#        define BOOST_PP_LIST_ENUM_NOT_EMPTY(list) BOOST_PP_LIST_FOR_EACH_I(BOOST_PP_LIST_ENUM_O, BOOST_PP_NIL, list)
#    else
#        define BOOST_PP_LIST_ENUM_NOT_EMPTY(list) BOOST_PP_LIST_ENUM_I(list)
#        define BOOST_PP_LIST_ENUM_I(list) BOOST_PP_LIST_FOR_EACH_I(BOOST_PP_LIST_ENUM_O, BOOST_PP_NIL, list)
#    endif
#    define BOOST_PP_LIST_ENUM_EMPTY(list)
#    define BOOST_PP_LIST_ENUM(list) BOOST_PP_IIF(BOOST_PP_LIST_IS_NIL(list),BOOST_PP_LIST_ENUM_EMPTY,BOOST_PP_LIST_ENUM_NOT_EMPTY)(list)
# elif ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_ENUM(list) BOOST_PP_LIST_FOR_EACH_I(BOOST_PP_LIST_ENUM_O, BOOST_PP_NIL, list)
# else
#    define BOOST_PP_LIST_ENUM(list) BOOST_PP_LIST_ENUM_I(list)
#    define BOOST_PP_LIST_ENUM_I(list) BOOST_PP_LIST_FOR_EACH_I(BOOST_PP_LIST_ENUM_O, BOOST_PP_NIL, list)
# endif
#
# define BOOST_PP_LIST_ENUM_O(r, _, i, elem) BOOST_PP_COMMA_IF(i) elem
#
# /* BOOST_PP_LIST_ENUM_R */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_ENUM_R(r, list) BOOST_PP_LIST_FOR_EACH_I_R(r, BOOST_PP_LIST_ENUM_O, BOOST_PP_NIL, list)
# else
#    define BOOST_PP_LIST_ENUM_R(r, list) BOOST_PP_LIST_ENUM_R_I(r, list)
#    define BOOST_PP_LIST_ENUM_R_I(r, list) BOOST_PP_LIST_FOR_EACH_I_R(r, BOOST_PP_LIST_ENUM_O, BOOST_PP_NIL, list)
# endif
#
# endif
