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
# /* Revised by Edward Diener (2015) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_FACILITIES_IDENTITY_HPP
# define BOOST_PREPROCESSOR_FACILITIES_IDENTITY_HPP
#
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/tuple/eat.hpp>
#
# /* BOOST_PP_IDENTITY */
#
# define BOOST_PP_IDENTITY(item) item BOOST_PP_EMPTY
#
# define BOOST_PP_IDENTITY_N(item,n) item BOOST_PP_TUPLE_EAT_N(n)
#
# endif
