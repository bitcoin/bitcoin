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
# ifndef BOOST_PREPROCESSOR_ARRAY_REVERSE_HPP
# define BOOST_PREPROCESSOR_ARRAY_REVERSE_HPP
#
# include <boost/preprocessor/array/data.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/tuple/reverse.hpp>
#
# /* BOOST_PP_ARRAY_REVERSE */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ARRAY_REVERSE(array) (BOOST_PP_ARRAY_SIZE(array), BOOST_PP_TUPLE_REVERSE(BOOST_PP_ARRAY_SIZE(array), BOOST_PP_ARRAY_DATA(array)))
# else
#    define BOOST_PP_ARRAY_REVERSE(array) BOOST_PP_ARRAY_REVERSE_I(array)
#    define BOOST_PP_ARRAY_REVERSE_I(array) (BOOST_PP_ARRAY_SIZE(array), BOOST_PP_TUPLE_REVERSE(BOOST_PP_ARRAY_SIZE(array), BOOST_PP_ARRAY_DATA(array)))
# endif
#
# endif
