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
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_REPETITION_ENUM_PARAMS_WITH_DEFAULTS_HPP
# define BOOST_PREPROCESSOR_REPETITION_ENUM_PARAMS_WITH_DEFAULTS_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/repetition/enum_binary_params.hpp>
#
# /* BOOST_PP_ENUM_PARAMS_WITH_DEFAULTS */
#
# define BOOST_PP_ENUM_PARAMS_WITH_DEFAULTS(count, param, def) BOOST_PP_ENUM_BINARY_PARAMS(count, param, = def)
#
# endif
