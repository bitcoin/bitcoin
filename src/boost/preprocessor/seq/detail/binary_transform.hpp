# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2011.                                  *
#  *     Distributed under the Boost Software License, Version 1.0. (See      *
#  *     accompanying file LICENSE_1_0.txt or copy at                         *
#  *     http://www.boost.org/LICENSE_1_0.txt)                                *
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_SEQ_DETAIL_BINARY_TRANSFORM_HPP
# define BOOST_PREPROCESSOR_SEQ_DETAIL_BINARY_TRANSFORM_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/tuple/eat.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
# include <boost/preprocessor/variadic/detail/is_single_return.hpp>
#
# /* BOOST_PP_SEQ_BINARY_TRANSFORM */
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    define BOOST_PP_SEQ_BINARY_TRANSFORM(seq) BOOST_PP_SEQ_BINARY_TRANSFORM_I(, seq)
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_I(p, seq) BOOST_PP_SEQ_BINARY_TRANSFORM_II(p ## seq)
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_II(seq) BOOST_PP_SEQ_BINARY_TRANSFORM_III(seq)
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_III(seq) BOOST_PP_CAT(BOOST_PP_SEQ_BINARY_TRANSFORM_A seq, 0)
# else
#    define BOOST_PP_SEQ_BINARY_TRANSFORM(seq) BOOST_PP_CAT(BOOST_PP_SEQ_BINARY_TRANSFORM_A seq, 0)
# endif
# if BOOST_PP_VARIADICS_MSVC
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_REM(data) data
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_A(...) (BOOST_PP_SEQ_BINARY_TRANSFORM_REM, __VA_ARGS__)() BOOST_PP_SEQ_BINARY_TRANSFORM_B
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_B(...) (BOOST_PP_SEQ_BINARY_TRANSFORM_REM, __VA_ARGS__)() BOOST_PP_SEQ_BINARY_TRANSFORM_A
# else
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_A(...) (BOOST_PP_REM, __VA_ARGS__)() BOOST_PP_SEQ_BINARY_TRANSFORM_B
#    define BOOST_PP_SEQ_BINARY_TRANSFORM_B(...) (BOOST_PP_REM, __VA_ARGS__)() BOOST_PP_SEQ_BINARY_TRANSFORM_A
# endif
# define BOOST_PP_SEQ_BINARY_TRANSFORM_A0 (BOOST_PP_EAT, ?)
# define BOOST_PP_SEQ_BINARY_TRANSFORM_B0 (BOOST_PP_EAT, ?)
#
# endif
