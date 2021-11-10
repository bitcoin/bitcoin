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
# ifndef BOOST_PREPROCESSOR_LIST_FOLD_RIGHT_HPP
# define BOOST_PREPROCESSOR_LIST_FOLD_RIGHT_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
#
# if 0
#    define BOOST_PP_LIST_FOLD_RIGHT(op, state, list)
# endif
#
# define BOOST_PP_LIST_FOLD_RIGHT BOOST_PP_CAT(BOOST_PP_LIST_FOLD_RIGHT_, BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256))
#
# define BOOST_PP_LIST_FOLD_RIGHT_257(o, s, l) BOOST_PP_ERROR(0x0004)
#
# define BOOST_PP_LIST_FOLD_RIGHT_D(d, o, s, l) BOOST_PP_LIST_FOLD_RIGHT_ ## d(o, s, l)
# define BOOST_PP_LIST_FOLD_RIGHT_2ND BOOST_PP_LIST_FOLD_RIGHT
# define BOOST_PP_LIST_FOLD_RIGHT_2ND_D BOOST_PP_LIST_FOLD_RIGHT_D
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    include <boost/preprocessor/list/detail/edg/fold_right.hpp>
# else
#    include <boost/preprocessor/list/detail/fold_right.hpp>
# endif
#
# else
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
#
# if 0
#    define BOOST_PP_LIST_FOLD_RIGHT(op, state, list)
# endif
#
# include <boost/preprocessor/config/limits.hpp>
#
# if BOOST_PP_LIMIT_WHILE == 256
# define BOOST_PP_LIST_FOLD_RIGHT BOOST_PP_CAT(BOOST_PP_LIST_FOLD_RIGHT_, BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256)))
# define BOOST_PP_LIST_FOLD_RIGHT_257(o, s, l) BOOST_PP_ERROR(0x0004)
# elif BOOST_PP_LIMIT_WHILE == 512
# define BOOST_PP_LIST_FOLD_RIGHT BOOST_PP_CAT(BOOST_PP_LIST_FOLD_RIGHT_, BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 512)))
# define BOOST_PP_LIST_FOLD_RIGHT_513(o, s, l) BOOST_PP_ERROR(0x0004)
# elif BOOST_PP_LIMIT_WHILE == 1024
# define BOOST_PP_LIST_FOLD_RIGHT BOOST_PP_CAT(BOOST_PP_LIST_FOLD_RIGHT_, BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 1024)))
# define BOOST_PP_LIST_FOLD_RIGHT_1025(o, s, l) BOOST_PP_ERROR(0x0004)
# else
# error Incorrect value for the BOOST_PP_LIMIT_WHILE limit
# endif
#
# define BOOST_PP_LIST_FOLD_RIGHT_D(d, o, s, l) BOOST_PP_LIST_FOLD_RIGHT_ ## d(o, s, l)
# define BOOST_PP_LIST_FOLD_RIGHT_2ND BOOST_PP_LIST_FOLD_RIGHT
# define BOOST_PP_LIST_FOLD_RIGHT_2ND_D BOOST_PP_LIST_FOLD_RIGHT_D
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    include <boost/preprocessor/list/detail/edg/fold_right.hpp>
# else
#    include <boost/preprocessor/list/detail/fold_right.hpp>
# endif
#
# endif
#
# endif
