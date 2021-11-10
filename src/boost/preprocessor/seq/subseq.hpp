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
# ifndef BOOST_PREPROCESSOR_SEQ_SUBSEQ_HPP
# define BOOST_PREPROCESSOR_SEQ_SUBSEQ_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/seq/first_n.hpp>
# include <boost/preprocessor/seq/rest_n.hpp>
#
# /* BOOST_PP_SEQ_SUBSEQ */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_EDG()
#    define BOOST_PP_SEQ_SUBSEQ(seq, i, len) BOOST_PP_SEQ_FIRST_N(len, BOOST_PP_SEQ_REST_N(i, seq))
# else
#    define BOOST_PP_SEQ_SUBSEQ(seq, i, len) BOOST_PP_SEQ_SUBSEQ_I(seq, i, len)
#    define BOOST_PP_SEQ_SUBSEQ_I(seq, i, len) BOOST_PP_SEQ_FIRST_N(len, BOOST_PP_SEQ_REST_N(i, seq))
# endif
#
# endif
