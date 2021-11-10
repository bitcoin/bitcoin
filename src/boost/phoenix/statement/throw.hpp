/*==============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_STATEMENT_THROW_HPP
#define BOOST_PHOENIX_STATEMENT_THROW_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/terminal.hpp>
#include <boost/phoenix/core/value.hpp>

namespace boost { namespace phoenix
{
    namespace tag
    {
        struct throw_ {};
    }

    namespace expression
    {
        template <typename A>
        struct throw_
            : expr<tag::throw_, A>
        {};
    }

    namespace rule
    {
        struct throw_
            : expression::throw_<meta_grammar>
        {};
    }

    template <typename Dummy>
    struct meta_grammar::case_<tag::throw_, Dummy>
        : enable_rule<rule::throw_, Dummy>
    {};

    struct throw_eval
    {
        typedef void result_type;

        template <typename ThrowExpr, typename Context>
        result_type
        operator()(ThrowExpr const& throw_expr, Context const & ctx) const
        {
            throw boost::phoenix::eval(throw_expr, ctx);
        }
    };
    
    template <typename Dummy>
    struct default_actions::when<rule::throw_, Dummy>
        : call<throw_eval>
    {};

    template <typename ThrowExpr>
    inline
    typename expression::throw_<ThrowExpr>::type const
    throw_(ThrowExpr const& throw_expr)
    {
        return expression::throw_<ThrowExpr>::make(throw_expr);
    }
    
    namespace detail
    {
        struct rethrow {};
    }
    
    namespace expression
    {
        struct rethrow
            : expression::value<detail::rethrow>
        {};
    }
    
    template<typename Dummy>
    struct is_custom_terminal<detail::rethrow, Dummy>
      : mpl::true_
    {};

    template<typename Dummy>
    struct custom_terminal<detail::rethrow, Dummy>
    {
        typedef void result_type;
      //#ifndef BOOST_PHOENIX_NO_SPECIALIZE_CUSTOM_TERMINAL
        typedef void _is_throw_custom_terminal; // fix for #7730
      //#endif

        template <typename Context>
        void operator()(detail::rethrow, Context &) const
        {
            throw;
        }
    };

    inline
    expression::rethrow::type const
    throw_()
    {
        return expression::rethrow::make(detail::rethrow());
    }

}}

#endif
