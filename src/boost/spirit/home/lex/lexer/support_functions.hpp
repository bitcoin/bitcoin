//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_LEX_LEXER_SUPPORT_FUNCTIONS_HPP
#define BOOST_SPIRIT_LEX_LEXER_SUPPORT_FUNCTIONS_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/scoped_enum_emulation.hpp>
#include <boost/spirit/home/lex/lexer/pass_flags.hpp>
#include <boost/spirit/home/lex/lexer/support_functions_expression.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/as_actor.hpp>
#include <boost/phoenix/core/value.hpp> // includes as_actor specialization

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    // The function object less_type is used by the implementation of the 
    // support function lex::less(). Its functionality is equivalent to flex' 
    // function yyless(): it returns an iterator positioned to the nth input 
    // character beyond the current start iterator (i.e. by assigning the 
    // return value to the placeholder '_end' it is possible to return all but
    // the first n characters of the current token back to the input stream. 
    //
    //  This Phoenix actor is invoked whenever the function lex::less(n) is 
    //  used inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ _end = lex::less(4) ];
    //
    //  The example shows how to limit the length of the matched identifier to 
    //  four characters.
    //
    //  Note: the function lex::less() has no effect if used on it's own, you 
    //        need to use the returned result in order to make use of its 
    //        functionality.
    template <typename Actor>
    struct less_type
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename remove_reference< 
                typename remove_const<
                    typename mpl::at_c<typename Env::args_type, 4>::type
                >::type
            >::type context_type;
            typedef typename context_type::base_iterator_type type;
        };

        template <typename Env>
        typename result<Env>::type 
        eval(Env const& env) const
        {
            typename result<Env>::type it;
            return fusion::at_c<4>(env.args()).less(it, actor_());
        }

        less_type(Actor const& actor)
          : actor_(actor) {}

        Actor actor_;
    };

    //  The function lex::less() is used to create a Phoenix actor allowing to
    //  implement functionality similar to flex' function yyless().
    template <typename T>
    inline typename expression::less<
        typename phoenix::as_actor<T>::type
    >::type const
    less(T const& v)
    {
        return expression::less<T>::make(phoenix::as_actor<T>::convert(v));
    }

    ///////////////////////////////////////////////////////////////////////////
    // The function object more_type is used by the implementation of the  
    // support function lex::more(). Its functionality is equivalent to flex' 
    // function yymore(): it tells the lexer that the next time it matches a 
    // rule, the corresponding token should be appended onto the current token 
    // value rather than replacing it.
    //
    //  This Phoenix actor is invoked whenever the function lex::more(n) is 
    //  used inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ lex::more() ];
    //
    //  The example shows how prefix the next matched token with the matched
    //  identifier.
    struct more_type
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef void type;
        };

        template <typename Env>
        void eval(Env const& env) const
        {
            fusion::at_c<4>(env.args()).more();
        }
    };

    //  The function lex::more() is used to create a Phoenix actor allowing to
    //  implement functionality similar to flex' function yymore(). 
    //inline expression::more<mpl::void_>::type const
    inline phoenix::actor<more_type> more()
    {
        return phoenix::actor<more_type>();
    }

    ///////////////////////////////////////////////////////////////////////////
    // The function object lookahead_type is used by the implementation of the  
    // support function lex::lookahead(). Its functionality is needed to 
    // emulate the flex' lookahead operator a/b. Use lex::lookahead() inside
    // of lexer semantic actions to test whether the argument to this function
    // matches the current look ahead input. lex::lookahead() can be used with
    // either a token id or a token_def instance as its argument. It returns
    // a bool indicating whether the look ahead has been matched.
    template <typename IdActor, typename StateActor>
    struct lookahead_type
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef bool type;
        };

        template <typename Env>
        bool eval(Env const& env) const
        {
            return fusion::at_c<4>(env.args()).
                lookahead(id_actor_(), state_actor_());
        }

        lookahead_type(IdActor const& id_actor, StateActor const& state_actor)
          : id_actor_(id_actor), state_actor_(state_actor) {}

        IdActor id_actor_;
        StateActor state_actor_;
    };

    //  The function lex::lookahead() is used to create a Phoenix actor 
    //  allowing to implement functionality similar to flex' lookahead operator
    //  a/b.
    template <typename T>
    inline typename expression::lookahead<
        typename phoenix::as_actor<T>::type
      , typename phoenix::as_actor<std::size_t>::type
    >::type const
    lookahead(T const& id)
    {
        typedef typename phoenix::as_actor<T>::type id_actor_type;
        typedef typename phoenix::as_actor<std::size_t>::type state_actor_type;

        return expression::lookahead<id_actor_type, state_actor_type>::make(
            phoenix::as_actor<T>::convert(id),
            phoenix::as_actor<std::size_t>::convert(std::size_t(~0)));
    }

    template <typename Attribute, typename Char, typename Idtype>
    inline typename expression::lookahead<
        typename phoenix::as_actor<Idtype>::type
      , typename phoenix::as_actor<std::size_t>::type
    >::type const
    lookahead(token_def<Attribute, Char, Idtype> const& tok)
    {
        typedef typename phoenix::as_actor<Idtype>::type id_actor_type;
        typedef typename phoenix::as_actor<std::size_t>::type state_actor_type;

        std::size_t state = tok.state();

        // The following assertion fires if you pass a token_def instance to 
        // lex::lookahead without first associating this instance with the 
        // lexer.
        BOOST_ASSERT(std::size_t(~0) != state && 
            "token_def instance not associated with lexer yet");

        return expression::lookahead<id_actor_type, state_actor_type>::make(
            phoenix::as_actor<Idtype>::convert(tok.id()),
            phoenix::as_actor<std::size_t>::convert(state));
    }

    ///////////////////////////////////////////////////////////////////////////
    inline BOOST_SCOPED_ENUM(pass_flags) ignore()
    {
        return pass_flags::pass_ignore;
    }

}}}

#endif
