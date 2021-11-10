/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_STATEMENT_DO_WHILE_HPP
#define BOOST_PHOENIX_STATEMENT_DO_WHILE_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(do_while)
  , (meta_grammar) // Cond
    (meta_grammar) // Do
)

namespace boost { namespace phoenix
{
    struct do_while_eval
    {
        typedef void result_type;

        template <typename Cond, typename Do, typename Context>
        result_type
        operator()(Cond const& cond, Do const& do_it, Context const & ctx) const
        {
            do
                boost::phoenix::eval(do_it, ctx);
            while (boost::phoenix::eval(cond, ctx));
        }
    };
    
    template <typename Dummy>
    struct default_actions::when<rule::do_while, Dummy>
        : call<do_while_eval, Dummy>
    {};

    template <typename Do>
    struct do_while_gen
    {
        do_while_gen(Do const& do_it)
            : do_(do_it) {}

        template <typename Cond>
        typename expression::do_while<Cond, Do>::type const
        while_(Cond const& cond) const
        {
            return expression::do_while<Cond, Do>::make(cond, do_);
        }

        Do const& do_;
    };

    struct do_gen
    {
        template <typename Do>
        do_while_gen<Do> const
        operator[](Do const& do_) const
        {
            return do_while_gen<Do>(do_);
        }
    };

#ifndef BOOST_PHOENIX_NO_PREDEFINED_TERMINALS
    do_gen const do_ = {};
#endif

}}

#endif
