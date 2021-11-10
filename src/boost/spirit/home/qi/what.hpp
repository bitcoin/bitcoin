/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_WHAT_APRIL_21_2007_0732AM)
#define BOOST_SPIRIT_WHAT_APRIL_21_2007_0732AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/assert.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>

namespace boost { namespace spirit { namespace qi
{
    template <typename Expr>
    inline info what(Expr const& expr)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_expr_is_not_convertible_to_a_parser
        // error message here, then the expression (expr) is not a
        // valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Expr);
        return compile<qi::domain>(expr).what(unused);
    }
}}}

#endif

