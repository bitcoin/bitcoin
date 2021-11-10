# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2011.
#  *     (C) Copyright Edward Diener 2011,2014.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef BOOST_PREPROCESSOR_LIST_TO_ARRAY_HPP
# define BOOST_PREPROCESSOR_LIST_TO_ARRAY_HPP
#
# include <boost/preprocessor/arithmetic/dec.hpp>
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/config/config.hpp>
# include <boost/preprocessor/control/while.hpp>
# include <boost/preprocessor/list/adt.hpp>
# include <boost/preprocessor/tuple/elem.hpp>
# include <boost/preprocessor/tuple/rem.hpp>
# if BOOST_PP_VARIADICS_MSVC && (_MSC_VER <= 1400)
# include <boost/preprocessor/control/iif.hpp>
# endif
#
# /* BOOST_PP_LIST_TO_ARRAY */
#
# if BOOST_PP_VARIADICS_MSVC && (_MSC_VER <= 1400)
# define BOOST_PP_LIST_TO_ARRAY(list) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_LIST_IS_NIL(list), \
        BOOST_PP_LIST_TO_ARRAY_VC8ORLESS_EMPTY, \
        BOOST_PP_LIST_TO_ARRAY_VC8ORLESS_DO \
        ) \
    (list) \
/**/
# define BOOST_PP_LIST_TO_ARRAY_VC8ORLESS_EMPTY(list) (0,())
# define BOOST_PP_LIST_TO_ARRAY_VC8ORLESS_DO(list) BOOST_PP_LIST_TO_ARRAY_I(BOOST_PP_WHILE, list)
# else
# define BOOST_PP_LIST_TO_ARRAY(list) BOOST_PP_LIST_TO_ARRAY_I(BOOST_PP_WHILE, list)
# endif

# if BOOST_PP_CONFIG_FLAGS() & BOOST_PP_CONFIG_MSVC()
#    define BOOST_PP_LIST_TO_ARRAY_I(w, list) \
        BOOST_PP_LIST_TO_ARRAY_II(((BOOST_PP_TUPLE_REM_CTOR( \
            3, \
            w(BOOST_PP_LIST_TO_ARRAY_P, BOOST_PP_LIST_TO_ARRAY_O, (list, 1, (~))) \
        )))) \
        /**/
#    define BOOST_PP_LIST_TO_ARRAY_II(p) BOOST_PP_LIST_TO_ARRAY_II_B(p)
#    define BOOST_PP_LIST_TO_ARRAY_II_B(p) BOOST_PP_LIST_TO_ARRAY_II_C ## p
#    define BOOST_PP_LIST_TO_ARRAY_II_C(p) BOOST_PP_LIST_TO_ARRAY_III p
# else
#    define BOOST_PP_LIST_TO_ARRAY_I(w, list) \
        BOOST_PP_LIST_TO_ARRAY_II(BOOST_PP_TUPLE_REM_CTOR( \
            3, \
            w(BOOST_PP_LIST_TO_ARRAY_P, BOOST_PP_LIST_TO_ARRAY_O, (list, 1, (~))) \
        )) \
        /**/
#    define BOOST_PP_LIST_TO_ARRAY_II(im) BOOST_PP_LIST_TO_ARRAY_III(im)
# endif
# define BOOST_PP_LIST_TO_ARRAY_III(list, size, tuple) (BOOST_PP_DEC(size), BOOST_PP_LIST_TO_ARRAY_IV tuple)
# define BOOST_PP_LIST_TO_ARRAY_IV(_, ...) (__VA_ARGS__)
# define BOOST_PP_LIST_TO_ARRAY_P(d, state) BOOST_PP_LIST_IS_CONS(BOOST_PP_TUPLE_ELEM(3, 0, state))
# define BOOST_PP_LIST_TO_ARRAY_O(d, state) BOOST_PP_LIST_TO_ARRAY_O_I state
# define BOOST_PP_LIST_TO_ARRAY_O_I(list, size, tuple) (BOOST_PP_LIST_REST(list), BOOST_PP_INC(size), (BOOST_PP_TUPLE_REM(size) tuple, BOOST_PP_LIST_FIRST(list)))
#
# /* BOOST_PP_LIST_TO_ARRAY_D */
#
# if BOOST_PP_VARIADICS_MSVC && (_MSC_VER <= 1400)
# define BOOST_PP_LIST_TO_ARRAY_D(d, list) \
    BOOST_PP_IIF \
        ( \
        BOOST_PP_LIST_IS_NIL(list), \
        BOOST_PP_LIST_TO_ARRAY_D_VC8ORLESS_EMPTY, \
        BOOST_PP_LIST_TO_ARRAY_D_VC8ORLESS_DO \
        ) \
    (d, list) \
/**/
# define BOOST_PP_LIST_TO_ARRAY_D_VC8ORLESS_EMPTY(d, list) (0,())
# define BOOST_PP_LIST_TO_ARRAY_D_VC8ORLESS_DO(d, list) BOOST_PP_LIST_TO_ARRAY_I(BOOST_PP_WHILE_ ## d, list)
# else
# define BOOST_PP_LIST_TO_ARRAY_D(d, list) BOOST_PP_LIST_TO_ARRAY_I(BOOST_PP_WHILE_ ## d, list)
# endif
#
# endif /* BOOST_PREPROCESSOR_LIST_TO_ARRAY_HPP */
