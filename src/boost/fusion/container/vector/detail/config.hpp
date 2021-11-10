/*=============================================================================
    Copyright (c) 2014-2015 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_VECTOR_CONFIG_11052014_1720
#define FUSION_VECTOR_CONFIG_11052014_1720

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/fusion/support/config.hpp>

#if (defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) \
  || defined(BOOST_NO_CXX11_RVALUE_REFERENCES) \
  || defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)  \
  || defined(BOOST_NO_CXX11_DECLTYPE)) \
  || defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS) \
  || defined(BOOST_FUSION_DISABLE_VARIADIC_VECTOR) \
  || (defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES))
# if defined(BOOST_FUSION_HAS_VARIADIC_VECTOR)
#   undef BOOST_FUSION_HAS_VARIADIC_VECTOR
# endif
#else
# if !defined(BOOST_FUSION_HAS_VARIADIC_VECTOR)
#   define BOOST_FUSION_HAS_VARIADIC_VECTOR
# endif
#endif

#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
# if defined(BOOST_FUSION_HAS_VARIADIC_VECTOR)
#   undef BOOST_FUSION_HAS_VARIADIC_VECTOR
# endif
#endif

#endif

