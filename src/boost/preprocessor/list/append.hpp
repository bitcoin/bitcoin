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
# ifndef BOOST_PREPROCESSOR_LIST_APPEND_HPP
# define BOOST_PREPROCESSOR_LIST_APPEND_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/list/fold_right.hpp>
#
# /* BOOST_PP_LIST_APPEND */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_APPEND(a, b) BOOST_PP_LIST_FOLD_RIGHT(BOOST_PP_LIST_APPEND_O, b, a)
# else
#    define BOOST_PP_LIST_APPEND(a, b) BOOST_PP_LIST_APPEND_I(a, b)
#    define BOOST_PP_LIST_APPEND_I(a, b) BOOST_PP_LIST_FOLD_RIGHT(BOOST_PP_LIST_APPEND_O, b, a)
# endif
#
# define BOOST_PP_LIST_APPEND_O(d, s, x) (x, s)
#
# /* BOOST_PP_LIST_APPEND_D */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_APPEND_D(d, a, b) BOOST_PP_LIST_FOLD_RIGHT_ ## d(BOOST_PP_LIST_APPEND_O, b, a)
# else
#    define BOOST_PP_LIST_APPEND_D(d, a, b) BOOST_PP_LIST_APPEND_D_I(d, a, b)
#    define BOOST_PP_LIST_APPEND_D_I(d, a, b) BOOST_PP_LIST_FOLD_RIGHT_ ## d(BOOST_PP_LIST_APPEND_O, b, a)
# endif
#
# endif
