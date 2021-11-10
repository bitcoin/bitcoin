# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2013.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_TUPLE_PUSH_BACK_HPP
# define BOOST_PREPROCESSOR_TUPLE_PUSH_BACK_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/array/push_back.hpp>
# include <boost/preprocessor/array/to_tuple.hpp>
# include <boost/preprocessor/tuple/to_array.hpp>
#
# /* BOOST_PP_TUPLE_PUSH_BACK */
#
# define BOOST_PP_TUPLE_PUSH_BACK(tuple, elem) \
    BOOST_PP_ARRAY_TO_TUPLE(BOOST_PP_ARRAY_PUSH_BACK(BOOST_PP_TUPLE_TO_ARRAY(tuple), elem)) \
/**/
#
# endif // BOOST_PREPROCESSOR_TUPLE_PUSH_BACK_HPP
