# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2015.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_SEQ_DETAIL_IS_EMPTY_HPP
# define BOOST_PREPROCESSOR_SEQ_DETAIL_IS_EMPTY_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/logical/bool.hpp>
# include <boost/preprocessor/logical/compl.hpp>
# include <boost/preprocessor/seq/size.hpp>
#
/* An empty seq is one that is just BOOST_PP_SEQ_NIL */
#
# define BOOST_PP_SEQ_DETAIL_IS_EMPTY(seq) \
    BOOST_PP_COMPL \
        ( \
        BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq) \
        ) \
/**/
#
# define BOOST_PP_SEQ_DETAIL_IS_EMPTY_SIZE(size) \
    BOOST_PP_COMPL \
        ( \
        BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(size) \
        ) \
/**/
#
# define BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq) \
    BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(BOOST_PP_SEQ_DETAIL_EMPTY_SIZE(seq)) \
/**/
#
# define BOOST_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(size) \
    BOOST_PP_BOOL(size) \
/**/
#
# define BOOST_PP_SEQ_DETAIL_EMPTY_SIZE(seq) \
    BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(seq (nil))) \
/**/
#
# endif
