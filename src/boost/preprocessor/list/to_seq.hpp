# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2011.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* Revised by Paul Mensonides (2011) */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_LIST_TO_SEQ_HPP
# define BOOST_PREPROCESSOR_LIST_TO_SEQ_HPP
#
# include <boost/preprocessor/list/for_each.hpp>
#
# /* BOOST_PP_LIST_TO_SEQ */
#
# define BOOST_PP_LIST_TO_SEQ(list) \
    BOOST_PP_LIST_FOR_EACH(BOOST_PP_LIST_TO_SEQ_MACRO, ~, list) \
    /**/
# define BOOST_PP_LIST_TO_SEQ_MACRO(r, data, elem) (elem)
#
# /* BOOST_PP_LIST_TO_SEQ_R */
#
# define BOOST_PP_LIST_TO_SEQ_R(r, list) \
    BOOST_PP_LIST_FOR_EACH_R(r, BOOST_PP_LIST_TO_SEQ_MACRO, ~, list) \
    /**/
#
# endif /* BOOST_PREPROCESSOR_LIST_TO_SEQ_HPP */
