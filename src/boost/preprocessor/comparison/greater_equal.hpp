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
# ifndef BOOST_PREPROCESSOR_COMPARISON_GREATER_EQUAL_HPP
# define BOOST_PREPROCESSOR_COMPARISON_GREATER_EQUAL_HPP
#
# include <boost/preprocessor/comparison/less_equal.hpp>
# include <boost/preprocessor/config/config.hpp>
#
# /* BOOST_PP_GREATER_EQUAL */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_GREATER_EQUAL(x, y) BOOST_PP_LESS_EQUAL(y, x)
# else
#    define BOOST_PP_GREATER_EQUAL(x, y) BOOST_PP_GREATER_EQUAL_I(x, y)
#    define BOOST_PP_GREATER_EQUAL_I(x, y) BOOST_PP_LESS_EQUAL(y, x)
# endif
#
# /* BOOST_PP_GREATER_EQUAL_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_GREATER_EQUAL_D(d, x, y) BOOST_PP_LESS_EQUAL_D(d, y, x)
# else
#    define BOOST_PP_GREATER_EQUAL_D(d, x, y) BOOST_PP_GREATER_EQUAL_D_I(d, x, y)
#    define BOOST_PP_GREATER_EQUAL_D_I(d, x, y) BOOST_PP_LESS_EQUAL_D(d, y, x)
# endif
#
# endif
