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
# ifndef BOOST_PREPROCESSOR_SEQ_SEQ_HPP
# define BOOST_PREPROCESSOR_SEQ_SEQ_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/seq/elem.hpp>
#
# /* BOOST_PP_SEQ_HEAD */
#
# define BOOST_PP_SEQ_HEAD(seq) BOOST_PP_SEQ_ELEM(0, seq)
#
# /* BOOST_PP_SEQ_TAIL */
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_SEQ_TAIL(seq) BOOST_PP_SEQ_TAIL_1((seq))
#    define BOOST_PP_SEQ_TAIL_1(par) BOOST_PP_SEQ_TAIL_2 ## par
#    define BOOST_PP_SEQ_TAIL_2(seq) BOOST_PP_SEQ_TAIL_I ## seq
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    define BOOST_PP_SEQ_TAIL(seq) BOOST_PP_SEQ_TAIL_ID(BOOST_PP_SEQ_TAIL_I seq)
#    define BOOST_PP_SEQ_TAIL_ID(id) id
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SEQ_TAIL(seq) BOOST_PP_SEQ_TAIL_D(seq)
#    define BOOST_PP_SEQ_TAIL_D(seq) BOOST_PP_SEQ_TAIL_I seq
# else
#    define BOOST_PP_SEQ_TAIL(seq) BOOST_PP_SEQ_TAIL_I seq
# endif
#
# define BOOST_PP_SEQ_TAIL_I(x)
#
# /* BOOST_PP_SEQ_NIL */
#
# define BOOST_PP_SEQ_NIL(x) (x)
#
# endif
