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
# ifndef BOOST_PREPROCESSOR_VARIADIC_TO_LIST_HPP
# define BOOST_PREPROCESSOR_VARIADIC_TO_LIST_HPP
#
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/tuple/to_list.hpp>
# include <boost/preprocessor/variadic/has_opt.hpp>
# include <boost/preprocessor/variadic/size.hpp>
#
# /* BOOST_PP_VARIADIC_TO_LIST */
#
# if BOOST_PP_VARIADIC_HAS_OPT()
#     define BOOST_PP_VARIADIC_TO_LIST_NOT_EMPTY(...) BOOST_PP_TUPLE_TO_LIST((__VA_ARGS__))
#     define BOOST_PP_VARIADIC_TO_LIST_EMPTY(...) BOOST_PP_NIL
#     define BOOST_PP_VARIADIC_TO_LIST(...) BOOST_PP_IF(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),BOOST_PP_VARIADIC_TO_LIST_NOT_EMPTY,BOOST_PP_VARIADIC_TO_LIST_EMPTY)(__VA_ARGS__)
# else
#     define BOOST_PP_VARIADIC_TO_LIST(...) BOOST_PP_TUPLE_TO_LIST((__VA_ARGS__))
# endif
#
# endif
