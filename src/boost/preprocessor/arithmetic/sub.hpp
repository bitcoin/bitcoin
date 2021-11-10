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
# /* Revised by Edward Diener (2020) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_ARITHMETIC_SUB_HPP
# define BOOST_PREPROCESSOR_ARITHMETIC_SUB_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_SUB */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SUB(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# else
#    define BOOST_PP_SUB(x, y) BOOST_PP_SUB_I(x, y)
#    define BOOST_PP_SUB_I(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# endif
#
# define BOOST_PP_SUB_P(d, xy) BOOST_PP_TUPLE_ELEM(2, 1, xy)
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_SUB_O(d, xy) BOOST_PP_SUB_O_I xy
# else
#    define BOOST_PP_SUB_O(d, xy) BOOST_PP_SUB_O_I(BOOST_PP_TUPLE_ELEM(2, 0, xy), BOOST_PP_TUPLE_ELEM(2, 1, xy))
# endif
#
# define BOOST_PP_SUB_O_I(x, y) (BOOST_PP_DEC(x), BOOST_PP_DEC(y))
#
# /* BOOST_PP_SUB_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SUB_D(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# else
#    define BOOST_PP_SUB_D(d, x, y) BOOST_PP_SUB_D_I(d, x, y)
#    define BOOST_PP_SUB_D_I(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# endif
#
# else
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/facilities/identity.hpp>
# include <boost/preprocessor/logical/and.hpp>
# include <boost/preprocessor/logical/bitor.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/arithmetic/detail/is_maximum_number.hpp>
# include <boost/preprocessor/arithmetic/detail/is_minimum_number.hpp>
#
# /* BOOST_PP_SUB */
#
#    define BOOST_PP_SUB(x, y) BOOST_PP_IIF(BOOST_PP_BITOR(BOOST_PP_DETAIL_IS_MAXIMUM_NUMBER(y),BOOST_PP_DETAIL_IS_MINIMUM_NUMBER(x)),BOOST_PP_IDENTITY_N(0,2),BOOST_PP_SUB_DO)(x,y)
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SUB_DO(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# else
#    define BOOST_PP_SUB_DO(x, y) BOOST_PP_SUB_I(x, y)
#    define BOOST_PP_SUB_I(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# endif
#
# define BOOST_PP_SUB_P(d, xy) BOOST_PP_AND(BOOST_PP_TUPLE_ELEM(2, 1, xy),BOOST_PP_TUPLE_ELEM(2, 0, xy))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_SUB_O(d, xy) BOOST_PP_SUB_O_I xy
# else
#    define BOOST_PP_SUB_O(d, xy) BOOST_PP_SUB_O_I(BOOST_PP_TUPLE_ELEM(2, 0, xy), BOOST_PP_TUPLE_ELEM(2, 1, xy))
# endif
#
# define BOOST_PP_SUB_O_I(x, y) (BOOST_PP_DEC(x), BOOST_PP_DEC(y))
#
# /* BOOST_PP_SUB_D */
#
#    define BOOST_PP_SUB_D(d, x, y) BOOST_PP_IIF(BOOST_PP_BITOR(BOOST_PP_DETAIL_IS_MAXIMUM_NUMBER(y),BOOST_PP_DETAIL_IS_MINIMUM_NUMBER(x)),BOOST_PP_IDENTITY_N(0,3),BOOST_PP_SUB_DO_D)(d,x,y)
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SUB_DO_D(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# else
#    define BOOST_PP_SUB_DO_D(d, x, y) BOOST_PP_SUB_D_I(d, x, y)
#    define BOOST_PP_SUB_D_I(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_SUB_P, BOOST_PP_SUB_O, (x, y)))
# endif
#
# endif
#
# endif
