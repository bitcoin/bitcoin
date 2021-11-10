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
# ifndef BOOST_PREPROCESSOR_LIST_FOR_EACH_HPP
# define BOOST_PREPROCESSOR_LIST_FOR_EACH_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/list/for_each_i.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_LIST_FOR_EACH */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_FOR_EACH(macro, data, list) BOOST_PP_LIST_FOR_EACH_I(BOOST_PP_LIST_FOR_EACH_O, (macro, data), list)
# else
#    define BOOST_PP_LIST_FOR_EACH(macro, data, list) BOOST_PP_LIST_FOR_EACH_X(macro, data, list)
#    define BOOST_PP_LIST_FOR_EACH_X(macro, data, list) BOOST_PP_LIST_FOR_EACH_I(BOOST_PP_LIST_FOR_EACH_O, (macro, data), list)
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_FOR_EACH_O(r, md, i, elem) BOOST_PP_LIST_FOR_EACH_O_D(r, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md), elem)
# else
#    define BOOST_PP_LIST_FOR_EACH_O(r, md, i, elem) BOOST_PP_LIST_FOR_EACH_O_I(r, BOOST_PP_TUPLE_REM_2 md, elem)
#    define BOOST_PP_LIST_FOR_EACH_O_I(r, im, elem) BOOST_PP_LIST_FOR_EACH_O_D(r, im, elem)
# endif
#
# define BOOST_PP_LIST_FOR_EACH_O_D(r, m, d, elem) m(r, d, elem)
#
# /* BOOST_PP_LIST_FOR_EACH_R */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LIST_FOR_EACH_R(r, macro, data, list) BOOST_PP_LIST_FOR_EACH_I_R(r, BOOST_PP_LIST_FOR_EACH_O, (macro, data), list)
# else
#    define BOOST_PP_LIST_FOR_EACH_R(r, macro, data, list) BOOST_PP_LIST_FOR_EACH_R_X(r, macro, data, list)
#    define BOOST_PP_LIST_FOR_EACH_R_X(r, macro, data, list) BOOST_PP_LIST_FOR_EACH_I_R(r, BOOST_PP_LIST_FOR_EACH_O, (macro, data), list)
# endif
#
# endif
