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
# ifndef BOOST_PREPROCESSOR_ARRAY_TO_LIST_HPP
# define BOOST_PREPROCESSOR_ARRAY_TO_LIST_HPP
#
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/array/size.hpp>
# include <boost/preprocessor/control/if.hpp>
# include <boost/preprocessor/tuple/to_list.hpp>
#
# /* BOOST_PP_ARRAY_TO_LIST */
#
#    define BOOST_PP_ARRAY_TO_LIST(array) \
        BOOST_PP_IF \
            ( \
            BOOST_PP_ARRAY_SIZE(array), \
            BOOST_PP_ARRAY_TO_LIST_DO, \
            BOOST_PP_ARRAY_TO_LIST_EMPTY \
            ) \
        (array) \
/**/
#
#    define BOOST_PP_ARRAY_TO_LIST_EMPTY(array) BOOST_PP_NIL
#
# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    define BOOST_PP_ARRAY_TO_LIST_DO(array) BOOST_PP_ARRAY_TO_LIST_I(BOOST_PP_TUPLE_TO_LIST, array)
#    define BOOST_PP_ARRAY_TO_LIST_I(m, args) BOOST_PP_ARRAY_TO_LIST_II(m, args)
#    define BOOST_PP_ARRAY_TO_LIST_II(m, args) BOOST_PP_CAT(m ## args,)
# elif BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MWCC()
#    define BOOST_PP_ARRAY_TO_LIST_DO(array) BOOST_PP_ARRAY_TO_LIST_I(array)
#    define BOOST_PP_ARRAY_TO_LIST_I(array) BOOST_PP_TUPLE_TO_LIST ## array
# else
#    define BOOST_PP_ARRAY_TO_LIST_DO(array) BOOST_PP_TUPLE_TO_LIST array
# endif
#
# endif
