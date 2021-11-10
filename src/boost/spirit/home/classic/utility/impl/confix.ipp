/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_CONFIX_IPP
#define BOOST_SPIRIT_CONFIX_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/meta/refactoring.hpp>
#include <boost/spirit/home/classic/core/composite/impl/directives.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  Types to distinguish nested and non-nested confix parsers
//
///////////////////////////////////////////////////////////////////////////////
struct is_nested {};
struct non_nested {};

///////////////////////////////////////////////////////////////////////////////
//
//  Types to distinguish between confix parsers, which are implicitly lexems
//  and without this behaviour
//
///////////////////////////////////////////////////////////////////////////////
struct is_lexeme {};
struct non_lexeme {};

///////////////////////////////////////////////////////////////////////////////
//
//  confix_parser_type class implementation
//
///////////////////////////////////////////////////////////////////////////////
namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    //  implicitly insert a lexeme_d into the parsing process

    template <typename LexemeT>
    struct select_confix_parse_lexeme;

    template <>
    struct select_confix_parse_lexeme<is_lexeme> {

        template <typename ParserT, typename ScannerT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const& p, ScannerT const& scan)
        {
            typedef typename parser_result<ParserT, ScannerT>::type result_t;
            return contiguous_parser_parse<result_t>(p, scan, scan);
        }
    };

    template <>
    struct select_confix_parse_lexeme<non_lexeme> {

        template <typename ParserT, typename ScannerT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const& p, ScannerT const& scan)
        {
            return p.parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  parse confix sequences with refactoring

    template <typename NestedT>
    struct select_confix_parse_refactor;

    template <>
    struct select_confix_parse_refactor<is_nested> {

        template <
            typename LexemeT, typename ParserT, typename ScannerT,
            typename OpenT, typename ExprT, typename CloseT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(
            LexemeT const &, ParserT const& this_, ScannerT const& scan,
            OpenT const& open, ExprT const& expr, CloseT const& close)
        {
            typedef refactor_action_gen<refactor_unary_gen<> > refactor_t;
            const refactor_t refactor_body_d = refactor_t(refactor_unary_d);

            return select_confix_parse_lexeme<LexemeT>::parse((
                            open
                        >>  (this_ | refactor_body_d[expr - close])
                        >>  close
                    ),  scan);
        }
    };

    template <>
    struct select_confix_parse_refactor<non_nested> {

        template <
            typename LexemeT, typename ParserT, typename ScannerT,
            typename OpenT, typename ExprT, typename CloseT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(
            LexemeT const &, ParserT const& /*this_*/, ScannerT const& scan,
            OpenT const& open, ExprT const& expr, CloseT const& close)
        {
            typedef refactor_action_gen<refactor_unary_gen<> > refactor_t;
            const refactor_t refactor_body_d = refactor_t(refactor_unary_d);

            return select_confix_parse_lexeme<LexemeT>::parse((
                            open
                        >>  refactor_body_d[expr - close]
                        >>  close
                    ),  scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  parse confix sequences without refactoring

    template <typename NestedT>
    struct select_confix_parse_no_refactor;

    template <>
    struct select_confix_parse_no_refactor<is_nested> {

        template <
            typename LexemeT, typename ParserT, typename ScannerT,
            typename OpenT, typename ExprT, typename CloseT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(
            LexemeT const &, ParserT const& this_, ScannerT const& scan,
            OpenT const& open, ExprT const& expr, CloseT const& close)
        {
            return select_confix_parse_lexeme<LexemeT>::parse((
                            open
                        >>  (this_ | (expr - close))
                        >>  close
                    ),  scan);
        }
    };

    template <>
    struct select_confix_parse_no_refactor<non_nested> {

        template <
            typename LexemeT, typename ParserT, typename ScannerT,
            typename OpenT, typename ExprT, typename CloseT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(
            LexemeT const &, ParserT const & /*this_*/, ScannerT const& scan,
            OpenT const& open, ExprT const& expr, CloseT const& close)
        {
            return select_confix_parse_lexeme<LexemeT>::parse((
                            open
                        >>  (expr - close)
                        >>  close
                    ),  scan);
        }
    };

    // the refactoring is handled by the refactoring parsers, so here there
    // is no need to pay attention to these issues.

    template <typename CategoryT>
    struct confix_parser_type {

        template <
            typename NestedT, typename LexemeT,
            typename ParserT, typename ScannerT,
            typename OpenT, typename ExprT, typename CloseT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(
            NestedT const &, LexemeT const &lexeme,
            ParserT const& this_, ScannerT const& scan,
            OpenT const& open, ExprT const& expr, CloseT const& close)
        {
            return select_confix_parse_refactor<NestedT>::
                parse(lexeme, this_, scan, open, expr, close);
        }
    };

    template <>
    struct confix_parser_type<plain_parser_category> {

        template <
            typename NestedT, typename LexemeT,
            typename ParserT, typename ScannerT,
            typename OpenT, typename ExprT, typename CloseT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(
            NestedT const &, LexemeT const &lexeme,
            ParserT const& this_, ScannerT const& scan,
            OpenT const& open, ExprT const& expr, CloseT const& close)
        {
            return select_confix_parse_no_refactor<NestedT>::
                parse(lexeme, this_, scan, open, expr, close);
        }
    };

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

