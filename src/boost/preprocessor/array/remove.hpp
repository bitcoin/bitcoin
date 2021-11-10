# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* Revised by Edward Diener (2020) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_ARRAY_REMOVE_HPP
# define BOOST_PREPROCESSOR_ARRAY_REMOVE_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/array/elem.hpp>
# include <boost/preprocessor/array/push_back.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/comparison/not_equal.hpp>
# include <boost/preprocessor/control/deduce_d.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/tuple/eat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_ARRAY_REMOVE */
#
# define BOOST_PP_ARRAY_REMOVE(array, i) BOOST_PP_ARRAY_REMOVE_I(BOOST_PP_DEDUCE_D(), array, i)
# define BOOST_PP_ARRAY_REMOVE_I(d, array, i) BOOST_PP_ARRAY_REMOVE_D(d, array, i)
#
# /* BOOST_PP_ARRAY_REMOVE_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ARRAY_REMOVE_D(d, array, i) BOOST_PP_TUPLE_ELEM(4, 2, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_REMOVE_P, BOOST_PP_ARRAY_REMOVE_O, (0, i, (0, ()), array)))
# else
#    define BOOST_PP_ARRAY_REMOVE_D(d, array, i) BOOST_PP_ARRAY_REMOVE_D_I(d, array, i)
#    define BOOST_PP_ARRAY_REMOVE_D_I(d, array, i) BOOST_PP_TUPLE_ELEM(4, 2, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_REMOVE_P, BOOST_PP_ARRAY_REMOVE_O, (0, i, (0, ()), array)))
# endif
#
# define BOOST_PP_ARRAY_REMOVE_P(d, st) BOOST_PP_NOT_EQUAL(BOOST_PP_TUPLE_ELEM(4, 0, st), BOOST_PP_ARRAY_SIZE(BOOST_PP_TUPLE_ELEM(4, 3, st)))
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ARRAY_REMOVE_O(d, st) BOOST_PP_ARRAY_REMOVE_O_I st
# else
#    define BOOST_PP_ARRAY_REMOVE_O(d, st) BOOST_PP_ARRAY_REMOVE_O_I(BOOST_PP_TUPLE_ELEM(4, 0, st), BOOST_PP_TUPLE_ELEM(4, 1, st), BOOST_PP_TUPLE_ELEM(4, 2, st), BOOST_PP_TUPLE_ELEM(4, 3, st))
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
#    define BOOST_PP_ARRAY_REMOVE_O_I(n, i, res, arr) (BOOST_PP_INC(n), i, BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(n, i), BOOST_PP_ARRAY_PUSH_BACK, res BOOST_PP_TUPLE_EAT_2)(res, BOOST_PP_ARRAY_ELEM(n, arr)), arr)
# else
#    define BOOST_PP_ARRAY_REMOVE_O_I(n, i, res, arr) (BOOST_PP_INC(n), i, BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(n, i), BOOST_PP_ARRAY_PUSH_BACK, BOOST_PP_TUPLE_ELEM_2_0)(res, BOOST_PP_ARRAY_ELEM(n, arr)), arr)
# endif
#
# else
#
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/array/elem.hpp>
# include <boost/preprocessor/array/push_back.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/comparison/not_equal.hpp>
# include <boost/preprocessor/control/deduce_d.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/control/while.hpp>
# if BOOST_PP_LIMIT_TUPLE == 256
# include <boost/preprocessor/logical/not.hpp>
# endif
# include <boost/preprocessor/tuple/eat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_ARRAY_REMOVE */
#
# define BOOST_PP_ARRAY_REMOVE(array, i) BOOST_PP_ARRAY_REMOVE_I(BOOST_PP_DEDUCE_D(), array, i)
# define BOOST_PP_ARRAY_REMOVE_I(d, array, i) BOOST_PP_ARRAY_REMOVE_D(d, array, i)
#
# /* BOOST_PP_ARRAY_REMOVE_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
# if BOOST_PP_LIMIT_TUPLE == 256
#    define BOOST_PP_ARRAY_REMOVE_D(d, array, i) BOOST_PP_ARRAY_REMOVE_ZERO_D(d, array, i, BOOST_PP_NOT(i))
# else
#    define BOOST_PP_ARRAY_REMOVE_D(d, array, i) BOOST_PP_TUPLE_ELEM(4, 2, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_REMOVE_P, BOOST_PP_ARRAY_REMOVE_O, (0, i, (0, ()), array)))
# endif
# else
#    define BOOST_PP_ARRAY_REMOVE_D(d, array, i) BOOST_PP_ARRAY_REMOVE_D_I(d, array, i)
# if BOOST_PP_LIMIT_TUPLE == 256
#    define BOOST_PP_ARRAY_REMOVE_D_I(d, array, i) BOOST_PP_ARRAY_REMOVE_ZERO_D(d, array, i, BOOST_PP_NOT(i))
# else
#    define BOOST_PP_ARRAY_REMOVE_D_I(d, array, i) BOOST_PP_TUPLE_ELEM(4, 2, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_REMOVE_P, BOOST_PP_ARRAY_REMOVE_O, (0, i, (0, ()), array)))
# endif
# endif
#
# if BOOST_PP_LIMIT_TUPLE == 256
# define BOOST_PP_ARRAY_REMOVE_ZERO_D(d, array, i, zero) \
         BOOST_PP_TUPLE_ELEM(4, 2, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_REMOVE_P, BOOST_PP_ARRAY_REMOVE_O, \
         (1, i, BOOST_PP_IIF(zero,(0, ()),(1, (BOOST_PP_ARRAY_ELEM(0,array)))), array)))
# endif
#
# define BOOST_PP_ARRAY_REMOVE_P(d, st) BOOST_PP_NOT_EQUAL(BOOST_PP_TUPLE_ELEM(4, 0, st), BOOST_PP_ARRAY_SIZE(BOOST_PP_TUPLE_ELEM(4, 3, st)))
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ARRAY_REMOVE_O(d, st) BOOST_PP_ARRAY_REMOVE_O_I st
# else
#    define BOOST_PP_ARRAY_REMOVE_O(d, st) BOOST_PP_ARRAY_REMOVE_O_I(BOOST_PP_TUPLE_ELEM(4, 0, st), BOOST_PP_TUPLE_ELEM(4, 1, st), BOOST_PP_TUPLE_ELEM(4, 2, st), BOOST_PP_TUPLE_ELEM(4, 3, st))
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
#    define BOOST_PP_ARRAY_REMOVE_O_I(n, i, res, arr) (BOOST_PP_INC(n), i, BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(n, i), BOOST_PP_ARRAY_PUSH_BACK, res BOOST_PP_TUPLE_EAT_2)(res, BOOST_PP_ARRAY_ELEM(n, arr)), arr)
# else
#    define BOOST_PP_ARRAY_REMOVE_O_I(n, i, res, arr) (BOOST_PP_INC(n), i, BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(n, i), BOOST_PP_ARRAY_PUSH_BACK, BOOST_PP_TUPLE_ELEM_2_0)(res, BOOST_PP_ARRAY_ELEM(n, arr)), arr)
# endif
#
# endif
#
# endif
