/*=============================================================================
  Copyright (c) 2001-2011 Joel de Guzman
  http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_META_COMPILER_OCTOBER_16_2008_1258PM
#define BOOST_SPIRIT_META_COMPILER_OCTOBER_16_2008_1258PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/home/support/make_component.hpp>
#include <boost/spirit/home/support/modify.hpp>
#include <boost/spirit/home/support/detail/make_cons.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/proto/matches.hpp>
#include <boost/proto/tags.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/proto_fwd.hpp> // for transform placeholders
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace spirit
{
    // Some defaults...

    template <typename Domain, typename Tag, typename Enable = void>
    struct use_operator : mpl::false_ {};

    template <typename Domain, typename T, typename Enable = void>
    struct use_function : mpl::false_ {};

    template <typename Domain, typename T, typename Enable = void>
    struct use_directive : mpl::false_ {};

    template <typename Domain, typename T, typename Enable /* = void */>
    struct is_modifier_directive : mpl::false_ {};

    template <typename Domain, typename T, typename Enable = void>
    struct use_terminal : mpl::false_ {};

    template <typename Domain, typename T, typename Enable /*= void*/>
    struct flatten_tree : mpl::false_ {};

    // Our meta-compiler. This is the main engine that hooks Spirit
    // to the proto expression template engine.

    template <typename Domain>
    struct meta_compiler
    {
        struct meta_grammar;

        BOOST_SPIRIT_ASSERT_MSG((
            !use_operator<Domain, proto::tag::subscript>::value
        ), error_proto_tag_subscript_cannot_be_used, ());

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1400)
        // this is the non-broken part for compilers properly supporting 
        // partial template specialization (VC7.1 does not)
        struct cases
        {
            template <typename Tag, typename Enable = void>
            struct case_
              : proto::not_<proto::_>
            {};

            ///////////////////////////////////////////////////////////////////
            // terminals
            ///////////////////////////////////////////////////////////////////
            template <typename Enable>
            struct case_<proto::tag::terminal, Enable>
              : proto::when<
                    proto::if_<use_terminal<Domain, proto::_value>()>,
                    detail::make_terminal<Domain>
                >
            {};

            template <typename Tag>
            struct case_<Tag, typename enable_if<use_operator<Domain, Tag> >::type>
              : proto::or_<
            ///////////////////////////////////////////////////////////////////
            // binary operators
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::binary_expr<Tag, meta_grammar, meta_grammar>,
                        detail::make_binary<Domain, Tag, meta_grammar>
                    >,
            ///////////////////////////////////////////////////////////////////
            // unary operators
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::unary_expr<Tag, meta_grammar>,
                        detail::make_unary<Domain, Tag, meta_grammar>
                    >
                >
            {};

            template <typename Enable>
            struct case_<proto::tag::subscript, Enable>
              : proto::or_<
            ///////////////////////////////////////////////////////////////////
            // directives
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::binary_expr<proto::tag::subscript
                      , proto::and_<
                            proto::terminal<proto::_>
                          , proto::if_<use_directive<Domain, proto::_value >()> >
                      , meta_grammar>,
                        detail::make_directive<Domain, meta_grammar>
                    >,
            ///////////////////////////////////////////////////////////////////
            // semantic actions
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::binary_expr<proto::tag::subscript
                      , meta_grammar, proto::_>,
                        detail::make_action<Domain, meta_grammar>
                    >
                >
            {};
        };
#else
        // this part actually constitutes invalid C++ code, but it allows us to
        // convince VC7.1 to do what we want
        struct cases
        {
            template <typename Tag, typename Enable = void>
            struct case_
              : proto::not_<proto::_>
            {};

            ///////////////////////////////////////////////////////////////////
            // terminals
            ///////////////////////////////////////////////////////////////////
            template <>
            struct case_<proto::tag::terminal>
              : proto::when<
                    proto::if_<use_terminal<Domain, proto::_value>()>,
                    detail::make_terminal<Domain>
                >
            {};

            template <typename Tag>
            struct case_<Tag>
              : proto::or_<
            ///////////////////////////////////////////////////////////////////
            // binary operators
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::binary_expr<
                        typename enable_if<use_operator<Domain, Tag>, Tag>::type
                          , meta_grammar, meta_grammar>
                      , detail::make_binary<Domain, Tag, meta_grammar>
                    >,
            ///////////////////////////////////////////////////////////////////
            // unary operators
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::unary_expr<
                        typename enable_if<use_operator<Domain, Tag>, Tag>::type
                          , meta_grammar>
                      , detail::make_unary<Domain, Tag, meta_grammar>
                    >
                >
            {};

            template <>
            struct case_<proto::tag::subscript>
              : proto::or_<
            ///////////////////////////////////////////////////////////////////
            // directives
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::binary_expr<proto::tag::subscript
                      , proto::and_<
                            proto::terminal<proto::_>
                          , proto::if_<use_directive<Domain, proto::_value >()> >
                      , meta_grammar>,
                        detail::make_directive<Domain, meta_grammar>
                    >,
            ///////////////////////////////////////////////////////////////////
            // semantic actions
            ///////////////////////////////////////////////////////////////////
                    proto::when<proto::binary_expr<proto::tag::subscript
                      , meta_grammar, proto::_>,
                        detail::make_action<Domain, meta_grammar>
                    >
                >
            {};
        };
#endif

        struct meta_grammar
          : proto::switch_<cases>
        {};
    };

    namespace result_of
    {
        // Default case
        template <typename Domain, typename Expr
          , typename Modifiers = unused_type, typename Enable = void>
        struct compile
        {
            typedef typename meta_compiler<Domain>::meta_grammar meta_grammar;
            typedef typename meta_grammar::
                template result<meta_grammar(Expr, mpl::void_, Modifiers)>::type
            type;
        };

        // If Expr is not a proto expression, make it a terminal
        template <typename Domain, typename Expr, typename Modifiers>
        struct compile<Domain, Expr, Modifiers,
            typename disable_if<proto::is_expr<Expr> >::type>
          : compile<Domain, typename proto::terminal<Expr>::type, Modifiers> {};
    }

    namespace traits
    {
        // Check if Expr matches the domain's grammar
        template <typename Domain, typename Expr>
        struct matches :
            proto::matches<
                typename proto::result_of::as_expr<
                    typename remove_reference<Expr>::type>::type,
                typename meta_compiler<Domain>::meta_grammar
            >
        {
        };
    }

    namespace detail
    {
        template <typename Domain>
        struct compiler
        {
            // Default case
            template <typename Expr, typename Modifiers>
            static typename spirit::result_of::compile<Domain, Expr, Modifiers>::type
            compile(Expr const& expr, Modifiers modifiers, mpl::true_)
            {
                typename meta_compiler<Domain>::meta_grammar compiler;
                return compiler(expr, mpl::void_(), modifiers);
            }

            // If Expr is not a proto expression, make it a terminal
            template <typename Expr, typename Modifiers>
            static typename spirit::result_of::compile<Domain, Expr, Modifiers>::type
            compile(Expr const& expr, Modifiers modifiers, mpl::false_)
            {
                typename meta_compiler<Domain>::meta_grammar compiler;
                typedef typename detail::as_meta_element<Expr>::type expr_;
                typename proto::terminal<expr_>::type term = {expr};
                return compiler(term, mpl::void_(), modifiers);
            }
        };
    }

    template <typename Domain, typename Expr>
    inline typename result_of::compile<Domain, Expr, unused_type>::type
    compile(Expr const& expr)
    {
        typedef typename proto::is_expr<Expr>::type is_expr;
        return detail::compiler<Domain>::compile(expr, unused, is_expr());
    }

    template <typename Domain, typename Expr, typename Modifiers>
    inline typename result_of::compile<Domain, Expr, Modifiers>::type
    compile(Expr const& expr, Modifiers modifiers)
    {
        typedef typename proto::is_expr<Expr>::type is_expr;
        return detail::compiler<Domain>::compile(expr, modifiers, is_expr());
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, template <typename Subject> class generator>
    struct make_unary_composite
    {
        typedef typename
            fusion::result_of::value_at_c<Elements, 0>::type
        element_type;
        typedef generator<element_type> result_type;
        result_type operator()(Elements const& elements, unused_type) const
        {
            return result_type(fusion::at_c<0>(elements));
        }
    };

    template <typename Elements, template <typename Left, typename Right> class generator>
    struct make_binary_composite
    {
        typedef typename
            fusion::result_of::value_at_c<Elements, 0>::type
        left_type;
        typedef typename
            fusion::result_of::value_at_c<Elements, 1>::type
        right_type;
        typedef generator<left_type, right_type> result_type;

        result_type operator()(Elements const& elements, unused_type) const
        {
            return result_type(
                fusion::at_c<0>(elements)
              , fusion::at_c<1>(elements)
            );
        }
    };

    template <typename Elements, template <typename Elements_> class generator>
    struct make_nary_composite
    {
        typedef generator<Elements> result_type;
        result_type operator()(Elements const& elements, unused_type) const
        {
            return result_type(elements);
        }
    };

}}

#endif
