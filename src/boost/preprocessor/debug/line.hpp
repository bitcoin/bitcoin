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
# ifndef BOOST_PREPROCESSOR_DEBUG_LINE_HPP
# define BOOST_PREPROCESSOR_DEBUG_LINE_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/iteration/iterate.hpp>
# include <boost/preprocessor/stringize.hpp>
#
# /* BOOST_PP_LINE */
#
# if BOOST_PP_CONFIG_EXTENDED_LINE_INFO
#    define BOOST_PP_LINE(line, file) line BOOST_PP_CAT(BOOST_PP_LINE_, BOOST_PP_IS_ITERATING)(file)
#    define BOOST_PP_LINE_BOOST_PP_IS_ITERATING(file) #file
#    define BOOST_PP_LINE_1(file) BOOST_PP_STRINGIZE(file BOOST_PP_CAT(BOOST_PP_LINE_I_, BOOST_PP_ITERATION_DEPTH())())
#    define BOOST_PP_LINE_I_1() [BOOST_PP_FRAME_ITERATION(1)]
#    define BOOST_PP_LINE_I_2() BOOST_PP_LINE_I_1()[BOOST_PP_FRAME_ITERATION(2)]
#    define BOOST_PP_LINE_I_3() BOOST_PP_LINE_I_2()[BOOST_PP_FRAME_ITERATION(3)]
#    define BOOST_PP_LINE_I_4() BOOST_PP_LINE_I_3()[BOOST_PP_FRAME_ITERATION(4)]
#    define BOOST_PP_LINE_I_5() BOOST_PP_LINE_I_4()[BOOST_PP_FRAME_ITERATION(5)]
# else
#    define BOOST_PP_LINE(line, file) line __FILE__
# endif
#
# endif
