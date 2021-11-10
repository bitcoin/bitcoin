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
# ifndef BOOST_PREPROCESSOR_DEBUG_ASSERT_HPP
# define BOOST_PREPROCESSOR_DEBUG_ASSERT_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/expr_iif.hpp>
# include <boost/preprocessor/control/iif.hpp>
# include <boost/preprocessor/logical/not.hpp>
# include <boost/preprocessor/tuple/eat.hpp>
#
# /* BOOST_PP_ASSERT */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ASSERT BOOST_PP_ASSERT_D
# else
#    define BOOST_PP_ASSERT(cond) BOOST_PP_ASSERT_D(cond)
# endif
#
# define BOOST_PP_ASSERT_D(cond) BOOST_PP_IIF(BOOST_PP_NOT(cond), BOOST_PP_ASSERT_ERROR, BOOST_PP_TUPLE_EAT_1)(...)
# define BOOST_PP_ASSERT_ERROR(x, y, z)
#
# /* BOOST_PP_ASSERT_MSG */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ASSERT_MSG BOOST_PP_ASSERT_MSG_D
# else
#    define BOOST_PP_ASSERT_MSG(cond, msg) BOOST_PP_ASSERT_MSG_D(cond, msg)
# endif
#
# define BOOST_PP_ASSERT_MSG_D(cond, msg) BOOST_PP_EXPR_IIF(BOOST_PP_NOT(cond), msg)
#
# endif
