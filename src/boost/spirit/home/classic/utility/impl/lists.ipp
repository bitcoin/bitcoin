/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_LISTS_IPP
#define BOOST_SPIRIT_LISTS_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/meta/refactoring.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  list_parser_type class implementation
//
///////////////////////////////////////////////////////////////////////////////
struct no_list_endtoken { typedef no_list_endtoken embed_t; };

namespace impl {

///////////////////////////////////////////////////////////////////////////////
//
//  Refactor the original list item parser
//
///////////////////////////////////////////////////////////////////////////////

    //  match list with 'extended' syntax
    template <typename EndT>
    struct select_list_parse_refactor {

        template <
            typename ParserT, typename ScannerT,
            typename ItemT, typename DelimT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const& /*p*/,
            ItemT const &item, DelimT const &delim, EndT const &end)
        {
            typedef refactor_action_gen<refactor_unary_gen<> > refactor_t;
            const refactor_t refactor_item_d = refactor_t(refactor_unary_d);

            return (
                    refactor_item_d[item - (end | delim)]
                >> *(delim >> refactor_item_d[item - (end | delim)])
                >> !(delim >> end)
            ).parse(scan);
        }
    };

    //  match list with 'normal' syntax (without an 'end' parser)
    template <>
    struct select_list_parse_refactor<no_list_endtoken> {

        template <
            typename ParserT, typename ScannerT,
            typename ItemT, typename DelimT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const& /*p*/,
            ItemT const &item, DelimT const &delim, no_list_endtoken const&)
        {
            typedef refactor_action_gen<refactor_unary_gen<> > refactor_t;
            const refactor_t refactor_item_d = refactor_t(refactor_unary_d);

            return (
                    refactor_item_d[item - delim]
                >> *(delim >> refactor_item_d[item - delim])
            ).parse(scan);
        }
    };

///////////////////////////////////////////////////////////////////////////////
//
//  Do not refactor the original list item parser.
//
///////////////////////////////////////////////////////////////////////////////

    //  match list with 'extended' syntax
    template <typename EndT>
    struct select_list_parse_no_refactor {

        template <
            typename ParserT, typename ScannerT,
            typename ItemT, typename DelimT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const& /*p*/,
            ItemT const &item, DelimT const &delim, EndT const &end)
        {
            return (
                    (item - (end | delim))
                >> *(delim >> (item - (end | delim)))
                >> !(delim >> end)
            ).parse(scan);
        }
    };

    //  match list with 'normal' syntax (without an 'end' parser)
    template <>
    struct select_list_parse_no_refactor<no_list_endtoken> {

        template <
            typename ParserT, typename ScannerT,
            typename ItemT, typename DelimT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const& /*p*/,
            ItemT const &item, DelimT const &delim, no_list_endtoken const&)
        {
            return (
                    (item - delim)
                >> *(delim >> (item - delim))
            ).parse(scan);
        }
    };

    // the refactoring is handled by the refactoring parsers, so here there
    // is no need to pay attention to these issues.

    template <typename CategoryT>
    struct list_parser_type {

        template <
            typename ParserT, typename ScannerT,
            typename ItemT, typename DelimT, typename EndT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const& p,
              ItemT const &item, DelimT const &delim, EndT const &end)
        {
            return select_list_parse_refactor<EndT>::
                parse(scan, p, item, delim, end);
        }
    };

    template <>
    struct list_parser_type<plain_parser_category> {

        template <
            typename ParserT, typename ScannerT,
            typename ItemT, typename DelimT, typename EndT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const& p,
              ItemT const &item, DelimT const &delim, EndT const &end)
        {
            return select_list_parse_no_refactor<EndT>::
                parse(scan, p, item, delim, end);
        }
    };

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

