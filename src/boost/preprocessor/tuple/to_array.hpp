# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2011.                                    *
#  *     (C) Copyright Paul Mensonides 2011.                                  *
#  *     Distributed under the Boost Software License, Version 1.0. (See      *
#  *     accompanying file LICENSE_1_0.txt or copy at                         *
#  *     http://www.boost.org/LICENSE_1_0.txt)                                *
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_TUPLE_TO_ARRAY_HPP
# define BOOST_PREPROCESSOR_TUPLE_TO_ARRAY_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/facilities/overload.hpp>
# include <boost/preprocessor/tuple/size.hpp>
# include <boost/preprocessor/variadic/size.hpp>
# include <boost/preprocessor/variadic/has_opt.hpp>
#
# /* BOOST_PP_TUPLE_TO_ARRAY */
#
# if BOOST_PP_VARIADICS_MSVC
#     define BOOST_PP_TUPLE_TO_ARRAY(...) BOOST_PP_TUPLE_TO_ARRAY_I(BOOST_PP_OVERLOAD(BOOST_PP_TUPLE_TO_ARRAY_, __VA_ARGS__), (__VA_ARGS__))
#     define BOOST_PP_TUPLE_TO_ARRAY_I(m, args) BOOST_PP_TUPLE_TO_ARRAY_II(m, args)
#     define BOOST_PP_TUPLE_TO_ARRAY_II(m, args) BOOST_PP_CAT(m ## args,)
#     define BOOST_PP_TUPLE_TO_ARRAY_1(tuple) (BOOST_PP_TUPLE_SIZE(tuple), tuple)
# else
#     define BOOST_PP_TUPLE_TO_ARRAY(...) BOOST_PP_OVERLOAD(BOOST_PP_TUPLE_TO_ARRAY_, __VA_ARGS__)(__VA_ARGS__)
#     if BOOST_PP_VARIADIC_HAS_OPT()
#         define BOOST_PP_TUPLE_TO_ARRAY_1(tuple) BOOST_PP_TUPLE_TO_ARRAY_1_SIZE(BOOST_PP_VARIADIC_SIZE tuple, tuple)
#         define BOOST_PP_TUPLE_TO_ARRAY_1_SIZE(size,tuple) (BOOST_PP_IF(size,size,1), tuple)
#     else
#         define BOOST_PP_TUPLE_TO_ARRAY_1(tuple) (BOOST_PP_VARIADIC_SIZE tuple, tuple)
#     endif
# endif
# define BOOST_PP_TUPLE_TO_ARRAY_2(size, tuple) (size, tuple)
#
# endif
