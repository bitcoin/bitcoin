/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2014 Agustin Berge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_IS_PARSER_MAY_20_2013_0235PM)
#define BOOST_SPIRIT_X3_IS_PARSER_MAY_20_2013_0235PM

#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/utility/sfinae.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // is_parser<T>: metafunction that evaluates to mpl::true_ if a type T 
    // can be used as a parser, mpl::false_ otherwise
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct is_parser
      : mpl::false_
    {};

    template <typename T>
    struct is_parser<T, typename disable_if_substitution_failure<
        typename extension::as_parser<T>::type>::type>
      : mpl::true_
    {};
}}}}

#endif
