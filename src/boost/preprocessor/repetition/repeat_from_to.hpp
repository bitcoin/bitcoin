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
# ifndef BOOST_PREPROCESSOR_REPETITION_REPEAT_FROM_TO_HPP
# define BOOST_PREPROCESSOR_REPETITION_REPEAT_FROM_TO_HPP
#
# include <boost/preprocessor/arithmetic/add.hpp>
# include <boost/preprocessor/arithmetic/sub.hpp>
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
# include <boost/preprocessor/repetition/repeat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_REPEAT_FROM_TO */
#
# if 0
#    define BOOST_PP_REPEAT_FROM_TO(first, last, macro, data)
# endif
#
# define BOOST_PP_REPEAT_FROM_TO BOOST_PP_CAT(BOOST_PP_REPEAT_FROM_TO_, BOOST_PP_AUTO_REC(BOOST_PP_REPEAT_P, 4))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# define BOOST_PP_REPEAT_FROM_TO_1(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_1(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_2(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_2(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_3(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_3(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256), f, l, m, dt)
#
# else
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/config/limits.hpp>
#
# if BOOST_PP_LIMIT_REPEAT == 256
# define BOOST_PP_REPEAT_FROM_TO_1(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_1(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256)), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_2(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_2(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256)), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_3(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_3(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256)), f, l, m, dt)
# elif BOOST_PP_LIMIT_REPEAT == 512
# define BOOST_PP_REPEAT_FROM_TO_1(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_1(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 512)), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_2(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_2(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 512)), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_3(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_3(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 512)), f, l, m, dt)
# elif BOOST_PP_LIMIT_REPEAT == 1024
# define BOOST_PP_REPEAT_FROM_TO_1(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_1(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 1024)), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_2(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_2(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 1024)), f, l, m, dt)
# define BOOST_PP_REPEAT_FROM_TO_3(f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_3(BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 1024)), f, l, m, dt)
# else
# error Incorrect value for the BOOST_PP_LIMIT_REPEAT limit
# endif
#
# endif
#
# define BOOST_PP_REPEAT_FROM_TO_4(f, l, m, dt) BOOST_PP_ERROR(0x0003)
#
# define BOOST_PP_REPEAT_FROM_TO_1ST BOOST_PP_REPEAT_FROM_TO_1
# define BOOST_PP_REPEAT_FROM_TO_2ND BOOST_PP_REPEAT_FROM_TO_2
# define BOOST_PP_REPEAT_FROM_TO_3RD BOOST_PP_REPEAT_FROM_TO_3
#
# /* BOOST_PP_REPEAT_FROM_TO_D */
#
# if 0
#    define BOOST_PP_REPEAT_FROM_TO_D(d, first, last, macro, data)
# endif
#
# define BOOST_PP_REPEAT_FROM_TO_D BOOST_PP_CAT(BOOST_PP_REPEAT_FROM_TO_D_, BOOST_PP_AUTO_REC(BOOST_PP_REPEAT_P, 4))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_REPEAT_FROM_TO_D_1(d, f, l, m, dt) BOOST_PP_REPEAT_1(BOOST_PP_SUB_D(d, l, f), BOOST_PP_REPEAT_FROM_TO_M_1, (d, f, m, dt))
#    define BOOST_PP_REPEAT_FROM_TO_D_2(d, f, l, m, dt) BOOST_PP_REPEAT_2(BOOST_PP_SUB_D(d, l, f), BOOST_PP_REPEAT_FROM_TO_M_2, (d, f, m, dt))
#    define BOOST_PP_REPEAT_FROM_TO_D_3(d, f, l, m, dt) BOOST_PP_REPEAT_3(BOOST_PP_SUB_D(d, l, f), BOOST_PP_REPEAT_FROM_TO_M_3, (d, f, m, dt))
# else
#    define BOOST_PP_REPEAT_FROM_TO_D_1(d, f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_1_I(d, f, l, m, dt)
#    define BOOST_PP_REPEAT_FROM_TO_D_2(d, f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_2_I(d, f, l, m, dt)
#    define BOOST_PP_REPEAT_FROM_TO_D_3(d, f, l, m, dt) BOOST_PP_REPEAT_FROM_TO_D_3_I(d, f, l, m, dt)
#    define BOOST_PP_REPEAT_FROM_TO_D_1_I(d, f, l, m, dt) BOOST_PP_REPEAT_1(BOOST_PP_SUB_D(d, l, f), BOOST_PP_REPEAT_FROM_TO_M_1, (d, f, m, dt))
#    define BOOST_PP_REPEAT_FROM_TO_D_2_I(d, f, l, m, dt) BOOST_PP_REPEAT_2(BOOST_PP_SUB_D(d, l, f), BOOST_PP_REPEAT_FROM_TO_M_2, (d, f, m, dt))
#    define BOOST_PP_REPEAT_FROM_TO_D_3_I(d, f, l, m, dt) BOOST_PP_REPEAT_3(BOOST_PP_SUB_D(d, l, f), BOOST_PP_REPEAT_FROM_TO_M_3, (d, f, m, dt))
# endif
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_REPEAT_FROM_TO_M_1(z, n, dfmd) BOOST_PP_REPEAT_FROM_TO_M_1_IM(z, n, BOOST_PP_TUPLE_REM_4 dfmd)
#    define BOOST_PP_REPEAT_FROM_TO_M_2(z, n, dfmd) BOOST_PP_REPEAT_FROM_TO_M_2_IM(z, n, BOOST_PP_TUPLE_REM_4 dfmd)
#    define BOOST_PP_REPEAT_FROM_TO_M_3(z, n, dfmd) BOOST_PP_REPEAT_FROM_TO_M_3_IM(z, n, BOOST_PP_TUPLE_REM_4 dfmd)
#    define BOOST_PP_REPEAT_FROM_TO_M_1_IM(z, n, im) BOOST_PP_REPEAT_FROM_TO_M_1_I(z, n, im)
#    define BOOST_PP_REPEAT_FROM_TO_M_2_IM(z, n, im) BOOST_PP_REPEAT_FROM_TO_M_2_I(z, n, im)
#    define BOOST_PP_REPEAT_FROM_TO_M_3_IM(z, n, im) BOOST_PP_REPEAT_FROM_TO_M_3_I(z, n, im)
# else
#    define BOOST_PP_REPEAT_FROM_TO_M_1(z, n, dfmd) BOOST_PP_REPEAT_FROM_TO_M_1_I(z, n, BOOST_PP_TUPLE_ELEM(4, 0, dfmd), BOOST_PP_TUPLE_ELEM(4, 1, dfmd), BOOST_PP_TUPLE_ELEM(4, 2, dfmd), BOOST_PP_TUPLE_ELEM(4, 3, dfmd))
#    define BOOST_PP_REPEAT_FROM_TO_M_2(z, n, dfmd) BOOST_PP_REPEAT_FROM_TO_M_2_I(z, n, BOOST_PP_TUPLE_ELEM(4, 0, dfmd), BOOST_PP_TUPLE_ELEM(4, 1, dfmd), BOOST_PP_TUPLE_ELEM(4, 2, dfmd), BOOST_PP_TUPLE_ELEM(4, 3, dfmd))
#    define BOOST_PP_REPEAT_FROM_TO_M_3(z, n, dfmd) BOOST_PP_REPEAT_FROM_TO_M_3_I(z, n, BOOST_PP_TUPLE_ELEM(4, 0, dfmd), BOOST_PP_TUPLE_ELEM(4, 1, dfmd), BOOST_PP_TUPLE_ELEM(4, 2, dfmd), BOOST_PP_TUPLE_ELEM(4, 3, dfmd))
# endif
#
# define BOOST_PP_REPEAT_FROM_TO_M_1_I(z, n, d, f, m, dt) BOOST_PP_REPEAT_FROM_TO_M_1_II(z, BOOST_PP_ADD_D(d, n, f), m, dt)
# define BOOST_PP_REPEAT_FROM_TO_M_2_I(z, n, d, f, m, dt) BOOST_PP_REPEAT_FROM_TO_M_2_II(z, BOOST_PP_ADD_D(d, n, f), m, dt)
# define BOOST_PP_REPEAT_FROM_TO_M_3_I(z, n, d, f, m, dt) BOOST_PP_REPEAT_FROM_TO_M_3_II(z, BOOST_PP_ADD_D(d, n, f), m, dt)
#
# define BOOST_PP_REPEAT_FROM_TO_M_1_II(z, n, m, dt) m(z, n, dt)
# define BOOST_PP_REPEAT_FROM_TO_M_2_II(z, n, m, dt) m(z, n, dt)
# define BOOST_PP_REPEAT_FROM_TO_M_3_II(z, n, m, dt) m(z, n, dt)
#
# endif
