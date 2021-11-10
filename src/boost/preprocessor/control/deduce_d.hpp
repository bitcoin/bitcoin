# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* Revised by Edward Diener (2020) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_CONTROL_DEDUCE_D_HPP
# define BOOST_PREPROCESSOR_CONTROL_DEDUCE_D_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
#
# /* BOOST_PP_DEDUCE_D */
#
# define BOOST_PP_DEDUCE_D() BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256)
#
# else
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/detail/auto_rec.hpp>
# include <boost/preprocessor/config/limits.hpp>
#
# /* BOOST_PP_DEDUCE_D */
#
# if BOOST_PP_LIMIT_WHILE == 256
# define BOOST_PP_DEDUCE_D() BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 256))
# elif BOOST_PP_LIMIT_WHILE == 512
# define BOOST_PP_DEDUCE_D() BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 512))
# elif BOOST_PP_LIMIT_WHILE == 1024
# define BOOST_PP_DEDUCE_D() BOOST_PP_DEC(BOOST_PP_AUTO_REC(BOOST_PP_WHILE_P, 1024))
# else
# error Incorrect value for the BOOST_PP_LIMIT_WHILE limit
# endif
#
# endif
#
# endif
