/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_STATEMENT_IF_HPP
#define BOOST_PHOENIX_STATEMENT_IF_HPP

#include <boost/phoenix/config.hpp>
#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/is_actor.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' used in base member initializer list
#endif

namespace boost { namespace phoenix
{
    template <typename> struct if_actor;
}}
    
BOOST_PHOENIX_DEFINE_EXPRESSION_EXT(
    if_actor
  , (boost)(phoenix)(if_)
  , (meta_grammar) // Cond
    (meta_grammar) // Then
)
    
BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(if_else_statement)
  , (meta_grammar) // Cond
    (meta_grammar) // Then
    (meta_grammar) // Else
)

namespace boost { namespace phoenix
{
    ////////////////////////////////////////////////////////////////////////////
    // If-Else statements
    ////////////////////////////////////////////////////////////////////////////
    
    // Function for evaluating lambdas like:
    // if_( foo )[ bar ]
    // and
    // if_( foo )[ bar ].else_[ baz ]
    struct if_else_eval
    {
        typedef void result_type;
        
        template<typename Cond, typename Then, typename Context>
        result_type
        operator()(Cond const & cond, Then const & then, Context const & ctx) const
        {
            if(boost::phoenix::eval(cond, ctx))
                boost::phoenix::eval(then, ctx);
        }
        
        template<typename Cond, typename Then, typename Else, typename Context>
        result_type
        operator()(
              Cond const & cond
            , Then const & then
            , Else const & else_
            , Context const & ctx
        ) const
        {
            if(boost::phoenix::eval(cond, ctx))
                boost::phoenix::eval(then, ctx);
            else
                boost::phoenix::eval(else_, ctx);
        }
    };
    
    template <typename Dummy>
    struct default_actions::when<rule::if_, Dummy>
        : call<if_else_eval, Dummy>
    {};
    
    template <typename Dummy>
    struct default_actions::when<rule::if_else_statement, Dummy>
        : call<if_else_eval, Dummy>
    {};


    // Generator for .else_[ expr ] branch.
    template<typename Cond, typename Then>
    struct else_gen
    {
        else_gen(Cond const & cond_, Then const & then_)
            : cond(cond_)
            , then(then_) {}

        template<typename Else>
        typename expression::if_else_statement<Cond, Then, Else>::type const
        operator[](Else const & else_) const
        {
            return expression::if_else_statement<Cond, Then, Else>::make(cond, then, else_);
        }

        Cond cond;
        Then then;
    };

    // We subclass actor so we can provide the member else_ (which is an
    // else_gen responsible for the .else_[ expr ] branch).
    template<typename Expr>
    struct if_actor : actor<Expr>
    {
        typedef actor<Expr> base_type;

        if_actor(base_type const & base)
            : base_type(base)
            , else_(proto::child_c<0>(*this), proto::child_c<1>(*this))
        {}

        typedef typename proto::result_of::child_c<Expr, 0>::type cond_type;
        typedef typename proto::result_of::child_c<Expr, 1>::type then_type;

        else_gen<cond_type, then_type> else_;
    };

    template <typename Expr>
    struct is_actor<if_actor<Expr> >
        : mpl::true_
    {};

    // Generator for if( cond )[ then ] branch.
    template<typename Cond>
    struct if_gen
    {
        if_gen(Cond const & cond_)
            : cond(cond_) {}

        template<typename Then>
        typename expression::if_<Cond, Then>::type const
        operator[](Then const & then) const
        {
            return expression::if_<Cond, Then>::make(cond, then);
        }

        Cond cond;
    };

    template<typename Cond>
    inline
    if_gen<Cond> const
    if_(Cond const & cond)
    {
        return if_gen<Cond>(cond);
    }
 
}}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
