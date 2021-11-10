/*=============================================================================
    Copyright (c) 2001-2015 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3__ANNOTATE_ON_SUCCESS_HPP)
#define BOOST_SPIRIT_X3__ANNOTATE_ON_SUCCESS_HPP

#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    //  The on_success handler tags the AST with the iterator position
    //  for error handling.
    //
    //  The on_success handler also ties the AST to a vector of iterator
    //  positions for the purpose of subsequent semantic error handling
    //  when the program is being compiled. See x3::position_cache in
    //  x3/support/ast.
    //
    //  We'll ask the X3's error_handler utility to do these.
    ///////////////////////////////////////////////////////////////////////////

    struct annotate_on_success
    {
        template <typename Iterator, typename Context, typename... Types>
        inline void on_success(Iterator const& first, Iterator const& last
          , variant<Types...>& ast, Context const& context)
        {
            ast.apply_visitor(x3::make_lambda_visitor<void>([&](auto& node)
            {
                this->on_success(first, last, node, context);
            }));
        }

        template <typename T, typename Iterator, typename Context>
        inline void on_success(Iterator const& first, Iterator const& last
          , T& ast, Context const& context)
        {
            auto& error_handler = x3::get<error_handler_tag>(context).get();
            error_handler.tag(ast, first, last);
        }
    };
}}}

#endif
