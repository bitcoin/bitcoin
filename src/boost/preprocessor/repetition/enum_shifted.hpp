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
# ifndef BOOST_PREPROCESSOR_REPETITION_ENUM_SHIFTED_HPP
# define BOOST_PREPROCESSOR_REPETITION_ENUM_SHIFTED_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
# include <boost/preprocessor/punctuation/comma_if.hpp>
# include <boost/preprocessor/repetition/repeat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_ENUM_SHIFTED */
#
# if 0
#    define BOOST_PP_ENUM_SHIFTED(count, macro, data)
# endif
#
# define BOOST_PP_ENUM_SHIFTED BOOST_PP_CAT(BOOST_PP_ENUM_SHIFTED_, BOOST_PP_AUTO_REC(BOOST_PP_REPEAT_P, 4))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ENUM_SHIFTED_1(c, m, d) BOOST_PP_REPEAT_1(BOOST_PP_DEC(c), BOOST_PP_ENUM_SHIFTED_M_1, (m, d))
#    define BOOST_PP_ENUM_SHIFTED_2(c, m, d) BOOST_PP_REPEAT_2(BOOST_PP_DEC(c), BOOST_PP_ENUM_SHIFTED_M_2, (m, d))
#    define BOOST_PP_ENUM_SHIFTED_3(c, m, d) BOOST_PP_REPEAT_3(BOOST_PP_DEC(c), BOOST_PP_ENUM_SHIFTED_M_3, (m, d))
# else
#    define BOOST_PP_ENUM_SHIFTED_1(c, m, d) BOOST_PP_ENUM_SHIFTED_1_I(c, m, d)
#    define BOOST_PP_ENUM_SHIFTED_2(c, m, d) BOOST_PP_ENUM_SHIFTED_1_2(c, m, d)
#    define BOOST_PP_ENUM_SHIFTED_3(c, m, d) BOOST_PP_ENUM_SHIFTED_1_3(c, m, d)
#    define BOOST_PP_ENUM_SHIFTED_1_I(c, m, d) BOOST_PP_REPEAT_1(BOOST_PP_DEC(c), BOOST_PP_ENUM_SHIFTED_M_1, (m, d))
#    define BOOST_PP_ENUM_SHIFTED_2_I(c, m, d) BOOST_PP_REPEAT_2(BOOST_PP_DEC(c), BOOST_PP_ENUM_SHIFTED_M_2, (m, d))
#    define BOOST_PP_ENUM_SHIFTED_3_I(c, m, d) BOOST_PP_REPEAT_3(BOOST_PP_DEC(c), BOOST_PP_ENUM_SHIFTED_M_3, (m, d))
# endif
#
# define BOOST_PP_ENUM_SHIFTED_4(c, m, d) BOOST_PP_ERROR(0x0003)
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ENUM_SHIFTED_M_1(z, n, md) BOOST_PP_ENUM_SHIFTED_M_1_IM(z, n, BOOST_PP_TUPLE_REM_2 md)
#    define BOOST_PP_ENUM_SHIFTED_M_2(z, n, md) BOOST_PP_ENUM_SHIFTED_M_2_IM(z, n, BOOST_PP_TUPLE_REM_2 md)
#    define BOOST_PP_ENUM_SHIFTED_M_3(z, n, md) BOOST_PP_ENUM_SHIFTED_M_3_IM(z, n, BOOST_PP_TUPLE_REM_2 md)
#    define BOOST_PP_ENUM_SHIFTED_M_1_IM(z, n, im) BOOST_PP_ENUM_SHIFTED_M_1_I(z, n, im)
#    define BOOST_PP_ENUM_SHIFTED_M_2_IM(z, n, im) BOOST_PP_ENUM_SHIFTED_M_2_I(z, n, im)
#    define BOOST_PP_ENUM_SHIFTED_M_3_IM(z, n, im) BOOST_PP_ENUM_SHIFTED_M_3_I(z, n, im)
# else
#    define BOOST_PP_ENUM_SHIFTED_M_1(z, n, md) BOOST_PP_ENUM_SHIFTED_M_1_I(z, n, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md))
#    define BOOST_PP_ENUM_SHIFTED_M_2(z, n, md) BOOST_PP_ENUM_SHIFTED_M_2_I(z, n, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md))
#    define BOOST_PP_ENUM_SHIFTED_M_3(z, n, md) BOOST_PP_ENUM_SHIFTED_M_3_I(z, n, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md))
# endif
#
# define BOOST_PP_ENUM_SHIFTED_M_1_I(z, n, m, d) BOOST_PP_COMMA_IF(n) m(z, BOOST_PP_INC(n), d)
# define BOOST_PP_ENUM_SHIFTED_M_2_I(z, n, m, d) BOOST_PP_COMMA_IF(n) m(z, BOOST_PP_INC(n), d)
# define BOOST_PP_ENUM_SHIFTED_M_3_I(z, n, m, d) BOOST_PP_COMMA_IF(n) m(z, BOOST_PP_INC(n), d)
#
# endif
