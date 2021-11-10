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
# ifndef BOOST_PREPROCESSOR_FACILITIES_APPLY_HPP
# define BOOST_PREPROCESSOR_FACILITIES_APPLY_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/expr_iif.hpp>
# include <boost/preprocessor/detail/is_unary.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_APPLY */
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_APPLY(x) BOOST_PP_APPLY_I(x)
#    define BOOST_PP_APPLY_I(x) BOOST_PP_EXPR_IIF(BOOST_PP_IS_UNARY(x), BOOST_PP_TUPLE_REM_1 x)
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_BCC()
#    define BOOST_PP_APPLY(x) BOOST_PP_APPLY_I(x)
#    define BOOST_PP_APPLY_I(x) BOOST_PP_APPLY_ ## x
#    define BOOST_PP_APPLY_(x) x
#    define BOOST_PP_APPLY_BOOST_PP_NIL
# else
#    define BOOST_PP_APPLY(x) BOOST_PP_EXPR_IIF(BOOST_PP_IS_UNARY(x), BOOST_PP_TUPLE_REM_1 x)
# endif
#
# endif
