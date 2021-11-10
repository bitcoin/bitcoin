# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Edward Diener 2014.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
#ifndef BOOST_PREPROCESSOR_DETAIL_IS_EMPTY_HPP
#define BOOST_PREPROCESSOR_DETAIL_IS_EMPTY_HPP

#include <boost/preprocessor/punctuation/is_begin_parens.hpp>

#if BOOST_PP_VARIADICS_MSVC

# pragma warning(once:4002)

#define BOOST_PP_DETAIL_IS_EMPTY_IIF_0(t, b) b
#define BOOST_PP_DETAIL_IS_EMPTY_IIF_1(t, b) t

#else

#define BOOST_PP_DETAIL_IS_EMPTY_IIF_0(t, ...) __VA_ARGS__
#define BOOST_PP_DETAIL_IS_EMPTY_IIF_1(t, ...) t

#endif

#if BOOST_PP_VARIADICS_MSVC && _MSC_VER <= 1400

#define BOOST_PP_DETAIL_IS_EMPTY_PROCESS(param) \
    BOOST_PP_IS_BEGIN_PARENS \
        ( \
        BOOST_PP_DETAIL_IS_EMPTY_NON_FUNCTION_C param () \
        ) \
/**/

#else

#define BOOST_PP_DETAIL_IS_EMPTY_PROCESS(...) \
    BOOST_PP_IS_BEGIN_PARENS \
        ( \
        BOOST_PP_DETAIL_IS_EMPTY_NON_FUNCTION_C __VA_ARGS__ () \
        ) \
/**/

#endif

#define BOOST_PP_DETAIL_IS_EMPTY_PRIMITIVE_CAT(a, b) a ## b
#define BOOST_PP_DETAIL_IS_EMPTY_IIF(bit) BOOST_PP_DETAIL_IS_EMPTY_PRIMITIVE_CAT(BOOST_PP_DETAIL_IS_EMPTY_IIF_,bit)
#define BOOST_PP_DETAIL_IS_EMPTY_NON_FUNCTION_C(...) ()

#endif /* BOOST_PREPROCESSOR_DETAIL_IS_EMPTY_HPP */
