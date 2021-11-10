/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_OPERATOR_IF_ELSE_HPP
#define BOOST_PHOENIX_OPERATOR_IF_ELSE_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/proto/operators.hpp>

namespace boost { namespace phoenix
{
    namespace tag
    {
        typedef proto::tag::if_else_ if_else_operator;
    }

    namespace expression
    {
        template <typename A0, typename A1, typename A2>
        struct if_else_operator
            : expr<tag::if_else_operator, A0, A1, A2>
        {};
    }

    namespace rule
    {
        struct if_else_operator
            : expression::if_else_operator<
                meta_grammar
              , meta_grammar
              , meta_grammar
            >
        {};
    }

    template <typename Dummy>
    struct meta_grammar::case_<tag::if_else_operator, Dummy>
        : enable_rule<rule::if_else_operator, Dummy>
    {};

    using proto::if_else;
}}

#endif
