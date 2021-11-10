# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# ifndef BOOST_PREPROCESSOR_DETAIL_SPLIT_HPP
# define BOOST_PREPROCESSOR_DETAIL_SPLIT_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# /* BOOST_PP_SPLIT */
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_SPLIT(n, im) BOOST_PP_SPLIT_I((n, im))
#    define BOOST_PP_SPLIT_I(par) BOOST_PP_SPLIT_II ## par
#    define BOOST_PP_SPLIT_II(n, a, b) BOOST_PP_SPLIT_ ## n(a, b)
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    define BOOST_PP_SPLIT(n, im) BOOST_PP_SPLIT_I(n((im)))
#    define BOOST_PP_SPLIT_I(n) BOOST_PP_SPLIT_ID(BOOST_PP_SPLIT_II_ ## n)
#    define BOOST_PP_SPLIT_II_0(s) BOOST_PP_SPLIT_ID(BOOST_PP_SPLIT_0 s)
#    define BOOST_PP_SPLIT_II_1(s) BOOST_PP_SPLIT_ID(BOOST_PP_SPLIT_1 s)
#    define BOOST_PP_SPLIT_ID(id) id
# else
#    define BOOST_PP_SPLIT(n, im) BOOST_PP_SPLIT_I(n)(im)
#    define BOOST_PP_SPLIT_I(n) BOOST_PP_SPLIT_ ## n
# endif
#
# define BOOST_PP_SPLIT_0(a, b) a
# define BOOST_PP_SPLIT_1(a, b) b
#
# endif
