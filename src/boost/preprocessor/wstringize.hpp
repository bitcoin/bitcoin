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
# ifndef BOOST_PREPROCESSOR_WSTRINGIZE_HPP
# define BOOST_PREPROCESSOR_WSTRINGIZE_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# /* BOOST_PP_WSTRINGIZE */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_WSTRINGIZE(text) BOOST_PP_WSTRINGIZE_I(text)
# else
#    define BOOST_PP_WSTRINGIZE(text) BOOST_PP_WSTRINGIZE_OO((text))
#    define BOOST_PP_WSTRINGIZE_OO(par) BOOST_PP_WSTRINGIZE_I ## par
# endif
#
# define BOOST_PP_WSTRINGIZE_I(...) BOOST_PP_WSTRINGIZE_II(#__VA_ARGS__)
# define BOOST_PP_WSTRINGIZE_II(str) L ## str
#
# endif
