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
# ifndef BOOST_PREPROCESSOR_ARITHMETIC_ADD_HPP
# define BOOST_PREPROCESSOR_ARITHMETIC_ADD_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_ADD */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ADD(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# else
#    define BOOST_PP_ADD(x, y) BOOST_PP_ADD_I(x, y)
#    define BOOST_PP_ADD_I(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# endif
#
# define BOOST_PP_ADD_P(d, xy) BOOST_PP_TUPLE_ELEM(2, 1, xy)
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_ADD_O(d, xy) BOOST_PP_ADD_O_I xy
# else
#    define BOOST_PP_ADD_O(d, xy) BOOST_PP_ADD_O_I(BOOST_PP_TUPLE_ELEM(2, 0, xy), BOOST_PP_TUPLE_ELEM(2, 1, xy))
# endif
#
# define BOOST_PP_ADD_O_I(x, y) (BOOST_PP_INC(x), BOOST_PP_DEC(y))
#
# /* BOOST_PP_ADD_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ADD_D(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# else
#    define BOOST_PP_ADD_D(d, x, y) BOOST_PP_ADD_D_I(d, x, y)
#    define BOOST_PP_ADD_D_I(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# endif
#
# else
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/facilities/identity.hpp>
# include <boost/preprocessor/logical/bitand.hpp>
# include <boost/preprocessor/logical/bitor.hpp>
# include <boost/preprocessor/logical/bool.hpp>
# include <boost/preprocessor/logical/compl.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/arithmetic/detail/is_maximum_number.hpp>
# include <boost/preprocessor/arithmetic/detail/is_minimum_number.hpp>
#
# /* BOOST_PP_ADD */
#
#    define BOOST_PP_ADD(x, y) BOOST_PP_IIF(BOOST_PP_BITOR(BOOST_PP_DETAIL_IS_MAXIMUM_NUMBER(y),BOOST_PP_DETAIL_IS_MINIMUM_NUMBER(x)),BOOST_PP_IDENTITY_N(y,2),BOOST_PP_ADD_DO)(x,y)
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ADD_DO(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# else
#    define BOOST_PP_ADD_DO(x, y) BOOST_PP_ADD_I(x, y)
#    define BOOST_PP_ADD_I(x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# endif
#
# define BOOST_PP_ADD_P(d, xy) BOOST_PP_BITAND(BOOST_PP_BOOL(BOOST_PP_TUPLE_ELEM(2, 1, xy)),BOOST_PP_COMPL(BOOST_PP_DETAIL_IS_MAXIMUM_NUMBER(BOOST_PP_TUPLE_ELEM(2, 0, xy))))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_ADD_O(d, xy) BOOST_PP_ADD_O_I xy
# else
#    define BOOST_PP_ADD_O(d, xy) BOOST_PP_ADD_O_I(BOOST_PP_TUPLE_ELEM(2, 0, xy), BOOST_PP_TUPLE_ELEM(2, 1, xy))
# endif
#
# define BOOST_PP_ADD_O_I(x, y) (BOOST_PP_INC(x), BOOST_PP_DEC(y))
#
# /* BOOST_PP_ADD_D */
#
#    define BOOST_PP_ADD_D(d, x, y) BOOST_PP_IIF(BOOST_PP_BITOR(BOOST_PP_DETAIL_IS_MAXIMUM_NUMBER(y),BOOST_PP_DETAIL_IS_MINIMUM_NUMBER(x)),BOOST_PP_IDENTITY_N(y,3),BOOST_PP_ADD_DO_D)(d,x,y)
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ADD_DO_D(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# else
#    define BOOST_PP_ADD_DO_D(d, x, y) BOOST_PP_ADD_D_I(d, x, y)
#    define BOOST_PP_ADD_D_I(d, x, y) BOOST_PP_TUPLE_ELEM(2, 0, BOOST_PP_WHILE_ ## d(BOOST_PP_ADD_P, BOOST_PP_ADD_O, (x, y)))
# endif
#
# endif
#
# endif
