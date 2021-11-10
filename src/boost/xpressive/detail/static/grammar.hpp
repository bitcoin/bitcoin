///////////////////////////////////////////////////////////////////////////////
// grammar.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_GRAMMAR_HPP_EAN_11_12_2006
#define BOOST_XPRESSIVE_DETAIL_STATIC_GRAMMAR_HPP_EAN_11_12_2006

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/proto/core.hpp>
#include <boost/xpressive/detail/static/is_pure.hpp>
#include <boost/xpressive/detail/static/transforms/as_matcher.hpp>
#include <boost/xpressive/detail/static/transforms/as_alternate.hpp>
#include <boost/xpressive/detail/static/transforms/as_sequence.hpp>
#include <boost/xpressive/detail/static/transforms/as_quantifier.hpp>
#include <boost/xpressive/detail/static/transforms/as_marker.hpp>
#include <boost/xpressive/detail/static/transforms/as_set.hpp>
#include <boost/xpressive/detail/static/transforms/as_independent.hpp>
#include <boost/xpressive/detail/static/transforms/as_modifier.hpp>
#include <boost/xpressive/detail/static/transforms/as_inverse.hpp>
#include <boost/xpressive/detail/static/transforms/as_action.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>

#define BOOST_XPRESSIVE_CHECK_REGEX(Expr, Char)\
    BOOST_MPL_ASSERT\
    ((\
        typename boost::mpl::if_c<\
            boost::xpressive::is_valid_regex<Expr, Char>::value\
          , boost::mpl::true_\
          , boost::xpressive::INVALID_REGULAR_EXPRESSION\
        >::type\
    ));

//////////////////////////////////////////////////////////////////////////
//**********************************************************************//
//*                            << NOTE! >>                             *//
//*                                                                    *//
//* Whenever you change this grammar, you MUST also make corresponding *//
//* changes to width_of.hpp and is_pure.hpp.                           *//
//*                                                                    *//
//**********************************************************************//
//////////////////////////////////////////////////////////////////////////

namespace boost { namespace xpressive
{
    template<typename Char>
    struct Grammar;

    template<typename Char>
    struct ActionableGrammar;

    namespace grammar_detail
    {
        ///////////////////////////////////////////////////////////////////////////
        // CharLiteral
        template<typename Char>
        struct CharLiteral;

        ///////////////////////////////////////////////////////////////////////////
        // ListSet
        template<typename Char>
        struct ListSet;

        ///////////////////////////////////////////////////////////////////////////
        // as_repeat
        template<typename Char, typename Gram, typename Greedy>
        struct as_repeat
          : if_<
                make<detail::use_simple_repeat<_child, Char> >
              , as_simple_quantifier<Gram, Greedy>
              , as_default_quantifier<Greedy>
            >
        {};

        ///////////////////////////////////////////////////////////////////////////
        // NonGreedyRepeatCases
        template<typename Gram>
        struct NonGreedyRepeatCases
        {
            template<typename Tag, typename Dummy = void>
            struct case_
              : not_<_>
            {};

            template<typename Dummy>
            struct case_<tag::dereference, Dummy>
              : dereference<Gram>
            {};

            template<typename Dummy>
            struct case_<tag::unary_plus, Dummy>
              : unary_plus<Gram>
            {};

            template<typename Dummy>
            struct case_<tag::logical_not, Dummy>
              : logical_not<Gram>
            {};

            template<uint_t Min, uint_t Max, typename Dummy>
            struct case_<detail::generic_quant_tag<Min, Max>, Dummy>
              : unary_expr<detail::generic_quant_tag<Min, Max>, Gram>
            {};
        };

        ///////////////////////////////////////////////////////////////////////////
        // InvertibleCases
        template<typename Char, typename Gram>
        struct InvertibleCases
        {
            template<typename Tag, typename Dummy = void>
            struct case_
              : not_<_>
            {};

            template<typename Dummy>
            struct case_<tag::comma, Dummy>
              : when<ListSet<Char>, as_list_set_matcher<Char> >
            {};

            template<typename Dummy>
            struct case_<tag::assign, Dummy>
              : when<ListSet<Char>, as_list_set_matcher<Char> >
            {};

            template<typename Dummy>
            struct case_<tag::subscript, Dummy>
              : when<subscript<detail::set_initializer_type, Gram>, call<as_set_matcher<Gram>(_right)> >
            {};

            template<typename Dummy>
            struct case_<detail::lookahead_tag, Dummy>
              : when<
                    unary_expr<detail::lookahead_tag, Gram>
                  , as_lookahead<Gram>
                >
            {};

            template<typename Dummy>
            struct case_<detail::lookbehind_tag, Dummy>
              : when<
                    unary_expr<detail::lookbehind_tag, Gram>
                  , as_lookbehind<Gram>
                >
            {};

            template<typename Dummy>
            struct case_<tag::terminal, Dummy>
              : when<
                    or_<
                        CharLiteral<Char>
                      , terminal<detail::posix_charset_placeholder>
                      , terminal<detail::range_placeholder<_> >
                      , terminal<detail::logical_newline_placeholder>
                      , terminal<detail::assert_word_placeholder<detail::word_boundary<mpl::true_> > >
                    >
                  , as_matcher
                >
            {};
        };

        ///////////////////////////////////////////////////////////////////////////
        // Cases
        template<typename Char, typename Gram>
        struct Cases
        {
            template<typename Tag, typename Dummy = void>
            struct case_
              : not_<_>
            {};

            template<typename Dummy>
            struct case_<tag::terminal, Dummy>
              : when<
                    _
                  , in_sequence<as_matcher>
                >
            {};

            template<typename Dummy>
            struct case_<tag::shift_right, Dummy>
              : when<
                    shift_right<Gram, Gram>
                  , reverse_fold<_, _state, Gram>
                >
            {};

            template<typename Dummy>
            struct case_<tag::bitwise_or, Dummy>
              : when<
                    bitwise_or<Gram, Gram>
                  , in_sequence<
                        as_alternate_matcher<
                            reverse_fold_tree<_, make<fusion::nil>, in_alternate_list<Gram> >
                        >
                    >
                >
            {};

            template<typename Dummy, typename Greedy>
            struct case_<optional_tag<Greedy> , Dummy>
              : when<
                    unary_expr<optional_tag<Greedy>, Gram>
                  , in_sequence<call<as_optional<Gram, Greedy>(_child)> >
                >
            {};

            template<typename Dummy>
            struct case_<tag::dereference, Dummy>
              : when<
                    dereference<Gram>
                  , call<Gram(as_repeat<Char, Gram, mpl::true_>)>
                >
            {};

            template<typename Dummy>
            struct case_<tag::unary_plus, Dummy>
              : when<
                    unary_plus<Gram>
                  , call<Gram(as_repeat<Char, Gram, mpl::true_>)>
                >
            {};

            template<typename Dummy>
            struct case_<tag::logical_not, Dummy>
              : when<
                    logical_not<Gram>
                  , call<Gram(as_repeat<Char, Gram, mpl::true_>)>
                >
            {};

            template<uint_t Min, uint_t Max, typename Dummy>
            struct case_<detail::generic_quant_tag<Min, Max>, Dummy>
              : when<
                    unary_expr<detail::generic_quant_tag<Min, Max>, Gram>
                  , call<Gram(as_repeat<Char, Gram, mpl::true_>)>
                >
            {};

            template<typename Dummy>
            struct case_<tag::negate, Dummy>
              : when<
                    negate<switch_<NonGreedyRepeatCases<Gram> > >
                  , call<Gram(call<as_repeat<Char, Gram, mpl::false_>(_child)>)>
                >
            {};

            template<typename Dummy>
            struct case_<tag::complement, Dummy>
              : when<
                    complement<switch_<InvertibleCases<Char, Gram> > >
                  , in_sequence<call<as_inverse(call<switch_<InvertibleCases<Char, Gram> >(_child)>)> >
                >
            {};

            template<typename Dummy>
            struct case_<detail::modifier_tag, Dummy>
              : when<binary_expr<detail::modifier_tag, _, Gram>, as_modifier<Gram> >
            {};

            template<typename Dummy>
            struct case_<detail::lookahead_tag, Dummy>
              : when<
                    unary_expr<detail::lookahead_tag, Gram>
                  , in_sequence<as_lookahead<Gram> >
                >
            {};

            template<typename Dummy>
            struct case_<detail::lookbehind_tag, Dummy>
              : when<
                    unary_expr<detail::lookbehind_tag, Gram>
                  , in_sequence<as_lookbehind<Gram> >
                >
            {};

            template<typename Dummy>
            struct case_<detail::keeper_tag, Dummy>
              : when<
                    unary_expr<detail::keeper_tag, Gram>
                  , in_sequence<as_keeper<Gram> >
                >
            {};

            template<typename Dummy>
            struct case_<tag::comma, Dummy>
              : when<ListSet<Char>, in_sequence<as_list_set_matcher<Char> > >
            {};

            template<typename Dummy>
            struct case_<tag::assign, Dummy>
              : or_<
                    when<assign<detail::basic_mark_tag, Gram>, call<Gram(as_marker)> >
                  , when<ListSet<Char>, in_sequence<as_list_set_matcher<Char> > >
                >
            {};

            template<typename Dummy>
            struct case_<tag::subscript, Dummy>
              : or_<
                    when<subscript<detail::set_initializer_type, Gram>, in_sequence<call<as_set_matcher<Gram>(_right)> > >
                  , when<subscript<ActionableGrammar<Char>, _>, call<ActionableGrammar<Char>(as_action)> >
                >
            {};
        };

        ///////////////////////////////////////////////////////////////////////////
        // ActionableCases
        template<typename Char, typename Gram>
        struct ActionableCases
        {
            template<typename Tag, typename Dummy = void>
            struct case_
              : Cases<Char, Gram>::template case_<Tag>
            {};

            // Only in sub-expressions with actions attached do we allow attribute assignements
            template<typename Dummy>
            struct case_<proto::tag::assign, Dummy>
              : or_<
                    typename Cases<Char, Gram>::template case_<proto::tag::assign>
                  , when<proto::assign<terminal<detail::attribute_placeholder<_> >, _>, in_sequence<as_attr_matcher> >
                >
            {};
        };

    } // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Grammar
    template<typename Char>
    struct Grammar
      : proto::switch_<grammar_detail::Cases<Char, Grammar<Char> > >
    {};

    template<typename Char>
    struct ActionableGrammar
      : proto::switch_<grammar_detail::ActionableCases<Char, ActionableGrammar<Char> > >
    {};

    ///////////////////////////////////////////////////////////////////////////
    // INVALID_REGULAR_EXPRESSION
    struct INVALID_REGULAR_EXPRESSION
      : mpl::false_
    {};

    ///////////////////////////////////////////////////////////////////////////
    // is_valid_regex
    template<typename Expr, typename Char>
    struct is_valid_regex
      : proto::matches<Expr, Grammar<Char> >
    {};

}} // namespace boost::xpressive

#endif
