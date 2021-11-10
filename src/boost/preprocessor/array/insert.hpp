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
# ifndef BOOST_PREPROCESSOR_ARRAY_INSERT_HPP
# define BOOST_PREPROCESSOR_ARRAY_INSERT_HPP
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
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_ARRAY_INSERT */
#
# define BOOST_PP_ARRAY_INSERT(array, i, elem) BOOST_PP_ARRAY_INSERT_I(BOOST_PP_DEDUCE_D(), array, i, elem)
# define BOOST_PP_ARRAY_INSERT_I(d, array, i, elem) BOOST_PP_ARRAY_INSERT_D(d, array, i, elem)
#
# /* BOOST_PP_ARRAY_INSERT_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ARRAY_INSERT_D(d, array, i, elem) BOOST_PP_TUPLE_ELEM(5, 3, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_INSERT_P, BOOST_PP_ARRAY_INSERT_O, (0, i, elem, (0, ()), array)))
# else
#    define BOOST_PP_ARRAY_INSERT_D(d, array, i, elem) BOOST_PP_ARRAY_INSERT_D_I(d, array, i, elem)
#    define BOOST_PP_ARRAY_INSERT_D_I(d, array, i, elem) BOOST_PP_TUPLE_ELEM(5, 3, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_INSERT_P, BOOST_PP_ARRAY_INSERT_O, (0, i, elem, (0, ()), array)))
# endif
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ARRAY_INSERT_P(d, state) BOOST_PP_ARRAY_INSERT_P_I state
# else
#    define BOOST_PP_ARRAY_INSERT_P(d, state) BOOST_PP_ARRAY_INSERT_P_I(nil, nil, nil, BOOST_PP_TUPLE_ELEM(5, 3, state), BOOST_PP_TUPLE_ELEM(5, 4, state))
# endif
#
# define BOOST_PP_ARRAY_INSERT_P_I(_i, _ii, _iii, res, arr) BOOST_PP_NOT_EQUAL(BOOST_PP_ARRAY_SIZE(res), BOOST_PP_INC(BOOST_PP_ARRAY_SIZE(arr)))
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ARRAY_INSERT_O(d, state) BOOST_PP_ARRAY_INSERT_O_I state
# else
#    define BOOST_PP_ARRAY_INSERT_O(d, state) BOOST_PP_ARRAY_INSERT_O_I(BOOST_PP_TUPLE_ELEM(5, 0, state), BOOST_PP_TUPLE_ELEM(5, 1, state), BOOST_PP_TUPLE_ELEM(5, 2, state), BOOST_PP_TUPLE_ELEM(5, 3, state), BOOST_PP_TUPLE_ELEM(5, 4, state))
# endif
#
# define BOOST_PP_ARRAY_INSERT_O_I(n, i, elem, res, arr) (BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(BOOST_PP_ARRAY_SIZE(res), i), BOOST_PP_INC(n), n), i, elem, BOOST_PP_ARRAY_PUSH_BACK(res, BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(BOOST_PP_ARRAY_SIZE(res), i), BOOST_PP_ARRAY_ELEM(n, arr), elem)), arr)
#
# else
#
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/array/elem.hpp>
# include <boost/preprocessor/array/push_back.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/comparison/not_equal.hpp>
# include <boost/preprocessor/config/limits.hpp>
# include <boost/preprocessor/control/deduce_d.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/control/while.hpp>
# if BOOST_PP_LIMIT_TUPLE == 256
# include <boost/preprocessor/facilities/identity.hpp>
# include <boost/preprocessor/logical/not.hpp>
# endif
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_ARRAY_INSERT */
#
# define BOOST_PP_ARRAY_INSERT(array, i, elem) BOOST_PP_ARRAY_INSERT_I(BOOST_PP_DEDUCE_D(), array, i, elem)
# define BOOST_PP_ARRAY_INSERT_I(d, array, i, elem) BOOST_PP_ARRAY_INSERT_D(d, array, i, elem)
#
# /* BOOST_PP_ARRAY_INSERT_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
# if BOOST_PP_LIMIT_TUPLE == 256
#    define BOOST_PP_ARRAY_INSERT_D(d, array, i, elem) \
            BOOST_PP_IIF(BOOST_PP_NOT(BOOST_PP_ARRAY_SIZE(array)),BOOST_PP_IDENTITY_N((1,(elem)),5),BOOST_PP_ARRAY_INSERT_ZERO_D)(d, array, i, elem, BOOST_PP_NOT(i))
# else
#    define BOOST_PP_ARRAY_INSERT_D(d, array, i, elem) BOOST_PP_TUPLE_ELEM(5, 3, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_INSERT_P, BOOST_PP_ARRAY_INSERT_O, (0, i, elem, (0, ()), array)))
# endif
# else
#    define BOOST_PP_ARRAY_INSERT_D(d, array, i, elem) BOOST_PP_ARRAY_INSERT_D_I(d, array, i, elem)
# if BOOST_PP_LIMIT_TUPLE == 256
#    define BOOST_PP_ARRAY_INSERT_D_I(d, array, i, elem) \
            BOOST_PP_IIF(BOOST_PP_NOT(BOOST_PP_ARRAY_SIZE(array)),BOOST_PP_IDENTITY_N((1,(elem)),5),BOOST_PP_ARRAY_INSERT_ZERO_D)(d, array, i, elem, BOOST_PP_NOT(i))
# else
#    define BOOST_PP_ARRAY_INSERT_D_I(d, array, i, elem) BOOST_PP_TUPLE_ELEM(5, 3, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_INSERT_P, BOOST_PP_ARRAY_INSERT_O, (0, i, elem, (0, ()), array)))
# endif
# endif
#
# if BOOST_PP_LIMIT_TUPLE == 256
# define BOOST_PP_ARRAY_INSERT_ZERO_D(d, array, i, elem, zero) \
         BOOST_PP_TUPLE_ELEM(5, 3, BOOST_PP_WHILE_ ## d(BOOST_PP_ARRAY_INSERT_P, BOOST_PP_ARRAY_INSERT_O, \
         (1, i, elem, BOOST_PP_IIF( zero, ( 2 , ( elem , BOOST_PP_ARRAY_ELEM(0,array) ) ) , ( 1 , ( BOOST_PP_ARRAY_ELEM(0,array) ) ) ), array)))
# endif
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ARRAY_INSERT_P(d, state) BOOST_PP_ARRAY_INSERT_P_I state
# else
#    define BOOST_PP_ARRAY_INSERT_P(d, state) BOOST_PP_ARRAY_INSERT_P_I(nil, nil, nil, BOOST_PP_TUPLE_ELEM(5, 3, state), BOOST_PP_TUPLE_ELEM(5, 4, state))
# endif
#
# define BOOST_PP_ARRAY_INSERT_P_I(_i, _ii, _iii, res, arr) BOOST_PP_NOT_EQUAL(BOOST_PP_ARRAY_SIZE(res), BOOST_PP_INC(BOOST_PP_ARRAY_SIZE(arr)))
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ARRAY_INSERT_O(d, state) BOOST_PP_ARRAY_INSERT_O_I state
# else
#    define BOOST_PP_ARRAY_INSERT_O(d, state) BOOST_PP_ARRAY_INSERT_O_I(BOOST_PP_TUPLE_ELEM(5, 0, state), BOOST_PP_TUPLE_ELEM(5, 1, state), BOOST_PP_TUPLE_ELEM(5, 2, state), BOOST_PP_TUPLE_ELEM(5, 3, state), BOOST_PP_TUPLE_ELEM(5, 4, state))
# endif
#
# define BOOST_PP_ARRAY_INSERT_O_I(n, i, elem, res, arr) (BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(BOOST_PP_ARRAY_SIZE(res), i), BOOST_PP_INC(n), n), i, elem, BOOST_PP_ARRAY_PUSH_BACK(res, BOOST_PP_IIF(BOOST_PP_NOT_EQUAL(BOOST_PP_ARRAY_SIZE(res), i), BOOST_PP_ARRAY_ELEM(n, arr), elem)), arr)
#
# endif
#
# endif
