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
# ifndef BOOST_PREPROCESSOR_SEQ_FILTER_HPP
# define BOOST_PREPROCESSOR_SEQ_FILTER_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/expr_if.hpp>
# include <boost/preprocessor/facilities/empty.hpp>
# include <boost/preprocessor/seq/fold_left.hpp>
# include <boost/preprocessor/seq/seq.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
#
# /* BOOST_PP_SEQ_FILTER */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SEQ_FILTER(pred, data, seq) BOOST_PP_SEQ_TAIL(BOOST_PP_TUPLE_ELEM(3, 2, BOOST_PP_SEQ_FOLD_LEFT(BOOST_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# else
#    define BOOST_PP_SEQ_FILTER(pred, data, seq) BOOST_PP_SEQ_FILTER_I(pred, data, seq)
#    define BOOST_PP_SEQ_FILTER_I(pred, data, seq) BOOST_PP_SEQ_TAIL(BOOST_PP_TUPLE_ELEM(3, 2, BOOST_PP_SEQ_FOLD_LEFT(BOOST_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# endif
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_STRICT()
#    define BOOST_PP_SEQ_FILTER_O(s, st, elem) BOOST_PP_SEQ_FILTER_O_IM(s, BOOST_PP_TUPLE_REM_3 st, elem)
#    define BOOST_PP_SEQ_FILTER_O_IM(s, im, elem) BOOST_PP_SEQ_FILTER_O_I(s, im, elem)
# else
#    define BOOST_PP_SEQ_FILTER_O(s, st, elem) BOOST_PP_SEQ_FILTER_O_I(s, BOOST_PP_TUPLE_ELEM(3, 0, st), BOOST_PP_TUPLE_ELEM(3, 1, st), BOOST_PP_TUPLE_ELEM(3, 2, st), elem)
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
#   define BOOST_PP_SEQ_FILTER_O_I(s, pred, data, res, elem) (pred, data, res BOOST_PP_EXPR_IF(pred(s, data, elem), (elem)))
# else
#   define BOOST_PP_SEQ_FILTER_O_I(s, pred, data, res, elem) (pred, data, res BOOST_PP_EXPR_IF(pred##(s, data, elem), (elem)))
# endif
#
# /* BOOST_PP_SEQ_FILTER_S */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SEQ_FILTER_S(s, pred, data, seq) BOOST_PP_SEQ_TAIL(BOOST_PP_TUPLE_ELEM(3, 2, BOOST_PP_SEQ_FOLD_LEFT_ ## s(BOOST_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# else
#    define BOOST_PP_SEQ_FILTER_S(s, pred, data, seq) BOOST_PP_SEQ_FILTER_S_I(s, pred, data, seq)
#    define BOOST_PP_SEQ_FILTER_S_I(s, pred, data, seq) BOOST_PP_SEQ_TAIL(BOOST_PP_TUPLE_ELEM(3, 2, BOOST_PP_SEQ_FOLD_LEFT_ ## s(BOOST_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# endif
#
# endif
