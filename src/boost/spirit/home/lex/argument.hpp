//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c)      2010 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_ARGUMENT_JUNE_07_2009_1106AM)
#define BOOST_SPIRIT_LEX_ARGUMENT_JUNE_07_2009_1106AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/lex/argument_phoenix.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/argument.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    //  The state_getter is a Phoenix actor used to access the name of the 
    //  current lexer state by calling get_state_name() on the context (which 
    //  is the 5th parameter to any lexer semantic actions).
    //
    //  This Phoenix actor is invoked whenever the placeholder '_state' is used
    //  as a rvalue inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ std::cout << _state ];
    //
    //  The example shows how to print the lexer state after matching a token
    //  'identifier'.
    struct state_getter
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef
                typename remove_reference<
                   typename remove_const<
                        typename mpl::at_c<typename Env::args_type, 4>::type
                    >::type
                >::type
            context_type;

            typedef typename context_type::state_name_type type;
        };

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            return fusion::at_c<4>(env.args()).get_state_name();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The state_setter is a Phoenix actor used to change the name of the 
    //  current lexer state by calling set_state_name() on the context (which 
    //  is the 5th parameter to any lexer semantic actions).
    //
    //  This Phoenix actor is invoked whenever the placeholder '_state' is used
    //  as a lvalue inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ _state = "SOME_LEXER_STATE" ];
    //
    //  The example shows how to change the lexer state after matching a token
    //  'identifier'.
    template <typename Actor>
    struct state_setter
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
            fusion::at_c<4>(env.args()).set_state_name(
                traits::get_c_string(actor_.eval(env)));
        }

        state_setter(Actor const& actor)
          : actor_(actor) {}

        Actor actor_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The value_getter is used to create the _val placeholder, which is a 
    //  Phoenix actor used to access the value of the current token.
    //
    //  This Phoenix actor is invoked whenever the placeholder '_val' is used
    //  as a rvalue inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ std::cout << _val ];
    //
    //  The example shows how to use _val to print the identifier name (which
    //  is the initial token value).
    struct value_getter
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef
                typename remove_reference<
                   typename remove_const<
                        typename mpl::at_c<typename Env::args_type, 4>::type
                    >::type
                >::type
            context_type;

            typedef typename context_type::get_value_type type;
        };

        template <typename Env>
        typename result<Env>::type 
        eval(Env const& env) const
        {
            return fusion::at_c<4>(env.args()).get_value();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The value_setter is a Phoenix actor used to change the name of the 
    //  current lexer state by calling set_state_name() on the context (which 
    //  is the 5th parameter to any lexer semantic actions).
    //
    //  This Phoenix actor is invoked whenever the placeholder '_val' is used
    //  as a lvalue inside a lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier [ _val = "identifier" ];
    //
    //  The example shows how to change the token value after matching a token
    //  'identifier'.
    template <typename Actor>
    struct value_setter
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
            fusion::at_c<4>(env.args()).set_value(actor_.eval(env));
        }

        value_setter(Actor const& actor)
          : actor_(actor) {}

        Actor actor_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The eoi_getter is used to create the _eoi placeholder, which is a 
    //  Phoenix actor used to access the end of input iterator pointing to the 
    //  end of the underlying input sequence.
    //
    //  This actor is invoked whenever the placeholder '_eoi' is used in a
    //  lexer semantic action:
    //
    //      lex::token_def<> identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
    //      this->self = identifier 
    //          [ std::cout << construct_<std::string>(_end, _eoi) ];
    //
    //  The example shows how to use _eoi to print all remaining input after
    //  matching a token 'identifier'.
    struct eoi_getter
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef
                typename remove_reference<
                   typename remove_const<
                        typename mpl::at_c<typename Env::args_type, 4>::type
                    >::type
                >::type
            context_type;

            typedef typename context_type::base_iterator_type const& type;
        };

        template <typename Env>
        typename result<Env>::type 
        eval(Env const& env) const
        {
            return fusion::at_c<4>(env.args()).get_eoi();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // '_start' and '_end' may be used to access the start and the end of 
    // the matched sequence of the current token
    typedef phoenix::arg_names::_1_type _start_type;
    typedef phoenix::arg_names::_2_type _end_type;
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    _start_type const _start = _start_type();
    _end_type const _end = _end_type();
#endif

    // We are reusing the placeholder '_pass' to access and change the pass
    // status of the current match (see support/argument.hpp for its 
    // definition).
    // typedef phoenix::arg_names::_3_type _pass_type;
    using boost::spirit::_pass_type;
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using boost::spirit::_pass;
#endif

    // '_tokenid' may be used to access and change the tokenid of the current 
    // token
    typedef phoenix::arg_names::_4_type _tokenid_type;
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    _tokenid_type const _tokenid = _tokenid_type();
#endif

    typedef phoenix::actor<value_context> _val_type;
    typedef phoenix::actor<state_context> _state_type;
    typedef phoenix::actor<eoi_getter> _eoi_type;
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    // '_val' may be used to access and change the token value of the current
    // token
    _val_type const _val = _val_type();
    // _state may be used to access and change the name of the current lexer 
    // state
    _state_type const _state = _state_type();
    // '_eoi' may be used to access the end of input iterator of the input 
    // stream used by the lexer to match tokens from
    _eoi_type const _eoi = _eoi_type();
#endif
}}}


#undef SPIRIT_DECLARE_ARG
#endif

