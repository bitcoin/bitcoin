# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2003.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_FACILITIES_IS_1_HPP
# define BOOST_PREPROCESSOR_FACILITIES_IS_1_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/facilities/is_empty.hpp>
#
# /* BOOST_PP_IS_1 */
#
# define BOOST_PP_IS_1(x) BOOST_PP_IS_EMPTY(BOOST_PP_CAT(BOOST_PP_IS_1_HELPER_, x))
# define BOOST_PP_IS_1_HELPER_1
#
# endif
