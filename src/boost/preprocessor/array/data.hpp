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
# ifndef BOOST_PREPROCESSOR_ARRAY_DATA_HPP
# define BOOST_PREPROCESSOR_ARRAY_DATA_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# /* BOOST_PP_ARRAY_DATA */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_ARRAY_DATA(array) BOOST_PP_TUPLE_ELEM(2, 1, array)
# else
#    define BOOST_PP_ARRAY_DATA(array) BOOST_PP_ARRAY_DATA_I(array)
#    define BOOST_PP_ARRAY_DATA_I(array) BOOST_PP_ARRAY_DATA_II array
#    define BOOST_PP_ARRAY_DATA_II(size, data) data
# endif
#
# endif
