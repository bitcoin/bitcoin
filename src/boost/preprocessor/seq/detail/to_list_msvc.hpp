# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2016.                                    *
#  *     Distributed under the Boost Software License, Version 1.0. (See      *
#  *     accompanying file LICENSE_1_0.txt or copy at                         *
#  *     http://www.boost.org/LICENSE_1_0.txt)                                *
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_SEQ_DETAIL_TO_LIST_MSVC_HPP
# define BOOST_PREPROCESSOR_SEQ_DETAIL_TO_LIST_MSVC_HPP
#
# include <boost/preprocessor/config/config.hpp>
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
#
# define BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_RESULT(state) \
    BOOST_PP_TUPLE_ELEM(2, 0, state) \
/**/
# define BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_SIZE(state) \
    BOOST_PP_TUPLE_ELEM(2, 1, state) \
/**/
# define BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_PRED(d,state) \
    BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_SIZE(state) \
/**/
# define BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_OP(d,state) \
    ( \
    BOOST_PP_CAT(BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_RESULT(state),), \
    BOOST_PP_DEC(BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_SIZE(state)) \
    ) \
/**/
#
# /* BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC */
#
# define BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC(result,seqsize) \
    BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_RESULT \
        ( \
        BOOST_PP_WHILE \
            ( \
            BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_PRED, \
            BOOST_PP_SEQ_DETAIL_TO_LIST_MSVC_OP, \
            (result,seqsize) \
            ) \
        ) \
/**/
# endif // BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#
# endif // BOOST_PREPROCESSOR_SEQ_DETAIL_TO_LIST_MSVC_HPP
