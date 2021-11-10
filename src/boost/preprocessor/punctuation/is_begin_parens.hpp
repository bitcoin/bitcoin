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
# ifndef BOOST_PREPROCESSOR_IS_BEGIN_PARENS_HPP
# define BOOST_PREPROCESSOR_IS_BEGIN_PARENS_HPP

#include <boost/preprocessor/config/config.hpp>
#include <boost/preprocessor/punctuation/detail/is_begin_parens.hpp>

#if BOOST_PP_VARIADICS_MSVC && _MSC_VER <= 1400

#define BOOST_PP_IS_BEGIN_PARENS(param) \
    BOOST_PP_DETAIL_IBP_SPLIT \
      ( \
      0, \
      BOOST_PP_DETAIL_IBP_CAT \
        ( \
        BOOST_PP_DETAIL_IBP_IS_VARIADIC_R_, \
        BOOST_PP_DETAIL_IBP_IS_VARIADIC_C param \
        ) \
      ) \
/**/

#else

#define BOOST_PP_IS_BEGIN_PARENS(...) \
    BOOST_PP_DETAIL_IBP_SPLIT \
      ( \
      0, \
      BOOST_PP_DETAIL_IBP_CAT \
        ( \
        BOOST_PP_DETAIL_IBP_IS_VARIADIC_R_, \
        BOOST_PP_DETAIL_IBP_IS_VARIADIC_C __VA_ARGS__ \
        ) \
      ) \
/**/

#endif /* BOOST_PP_VARIADICS_MSVC && _MSC_VER <= 1400 */
#endif /* BOOST_PREPROCESSOR_IS_BEGIN_PARENS_HPP */
