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
# ifndef BOOST_PREPROCESSOR_DETAIL_CHECK_HPP
# define BOOST_PREPROCESSOR_DETAIL_CHECK_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
#
# /* BOOST_PP_CHECK */
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_CHECK(x, type) BOOST_PP_CHECK_D(x, type)
# else
#    define BOOST_PP_CHECK(x, type) BOOST_PP_CHECK_OO((x, type))
#    define BOOST_PP_CHECK_OO(par) BOOST_PP_CHECK_D ## par
# endif
#
# if ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC() && ~BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_DMC()
#    define BOOST_PP_CHECK_D(x, type) BOOST_PP_CHECK_1(BOOST_PP_CAT(BOOST_PP_CHECK_RESULT_, type x))
#    define BOOST_PP_CHECK_1(chk) BOOST_PP_CHECK_2(chk)
#    define BOOST_PP_CHECK_2(res, _) res
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    define BOOST_PP_CHECK_D(x, type) BOOST_PP_CHECK_1(type x)
#    define BOOST_PP_CHECK_1(chk) BOOST_PP_CHECK_2(chk)
#    define BOOST_PP_CHECK_2(chk) BOOST_PP_CHECK_3((BOOST_PP_CHECK_RESULT_ ## chk))
#    define BOOST_PP_CHECK_3(im) BOOST_PP_CHECK_5(BOOST_PP_CHECK_4 im)
#    define BOOST_PP_CHECK_4(res, _) res
#    define BOOST_PP_CHECK_5(res) res
# else /* DMC */
#    define BOOST_PP_CHECK_D(x, type) BOOST_PP_CHECK_OO((type x))
#    define BOOST_PP_CHECK_OO(par) BOOST_PP_CHECK_0 ## par
#    define BOOST_PP_CHECK_0(chk) BOOST_PP_CHECK_1(BOOST_PP_CAT(BOOST_PP_CHECK_RESULT_, chk))
#    define BOOST_PP_CHECK_1(chk) BOOST_PP_CHECK_2(chk)
#    define BOOST_PP_CHECK_2(res, _) res
# endif
#
# define BOOST_PP_CHECK_RESULT_1 1, BOOST_PP_NIL
#
# endif
