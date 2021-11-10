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
# ifndef BOOST_PREPROCESSOR_VARIADIC_TO_ARRAY_HPP
# define BOOST_PREPROCESSOR_VARIADIC_TO_ARRAY_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/tuple/to_array.hpp>
# include <boost/preprocessor/variadic/has_opt.hpp>
# include <boost/preprocessor/variadic/size.hpp>
#
# /* BOOST_PP_VARIADIC_TO_ARRAY */
#
# if BOOST_PP_VARIADIC_HAS_OPT()
#     if BOOST_PP_VARIADICS_MSVC
#         define BOOST_PP_VARIADIC_TO_ARRAY_NON_EMPTY(...) BOOST_PP_TUPLE_TO_ARRAY_2(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),(__VA_ARGS__))
#     else
#         define BOOST_PP_VARIADIC_TO_ARRAY_NON_EMPTY(...) BOOST_PP_TUPLE_TO_ARRAY((__VA_ARGS__))
#     endif
#     define BOOST_PP_VARIADIC_TO_ARRAY_EMPTY(...) (0,())
#     define BOOST_PP_VARIADIC_TO_ARRAY(...) BOOST_PP_IF(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),BOOST_PP_VARIADIC_TO_ARRAY_NON_EMPTY,BOOST_PP_VARIADIC_TO_ARRAY_EMPTY)(__VA_ARGS__)
# elif BOOST_PP_VARIADICS_MSVC
#     define BOOST_PP_VARIADIC_TO_ARRAY(...) BOOST_PP_TUPLE_TO_ARRAY_2(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),(__VA_ARGS__))
# else
#     define BOOST_PP_VARIADIC_TO_ARRAY(...) BOOST_PP_TUPLE_TO_ARRAY((__VA_ARGS__))
# endif
#
# endif
