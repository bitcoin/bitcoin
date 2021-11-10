/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_LOCALS_APRIL_03_2007_0506PM)
#define BOOST_SPIRIT_LOCALS_APRIL_03_2007_0506PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/bool.hpp>

#if !defined(BOOST_SPIRIT_MAX_LOCALS_SIZE)
# define BOOST_SPIRIT_MAX_LOCALS_SIZE 10
#else
# if BOOST_SPIRIT_MAX_LOCALS_SIZE < 3
#   undef BOOST_SPIRIT_MAX_LOCALS_SIZE
#   define BOOST_SPIRIT_MAX_LOCALS_SIZE 10
# endif
#endif

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    template <
        BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
            BOOST_SPIRIT_MAX_LOCALS_SIZE, typename T, mpl::na)
    >
    struct locals
      : mpl::vector<BOOST_PP_ENUM_PARAMS(BOOST_SPIRIT_MAX_LOCALS_SIZE, T)> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T>
        struct is_locals
          : mpl::false_
        {};

        template <BOOST_PP_ENUM_PARAMS(BOOST_SPIRIT_MAX_LOCALS_SIZE, typename T)>
        struct is_locals<locals<BOOST_PP_ENUM_PARAMS(BOOST_SPIRIT_MAX_LOCALS_SIZE, T)> >
          : mpl::true_
        {};
    }
}}

#endif
