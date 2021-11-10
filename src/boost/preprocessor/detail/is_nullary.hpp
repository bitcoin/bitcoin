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
# ifndef BOOST_PREPROCESSOR_DETAIL_IS_NULLARY_HPP
# define BOOST_PREPROCESSOR_DETAIL_IS_NULLARY_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/detail/check.hpp>
#
# /* BOOST_PP_IS_NULLARY */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_IS_NULLARY(x) BOOST_PP_CHECK(x, BOOST_PP_IS_NULLARY_CHECK)
# else
#    define BOOST_PP_IS_NULLARY(x) BOOST_PP_IS_NULLARY_I(x)
#    define BOOST_PP_IS_NULLARY_I(x) BOOST_PP_CHECK(x, BOOST_PP_IS_NULLARY_CHECK)
# endif
#
# define BOOST_PP_IS_NULLARY_CHECK() 1
# define BOOST_PP_CHECK_RESULT_BOOST_PP_IS_NULLARY_CHECK 0, BOOST_PP_NIL
#
# endif
