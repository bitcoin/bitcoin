/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TERMINAL_NOVEMBER_04_2008_0906AM)
#define BOOST_SPIRIT_TERMINAL_NOVEMBER_04_2008_0906AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/spirit/home/support/meta_compiler.hpp>
#include <boost/spirit/home/support/detail/make_vector.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/detail/is_spirit_tag.hpp>
#include <boost/spirit/home/support/terminal_expression.hpp>
#include <boost/phoenix/core/as_actor.hpp>
#include <boost/phoenix/core/is_actor.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/value.hpp> // includes as_actor specialization
#include <boost/phoenix/function/function.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/traits.hpp>

namespace boost { namespace spirit
{
    template <typename Terminal, typename Args>
    struct terminal_ex
    {
        typedef Terminal terminal_type;
        typedef Args args_type;

        terminal_ex(Args const& args_)
          : args(args_) {}
        terminal_ex(Args const& args_, Terminal const& term_)
          : args(args_), term(term_) {}

        Args args;  // Args is guaranteed to be a fusion::vectorN so you
                    // can use that template for detection and specialization
        Terminal term;
    };

    template <typename Terminal, typename Actor, int Arity>
    struct lazy_terminal
    {
        typedef Terminal terminal_type;
        typedef Actor actor_type;
        static int const arity = Arity;

        lazy_terminal(Actor const& actor_)
          : actor(actor_) {}
        lazy_terminal(Actor const& actor_, Terminal const& term_)
          : actor(actor_), term(term_) {}

        Actor actor;
        Terminal term;
    };

    template <typename Domain, typename Terminal, int Arity, typename Enable = void>
    struct use_lazy_terminal : mpl::false_ {};

    template <typename Domain, typename Terminal, int Arity, typename Enable = void>
    struct use_lazy_directive : mpl::false_ {};

    template <typename Terminal>
    struct terminal;

    template <typename Domain, typename Terminal>
    struct use_terminal<Domain, terminal<Terminal> >
        : use_terminal<Domain, Terminal> {};

    template <typename Domain, typename Terminal, int Arity, typename Actor>
    struct use_terminal<Domain, lazy_terminal<Terminal, Actor, Arity> >
        : use_lazy_terminal<Domain, Terminal, Arity> {};

    template <typename Domain, typename Terminal, int Arity, typename Actor>
    struct use_directive<Domain, lazy_terminal<Terminal, Actor, Arity> >
        : use_lazy_directive<Domain, Terminal, Arity> {};

    template <
        typename F
      , typename A0 = unused_type
      , typename A1 = unused_type
      , typename A2 = unused_type
      , typename Unused = unused_type
    >
    struct make_lazy;

    template <typename F, typename A0>
    struct make_lazy<F, A0>
    {
        typedef typename
            proto::terminal<
                lazy_terminal<
                    typename F::terminal_type
                  , typename phoenix::detail::expression::function_eval<F, A0>::type
                  , 1 // arity
                >
            >::type
        result_type;
        typedef result_type type;

        result_type
        operator()(F f, A0 const& _0_) const
        {
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(child_type(
                phoenix::detail::expression::function_eval<F, A0>::make(f, _0_)
              , f.proto_base().child0
            ));
        }
    };

    template <typename F, typename A0, typename A1>
    struct make_lazy<F, A0, A1>
    {
        typedef typename
            proto::terminal<
               lazy_terminal<
                    typename F::terminal_type
                  , typename phoenix::detail::expression::function_eval<F, A0, A1>::type
                  , 2 // arity
                >
            >::type
        result_type;
        typedef result_type type;

        result_type
        operator()(F f, A0 const& _0_, A1 const& _1_) const
        {
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(child_type(
                phoenix::detail::expression::function_eval<F, A0, A1>::make(f, _0_, _1_)
              , f.proto_base().child0
            ));
        }
    };

    template <typename F, typename A0, typename A1, typename A2>
    struct make_lazy<F, A0, A1, A2>
    {
        typedef typename
            proto::terminal<
               lazy_terminal<
                    typename F::terminal_type
                  , typename phoenix::detail::expression::function_eval<F, A0, A1, A2>::type
                  , 3 // arity
                >
            >::type
        result_type;
        typedef result_type type;

        result_type
        operator()(F f, A0 const& _0_, A1 const& _1_, A2 const& _2_) const
        {
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(child_type(
                phoenix::detail::expression::function_eval<F, A0, A1, A2>::make(f, _0_, _1_, _2_)
              , f.proto_base().child0
            ));
        }
    };

    namespace detail
    {
        // Helper struct for SFINAE purposes
        template <bool C> struct bool_;

        template <>
        struct bool_<true> : mpl::bool_<true>
        { 
            typedef bool_<true>* is_true; 
        };

        template <>
        struct bool_<false> : mpl::bool_<false>
        { 
            typedef bool_<false>* is_false; 
        };

        // Metafunction to detect if at least one arg is a Phoenix actor
        template <
            typename A0
          , typename A1 = unused_type
          , typename A2 = unused_type
        >
        struct contains_actor
            : bool_<
                  phoenix::is_actor<A0>::value
               || phoenix::is_actor<A1>::value
               || phoenix::is_actor<A2>::value
              >
        {};

        // to_lazy_arg: convert a terminal arg type to the type make_lazy needs
        template <typename A>
        struct to_lazy_arg
          : phoenix::as_actor<A> // wrap A in a Phoenix actor if not already one
        {};

        template <typename A>
        struct to_lazy_arg<const A>
          : to_lazy_arg<A>
        {};
        
        template <typename A>
        struct to_lazy_arg<A &>
          : to_lazy_arg<A>
        {};

        template <>
        struct to_lazy_arg<unused_type>
        {
            // unused arg: make_lazy wants unused_type
            typedef unused_type type;
        };

        // to_nonlazy_arg: convert a terminal arg type to the type make_vector needs
        template <typename A>
        struct to_nonlazy_arg
        {
            // identity
            typedef A type;
        };

        template <typename A>
        struct to_nonlazy_arg<const A>
          : to_nonlazy_arg<A>
        {};

        template <typename A>
        struct to_nonlazy_arg<A &>
          : to_nonlazy_arg<A>
        {};

        // incomplete type: should not be appeared unused_type in nonlazy arg.
        template <>
        struct to_nonlazy_arg<unused_type>;
    }

    template <typename Terminal>
    struct terminal
      : proto::extends<
            typename proto::terminal<Terminal>::type
          , terminal<Terminal>
        >
    {
        typedef terminal<Terminal> this_type;
        typedef Terminal terminal_type;

        typedef proto::extends<
            typename proto::terminal<Terminal>::type
          , terminal<Terminal>
        > base_type;

        terminal() {}

        terminal(Terminal const& t)
          : base_type(proto::terminal<Terminal>::type::make(t)) 
        {}

#if defined(BOOST_MSVC)
#pragma warning(push)
// warning C4348: 'boost::spirit::terminal<...>::result_helper': redefinition of default parameter: parameter 3, 4
#pragma warning(disable: 4348)
#endif

        template <
            bool Lazy
          , typename A0
          , typename A1 = unused_type
          , typename A2 = unused_type
        >
        struct result_helper;

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

        template <
            typename A0
        >
        struct result_helper<false, A0>
        {
            typedef typename
                proto::terminal<
                    terminal_ex<
                        Terminal
                      , typename detail::result_of::make_vector<
                            typename detail::to_nonlazy_arg<A0>::type>::type>
                >::type
            type;
        };

        template <
            typename A0
          , typename A1
        >
        struct result_helper<false, A0, A1>
        {
            typedef typename
                proto::terminal<
                    terminal_ex<
                        Terminal
                      , typename detail::result_of::make_vector<
                            typename detail::to_nonlazy_arg<A0>::type
                          , typename detail::to_nonlazy_arg<A1>::type>::type>
                >::type
            type;
        };

        template <
            typename A0
          , typename A1
          , typename A2
        >
        struct result_helper<false, A0, A1, A2>
        {
            typedef typename
                proto::terminal<
                    terminal_ex<
                        Terminal
                      , typename detail::result_of::make_vector<
                            typename detail::to_nonlazy_arg<A0>::type
                          , typename detail::to_nonlazy_arg<A1>::type
                          , typename detail::to_nonlazy_arg<A2>::type>::type>
                >::type
            type;
        };

        template <
            typename A0
          , typename A1
          , typename A2
        >
        struct result_helper<true, A0, A1, A2>
        {
            typedef typename
                make_lazy<this_type
                  , typename detail::to_lazy_arg<A0>::type
                  , typename detail::to_lazy_arg<A1>::type
                  , typename detail::to_lazy_arg<A2>::type>::type
            type;
        };

        // FIXME: we need to change this to conform to the result_of protocol
        template <
            typename A0
          , typename A1 = unused_type
          , typename A2 = unused_type      // Support up to 3 args
        >
        struct result
        {
            typedef typename
                result_helper<
                    detail::contains_actor<A0, A1, A2>::value
                  , A0, A1, A2
                >::type
            type;
        };

        template <typename This, typename A0>
        struct result<This(A0)>
        {
            typedef typename
                result_helper<
                    detail::contains_actor<A0, unused_type, unused_type>::value
                  , A0, unused_type, unused_type
                >::type
            type;
        };

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
        {
            typedef typename
                result_helper<
                    detail::contains_actor<A0, A1, unused_type>::value
                  , A0, A1, unused_type
                >::type
            type;
        };


        template <typename This, typename A0, typename A1, typename A2>
        struct result<This(A0, A1, A2)>
        {
            typedef typename
                result_helper<
                     detail::contains_actor<A0, A1, A2>::value
                   , A0, A1, A2
                 >::type
                 type;
        };

        // Note: in the following overloads, SFINAE cannot
        // be done on return type because of gcc bug #24915:
        //   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=24915
        // Hence an additional, fake argument is used for SFINAE,
        // using a type which can never be a real argument type.

        // Non-lazy overloads. Only enabled when all
        // args are immediates (no Phoenix actor).

        template <typename A0>
        typename result<A0>::type
        operator()(A0 const& _0_
          , typename detail::contains_actor<A0>::is_false = 0) const
        {
            typedef typename result<A0>::type result_type;
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(
                child_type(
                    detail::make_vector(_0_)
                  , this->proto_base().child0)
            );
        }

        template <typename A0, typename A1>
        typename result<A0, A1>::type
        operator()(A0 const& _0_, A1 const& _1_
          , typename detail::contains_actor<A0, A1>::is_false = 0) const
        {
            typedef typename result<A0, A1>::type result_type;
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(
                child_type(
                    detail::make_vector(_0_, _1_)
                  , this->proto_base().child0)
            );
        }

        template <typename A0, typename A1, typename A2>
        typename result<A0, A1, A2>::type
        operator()(A0 const& _0_, A1 const& _1_, A2 const& _2_
          , typename detail::contains_actor<A0, A1, A2>::is_false = 0) const
        {
            typedef typename result<A0, A1, A2>::type result_type;
            typedef typename result_type::proto_child0 child_type;
            return result_type::make(
                child_type(
                    detail::make_vector(_0_, _1_, _2_)
                  , this->proto_base().child0)
            );
        }

        // Lazy overloads. Enabled when at
        // least one arg is a Phoenix actor.
        template <typename A0>
        typename result<A0>::type
        operator()(A0 const& _0_
          , typename detail::contains_actor<A0>::is_true = 0) const
        {
            return make_lazy<this_type
              , typename phoenix::as_actor<A0>::type>()(*this
              , phoenix::as_actor<A0>::convert(_0_));
        }

        template <typename A0, typename A1>
        typename result<A0, A1>::type
        operator()(A0 const& _0_, A1 const& _1_
          , typename detail::contains_actor<A0, A1>::is_true = 0) const
        {
            return make_lazy<this_type
              , typename phoenix::as_actor<A0>::type
              , typename phoenix::as_actor<A1>::type>()(*this
              , phoenix::as_actor<A0>::convert(_0_)
              , phoenix::as_actor<A1>::convert(_1_));
        }

        template <typename A0, typename A1, typename A2>
        typename result<A0, A1, A2>::type
        operator()(A0 const& _0_, A1 const& _1_, A2 const& _2_
          , typename detail::contains_actor<A0, A1, A2>::is_true = 0) const
        {
            return make_lazy<this_type
              , typename phoenix::as_actor<A0>::type
              , typename phoenix::as_actor<A1>::type
              , typename phoenix::as_actor<A2>::type>()(*this
              , phoenix::as_actor<A0>::convert(_0_)
              , phoenix::as_actor<A1>::convert(_1_)
              , phoenix::as_actor<A2>::convert(_2_));
        }

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(terminal& operator= (terminal const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace result_of
    {
        // Calculate the type of the compound terminal if generated by one of
        // the spirit::terminal::operator() overloads above

        // The terminal type itself is passed through without modification
        template <typename Tag>
        struct terminal
        {
            typedef spirit::terminal<Tag> type;
        };

        template <typename Tag, typename A0>
        struct terminal<Tag(A0)>
        {
            typedef typename spirit::terminal<Tag>::
                template result<A0>::type type;
        };

        template <typename Tag, typename A0, typename A1>
        struct terminal<Tag(A0, A1)>
        {
            typedef typename spirit::terminal<Tag>::
                template result<A0, A1>::type type;
        };

        template <typename Tag, typename A0, typename A1, typename A2>
        struct terminal<Tag(A0, A1, A2)>
        {
            typedef typename spirit::terminal<Tag>::
                template result<A0, A1, A2>::type type;
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // support for stateful tag types
    namespace tag
    {
        template <
            typename Data, typename Tag
          , typename DataTag1 = unused_type, typename DataTag2 = unused_type>
        struct stateful_tag
        {
            BOOST_SPIRIT_IS_TAG()

            typedef Data data_type;

            stateful_tag() {}
            stateful_tag(data_type const& data) : data_(data) {}

            data_type data_;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(stateful_tag& operator= (stateful_tag const&))
        };
    }

    template <
        typename Data, typename Tag
      , typename DataTag1 = unused_type, typename DataTag2 = unused_type>
    struct stateful_tag_type
      : spirit::terminal<tag::stateful_tag<Data, Tag, DataTag1, DataTag2> >
    {
        typedef tag::stateful_tag<Data, Tag, DataTag1, DataTag2> tag_type;

        stateful_tag_type() {}
        stateful_tag_type(Data const& data)
          : spirit::terminal<tag_type>(data) 
        {}

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(stateful_tag_type& operator= (stateful_tag_type const&))
    };

    namespace detail
    {
        // extract expression if this is a Tag
        template <typename StatefulTag>
        struct get_stateful_data
        {
            typedef typename StatefulTag::data_type data_type;

            // is invoked if given tag is != Tag
            template <typename Tag_>
            static data_type call(Tag_) { return data_type(); }

            // this is invoked if given tag is same as'Tag'
            static data_type const& call(StatefulTag const& t) { return t.data_; }
        };
    }

}}

namespace boost { namespace phoenix
{
    template <typename Tag>
    struct is_custom_terminal<Tag, typename Tag::is_spirit_tag>
      : mpl::true_
    {};

    template <typename Tag>
    struct custom_terminal<Tag, typename Tag::is_spirit_tag>
    {
#ifndef BOOST_PHOENIX_NO_SPECIALIZE_CUSTOM_TERMINAL
        typedef void _is_default_custom_terminal; // fix for #7730
#endif

        typedef spirit::terminal<Tag> result_type;

        template <typename Context>
        result_type operator()(Tag const & t, Context const &)
        {
            return spirit::terminal<Tag>(t);
        }
    };
}}

// Define a spirit terminal. This macro may be placed in any namespace.
// Common placeholders are placed in the main boost::spirit namespace
// (see common_terminals.hpp)

#define BOOST_SPIRIT_TERMINAL_X(x, y) ((x, y)) BOOST_SPIRIT_TERMINAL_Y
#define BOOST_SPIRIT_TERMINAL_Y(x, y) ((x, y)) BOOST_SPIRIT_TERMINAL_X
#define BOOST_SPIRIT_TERMINAL_X0
#define BOOST_SPIRIT_TERMINAL_Y0

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#define BOOST_SPIRIT_TERMINAL_NAME(name, type_name)                             \
    namespace tag { struct name { BOOST_SPIRIT_IS_TAG() }; }                    \
    typedef boost::proto::terminal<tag::name>::type type_name;                  \
    type_name const name = {{}};                                                \
    inline void BOOST_PP_CAT(silence_unused_warnings_, name)() { (void) name; } \
    /***/

#else

#define BOOST_SPIRIT_TERMINAL_NAME(name, type_name)                             \
    namespace tag { struct name { BOOST_SPIRIT_IS_TAG() }; }                    \
    typedef boost::proto::terminal<tag::name>::type type_name;                  \
    /***/

#endif

#define BOOST_SPIRIT_TERMINAL(name)                                             \
    BOOST_SPIRIT_TERMINAL_NAME(name, name ## _type)                             \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_NAME_A(r, _, names)                       \
    BOOST_SPIRIT_TERMINAL_NAME(                                                 \
        BOOST_PP_TUPLE_ELEM(2, 0, names),                                       \
        BOOST_PP_TUPLE_ELEM(2, 1, names)                                        \
    )                                                                           \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_NAME(seq)                                 \
    BOOST_PP_SEQ_FOR_EACH(BOOST_SPIRIT_DEFINE_TERMINALS_NAME_A, _,              \
        BOOST_PP_CAT(BOOST_SPIRIT_TERMINAL_X seq, 0))                           \
    /***/

// Define a spirit extended terminal. This macro may be placed in any namespace.
// Common placeholders are placed in the main boost::spirit namespace
// (see common_terminals.hpp)

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#define BOOST_SPIRIT_TERMINAL_NAME_EX(name, type_name)                          \
    namespace tag { struct name { BOOST_SPIRIT_IS_TAG() }; }                    \
    typedef boost::spirit::terminal<tag::name> type_name;                       \
    type_name const name = type_name();                                         \
    inline void BOOST_PP_CAT(silence_unused_warnings_, name)() { (void) name; } \
    /***/

#else

#define BOOST_SPIRIT_TERMINAL_NAME_EX(name, type_name)                          \
    namespace tag { struct name { BOOST_SPIRIT_IS_TAG() }; }                    \
    typedef boost::spirit::terminal<tag::name> type_name;                       \
    /***/

#endif

#define BOOST_SPIRIT_TERMINAL_EX(name)                                          \
    BOOST_SPIRIT_TERMINAL_NAME_EX(name, name ## _type)                          \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_NAME_EX_A(r, _, names)                    \
    BOOST_SPIRIT_TERMINAL_NAME_EX(                                              \
        BOOST_PP_TUPLE_ELEM(2, 0, names),                                       \
        BOOST_PP_TUPLE_ELEM(2, 1, names)                                        \
    )                                                                           \
    /***/

#define BOOST_SPIRIT_DEFINE_TERMINALS_NAME_EX(seq)                              \
    BOOST_PP_SEQ_FOR_EACH(BOOST_SPIRIT_DEFINE_TERMINALS_NAME_EX_A, _,           \
        BOOST_PP_CAT(BOOST_SPIRIT_TERMINAL_X seq, 0))                           \
    /***/

#endif


