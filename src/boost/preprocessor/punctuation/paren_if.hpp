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
# ifndef BOOST_PREPROCESSOR_PUNCTUATION_PAREN_IF_HPP
# define BOOST_PREPROCESSOR_PUNCTUATION_PAREN_IF_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/punctuation/paren.hpp>
#
# /* BOOST_PP_LPAREN_IF */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_LPAREN_IF(cond) BOOST_PP_IF(cond, BOOST_PP_LPAREN, BOOST_PP_EMPTY)()
# else
#    define BOOST_PP_LPAREN_IF(cond) BOOST_PP_LPAREN_IF_I(cond)
#    define BOOST_PP_LPAREN_IF_I(cond) BOOST_PP_IF(cond, BOOST_PP_LPAREN, BOOST_PP_EMPTY)()
# endif
#
# /* BOOST_PP_RPAREN_IF */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_RPAREN_IF(cond) BOOST_PP_IF(cond, BOOST_PP_RPAREN, BOOST_PP_EMPTY)()
# else
#    define BOOST_PP_RPAREN_IF(cond) BOOST_PP_RPAREN_IF_I(cond)
#    define BOOST_PP_RPAREN_IF_I(cond) BOOST_PP_IF(cond, BOOST_PP_RPAREN, BOOST_PP_EMPTY)()
# endif
#
# endif
