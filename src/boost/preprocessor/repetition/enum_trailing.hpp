# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_REPETITION_ENUM_TRAILING_HPP
# define BOOST_PREPROCESSOR_REPETITION_ENUM_TRAILING_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/debug/error.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
# include <boost/preprocessor/repetition/repeat.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_ENUM_TRAILING */
#
# if 0
#    define BOOST_PP_ENUM_TRAILING(count, macro, data)
# endif
#
# define BOOST_PP_ENUM_TRAILING BOOST_PP_CAT(BOOST_PP_ENUM_TRAILING_, BOOST_PP_AUTO_REC(BOOST_PP_REPEAT_P, 4))
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ENUM_TRAILING_1(c, m, d) BOOST_PP_REPEAT_1(c, BOOST_PP_ENUM_TRAILING_M_1, (m, d))
#    define BOOST_PP_ENUM_TRAILING_2(c, m, d) BOOST_PP_REPEAT_2(c, BOOST_PP_ENUM_TRAILING_M_2, (m, d))
#    define BOOST_PP_ENUM_TRAILING_3(c, m, d) BOOST_PP_REPEAT_3(c, BOOST_PP_ENUM_TRAILING_M_3, (m, d))
# else
#    define BOOST_PP_ENUM_TRAILING_1(c, m, d) BOOST_PP_ENUM_TRAILING_1_I(c, m, d)
#    define BOOST_PP_ENUM_TRAILING_2(c, m, d) BOOST_PP_ENUM_TRAILING_2_I(c, m, d)
#    define BOOST_PP_ENUM_TRAILING_3(c, m, d) BOOST_PP_ENUM_TRAILING_3_I(c, m, d)
#    define BOOST_PP_ENUM_TRAILING_1_I(c, m, d) BOOST_PP_REPEAT_1(c, BOOST_PP_ENUM_TRAILING_M_1, (m, d))
#    define BOOST_PP_ENUM_TRAILING_2_I(c, m, d) BOOST_PP_REPEAT_2(c, BOOST_PP_ENUM_TRAILING_M_2, (m, d))
#    define BOOST_PP_ENUM_TRAILING_3_I(c, m, d) BOOST_PP_REPEAT_3(c, BOOST_PP_ENUM_TRAILING_M_3, (m, d))
# endif
#
# define BOOST_PP_ENUM_TRAILING_4(c, m, d) BOOST_PP_ERROR(0x0003)
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_ENUM_TRAILING_M_1(z, n, md) BOOST_PP_ENUM_TRAILING_M_1_IM(z, n, BOOST_PP_TUPLE_REM_2 md)
#    define BOOST_PP_ENUM_TRAILING_M_2(z, n, md) BOOST_PP_ENUM_TRAILING_M_2_IM(z, n, BOOST_PP_TUPLE_REM_2 md)
#    define BOOST_PP_ENUM_TRAILING_M_3(z, n, md) BOOST_PP_ENUM_TRAILING_M_3_IM(z, n, BOOST_PP_TUPLE_REM_2 md)
#    define BOOST_PP_ENUM_TRAILING_M_1_IM(z, n, im) BOOST_PP_ENUM_TRAILING_M_1_I(z, n, im)
#    define BOOST_PP_ENUM_TRAILING_M_2_IM(z, n, im) BOOST_PP_ENUM_TRAILING_M_2_I(z, n, im)
#    define BOOST_PP_ENUM_TRAILING_M_3_IM(z, n, im) BOOST_PP_ENUM_TRAILING_M_3_I(z, n, im)
# else
#    define BOOST_PP_ENUM_TRAILING_M_1(z, n, md) BOOST_PP_ENUM_TRAILING_M_1_I(z, n, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md))
#    define BOOST_PP_ENUM_TRAILING_M_2(z, n, md) BOOST_PP_ENUM_TRAILING_M_2_I(z, n, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md))
#    define BOOST_PP_ENUM_TRAILING_M_3(z, n, md) BOOST_PP_ENUM_TRAILING_M_3_I(z, n, BOOST_PP_TUPLE_ELEM(2, 0, md), BOOST_PP_TUPLE_ELEM(2, 1, md))
# endif
#
# define BOOST_PP_ENUM_TRAILING_M_1_I(z, n, m, d) , m(z, n, d)
# define BOOST_PP_ENUM_TRAILING_M_2_I(z, n, m, d) , m(z, n, d)
# define BOOST_PP_ENUM_TRAILING_M_3_I(z, n, m, d) , m(z, n, d)
#
# endif
